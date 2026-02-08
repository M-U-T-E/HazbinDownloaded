#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQmlContext>

#include "ToolsManager.h"
#include "YoutubeService.h"
#include "SettingsManager.h"

int main(int argc, char *argv[])
{
    QQuickStyle::setStyle("Material");
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    ToolsManager toolsManager;
    engine.rootContext()->setContextProperty("toolsManager", &toolsManager);

    SettingsManager settingsManager;
    engine.rootContext()->setContextProperty("settingsManager", &settingsManager);

    YoutubeService youtubeService(&toolsManager, &settingsManager);
    engine.rootContext()->setContextProperty("youtubeService", &youtubeService);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("HazbinDownloaded", "Main");

    return app.exec();
}
