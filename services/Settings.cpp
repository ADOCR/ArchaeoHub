#include "Settings.h"
#include <QGuiApplication>
#include <QStyleHints>
#include <QDir>
#include <cmath>

namespace workbench {
Settings::Settings(const QString &directory, QObject *parent)
    : QObject(parent), m_settings(QDir(directory).filePath("preferences.ini"),QSettings::IniFormat) {
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this, &Settings::changed);
}
QString Settings::theme() const { return m_settings.value("appearance/theme","auto").toString(); }
bool Settings::systemDark() const { return QGuiApplication::styleHints()->colorScheme()==Qt::ColorScheme::Dark; }
bool Settings::reduceMotion() const { return m_settings.value("appearance/reduceMotion",false).toBool(); }
bool Settings::reduceTransparency() const { return m_settings.value("appearance/reduceTransparency",false).toBool(); }
bool Settings::highContrast() const { return m_settings.value("appearance/highContrast",false).toBool(); }
double Settings::textScale() const { return qBound(0.9,m_settings.value("appearance/textScale",1.0).toDouble(),1.4); }
void Settings::set(const QString &key,const QVariant &value) { if(m_settings.value(key)!=value){m_settings.setValue(key,value);m_settings.sync();emit changed();} }
void Settings::setTheme(const QString &value){ if(QStringList{"auto","light","dark"}.contains(value))set("appearance/theme",value); }
void Settings::setReduceMotion(bool value){set("appearance/reduceMotion",value);}
void Settings::setReduceTransparency(bool value){set("appearance/reduceTransparency",value);}
void Settings::setHighContrast(bool value){set("appearance/highContrast",value);}
void Settings::setTextScale(double value){if(std::isfinite(value))set("appearance/textScale",qBound(0.9,value,1.4));}
}
