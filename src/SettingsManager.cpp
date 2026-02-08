#include "SettingsManager.h"

SettingsManager::SettingsManager(QObject *parent)
    : QObject(parent), m_settings("YourOrganization", "HazbinDownloaded")
{
}

void SettingsManager::saveCookiePath(const QString &path)
{
    m_settings.setValue("cookiePath", path);
}

QString SettingsManager::loadCookiePath() const
{
    return m_settings.value("cookiePath", "").toString();
}

void SettingsManager::saveDenoPath(const QString &path)
{
    m_settings.setValue("denoPath", path);
}

QString SettingsManager::loadDenoPath() const
{
    return m_settings.value("denoPath", "").toString();
}
