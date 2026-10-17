#pragma once
#include <QObject>
#include <QSettings>

namespace workbench {
class Settings : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY changed)
    Q_PROPERTY(bool systemDark READ systemDark NOTIFY changed)
    Q_PROPERTY(bool reduceMotion READ reduceMotion WRITE setReduceMotion NOTIFY changed)
    Q_PROPERTY(bool reduceTransparency READ reduceTransparency WRITE setReduceTransparency NOTIFY changed)
    Q_PROPERTY(bool highContrast READ highContrast WRITE setHighContrast NOTIFY changed)
    Q_PROPERTY(double textScale READ textScale WRITE setTextScale NOTIFY changed)
public:
    explicit Settings(const QString &directory, QObject *parent = nullptr);
    QString theme() const;
    bool systemDark() const;
    bool reduceMotion() const;
    bool reduceTransparency() const;
    bool highContrast() const;
    double textScale() const;
    void setTheme(const QString &value);
    void setReduceMotion(bool value);
    void setReduceTransparency(bool value);
    void setHighContrast(bool value);
    void setTextScale(double value);
signals:
    void changed();
private:
    void set(const QString &key, const QVariant &value);
    QSettings m_settings;
};
}
