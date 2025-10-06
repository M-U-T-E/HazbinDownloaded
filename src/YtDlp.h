#ifndef YTDLP_H
#define YTDLP_H

#include <QObject>
#include <QStringList>
#include <QVariantMap>

class YtDlp : public QObject
{
    Q_OBJECT
public:
    explicit YtDlp(QObject *parent = nullptr);
    virtual ~YtDlp() = default;

    virtual void execute(const QString &program, const QStringList &arguments) = 0;

signals:
    void processFinished(int exitCode);
    void processError(const QString &error);
    void processOutput(const QString &output);

    // Signals for structured video information
    void videoInfoReady(const QVariantMap &info);
    void videoInfoError(const QString &error);
};

#endif // YTDLP_H
