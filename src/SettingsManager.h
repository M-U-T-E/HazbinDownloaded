#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QObject>
#include <QString>
#include <QSettings>

class SettingsManager : public QObject
{
    Q_OBJECT
public:
    explicit SettingsManager(QObject *parent = nullptr);

    Q_INVOKABLE void saveCookiePath(const QString &path);
    Q_INVOKABLE QString loadCookiePath() const;

    Q_INVOKABLE void saveDenoPath(const QString &path);
    Q_INVOKABLE QString loadDenoPath() const;

private:
    QSettings m_settings;
};

#endif // SETTINGSMANAGER_H
