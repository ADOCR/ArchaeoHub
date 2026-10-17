#include "CoordinateText.h"
#include <QRegularExpression>
#include <cmath>

namespace workbench::coordinates {
NumberResult parseNumber(QString text,QChar decimal){
    text=text.trimmed();if(text.isEmpty())return {0,"input.empty"};
    if(text.size()>120)return {0,"input.invalid_number"};
    if(decimal!=QChar('.')&&decimal!=QChar(','))return {0,"input.decimal_mode"};
    if(text.contains(decimal==QChar('.')?QChar(','):QChar('.')))return {0,"input.decimal_separator"};
    if(decimal==QChar(','))text.replace(',','.');
    static const QRegularExpression numeric("^[+-]?(?:[0-9]+(?:\\.[0-9]*)?|\\.[0-9]+)(?:[eE][+-]?[0-9]+)?$");
    if(!numeric.match(text).hasMatch())return {0,"input.invalid_number"};
    bool ok=false;const auto value=text.toDouble(&ok);
    if(!ok||!std::isfinite(value))return {0,"input.non_finite"};
    return {value,{}};
}
NumberResult parseAngle(QString text,bool longitude,const QString &format,QChar decimal){
    text=text.trimmed().toUpper();if(text.isEmpty())return {0,"input.empty"};
    if(text.size()>200)return {0,"input.invalid_angle"};
    QChar hemisphere;
    const QString letters="NSEWO";
    if(letters.contains(text.front())){hemisphere=text.front();text.remove(0,1);}
    if(!text.isEmpty()&&letters.contains(text.back())){
        if(!hemisphere.isNull())return {0,"input.hemisphere"};hemisphere=text.back();text.chop(1);
    }
    text=text.trimmed();if(text.isEmpty())return {0,"input.empty"};
    if(!hemisphere.isNull()&&!(longitude?QString("EWO"):QString("NS")).contains(hemisphere))return {0,"input.axis_hemisphere"};
    const bool negative=text.startsWith('-'),positive=text.startsWith('+');
    const bool negativeHemisphere=QString("SWO").contains(hemisphere)&&!hemisphere.isNull();
    if(!hemisphere.isNull()&&((negative&&!negativeHemisphere)||(positive&&negativeHemisphere)))return {0,"input.sign_hemisphere"};
    for(const auto c:QString::fromUtf8("°º′’'″\""))text.replace(c,' ');
    const auto parts=text.simplified().split(' ',Qt::SkipEmptyParts);
    const int expected=format=="DD"?1:format=="DM"?2:format=="DMS"?3:0;
    if(!expected||parts.size()!=expected)return {0,"input.angle_format"};
    const auto degrees=parseNumber(parts[0],decimal);if(!degrees)return degrees;
    double magnitude=std::abs(degrees.value);
    if(expected>1){
        if(std::floor(magnitude)!=magnitude)return {0,"input.fractional_degrees"};
        const auto minutes=parseNumber(parts[1],decimal);if(!minutes)return minutes;
        if(minutes.value<0||minutes.value>=60)return {0,"input.minutes"};
        if(expected==3&&std::floor(minutes.value)!=minutes.value)return {0,"input.fractional_minutes"};
        magnitude+=minutes.value/60;
        if(expected==3){const auto seconds=parseNumber(parts[2],decimal);if(!seconds)return seconds;if(seconds.value<0||seconds.value>=60)return {0,"input.seconds"};magnitude+=seconds.value/3600;}
    }
    if(magnitude>(longitude?180:90))return {0,"input.angular_range"};
    return {std::copysign(magnitude,(hemisphere.isNull()?negative:negativeHemisphere)?-1.0:1.0),{}};
}
QString formatAngle(double value,bool longitude,const QString &format,int precision){
    if(!std::isfinite(value)||std::abs(value)>(longitude?180:90))return {};
    precision=qBound(0,precision,9);
    if(format=="DD")return QString::number(value,'f',precision);
    if(format!="DM"&&format!="DMS")return {};
    const auto scale=qint64(std::pow(10,precision));const auto units=format=="DMS"?3600:60;
    const auto total=qint64(std::llround(std::abs(value)*units*scale));
    const auto degrees=total/(units*scale);const auto remainder=total%(units*scale);
    const auto hemisphere=longitude?(std::signbit(value)?"W":"E"):(std::signbit(value)?"S":"N");
    if(format=="DM")return QString::fromUtf8("%1° %2′ %3").arg(degrees).arg(double(remainder)/scale,0,'f',precision).arg(hemisphere);
    const auto minutes=remainder/(60*scale);const auto seconds=double(remainder%(60*scale))/scale;
    return QString::fromUtf8("%1° %2′ %3″ %4").arg(degrees).arg(minutes).arg(seconds,0,'f',precision).arg(hemisphere);
}
TableResult parseTable(const QString &input,QChar delimiter){
    TableResult out;if(input.size()>4*1024*1024){out.error="input.too_large";return out;}
    if(delimiter!='\t'&&delimiter!=','&&delimiter!=';'){out.error="input.delimiter";return out;}
    QString text=input;if(text.startsWith(QChar(0xFEFF)))text.remove(0,1);
    QStringList row;QString field;bool quoted=false,closed=false;
    auto cell=[&]{row<<field;field.clear();closed=false;};
    auto record=[&]{cell();if(!(row.size()==1&&row.first().isEmpty()))out.rows<<row;row.clear();};
    for(qsizetype i=0;i<text.size();++i){
        const auto c=text[i];if(c.isNull()){out.error="input.invalid_character";return out;}
        if(quoted){if(c=='"'){if(i+1<text.size()&&text[i+1]=='"'){field+='"';++i;}else{quoted=false;closed=true;}}else field+=c;}
        else if(c==delimiter)cell();
        else if(c=='\n'||c=='\r'){if(c=='\r'&&i+1<text.size()&&text[i+1]=='\n')++i;record();}
        else if(c=='"'&&field.isEmpty()&&!closed)quoted=true;
        else if(closed){if(!c.isSpace()){out.error="input.csv_quote";return out;}}
        else field+=c;
        if(field.size()>4000||row.size()>128||out.rows.size()>10001){out.error="input.too_large";return out;}
    }
    if(quoted){out.error="input.csv_quote";return out;}
    if(!field.isEmpty()||!row.isEmpty()||closed)record();
    if(out.rows.isEmpty())out.error="input.empty_table";
    return out;
}
QString quoteCsv(QString text,bool protectFormulas){
    const auto first=text.trimmed();bool numeric=false;const auto value=first.toDouble(&numeric);
    if(protectFormulas&&!first.isEmpty()&&QString::fromUtf8("=+-@＝＋－＠").contains(first.front())&&!(numeric&&std::isfinite(value)))text.prepend('\'');
    text.replace('"',"\"\"");return '"'+text+'"';
}
}
