#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQmlContext>

#include "ToolsManager.h"
#include "YoutubeService.h"

int main(int argc, char *argv[])
{
    QQuickStyle::setStyle("Material");
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    ToolsManager toolsManager;
    engine.rootContext()->setContextProperty("toolsManager", &toolsManager);

    YoutubeService youtubeService(&toolsManager);
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
