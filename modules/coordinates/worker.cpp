#include "CoordinateText.h"
#include "CrsCatalog.h"
#include <proj.h>
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QLibrary>
#include <QRegularExpression>
#include <QCryptographicHash>
#include <QSet>
#include <QDateTime>
#include <memory>
#include <functional>
#include <stdexcept>
#include <cmath>
#include <cstdio>

using namespace workbench::coordinates;
namespace {
QString string(const char *s){return s?QString::fromUtf8(s):QString();}
void publish(const QJsonObject &o){const auto bytes=QJsonDocument(o).toJson(QJsonDocument::Compact);fwrite(bytes.constData(),1,size_t(bytes.size()),stdout);fputc('\n',stdout);fflush(stdout);}
struct Failure : std::runtime_error {QString code;explicit Failure(QString c):std::runtime_error(c.toStdString()),code(std::move(c)){} };
struct Api {
#define API(f) decltype(&f) f=nullptr;
    API(proj_context_create) API(proj_context_destroy) API(proj_context_set_search_paths)
    API(proj_context_set_enable_network) API(proj_context_get_database_metadata) API(proj_context_errno)
    API(proj_context_errno_string) API(proj_log_func) API(proj_info) API(proj_create) API(proj_destroy)
    API(proj_get_type) API(proj_get_name) API(proj_is_deprecated) API(proj_get_area_of_use)
    API(proj_normalize_for_visualization) API(proj_crs_get_coordinate_system) API(proj_cs_get_axis_count)
    API(proj_cs_get_axis_info) API(proj_crs_get_datum_forced) API(proj_get_ellipsoid)
    API(proj_get_celestial_body_name) API(proj_as_wkt) API(proj_as_proj_string)
    API(proj_create_crs_to_crs_from_pj) API(proj_crs_get_geodetic_crs) API(proj_trans)
    API(proj_errno) API(proj_errno_reset) API(proj_trans_get_last_used_operation)
    API(proj_coordoperation_get_accuracy) API(proj_coordoperation_get_grid_used_count)
    API(proj_coordoperation_get_grid_used) API(proj_coordoperation_has_ballpark_transformation)
    API(proj_get_crs_info_list_from_database) API(proj_crs_info_list_destroy)
#undef API
};
using Object=std::unique_ptr<PJ,std::function<void(PJ*)>>;
class Engine {
    QLibrary library;
    Api a;
    std::unique_ptr<PJ_CONTEXT,std::function<void(PJ_CONTEXT*)>> context;
public:
    explicit Engine(const QString &directory):context(nullptr,[](PJ_CONTEXT*){}){
        QFile file(QDir(directory).filePath("engine.json"));if(!file.open(QIODevice::ReadOnly))throw Failure("engine.not_found");
        const auto settings=QJsonDocument::fromJson(file.read(4096)).object();const auto name=settings["library"].toString();
        if(name.isEmpty()||name.contains('/')||name.contains('\\')||name.contains(':')||name.contains(".."))throw Failure("engine.configuration");
        library.setFileName(QDir(directory).filePath(name));if(!library.load())throw Failure("engine.load_failed");
#define LOAD(f) a.f=reinterpret_cast<decltype(a.f)>(library.resolve(#f));if(!a.f)throw Failure("engine.api_missing");
        LOAD(proj_context_create) LOAD(proj_context_destroy) LOAD(proj_context_set_search_paths)
        LOAD(proj_context_set_enable_network) LOAD(proj_context_get_database_metadata) LOAD(proj_context_errno)
        LOAD(proj_context_errno_string) LOAD(proj_log_func) LOAD(proj_info) LOAD(proj_create) LOAD(proj_destroy)
        LOAD(proj_get_type) LOAD(proj_get_name) LOAD(proj_is_deprecated) LOAD(proj_get_area_of_use)
        LOAD(proj_normalize_for_visualization) LOAD(proj_crs_get_coordinate_system) LOAD(proj_cs_get_axis_count)
        LOAD(proj_cs_get_axis_info) LOAD(proj_crs_get_datum_forced) LOAD(proj_get_ellipsoid)
        LOAD(proj_get_celestial_body_name) LOAD(proj_as_wkt) LOAD(proj_as_proj_string)
        LOAD(proj_create_crs_to_crs_from_pj) LOAD(proj_crs_get_geodetic_crs) LOAD(proj_trans)
        LOAD(proj_errno) LOAD(proj_errno_reset) LOAD(proj_trans_get_last_used_operation)
        LOAD(proj_coordoperation_get_accuracy) LOAD(proj_coordoperation_get_grid_used_count)
        LOAD(proj_coordoperation_get_grid_used) LOAD(proj_coordoperation_has_ballpark_transformation)
        LOAD(proj_get_crs_info_list_from_database) LOAD(proj_crs_info_list_destroy)
#undef LOAD
        const auto version=a.proj_info();if(version.major!=9||version.minor<2)throw Failure("engine.version");
        context={a.proj_context_create(),[this](PJ_CONTEXT *p){a.proj_context_destroy(p);}};if(!context)throw Failure("engine.context");
        const auto data=QDir(directory).filePath("data").toUtf8();const char *paths[]={data.constData()};
        a.proj_context_set_search_paths(context.get(),1,paths);a.proj_context_set_enable_network(context.get(),0);
        a.proj_log_func(context.get(),nullptr,[](void*,int,const char*){});
        if(string(a.proj_context_get_database_metadata(context.get(),"EPSG.VERSION")).isEmpty())throw Failure("engine.database");
    }
    Object own(PJ *p){return Object(p,[this](PJ *q){if(q)a.proj_destroy(q);});}
    Object crs(const QString &code){
        static const QRegularExpression id("^EPSG:[0-9]{1,8}$");if(!id.match(code).hasMatch())throw Failure("crs.invalid_code");
        auto object=own(a.proj_create(context.get(),code.toUtf8().constData()));if(!object)throw Failure("crs.not_found");return object;
    }
    QJsonObject version(){return {{"name","PROJ"},{"version",string(a.proj_info().version)},
        {"epsgVersion",string(a.proj_context_get_database_metadata(context.get(),"EPSG.VERSION"))},
        {"epsgDate",string(a.proj_context_get_database_metadata(context.get(),"EPSG.DATE"))},{"network",false}};}
    QJsonArray axes(PJ *object){
        auto cs=own(a.proj_crs_get_coordinate_system(context.get(),object));QJsonArray result;
        if(!cs||a.proj_cs_get_axis_count(context.get(),cs.get())!=2)throw Failure("crs.dimension");
        for(int i=0;i<2;++i){const char *name=nullptr,*abbreviation=nullptr,*direction=nullptr,*unit=nullptr;double factor=0;
            if(!a.proj_cs_get_axis_info(context.get(),cs.get(),i,&name,&abbreviation,&direction,&factor,&unit,nullptr,nullptr))throw Failure("crs.axes");
            result.append(QJsonObject{{"name",string(name)},{"abbreviation",string(abbreviation)},{"direction",string(direction)},{"unit",string(unit)},{"factor",factor}});
        }return result;
    }
    QJsonObject info(const QString &code){
        auto object=crs(code);const auto type=a.proj_get_type(object.get());const bool geographic=type==PJ_TYPE_GEOGRAPHIC_2D_CRS;
        if(!geographic&&type!=PJ_TYPE_PROJECTED_CRS)throw Failure("crs.dimension");
        if(a.proj_is_deprecated(object.get()))throw Failure("crs.deprecated");
        if(string(a.proj_get_celestial_body_name(context.get(),object.get()))!="Earth")throw Failure("crs.celestial_body");
        auto datum=own(a.proj_crs_get_datum_forced(context.get(),object.get()));
        if(datum&&a.proj_get_type(datum.get())==PJ_TYPE_DYNAMIC_GEODETIC_REFERENCE_FRAME)throw Failure("crs.epoch_required");
        auto ellipsoid=own(a.proj_get_ellipsoid(context.get(),object.get()));
        const auto originalAxes=axes(object.get());auto normalized=own(a.proj_normalize_for_visualization(context.get(),object.get()));
        if(!normalized)throw Failure("crs.axes");const auto visualAxes=axes(normalized.get());
        if(visualAxes[0].toObject()["direction"]!="east"||visualAxes[1].toObject()["direction"]!="north")throw Failure("crs.axes");
        if(geographic&&std::abs(visualAxes[0].toObject()["factor"].toDouble()-0.017453292519943295)>1e-14)throw Failure("crs.angular_unit");
        double west=0,south=0,east=0,north=0;const char *area=nullptr;
        if(!a.proj_get_area_of_use(context.get(),object.get(),&west,&south,&east,&north,&area))throw Failure("crs.area_missing");
        return {{"code",code},{"name",string(a.proj_get_name(object.get()))},{"geographic",geographic},
            {"datum",datum?string(a.proj_get_name(datum.get())):QString()},
            {"ellipsoid",ellipsoid?string(a.proj_get_name(ellipsoid.get())):QString()},
            {"officialAxes",originalAxes},{"axes",visualAxes},{"xLabel",geographic?"X · Longitud":"X · Este"},
            {"yLabel",geographic?"Y · Latitud":"Y · Norte"},{"xUnit",visualAxes[0].toObject()["unit"]},{"yUnit",visualAxes[1].toObject()["unit"]},
            {"area",string(area)},{"bbox",QJsonArray{west,south,east,north}},
            {"wkt",string(a.proj_as_wkt(context.get(),object.get(),PJ_WKT2_2019,nullptr))}};
    }
    QJsonObject search(QString query,QString group,int offset){
        if(query.size()>160)throw Failure("input.query");query=query.trimmed();int count=0;
        if(group.isEmpty())group=query.isEmpty()?"recommended":"all";
        if(group!="all"&&!crsGroups().contains(group))throw Failure("input.crs_group");
        if(offset<0||offset>100000)throw Failure("input.crs_offset");
        auto list=a.proj_get_crs_info_list_from_database(context.get(),"EPSG",nullptr,&count);if(!list)throw Failure("engine.database");
        const auto cleanup=std::unique_ptr<PROJ_CRS_INFO*,std::function<void(PROJ_CRS_INFO**)>>(list,[this](PROJ_CRS_INFO **p){a.proj_crs_info_list_destroy(p);});
        const auto selected=crsGroups().value(group);
        QJsonArray rows;int total=0;QSet<QString> seen;
        for(int i=0;i<count;++i){const auto &c=*list[i];if(c.deprecated||(c.type!=PJ_TYPE_GEOGRAPHIC_2D_CRS&&c.type!=PJ_TYPE_PROJECTED_CRS)||string(c.celestial_body_name)!="Earth")continue;
            const auto code=string(c.code),name=string(c.name);bool match=group=="all"||selected.contains(code);
            if(!query.isEmpty())for(const auto &token:query.split(' ',Qt::SkipEmptyParts))if(!name.contains(token,Qt::CaseInsensitive)&&!("EPSG:"+code).contains(token,Qt::CaseInsensitive))match=false;
            if(!match||seen.contains(code))continue;seen.insert(code);++total;if(total<=offset||rows.size()>=100)continue;
            rows.append(QJsonObject{{"code","EPSG:"+code},{"name",name},{"geographic",c.type==PJ_TYPE_GEOGRAPHIC_2D_CRS},{"area",string(c.area_name)}});
        }return {{"type","result"},{"ok",true},{"op","search"},{"matches",rows},{"total",total},{"offset",offset},{"limit",100},{"group",group},{"engine",version()}};
    }
    Object transform(PJ *source,PJ *target){
        const char *options[]={"ALLOW_BALLPARK=NO","ONLY_BEST=YES",nullptr};
        auto original=own(a.proj_create_crs_to_crs_from_pj(context.get(),source,target,nullptr,options));
        if(!original)throw Failure("operation.unavailable");auto normalized=own(a.proj_normalize_for_visualization(context.get(),original.get()));
        if(!normalized)throw Failure("operation.unavailable");return normalized;
    }
    PJ_COORD point(PJ *operation,double x,double y){
        a.proj_errno_reset(operation);PJ_COORD coordinate{};coordinate.v[0]=x;coordinate.v[1]=y;coordinate.v[2]=0;coordinate.v[3]=HUGE_VAL;
        const auto result=a.proj_trans(operation,PJ_FWD,coordinate);
        if(a.proj_errno(operation)||!std::isfinite(result.v[0])||!std::isfinite(result.v[1]))throw Failure("operation.point_failed");return result;
    }
    bool inArea(const QJsonObject &metadata,const PJ_COORD &geographic){
        const auto b=metadata["bbox"].toArray();const double x=geographic.v[0],y=geographic.v[1],epsilon=1e-9;
        const auto w=b[0].toDouble(),e=b[2].toDouble();return y>=b[1].toDouble()-epsilon&&y<=b[3].toDouble()+epsilon&&
            (w<=e?(x>=w-epsilon&&x<=e+epsilon):(x>=w-epsilon||x<=e+epsilon));
    }
    QJsonObject provenance(PJ *operation){
        if(a.proj_coordoperation_has_ballpark_transformation(context.get(),operation))throw Failure("operation.ballpark_blocked");
        const auto pipeline=string(a.proj_as_proj_string(context.get(),operation,PJ_PROJ_5,nullptr));
        const auto wkt=string(a.proj_as_wkt(context.get(),operation,PJ_WKT2_2019,nullptr));
        if(pipeline.isEmpty()&&wkt.isEmpty())throw Failure("operation.provenance");
        const double accuracy=a.proj_coordoperation_get_accuracy(context.get(),operation);QJsonArray grids;
        const int count=a.proj_coordoperation_get_grid_used_count(context.get(),operation);
        for(int i=0;i<count;++i){const char *name=nullptr;int available=0;
            a.proj_coordoperation_get_grid_used(context.get(),operation,i,&name,nullptr,nullptr,nullptr,nullptr,nullptr,&available);
            grids.append(QJsonObject{{"name",string(name)},{"available",available!=0}});if(!available)throw Failure("operation.grid_missing");
        }
        return {{"name",string(a.proj_get_name(operation))},{"pipeline",pipeline},{"wkt",wkt},
            {"accuracyMetres",accuracy<0?QJsonValue():QJsonValue(accuracy)},{"grids",grids},{"ballpark",false}};
    }
    QJsonObject convert(const QJsonObject &request){
        const auto sourceCode=request["source"].toString(),targetCode=request["target"].toString();
        const auto sourceInfo=info(sourceCode),targetInfo=info(targetCode);auto source=crs(sourceCode),target=crs(targetCode);
        auto operation=transform(source.get(),target.get());auto sourceGeodetic=own(a.proj_crs_get_geodetic_crs(context.get(),source.get()));
        auto targetGeodetic=own(a.proj_crs_get_geodetic_crs(context.get(),target.get()));
        if(!sourceGeodetic||!targetGeodetic)throw Failure("crs.axes");
        auto sourceToGeo=transform(source.get(),sourceGeodetic.get()),targetToGeo=transform(target.get(),targetGeodetic.get());
        auto sourceToTargetGeo=transform(source.get(),targetGeodetic.get());
        const auto format=request["inputFormat"].toString("DD"),outputFormat=request["outputFormat"].toString("DD");
        const auto decimal=request["decimal"].toString(".");if(decimal!="."&&decimal!=",")throw Failure("input.decimal_mode");
        const auto precision=request["precision"].toInt(7);if(precision<0||precision>9)throw Failure("input.precision");
        if(!QStringList{"DD","DM","DMS"}.contains(format)||!QStringList{"DD","DM","DMS"}.contains(outputFormat))throw Failure("input.angle_format");
        QJsonArray inputs=request["rows"].toArray();QStringList headers;
        if(request.contains("text")){
            const auto delimiter=request["delimiter"].toString("\t");if(delimiter.size()!=1)throw Failure("input.delimiter");
            if(decimal==delimiter)throw Failure("input.decimal_delimiter");
            auto table=parseTable(request["text"].toString(),delimiter[0]);if(!table)throw Failure(table.error);
            const int xi=request["xColumn"].toInt(1),yi=request["yColumn"].toInt(2),ii=request["idColumn"].toInt(0);
            if(xi<0||yi<0||xi==yi||ii==xi||ii==yi||ii< -1||qMax(xi,qMax(yi,ii))>127)throw Failure("input.columns");
            if(request["header"].toBool(true))headers=table.rows.takeFirst();
            for(const auto &fields:table.rows){QJsonArray values;for(const auto &field:fields)values.append(field);
                inputs.append(QJsonObject{{"id",ii>=0&&ii<fields.size()?fields[ii]:QString::number(inputs.size()+1)},
                    {"x",xi<fields.size()?fields[xi]:QString()},{"y",yi<fields.size()?fields[yi]:QString()},{"fields",values}});
            }
        }
        if(inputs.isEmpty()||inputs.size()>10000)throw Failure("input.row_count");
        QJsonArray results,operations;QHash<QString,int> ids;int succeeded=0,failed=0;
        for(qsizetype i=0;i<inputs.size();++i){
            if(!inputs[i].isObject())throw Failure("input.request");auto row=inputs[i].toObject();row["row"]=i+1;
            const auto xText=row["x"].toString(),yText=row["y"].toString();const bool angular=sourceInfo["geographic"].toBool();
            const auto x=angular?parseAngle(xText,true,format,decimal[0]):parseNumber(xText,decimal[0]);
            const auto y=angular?parseAngle(yText,false,format,decimal[0]):parseNumber(yText,decimal[0]);
            try{
                if(!x)throw Failure(x.error);if(!y)throw Failure(y.error);
                if(!inArea(sourceInfo,point(sourceToGeo.get(),x.value,y.value)))throw Failure("input.outside_source_area");
                if(!inArea(targetInfo,point(sourceToTargetGeo.get(),x.value,y.value)))throw Failure("input.outside_target_area");
                const auto output=point(operation.get(),x.value,y.value);
                if(!inArea(targetInfo,point(targetToGeo.get(),output.v[0],output.v[1])))throw Failure("input.outside_target_area");
                auto actual=own(a.proj_trans_get_last_used_operation(operation.get()));const auto details=provenance(actual?actual.get():operation.get());
                const auto digest=QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(details).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex());
                if(!ids.contains(digest)){ids[digest]=int(operations.size());operations.append(details);}
                row["operation"]=ids[digest];row["inputX"]=x.value;row["inputY"]=y.value;row["resultX"]=output.v[0];row["resultY"]=output.v[1];
                row["outputX"]=targetInfo["geographic"].toBool()?formatAngle(output.v[0],true,outputFormat,precision):QString::number(output.v[0],'f',precision);
                row["outputY"]=targetInfo["geographic"].toBool()?formatAngle(output.v[1],false,outputFormat,precision):QString::number(output.v[1],'f',precision);
                row["status"]="ok";row["error"]="";++succeeded;
            }catch(const Failure &failure){row["status"]="error";row["error"]=failure.code;++failed;}
            results.append(row);
            if(i%100==0||i+1==inputs.size())publish({{"type","progress"},{"value",double(i+1)/inputs.size()}});
        }
        QJsonArray originalHeaders;for(const auto &h:headers)originalHeaders.append(h);
        return {{"type","result"},{"ok",true},{"op","convert"},{"source",sourceInfo},{"target",targetInfo},{"engine",version()},
            {"rows",results},{"headers",originalHeaders},{"operations",operations},{"succeeded",succeeded},{"failed",failed},
            {"inputFormat",format},{"outputFormat",outputFormat},{"decimal",decimal},{"precision",precision},
            {"axisConvention","X=longitude/easting; Y=latitude/northing"},{"createdAt",QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)}};
    }
};
}
int main(int argc,char **argv){
    QCoreApplication app(argc,argv);QCommandLineParser parser;parser.addOption({"engine-dir","Bundled engine directory","path"});parser.process(app);
    try{
        QFile input;if(!input.open(stdin,QIODevice::ReadOnly))throw Failure("input.request");const auto bytes=input.read(8*1024*1024+1);
        if(bytes.size()>8*1024*1024)throw Failure("input.too_large");QJsonParseError parseError;const auto document=QJsonDocument::fromJson(bytes,&parseError);
        if(parseError.error!=QJsonParseError::NoError||!document.isObject())throw Failure("input.request");const auto request=document.object();
        if(request["schemaVersion"].toDouble()!=1)throw Failure("input.request");
        const auto directory=parser.isSet("engine-dir")?parser.value("engine-dir"):QDir(app.applicationDirPath()).filePath("modules/coordinates/engine");
        Engine engine(directory);const auto op=request["op"].toString();
        if(op=="search"){
            if((request.contains("offset")&&(!request["offset"].isDouble()||request["offset"].toDouble()!=request["offset"].toInt(-1)))||
               (request.contains("group")&&!request["group"].isString()))throw Failure("input.request");
            publish(engine.search(request["query"].toString(),request["group"].toString(),request["offset"].toInt()));
        }
        else if(op=="describe")publish({{"type","result"},{"ok",true},{"op",op},{"source",engine.info(request["source"].toString())},{"target",engine.info(request["target"].toString())},{"engine",engine.version()}});
        else if(op=="convert")publish(engine.convert(request));else throw Failure("input.request");return 0;
    }catch(const Failure &failure){publish({{"type","error"},{"ok",false},{"code",failure.code}});return 2;}
    catch(const std::exception &){publish({{"type","error"},{"ok",false},{"code","engine.failure"}});return 3;}
}
