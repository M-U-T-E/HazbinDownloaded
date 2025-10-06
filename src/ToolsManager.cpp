#include "ToolsManager.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

#ifdef Q_OS_WIN
#define MINIZ_HEADER_FILE_ONLY
#include "../../vendor/miniz/miniz.c"
#endif

ToolsManager::ToolsManager(QObject *parent) : QObject(parent),
    m_currentTempDir(nullptr),
    m_tarProcess(nullptr),
    m_ytDlpVersionProcess(nullptr),
    m_githubApiManager(nullptr),
    m_overallProcessRunning(false),
    m_overallSuccess(true)
{
    m_networkManager = new QNetworkAccessManager(this);
    checkToolsOnStartup();
}

ToolsManager::~ToolsManager()
{
    if (m_currentTempDir) { delete m_currentTempDir; }
    if (m_tarProcess) { m_tarProcess->deleteLater(); }
    if (m_ytDlpVersionProcess) { m_ytDlpVersionProcess->deleteLater(); }
    if (m_githubApiManager) { m_githubApiManager->deleteLater(); }
}

QString ToolsManager::ytDlpPath() const
{
    return m_ytDlpPath;
}

QString ToolsManager::ffmpegPath() const
{
    return m_ffmpegPath;
}

void ToolsManager::checkToolsOnStartup()
{
    QString appPath = QCoreApplication::applicationDirPath();
    QString toolsInstallPath = appPath + "/tools"; // Common base path

    // Check for ffmpeg
    QString ffmpegExecutablePath = toolsInstallPath + (QSysInfo::productType() == "windows" ? "/ffmpeg/bin/ffmpeg.exe" : "/ffmpeg/bin/ffmpeg");
    if (QFileInfo::exists(ffmpegExecutablePath)) {
        m_ffmpegPath = ffmpegExecutablePath;
        emit ffmpegStatus(true);
    } else {
        m_ffmpegPath.clear();
        emit ffmpegStatus(false);
    }

    // Check for yt-dlp
    QString ytDlpExecutablePath = toolsInstallPath + (QSysInfo::productType() == "windows" ? "/yt-dlp.exe" : "/yt-dlp");
    if (QFileInfo::exists(ytDlpExecutablePath)) {
        m_ytDlpPath = ytDlpExecutablePath;
        emit ytDlpStatus(true);
    } else {
        m_ytDlpPath.clear();
        emit ytDlpStatus(false);
    }
}

void ToolsManager::checkYtDlpVersion()
{
    emit progressChanged(0, "Checking local yt-dlp version...");

    if (m_ytDlpVersionProcess) {
        m_ytDlpVersionProcess->deleteLater();
    }
    m_ytDlpVersionProcess = new QProcess(this);
    connect(m_ytDlpVersionProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &ToolsManager::onYtDlpVersionProcessFinished);

    if (m_ytDlpPath.isEmpty()) {
        emit localYtDlpVersion("Not Found");
        onYtDlpVersionProcessFinished(0, QProcess::NormalExit); // Simulate success to proceed to latest version check
        return;
    }

    m_ytDlpVersionProcess->start(m_ytDlpPath, {"--version"});
    if (!m_ytDlpVersionProcess->waitForStarted()) {
        emit progressChanged(100, "Error: Failed to start yt-dlp process.");
        // Don't emit processFinished here, as it's part of a larger sequence
    }
}

void ToolsManager::onYtDlpVersionProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus != QProcess::NormalExit || exitCode != 0) {
        m_localYtDlpVersion = "Error";
    } else {
        m_localYtDlpVersion = QString::fromLatin1(m_ytDlpVersionProcess->readAllStandardOutput()).trimmed();
    }
    
    if (m_localYtDlpVersion.isEmpty()) {
        m_localYtDlpVersion = "Not Found";
    }

    emit localYtDlpVersion(m_localYtDlpVersion);

    emit progressChanged(50, "Checking latest yt-dlp version...");
    if (m_githubApiManager) {
        m_githubApiManager->deleteLater();
    }
    m_githubApiManager = new QNetworkAccessManager(this);
    connect(m_githubApiManager, &QNetworkAccessManager::finished, this, &ToolsManager::onYtDlpLatestReleaseFinished);

    QNetworkRequest request(QUrl("https://api.github.com/repos/yt-dlp/yt-dlp/releases/latest"));
    m_githubApiManager->get(request);
}

void ToolsManager::onYtDlpLatestReleaseFinished(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        emit latestYtDlpVersion("Error");
        emit ytDlpUpdateAvailable(false);
        reply->deleteLater();
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    reply->deleteLater();
    QString latestVersion = doc.object()["tag_name"].toString();
    emit latestYtDlpVersion(latestVersion);

    bool updateAvailable = (m_localYtDlpVersion == "Not Found" || m_localYtDlpVersion == "Error" || compareVersions(m_localYtDlpVersion, latestVersion) < 0);
    emit ytDlpUpdateAvailable(updateAvailable);
}

int ToolsManager::compareVersions(const QString &v1, const QString &v2)
{
    QStringList parts1 = v1.split('.');
    QStringList parts2 = v2.split('.');
    int count = qMax(parts1.count(), parts2.count());
    for (int i = 0; i < count; ++i) {
        int p1 = (i < parts1.count()) ? parts1[i].toInt() : 0;
        int p2 = (i < parts2.count()) ? parts2[i].toInt() : 0;
        if (p1 < p2) return -1;
        if (p1 > p2) return 1;
    }
    return 0;
}

QString ToolsManager::executeCommand(const QString &program, const QStringList &arguments) {
    QProcess process;
    process.start(program, arguments);
    process.waitForFinished();
    return QString::fromLatin1(process.readAllStandardOutput()).trimmed();
}

QString ToolsManager::getInstalledFFmpegVersion() {
    if (m_ffmpegPath.isEmpty()) {
        return "Not Found";
    }
    QString versionString = executeCommand(m_ffmpegPath, {"-version"});
    // Parse the version string.  This is just an example, you might need to adjust it.
    QRegularExpression re("ffmpeg version (\\d+\\.\\d+(?:\\.\\d+)?) ");
    QRegularExpressionMatch match = re.match(versionString);
    if (match.hasMatch()) {
        return match.captured(1);
    }
    return "Unknown";
}

void ToolsManager::startDownload(const QString &toolName, const QString &urlString, const QString &fileExtension)
{
    if (!m_overallProcessRunning) {
        emit processStarted();
        m_overallProcessRunning = true;
        m_overallSuccess = true;
    }

    if (toolName == "ffmpeg" && !m_ffmpegPath.isEmpty()) {
        emit progressChanged(100, "FFmpeg is already installed. Skipping update.");
        QMetaObject::invokeMethod(this, "processNextDownload");
        return; // Skip adding to the queue
    }

    m_downloadQueue.enqueue({toolName, {urlString, fileExtension}});
    if (m_currentToolName.isEmpty()) {
        processNextDownload();
    }
}

void ToolsManager::processNextDownload()
{
    if (m_downloadQueue.isEmpty()) {
        m_currentToolName.clear();
        if (m_overallProcessRunning) {
            emit processFinished(m_overallSuccess);
            m_overallProcessRunning = false;
        }
        return;
    }

    QPair<QString, QStringList> nextItem = m_downloadQueue.dequeue();
    m_currentToolName = nextItem.first;
    QString urlString = nextItem.second.at(0);
    m_currentFileExtension = nextItem.second.at(1);

    emit progressChanged(0, QString("Starting download for %1...").arg(m_currentToolName));

    QNetworkRequest request(urlString);
    QNetworkReply *reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::downloadProgress, this, &ToolsManager::onDownloadProgress);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { onDownloadFinished(reply); });
}

void ToolsManager::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal > 0) {
        int percentage = static_cast<int>((bytesReceived * 100) / bytesTotal);
        emit progressChanged(percentage, QString("Downloading %1... %2%").arg(m_currentToolName).arg(percentage));
    } else {
        emit progressChanged(0, QString("Downloading %1... (%2 bytes received)").arg(m_currentToolName).arg(bytesReceived));
    }
}

void ToolsManager::onDownloadFinished(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        emit progressChanged(100, QString("Download failed for %1: %2").arg(m_currentToolName).arg(reply->errorString()));
        m_overallSuccess = false;
        reply->deleteLater();
        if (m_currentTempDir) { delete m_currentTempDir; m_currentTempDir = nullptr; }
        processNextDownload();
        return;
    }

    QByteArray downloadedData = reply->readAll();
    reply->deleteLater();

    if (downloadedData.isEmpty()) {
        emit progressChanged(100, QString("Download failed for %1: Received empty file.").arg(m_currentToolName));
        m_overallSuccess = false;
        if (m_currentTempDir) { delete m_currentTempDir; m_currentTempDir = nullptr; }
        processNextDownload();
        return;
    }

    emit progressChanged(100, QString("Download successful for %1. Saving...").arg(m_currentToolName));

    if (m_currentTempDir) {
        delete m_currentTempDir;
    }
    m_currentTempDir = new QTemporaryDir();

    if (!m_currentTempDir->isValid()) {
        emit progressChanged(100, "Failed to create temporary directory.");
        m_overallSuccess = false;
        if (m_currentTempDir) { delete m_currentTempDir; m_currentTempDir = nullptr; }
        processNextDownload();
        return;
    }

    QString tempFilePath = m_currentTempDir->path() + "/download." + m_currentFileExtension;
    QFile tempFile(tempFilePath);
    if (!tempFile.open(QIODevice::WriteOnly)) {
        emit progressChanged(100, "Failed to open temporary file for writing.");
        m_overallSuccess = false;
        if (m_currentTempDir) { delete m_currentTempDir; m_currentTempDir = nullptr; }
        processNextDownload();
        return;
    }

    tempFile.write(downloadedData);
    tempFile.close();

    if (m_currentToolName == "ffmpeg") {
        if (m_currentFileExtension == "zip") {
            extractZipArchive(tempFilePath);
        } else if (m_currentFileExtension == "tar.xz") {
            extractTarXzArchive(tempFilePath);
        } else {
            emit progressChanged(100, QString("Error: Unsupported file extension for FFmpeg: %1").arg(m_currentFileExtension));
            m_overallSuccess = false;
            if (m_currentTempDir) { delete m_currentTempDir; m_currentTempDir = nullptr; }
            processNextDownload();
        }
    } else if (m_currentToolName == "yt-dlp") {
        installYtDlpBinary(tempFilePath);
    } else {
        emit progressChanged(100, QString("Error: Unknown tool to install: %1").arg(m_currentToolName));
        m_overallSuccess = false;
        if (m_currentTempDir) { delete m_currentTempDir; m_currentTempDir = nullptr; }
        processNextDownload();
    }
}

void ToolsManager::extractZipArchive(const QString &zipPath)
{
#ifdef Q_OS_WIN
    emit progressChanged(100, "Extracting ZIP for FFmpeg...");

    QFutureWatcher<bool> *watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this, [this, watcher]() {
        bool success = watcher->result();
        if (success) {
            emit progressChanged(100, "FFmpeg installation complete!");
            checkToolsOnStartup();
        } else {
            m_overallSuccess = false;
        }
        watcher->deleteLater();
        if (m_currentTempDir) { delete m_currentTempDir; m_currentTempDir = nullptr; }
        processNextDownload();
    });

    QFuture<bool> future = QtConcurrent::run([this, zipPath]() {
        QFile file(zipPath);
        if (!file.open(QIODevice::ReadOnly)) {
            QMetaObject::invokeMethod(this, "progressChanged", Qt::QueuedConnection,
                                      Q_ARG(int, 100), Q_ARG(QString, "Error: Cannot open downloaded file for validation."));
            return false;
        }
        QByteArray magic = file.read(4);
        file.close();

        if (magic != QByteArray("PK\x03\x04")) {
            QMetaObject::invokeMethod(this, "progressChanged", Qt::QueuedConnection,
                                      Q_ARG(int, 100), Q_ARG(QString, "Error: Downloaded file is not a valid ZIP archive (magic number mismatch)."));
            return false;
        }

        mz_zip_archive zip_archive = {};
        if (!mz_zip_reader_init_file(&zip_archive, zipPath.toUtf8().constData(), 0)) {
            QMetaObject::invokeMethod(this, "progressChanged", Qt::QueuedConnection,
                                      Q_ARG(int, 100), Q_ARG(QString, "Error: Failed to initialize zip reader (corrupted or unsupported format)."));
            return false;
        }

        QString installPath = QCoreApplication::applicationDirPath() + "/tools/ffmpeg";
        QDir dir(installPath);
        if (!dir.exists()) {
            dir.mkpath(".");
        }

        for (mz_uint i = 0; i < mz_zip_reader_get_num_files(&zip_archive); i++) {
            mz_zip_archive_file_stat file_stat;
            if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) {
                QMetaObject::invokeMethod(this, "progressChanged", Qt::QueuedConnection,
                                          Q_ARG(int, 100), Q_ARG(QString, "Error: Failed to get file stats from zip."));
                mz_zip_reader_end(&zip_archive);
                return false;
            }

            QString fullPath = QString::fromUtf8(file_stat.m_filename);
            int firstSlash = fullPath.indexOf('/');
            if (firstSlash == -1) continue; // Skip top-level files like LICENSE

            QString pathAfterTopLevel = fullPath.mid(firstSlash + 1);

            // Only process files/directories that are within the "bin/" path after the top-level folder
            if (pathAfterTopLevel.startsWith("bin/")) {
                QString targetSubPath = pathAfterTopLevel;
                QString targetFullPath = installPath + "/" + targetSubPath;

                if (mz_zip_reader_is_file_a_directory(&zip_archive, i)) {
                    QDir().mkpath(targetFullPath);
                } else {
                    QDir targetDir = QFileInfo(targetFullPath).dir();
                    if (!targetDir.exists()) {
                        targetDir.mkpath(".");
                    }
                    if (!mz_zip_reader_extract_to_file(&zip_archive, i, targetFullPath.toUtf8().constData(), 0)) {
                        QMetaObject::invokeMethod(this, "progressChanged", Qt::QueuedConnection,
                                                  Q_ARG(int, 100), Q_ARG(QString, "Error: Failed to extract file: " + targetSubPath));
                        mz_zip_reader_end(&zip_archive);
                        return false;
                    }
                }
            }
        }

        mz_zip_reader_end(&zip_archive);
        return true;
    });

    watcher->setFuture(future);
#else
    Q_UNUSED(zipPath);
    m_overallSuccess = false;
    QMetaObject::invokeMethod(this, "progressChanged", Qt::QueuedConnection,
                              Q_ARG(int, 100), Q_ARG(QString, "Error: ZIP extraction not supported on this OS."));
    if (m_currentTempDir) { delete m_currentTempDir; m_currentTempDir = nullptr; }
    QMetaObject::invokeMethod(this, "processNextDownload", Qt::QueuedConnection);
#endif
}

void ToolsManager::extractTarXzArchive(const QString &tarXzPath)
{
#ifndef Q_OS_WIN
    emit progressChanged(100, "Extracting .tar.xz for FFmpeg...");

    if (m_tarProcess) {
        m_tarProcess->deleteLater();
    }
    m_tarProcess = new QProcess(this);
    connect(m_tarProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &ToolsManager::onTarProcessFinished);
    connect(m_tarProcess, &QProcess::errorOccurred, this, &ToolsManager::onTarProcessError);

    QString installPath = QCoreApplication::applicationDirPath() + "/tools/ffmpeg"; // Updated path
    QDir dir(installPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QStringList arguments;
    arguments << "-x" << "-f" << tarXzPath << "-J" << "-C" << installPath;

    m_tarProcess->start("tar", arguments);
    if (!m_tarProcess->waitForStarted()) {
        emit progressChanged(100, "Error: Failed to start tar process.");
        m_overallSuccess = false;
        if (m_currentTempDir) { delete m_currentTempDir; m_currentTempDir = nullptr; }
        processNextDownload();
        return;
    }

    connect(m_tarProcess, &QProcess::readyReadStandardOutput, this, [this]() {
        emit progressChanged(100, "tar stdout: " + m_tarProcess->readAllStandardOutput());
    });
    connect(m_tarProcess, &QProcess::readyReadStandardError, this, [this]() {
        emit progressChanged(100, "tar stderr: " + m_tarProcess->readAllStandardError());
    });

#else
    Q_UNUSED(tarXzPath);
    m_overallSuccess = false;
    QMetaObject::invokeMethod(this, "progressChanged", Qt::QueuedConnection,
                              Q_ARG(int, 100), Q_ARG(QString, "Error: .tar.xz extraction not supported on Windows."));
    if (m_currentTempDir) { delete m_currentTempDir; m_currentTempDir = nullptr; }
    QMetaObject::invokeMethod(this, "processNextDownload", Qt::QueuedConnection);
#endif
}

void ToolsManager::onTarProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        emit progressChanged(100, QString("%1 installation complete!").arg(m_currentToolName));
        checkToolsOnStartup();
    } else {
        emit progressChanged(100, QString("Error: %1 tar process failed with exit code %2.").arg(m_currentToolName).arg(exitCode));
        m_overallSuccess = false;
    }
    if (m_currentTempDir) { delete m_currentTempDir; m_currentTempDir = nullptr; }
    m_tarProcess->deleteLater();
    m_tarProcess = nullptr;
    processNextDownload();
}

void ToolsManager::onTarProcessError(QProcess::ProcessError error)
{
    emit progressChanged(100, QString("Error: %1 tar process error: %2.").arg(m_currentToolName).arg(error));
    m_overallSuccess = false;
    if (m_currentTempDir) { delete m_currentTempDir; m_currentTempDir = nullptr; }
    m_tarProcess->deleteLater();
    m_tarProcess = nullptr;
    processNextDownload();
}

void ToolsManager::installYtDlpBinary(const QString &downloadedFilePath)
{
    emit progressChanged(100, "Installing yt-dlp...");
    QString appPath = QCoreApplication::applicationDirPath();
    QString targetPath = appPath + (QSysInfo::productType() == "windows" ? "/tools/yt-dlp.exe" : "/tools/yt-dlp"); // Updated path

    QFile::remove(targetPath);
    if (!QFile::copy(downloadedFilePath, targetPath)) {
        emit progressChanged(100, "Error: Failed to copy yt-dlp binary.");
        m_overallSuccess = false;
        if (m_currentTempDir) { delete m_currentTempDir; m_currentTempDir = nullptr; }
        processNextDownload();
        return;
    }

#ifndef Q_OS_WIN
    QFile::setPermissions(targetPath, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner | QFile::ReadGroup | QFile::ExeGroup | QFile::ReadOther | QFile::ExeOther);
#endif

    emit progressChanged(100, "yt-dlp installation complete!");
    checkToolsOnStartup();
    checkYtDlpVersion();    // and check the new version

    if (m_currentTempDir) { delete m_currentTempDir; m_currentTempDir = nullptr; }
    processNextDownload();
}
