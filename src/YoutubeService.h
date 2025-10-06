#ifndef YOUTUBESERVICE_H
#define YOUTUBESERVICE_H

#include "YtDlp.h"
#include <QProcess>
#include <QVariantMap>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class ToolsManager; // Forward declaration

class YoutubeService : public YtDlp
{
    Q_OBJECT
public:
    explicit YoutubeService(ToolsManager *toolsManager, QObject *parent = nullptr);

    Q_INVOKABLE void execute(const QString &program, const QStringList &arguments) override;
    Q_INVOKABLE void fetchVideoInfo(const QString &url);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessErrorOccurred(QProcess::ProcessError error);
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();
    void onJsonParseTimeout();
    void onThumbnailDownloaded(QNetworkReply *reply); // New slot for thumbnail download

private:
    void parseAndEmitVideoInfo();

    QProcess *m_process;
    ToolsManager *m_toolsManager;
    QByteArray m_outputBuffer;
    QStringList m_lastArguments;

    QTimer *m_jsonParseTimer;
    bool m_parsingInitiated;

    QNetworkAccessManager *m_thumbnailManager; // New network manager for thumbnails
    QVariantMap m_pendingVideoInfo; // To hold info while thumbnail downloads
};

#endif // YOUTUBESERVICE_H
