#ifndef MAINUPLOADER_H
#define MAINUPLOADER_H

#include <QObject>
#include <QProgressBar>

class AwsFileUploader : public QObject
{
    Q_OBJECT

public:
    AwsFileUploader(const QString& filePath, const QString& bucketName,
                    const QString& keyName,  const QString& secretKey);

    void start();

signals:
    void progressChanged(int);
    void finished();

private:
    void upload();

    QString m_filePath;
    QString m_bucketName;
    QString m_keyName;
    QString m_secretKey;
};


class GCSFileUploader : public QObject
{
    Q_OBJECT

public:
    GCSFileUploader(const QString& filePath, const QString& bucketName, const QString& keyName);

    void start();

signals:
    void progressChanged(int);
    void finished();

private:
    void upload();

    QString m_filePath;
    QString m_bucketName;
    QString m_keyName;
};

class AzureFileUploader {
public:
    AzureFileUploader(const QString &filePath, const QString &containerName, const QString &blobName);

    void upload();

private:
    QString filePath_;
    QString containerName_;
    QString blobName_;
};



class ProgressBar : public QProgressBar
{
    Q_OBJECT

public:
    ProgressBar(AwsFileUploader* uploader);

private:
    AwsFileUploader* m_uploader;
};



#endif // MAINUPLOADER_H
