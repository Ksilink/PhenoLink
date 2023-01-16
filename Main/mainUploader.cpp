#include <QApplication>
#include <QObject>
#include <QCommandLineParser>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QFile>
#include <QThreadPool>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>

#include <fstream>

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/PutObjectRequest.h>


#include <google/cloud/storage/client.h>

//#include <azure/storage/blobs.h>



#include "mainUploader.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QCommandLineParser parser;
    parser.addPositionalArgument("bucket-name", "The name of the S3 bucket to upload the files to.");
    parser.addPositionalArgument("key-name", "The key name to use for the uploaded files.");

    parser.addPositionalArgument("files", "The files to upload.", "[files...]");

    parser.addOption(QCommandLineOption("threads", "The number of parallel threads to use for uploading.", "threads", "8"));

    parser.process(app);
    const QStringList files = parser.positionalArguments().mid(2);
    if (files.isEmpty()) {
        parser.showHelp(1);
    }

    const QString bucketName = parser.positionalArguments().at(0);
    const QString keyName = parser.positionalArguments().at(1);
    const int threads = parser.value("threads").toInt();

    Aws::SDKOptions options;
    Aws::InitAPI(options);
    {
        QThreadPool::globalInstance()->setMaxThreadCount(threads);

        QWidget window;
        QVBoxLayout* layout = new QVBoxLayout(&window);
        layout->setMargin(0);
        window.setLayout(layout);

        for (const QString& file : files) {
            AwsFileUploader* uploader = new AwsFileUploader(file, bucketName, keyName);
            ProgressBar* progressBar = new ProgressBar(uploader);
            layout->addWidget(progressBar);
            uploader->start();
        }

        window.show();
        app.exec();
    }

    Aws::ShutdownAPI(options);
    return 0;
}

AwsFileUploader::AwsFileUploader(const QString &filePath, const QString &bucketName, const QString &keyName)
    : m_filePath(filePath), m_bucketName(bucketName), m_keyName(keyName)
{
}

void AwsFileUploader::start()
{
    QFuture<void> future = QtConcurrent::run(this, &AwsFileUploader::upload);
}

void AwsFileUploader::upload()
{
    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit finished();
        return;
    }

    Aws::S3::S3Client s3Client;
    Aws::S3::Model::PutObjectRequest objectRequest;
    objectRequest.SetBucket(m_bucketName.toStdString());
    objectRequest.SetKey(m_keyName.toStdString());

    std::shared_ptr<Aws::IOStream> inputData = Aws::MakeShared<Aws::FStream>("PutObjectInputStream",
                                                   m_filePath.toStdString().c_str(), std::ios_base::in | std::ios_base::binary);

    objectRequest.SetBody(inputData);
    objectRequest.SetContentLength(static_cast<long>(file.size()));
//    objectRequest.SetDataSentEventHandler(); // Put a Lambda here to get transfert feedback

    auto putObjectOutcome = s3Client.PutObject(objectRequest);
    if (putObjectOutcome.IsSuccess()) {
        emit finished();
    } else {
        emit finished();
    }
}

ProgressBar::ProgressBar(AwsFileUploader *uploader)
    : m_uploader(uploader)
{
    connect(m_uploader, &AwsFileUploader::progressChanged, this, &ProgressBar::setValue);
    connect(m_uploader, &AwsFileUploader::finished, this, &ProgressBar::hide);
}

GCSFileUploader::GCSFileUploader(const QString &filePath, const QString &bucketName, const QString &keyName)
    : m_filePath(filePath), m_bucketName(bucketName), m_keyName(keyName)
{
}

void GCSFileUploader::start()
{
    QFuture<void> future = QtConcurrent::run(this, &GCSFileUploader::upload);
}

void GCSFileUploader::upload()
{
    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit finished();
        return;
    }
    google::cloud::StatusOr<google::cloud::storage::Client> client = google::cloud::storage::Client::CreateDefaultClient();
    if (!client) {
        emit finished();
        return;
    }
//    google::cloud::storage::ObjectMetadata metadata;
//    metadata.set_content_type("application/octet-stream");
////    metadata.set_cache_control("private, max-age=0, no-transform");
//    metadata.set_content_disposition("attachment");
//    metadata.set_content_encoding("identity");
    auto insert_object_response = client->InsertObject(m_bucketName.toStdString(), m_keyName.toStdString(), file.readAll().toStdString());
    if (!insert_object_response) {
        emit finished();
        return;
    }
    emit finished();
}

AzureFileUploader::AzureFileUploader(const QString &filePath, const QString &containerName, const QString &blobName)
    : filePath_(filePath), containerName_(containerName), blobName_(blobName) {}

void AzureFileUploader::upload() {
//    try {
//        azure::storage::cloud_blob_client client = azure::storage::cloud_blob_client::create_from_connection_string(connectionString);
//        azure::storage::cloud_blob_container container = client.get_container_reference(containerName_.toStdString());
//        container.create_if_not_exists();

//        azure::storage::cloud_block_blob blob = container.get_block_blob_reference(blobName_.toStdString());
//        blob.upload_from_file(filePath_.toStdString());

//        std::cout << "File uploaded successfully to Azure!" << std::endl;
//    } catch (const std::exception& e) {
//        std::cerr << "Error uploading file: " << e.what() << std::endl;
//    }
}
