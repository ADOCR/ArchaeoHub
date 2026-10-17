#include "CoordinateController.h"
#include "CoordinateText.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QSaveFile>
#include <QTemporaryFile>
#include <QJsonDocument>
#include <QProcessEnvironment>
#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

namespace workbench::coordinates {
QString coordinateError(const QString &code){
    const auto tr=[](const char *s){return QCoreApplication::translate("Coordinates",s);};
    if(code=="input.empty")return tr("Falta una coordenada.");
    if(code=="input.crs_group"||code=="input.crs_offset"||code=="input.query")return tr("Revisa el grupo, la búsqueda o la página de sistemas de coordenadas.");
    if(code=="input.empty_table")return tr("Pega o importa una tabla con coordenadas.");
    if(code=="input.decimal_separator"||code=="input.decimal_delimiter")return tr("Revisa el separador decimal y el de columnas; deben distinguirse.");
    if(code=="input.hemisphere"||code=="input.axis_hemisphere")return tr("El hemisferio no corresponde al eje o está repetido.");
    if(code=="input.sign_hemisphere")return tr("El signo contradice el hemisferio indicado.");
    if(code=="input.minutes"||code=="input.seconds")return tr("Minutos y segundos deben estar entre 0 y menos de 60.");
    if(code=="input.fractional_degrees"||code=="input.fractional_minutes"||code=="input.angle_format")return tr("La coordenada no coincide con el formato GD/GDM/GMS seleccionado.");
    if(code=="input.angular_range")return tr("Longitud fuera de ±180° o latitud fuera de ±90°.");
    if(code=="input.invalid_number"||code=="input.non_finite"||code=="input.invalid_angle")return tr("Introduce un número finito válido, sin separadores de miles.");
    if(code=="input.outside_source_area")return tr("Punto fuera de la envolvente de uso del CRS de origen.");
    if(code=="input.outside_target_area")return tr("Punto fuera de la envolvente de uso del CRS de destino. Revisa también la zona UTM.");
    if(code=="input.columns")return tr("Selecciona columnas distintas para ID, X y Y; usa 0 para omitir ID.");
    if(code=="input.csv_quote")return tr("La tabla contiene comillas sin cerrar o campos CSV incorrectos.");
    if(code=="input.too_large"||code=="input.row_count")return tr("El lote admite hasta 10.000 puntos y archivos de hasta 8 MB.");
    if(code=="crs.invalid_code"||code=="crs.not_found")return tr("Selecciona un CRS mediante un código EPSG válido. CRTM90 requiere una definición verificada.");
    if(code=="crs.dimension")return tr("Esta versión trabaja con CRS horizontales 2D. No transforma alturas ni coordenadas 3D.");
    if(code=="crs.deprecated")return tr("El CRS está retirado. Revisa su sustituto en la base EPSG.");
    if(code=="crs.epoch_required")return tr("Este datum dinámico requiere una época de coordenadas; todavía no está admitido.");
    if(code.startsWith("crs."))return tr("Los ejes, unidades o área de este CRS requieren una configuración aún no admitida.");
    if(code=="operation.unavailable"||code=="operation.grid_missing")return tr("No hay una operación fiable disponible sin conexión. Puede faltar una rejilla requerida; no se utilizará una aproximación automática.");
    if(code=="operation.point_failed")return tr("PROJ no pudo transformar este punto. Revisa valores, CRS y rejillas disponibles.");
    if(code.startsWith("operation."))return tr("No se pudo verificar la operación y su procedencia.");
    if(code=="file.exists")return tr("El archivo ya existe. Elige otro nombre para conservarlo.");
    if(code=="file.source")return tr("El archivo de entrada se conserva; elige otro destino.");
    if(code=="file.read_failed")return tr("No se pudo leer el archivo. Revisa formato, unidad y permisos.");
    if(code=="file.write_failed")return tr("No se pudo guardar el archivo. Revisa espacio, unidad y permisos.");
    if(code=="file.xlsx_formula_cache")return tr("El Excel contiene fórmulas sin resultado guardado. Ábrelo en Excel o LibreOffice, recalcula y guarda antes de importarlo. El Hub no ejecuta fórmulas.");
    if(code=="file.xlsx_limit")return tr("El Excel admite hasta 10.000 puntos, 128 columnas y 32 MB de archivo. Cada celda exportada admite hasta 32.767 caracteres.");
    if(code.startsWith("file.xlsx"))return tr("No se pudo leer o guardar el libro .xlsx. Revisa la hoja y sus celdas; el archivo original y los resultados anteriores se conservan.");
    if(code=="job.cancelled")return tr("Conversión cancelada. Se conservan la entrada y los resultados anteriores.");
    if(code=="job.timeout")return tr("La operación superó el tiempo permitido. Puedes reducir el lote y reintentar.");
    if(code.startsWith("engine."))return tr("No se pudo iniciar el motor geodésico incluido. Revisa la instalación de ArchaeoHub.");
    return tr("No se pudo completar la operación. Revisa los datos y consulta el código en detalles.");
}
QHash<int,QByteArray> CoordinateRows::roleNames() const{return {{Qt::UserRole+1,"pointId"},{Qt::UserRole+2,"inputX"},{Qt::UserRole+3,"inputY"},
    {Qt::UserRole+4,"outputX"},{Qt::UserRole+5,"outputY"},{Qt::UserRole+6,"valid"},{Qt::UserRole+7,"errorText"},{Qt::UserRole+8,"errorCode"},{Qt::UserRole+9,"rowNumber"}};}
QVariant CoordinateRows::data(const QModelIndex &index,int role) const{
    if(!index.isValid()||index.row()<0||index.row()>=m_rows.size())return {};const auto row=m_rows[index.row()].toObject();
    switch(role){case Qt::UserRole+1:return row["id"].toString();case Qt::UserRole+2:return row["x"].toString();case Qt::UserRole+3:return row["y"].toString();
    case Qt::UserRole+4:return row["outputX"].toString();case Qt::UserRole+5:return row["outputY"].toString();case Qt::UserRole+6:return row["status"]=="ok";
    case Qt::UserRole+7:return row["status"]=="ok"?QString():coordinateError(row["error"].toString());case Qt::UserRole+8:return row["error"].toString();case Qt::UserRole+9:return row["row"].toInt();default:return {};}
}
CoordinateController::CoordinateController(TaskManager &tasks,const QString &directory,QObject *parent)
    :QObject(parent),m_tasks(tasks),m_preferences(QDir(directory).filePath("coordinate-preferences.ini"),QSettings::IniFormat),m_rows(this){
    m_sourceCode=m_preferences.value("source","EPSG:4326").toString();m_targetCode=m_preferences.value("target","EPSG:8908").toString();
    connect(&m_process,&QProcess::readyReadStandardOutput,this,&CoordinateController::receive);
    connect(&m_process,&QProcess::readyReadStandardError,this,[this]{m_process.readAllStandardError();});
    connect(&m_process,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,&CoordinateController::finish);
    connect(&m_process,&QProcess::errorOccurred,this,[this](QProcess::ProcessError e){if(e==QProcess::FailedToStart)finish(-1,QProcess::CrashExit);});
    m_deadline.setSingleShot(true);connect(&m_deadline,&QTimer::timeout,this,[this]{m_code="job.timeout";m_process.kill();});
#ifdef Q_OS_WIN
    m_process.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *args){args->flags|=CREATE_NO_WINDOW;});
#endif
}
CoordinateController::~CoordinateController(){disconnect(&m_process,nullptr,this,nullptr);if(m_process.state()!=QProcess::NotRunning){m_process.kill();m_process.waitForFinished(500);}if(!m_task.isEmpty())m_tasks.endExternal(m_task,"interrupted");}
QVariantMap CoordinateController::summary() const{auto o=m_result;o.remove("rows");return o.toVariantMap();}
QVariantList CoordinateController::recentCrs() const{QVariantList rows;for(const auto &c:m_preferences.value("recent").toStringList())rows<<c;return rows;}
void CoordinateController::remember(const QString &code){auto recent=m_preferences.value("recent").toStringList();recent.removeAll(code);recent.prepend(code);m_preferences.setValue("recent",recent.mid(0,12));}
void CoordinateController::notice(const QString &code,bool error){m_code=code;m_error=error;m_message=coordinateError(code);emit changed();}
void CoordinateController::initialize(){if(m_source.isEmpty()&&!m_busy)describe(m_sourceCode,m_targetCode);}
void CoordinateController::describe(const QString &source,const QString &target){request({{"op","describe"},{"source",source},{"target",target}});}
void CoordinateController::search(const QString &query,const QString &group,int offset){request({{"op","search"},{"query",query},{"group",group},{"offset",offset}});}
bool CoordinateController::request(QJsonObject request){
    if(m_busy)return false;request["schemaVersion"]=1;const auto op=request["op"].toString();
    if(op=="convert"){if(!request.contains("source"))request["source"]=m_sourceCode;if(!request.contains("target"))request["target"]=m_targetCode;}
    const bool spreadsheet=op=="readSpreadsheet"||op=="writeSpreadsheet";
    const auto executable=QDir(QCoreApplication::applicationDirPath()).filePath(spreadsheet?
#ifdef Q_OS_WIN
        "modules/coordinates/spreadsheets/SpreadsheetWorker.exe":
#else
        "modules/coordinates/spreadsheets/SpreadsheetWorker":
#endif
#ifdef Q_OS_WIN
        "CoordinateWorker.exe"
#else
        "CoordinateWorker"
#endif
    );
    const auto engine=QDir(QCoreApplication::applicationDirPath()).filePath("modules/coordinates/engine");
    if(!QFileInfo::exists(executable)){notice("engine.not_found");return false;}
    auto environment=QProcessEnvironment::systemEnvironment();environment.insert("PROJ_NETWORK","OFF");
    environment.remove("PROJ_LIB");environment.remove("PROJ_DATA");environment.remove("PYTHONHOME");environment.remove("PYTHONPATH");environment.insert("PATH",engine+QDir::listSeparator()+environment.value("PATH"));
    m_process.setProcessEnvironment(environment);m_process.setProgram(executable);m_process.setArguments(spreadsheet?QStringList{}:QStringList{"--engine-dir",engine});
    m_busy=true;m_cancelled=false;m_progress=0;m_error=false;m_message.clear();m_code.clear();m_buffer.clear();m_reply={};m_operation=op;
    if(op=="convert")m_task=m_tasks.beginExternal(tr("Conversión de coordenadas"),"coordinates.convert",[this]{cancel();});
    if(spreadsheet)m_task=m_tasks.beginExternal(op=="readSpreadsheet"?tr("Importar Excel"):tr("Exportar Excel"),"coordinates."+op,[this]{cancel();});
    m_process.start();m_process.write(QJsonDocument(request).toJson(QJsonDocument::Compact));m_process.closeWriteChannel();m_deadline.start(op=="convert"?180000:30000);emit changed();return true;
}
void CoordinateController::convertSingle(const QString &id,const QString &x,const QString &y,const QVariantMap &options){auto r=QJsonObject::fromVariantMap(options);r["op"]="convert";r["rows"]=QJsonArray{QJsonObject{{"id",id},{"x",x},{"y",y}}};request(r);}
void CoordinateController::convertBatch(const QVariantMap &options){auto r=QJsonObject::fromVariantMap(options);r["op"]="convert";r["text"]=m_tableText;request(r);}
void CoordinateController::receive(){
    m_buffer+=m_process.readAllStandardOutput();if(m_buffer.size()>32*1024*1024){m_code="input.too_large";m_process.kill();return;}
    qsizetype newline=0;while((newline=m_buffer.indexOf('\n'))>=0){const auto bytes=m_buffer.left(newline);m_buffer.remove(0,newline+1);const auto o=QJsonDocument::fromJson(bytes).object();
        if(o["type"]=="progress"){m_progress=o["value"].toDouble();if(!m_task.isEmpty())m_tasks.externalProgress(m_task,m_progress);emit changed();}
        else if(o["type"]=="result"||o["type"]=="error")m_reply=o;
    }
}
void CoordinateController::finish(int exitCode,QProcess::ExitStatus status){
    if(!m_busy)return;m_deadline.stop();receive();m_busy=false;
    const bool ok=!m_cancelled&&exitCode==0&&status==QProcess::NormalExit&&m_reply["ok"].toBool();
    const bool converted=ok&&m_operation=="convert";
    bool showSheets=false,imported=false;
    if(ok){
        if(m_operation=="readSpreadsheet"){
            m_sheets.clear();for(const auto &sheet:m_reply["sheets"].toArray())m_sheets<<sheet.toString();
            m_sheetPath=m_reply["path"].toString();
            if(m_reply.contains("text")){m_tableText=m_reply["text"].toString();m_inputFile=m_sheetPath;m_inputSheet=m_reply["sheet"].toString();m_spreadsheetInput={{"path",m_inputFile},{"sheet",m_inputSheet},{"cachedFormulas",m_reply["cachedFormulas"]}};m_message=tr("Excel importado. Confirma encabezados, columnas y CRS antes de convertir.");imported=true;}
            else{m_message=tr("Selecciona la hoja del libro Excel.");showSheets=true;}
        }else if(m_operation=="writeSpreadsheet")m_message=tr("Excel guardado con coordenadas numéricas, columnas originales y hojas de procedencia.");
        else if(m_operation=="search"){m_search=m_reply["matches"].toArray();m_searchTotal=m_reply["total"].toInt();m_searchOffset=m_reply["offset"].toInt();m_message=tr("%1 sistemas encontrados · resultados %2–%3.").arg(m_searchTotal).arg(m_search.isEmpty()?0:m_searchOffset+1).arg(m_searchOffset+m_search.size());}
        else {m_source=m_reply["source"].toObject();m_target=m_reply["target"].toObject();m_sourceCode=m_source["code"].toString();m_targetCode=m_target["code"].toString();
            m_engine=m_reply["engine"].toObject();
            m_preferences.setValue("source",m_sourceCode);m_preferences.setValue("target",m_targetCode);remember(m_sourceCode);remember(m_targetCode);m_preferences.sync();
            if(m_operation=="convert"){m_result=m_reply;m_rows.replace(m_result["rows"].toArray());m_message=tr("%1 puntos convertidos · %2 con errores.").arg(m_result["succeeded"].toInt()).arg(m_result["failed"].toInt());}
        }
        m_code.clear();m_error=false;
    }else{const auto code=m_cancelled?QString("job.cancelled"):!m_code.isEmpty()?m_code:m_reply["code"].toString("engine.failure");notice(code);}
    if(!m_task.isEmpty()){const auto task=m_task;m_task.clear();m_tasks.endExternal(task,m_cancelled?"cancelled":ok?"succeeded":"failed",ok?m_message:QString(),ok?QString():m_code);}
    emit changed();if(converted)emit conversionCompleted();if(showSheets)emit spreadsheetSheetsAvailable();if(imported)emit spreadsheetImported();
}
void CoordinateController::cancel(){if(!m_busy)return;m_cancelled=true;m_process.kill();}
void CoordinateController::importText(const QUrl &file){
    if(QFileInfo(file.toLocalFile()).suffix().compare("xlsx",Qt::CaseInsensitive)==0){importSpreadsheet(file);return;}
    if(m_busy||!file.isLocalFile())return;QFile input(file.toLocalFile());
    if(!input.open(QIODevice::ReadOnly)){notice("file.read_failed");return;}
    const auto bytes=input.read(8*1024*1024+1);if(bytes.size()>8*1024*1024){notice("input.too_large");return;}
    const auto text=QString::fromUtf8(bytes);if(text.contains(QChar::ReplacementCharacter)||text.contains(QChar(0))){notice("file.read_failed");return;}
    m_tableText=text;m_inputFile=QFileInfo(file.toLocalFile()).absoluteFilePath();m_inputSheet.clear();m_spreadsheetInput={};m_message=tr("Tabla importada. Confirma separadores, columnas y CRS antes de convertir.");m_error=false;m_code.clear();emit changed();
}
void CoordinateController::importSpreadsheet(const QUrl &file,const QString &sheet){if(file.isLocalFile())request({{"op","readSpreadsheet"},{"path",file.toLocalFile()},{"sheet",sheet}});}
void CoordinateController::selectSheet(const QString &sheet){if(m_sheets.contains(sheet))importSpreadsheet(QUrl::fromLocalFile(m_sheetPath),sheet);}
QString CoordinateController::exportText(const QString &format) const{
    if(m_result.isEmpty())return {};if(format=="json")return QString::fromUtf8(QJsonDocument(m_result).toJson());
    return format=="csv"?csvText(true):QString();
}
QString CoordinateController::csvText(bool protectFormulas) const{
    QStringList lines;auto headers=m_result["headers"].toArray();int originalColumns=headers.size();
    for(const auto &value:m_result["rows"].toArray())originalColumns=qMax(originalColumns,int(value.toObject()["fields"].toArray().size()));
    QStringList names;for(int i=0;i<originalColumns;++i)names<<quoteCsv(i<headers.size()?headers[i].toString():tr("columna_%1").arg(i+1),protectFormulas);
    names<<QStringList{"id","input_x_text","input_y_text","result_x","result_y","formatted_x","formatted_y","status","error","source_crs","target_crs","datum_source","datum_target","x_unit","y_unit","operation_index","operation_name","accuracy_metres","proj_version","epsg_version"};lines<<names.join(',');
    for(const auto &value:m_result["rows"].toArray()){const auto row=value.toObject();QStringList cells;const auto fields=row["fields"].toArray();for(int i=0;i<originalColumns;++i)cells<<quoteCsv(i<fields.size()?fields[i].toString():QString(),protectFormulas);
        const auto opIndex=row["operation"].toInt(-1);const auto operations=m_result["operations"].toArray();const auto op=opIndex>=0&&opIndex<operations.size()?operations[opIndex].toObject():QJsonObject();
        const auto source=m_result["source"].toObject(),target=m_result["target"].toObject(),engine=m_result["engine"].toObject();
        const QStringList metadata{row["id"].toString(),row["x"].toString(),row["y"].toString(),row.contains("resultX")?QString::number(row["resultX"].toDouble(),'g',17):QString(),row.contains("resultY")?QString::number(row["resultY"].toDouble(),'g',17):QString(),row["outputX"].toString(),row["outputY"].toString(),row["status"].toString(),row["error"].toString(),source["code"].toString(),target["code"].toString(),source["datum"].toString(),target["datum"].toString(),target["xUnit"].toString(),target["yUnit"].toString(),opIndex>=0?QString::number(opIndex):QString(),op["name"].toString(),op["accuracyMetres"].isDouble()?QString::number(op["accuracyMetres"].toDouble(),'g',12):QString(),engine["version"].toString(),engine["epsgVersion"].toString()};
        for(const auto &cell:metadata)cells<<quoteCsv(cell,protectFormulas);lines<<cells.join(',');
    }return lines.join("\r\n")+"\r\n";
}
void CoordinateController::exportFile(const QUrl &file,const QString &format){
    if(m_busy||!file.isLocalFile()||m_result.isEmpty())return;const auto path=QFileInfo(file.toLocalFile()).absoluteFilePath();
    if(path.compare(m_inputFile,
#ifdef Q_OS_WIN
        Qt::CaseInsensitive
#else
        Qt::CaseSensitive
#endif
    )==0){notice("file.source");return;}
    if(QFileInfo::exists(path)||QFileInfo(path).isSymLink()){notice("file.exists");return;}
    if(format=="xlsx"){request({{"op","writeSpreadsheet"},{"path",path},{"csv",csvText(false)},{"result",m_result},{"inputSpreadsheet",m_spreadsheetInput}});return;}
    const auto text=exportText(format);if(text.isEmpty())return;
    // Renombrar dentro del mismo directorio rechaza un nombre final existente, incluso si el archivo
    // se creó después de la comprobación inicial. QSaveFile::commit lo sustituiría.
    QTemporaryFile target(QDir(QFileInfo(path).absolutePath()).filePath(".archaeohub-export-XXXXXX.tmp"));const auto bytes=text.toUtf8();
    if(!target.open()||target.write(bytes)!=bytes.size()||!target.flush()){notice("file.write_failed");return;}
    target.close();if(!target.rename(path)){notice(QFileInfo::exists(path)?"file.exists":"file.write_failed");return;}target.setAutoRemove(false);
    m_message=tr("Resultados guardados. La tabla de entrada se conserva.");m_error=false;m_code.clear();emit changed();
}
void CoordinateController::example(){if(m_busy)return;m_tableText="id\tx\ty\nP-001\t-84.0\t10.0\nP-002\t-83.9\t9.9\n";m_inputFile.clear();emit changed();}
}
