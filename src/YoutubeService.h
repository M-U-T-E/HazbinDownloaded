#ifndef YOUTUBESERVICE_H
#define YOUTUBESERVICE_H

#include "YtDlp.h"
#include <QProcess>
#include <QVariantMap>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class ToolsManager; // Forward declaration
class SettingsManager; // Forward declaration

class YoutubeService : public YtDlp
{
    Q_OBJECT
public:
    explicit YoutubeService(ToolsManager *toolsManager, SettingsManager *settingsManager, QObject *parent = nullptr);

    Q_INVOKABLE void execute(const QString &program, const QStringList &arguments) override;
    Q_INVOKABLE void fetchVideoInfo(const QString &url);
    Q_INVOKABLE void download(const QString &url, const QVariantMap &videoFormat, const QVariantMap &audioFormat);

signals:
    void downloadProgress(double progress);
    void downloadFinished(const QString &filePath);
    void downloadError(const QString &errorString);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessErrorOccurred(QProcess::ProcessError error);
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();
    void onJsonParseTimeout();
    void onThumbnailDownloaded(QNetworkReply *reply); // New slot for thumbnail download

    void onDownloadProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onDownloadProcessErrorOccurred(QProcess::ProcessError error);
    void onReadyReadDownloadProcessStandardOutput();
    void onReadyReadDownloadProcessStandardError();

private:
    void parseAndEmitVideoInfo();

    QProcess *m_process;
    QProcess *m_downloadProcess;
    ToolsManager *m_toolsManager;
    SettingsManager *m_settingsManager;
    QByteArray m_outputBuffer;
    QStringList m_lastArguments;

    QTimer *m_jsonParseTimer;
    bool m_parsingInitiated;

    QNetworkAccessManager *m_thumbnailManager; // New network manager for thumbnails
    QVariantMap m_pendingVideoInfo; // To hold info while thumbnail downloads
    QString m_downloadFilePath;
};

#endif // YOUTUBESERVICE_H
