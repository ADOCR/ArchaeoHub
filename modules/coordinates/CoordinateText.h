#pragma once
#include <QStringList>
#include <QList>
namespace workbench::coordinates {
struct NumberResult { double value=0; QString error; explicit operator bool() const{return error.isEmpty();} };
struct TableResult { QList<QStringList> rows; QString error; explicit operator bool() const{return error.isEmpty();} };
NumberResult parseNumber(QString text,QChar decimal='.');
NumberResult parseAngle(QString text,bool longitude,const QString &format,QChar decimal='.');
QString formatAngle(double value,bool longitude,const QString &format,int precision);
TableResult parseTable(const QString &text,QChar delimiter);
QString quoteCsv(QString text,bool protectFormulas=false);
}
