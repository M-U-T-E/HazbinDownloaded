#ifndef TOOLSMANAGER_H
#define TOOLSMANAGER_H

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTemporaryDir>
#include <QProcess>
#include <QQueue>
#include <QPair>

class ToolsManager : public QObject
{
    Q_OBJECT
public:
    explicit ToolsManager(QObject *parent = nullptr);
    ~ToolsManager();

    Q_INVOKABLE QString ytDlpPath() const;
    Q_INVOKABLE QString ffmpegPath() const;
    Q_INVOKABLE QString denoPath() const;

public slots:
    void startDownload(const QString &toolName, const QString &urlString, const QString &fileExtension);
    void checkToolsOnStartup();
    void checkYtDlpVersion(); // New: Initiates yt-dlp version check

signals:
    void processStarted(); // Emitted once at the beginning of the entire queue processing
    void progressChanged(int percentage, const QString &description);
    void processFinished(bool success); // Emitted once at the end of the entire queue processing

    void ffmpegStatus(bool found);
    void ytDlpStatus(bool found);
    void denoStatus(bool found);

    // New: Signals for yt-dlp version information
    void localYtDlpVersion(const QString &version);
    void latestYtDlpVersion(const QString &version);
    void ytDlpUpdateAvailable(bool available);

private slots:
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished(QNetworkReply *reply);
    void onTarProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onTarProcessError(QProcess::ProcessError error);

    // New: Slots for yt-dlp version checking
    void onYtDlpVersionProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onYtDlpLatestReleaseFinished(QNetworkReply *reply);
    void processNextDownload();

private:
    void extractZipArchive(const QString &zipPath);
    void extractDenoZipArchive(const QString &zipPath);
    void extractTarXzArchive(const QString &tarXzPath);
    void installYtDlpBinary(const QString &downloadedFilePath);
    int compareVersions(const QString &v1, const QString &v2); // Helper for version comparison
    QString executeCommand(const QString &program, const QStringList &arguments);
    QString getInstalledFFmpegVersion();

    QNetworkAccessManager *m_networkManager;
    QTemporaryDir *m_currentTempDir;
    QString m_currentToolName;
    QString m_currentFileExtension;
    QProcess *m_tarProcess;

    QProcess *m_ytDlpVersionProcess; // New: For running yt-dlp --version
    QNetworkAccessManager *m_githubApiManager; // New: For GitHub API requests
    QString m_localYtDlpVersion; // New: Stores local version for comparison

    QQueue<QPair<QString, QStringList>> m_downloadQueue;
    bool m_overallProcessRunning; // New: Tracks if the overall queue is being processed
    bool m_overallSuccess;        // New: Tracks overall success of the queue

    QString m_ytDlpPath;
    QString m_ffmpegPath;
    QString m_denoPath;
};

#endif // TOOLSMANAGER_H
