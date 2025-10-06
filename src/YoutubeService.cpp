#include "YoutubeService.h"
#include "ToolsManager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <cmath>
#include <QDebug>
#include <QUrl>
#include <QImage>
#include <QBuffer>

// Helper to format duration from seconds to HH:mm:ss
QString formatDuration(double totalSeconds) {
    if (totalSeconds < 0) return "N/A";
    int hours = static_cast<int>(totalSeconds / 3600);
    int minutes = static_cast<int>(fmod(totalSeconds, 3600) / 60);
    int seconds = static_cast<int>(fmod(totalSeconds, 60));
    return QString("%1:%2:%3")
        .arg(hours, 2, 10, QChar('0'))
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'));
}

// Helper to format bytes into a readable MB/GB string
QString formatSize(double bytes) {
    if (bytes <= 0) return "N/A";
    double mb = bytes / (1024.0 * 1024.0);
    if (mb < 1024) {
        return QString::asprintf("%.2f MB", mb);
    }
    return QString::asprintf("%.2f GB", mb / 1024.0);
}

YoutubeService::YoutubeService(ToolsManager *toolsManager, QObject *parent)
    : YtDlp{parent}
    , m_process(new QProcess(this))
    , m_toolsManager(toolsManager)
    , m_parsingInitiated(false)
{
    qDebug() << "YoutubeService: Initializing...";
    m_thumbnailManager = new QNetworkAccessManager(this);
    connect(m_thumbnailManager, &QNetworkAccessManager::finished, this, &YoutubeService::onThumbnailDownloaded);

    m_jsonParseTimer = new QTimer(this);
    m_jsonParseTimer->setInterval(250);
    m_jsonParseTimer->setSingleShot(true);
    connect(m_jsonParseTimer, &QTimer::timeout, this, &YoutubeService::onJsonParseTimeout);

    connect(m_process, &QProcess::finished, this, &YoutubeService::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &YoutubeService::onProcessErrorOccurred);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &YoutubeService::onReadyReadStandardOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &YoutubeService::onReadyReadStandardError);
}

void YoutubeService::execute(const QString &program, const QStringList &arguments)
{
    qDebug() << "YoutubeService: Executing command:" << program << arguments;
    m_lastArguments = arguments;
    m_outputBuffer.clear();
    m_parsingInitiated = false;

    QString executablePath;
    if (program == "yt-dlp") {
        executablePath = m_toolsManager->ytDlpPath();
    } else if (program == "ffmpeg") {
        executablePath = m_toolsManager->ffmpegPath();
    } else {
        emit processError(QString("Unknown program: %1").arg(program));
        return;
    }

    if (executablePath.isEmpty()) {
        emit processError(QString("%1 executable not found.").arg(program));
        return;
    }

    m_process->start(executablePath, arguments);
}

void YoutubeService::fetchVideoInfo(const QString &url)
{
    qDebug() << "YoutubeService: Fetching video info for URL:" << url;
    execute("yt-dlp", {"--no-progress", "--dump-json", url});
}

void YoutubeService::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    qDebug() << "YoutubeService: Process finished with exitCode:" << exitCode << ", exitStatus:" << exitStatus;
    if (m_parsingInitiated) return;
    m_parsingInitiated = true;
    m_jsonParseTimer->stop();

    if (m_lastArguments.contains("--dump-json")) {
        if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
            parseAndEmitVideoInfo();
        } else {
            emit videoInfoError(QString("yt-dlp process failed with exit code %1").arg(exitCode));
        }
    }
    emit processFinished(exitCode);
}

void YoutubeService::onProcessErrorOccurred(QProcess::ProcessError error)
{
    qDebug() << "YoutubeService: QProcess error occurred:" << error << ", errorString:" << m_process->errorString();
    if (m_process->state() == QProcess::NotRunning && !m_parsingInitiated) {
        m_parsingInitiated = true;
        m_jsonParseTimer->stop();
        emit videoInfoError(m_process->errorString());
    }
    emit processError(m_process->errorString());
}

void YoutubeService::onReadyReadStandardOutput()
{
    QByteArray newData = m_process->readAllStandardOutput();
    m_outputBuffer.append(newData);
    if (!m_lastArguments.contains("--dump-json")) {
        emit processOutput(QString::fromUtf8(newData));
    }
    if (m_lastArguments.contains("--dump-json")) {
        m_jsonParseTimer->start();
    }
}

void YoutubeService::onReadyReadStandardError()
{
    emit processError(QString::fromLatin1(m_process->readAllStandardError()));
}

void YoutubeService::onJsonParseTimeout()
{
    if (m_parsingInitiated) return;
    m_parsingInitiated = true;

    parseAndEmitVideoInfo();
    m_process->kill();
}

void YoutubeService::parseAndEmitVideoInfo()
{
    qDebug() << "YoutubeService: Attempting to parse JSON. Final buffer size:" << m_outputBuffer.size();
    QJsonDocument doc = QJsonDocument::fromJson(m_outputBuffer);
    if (doc.isNull() || !doc.isObject()) {
        emit videoInfoError("Failed to parse JSON from yt-dlp.");
        return;
    }

    QJsonObject root = doc.object();
    m_pendingVideoInfo.clear();
    m_pendingVideoInfo.insert("title", root.value("title").toString());
    m_pendingVideoInfo.insert("channel", root.value("channel").toString());
    m_pendingVideoInfo.insert("duration", formatDuration(root.value("duration").toDouble()));

    QVariantList formatsList;
    QJsonArray formats = root.value("formats").toArray();
    for (const QJsonValue &formatValue : formats) {
        QJsonObject formatObject = formatValue.toObject();
        if (formatObject.value("ext").toString() == "mhtml") continue;

        QVariantMap formatMap;
        formatMap.insert("format_id", formatObject.value("format_id").toString());
        formatMap.insert("ext", formatObject.value("ext").toString());
        formatMap.insert("vcodec", formatObject.value("vcodec").toString("none"));
        formatMap.insert("acodec", formatObject.value("acodec").toString("none"));
        formatMap.insert("abr", formatObject.value("abr").toDouble(-1.0));

        QString quality = formatObject.value("format_note").toString();
        if (quality.isEmpty()) {
            int height = formatObject.value("height").toInt(-1);
            if (height > 0) quality = QString("%1p").arg(height);
            else quality = "Audio";
        }
        formatMap.insert("quality", quality);
        formatMap.insert("fps", formatObject.value("fps").toInt(-1));

        double filesize = formatObject.contains("filesize") ? formatObject.value("filesize").toDouble() : formatObject.value("filesize_approx").toDouble();
        formatMap.insert("filesize", formatSize(filesize));

        formatsList.append(formatMap);
    }
    m_pendingVideoInfo.insert("formats", formatsList);

    // Intelligently find the best JPG thumbnail URL
    QString thumbnailUrl;
    QJsonArray thumbnails = root.value("thumbnails").toArray();
    if (!thumbnails.isEmpty()) {
        for (const QJsonValue &thumbValue : thumbnails) {
            QJsonObject thumbObject = thumbValue.toObject();
            QString url = thumbObject.value("url").toString();
            if (url.endsWith(".jpg")) {
                thumbnailUrl = url; // Keep overwriting to get the last (likely highest quality) one
            }
        }
    }

    // Fallback to the main thumbnail field if no JPG was found
    if (thumbnailUrl.isEmpty()) {
        thumbnailUrl = root.value("thumbnail").toString();
    }

    qDebug() << "YoutubeService: Downloading thumbnail from:" << thumbnailUrl;
    m_thumbnailManager->get(QNetworkRequest(QUrl(thumbnailUrl)));
}

void YoutubeService::onThumbnailDownloaded(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray imageData = reply->readAll();
        // The downloaded data is now a JPG, so we can use it directly.
        QString base64Image = imageData.toBase64();
        QString dataUrl = QString("data:image/jpeg;base64,%1").arg(base64Image);

        m_pendingVideoInfo.insert("thumbnail", dataUrl);
        qDebug() << "YoutubeService: Thumbnail downloaded and converted to JPG Data URL.";
    } else {
        qDebug() << "YoutubeService: Thumbnail download failed:" << reply->errorString();
        m_pendingVideoInfo.insert("thumbnail", ""); // Use an empty string on failure
    }

    emit videoInfoReady(m_pendingVideoInfo);
    qDebug() << "YoutubeService: Emitted videoInfoReady signal.";
    reply->deleteLater();
}
