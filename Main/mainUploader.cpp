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
#include <aws/core/auth/AWSCredentials.h>

#include <google/cloud/storage/client.h>

//#include <azure/storage/blobs.h>



#include "mainUploader.h"


#undef signals

#include <arrow/api.h>
#include <arrow/filesystem/filesystem.h>
#include <arrow/util/iterator.h>
#include <arrow/ipc/writer.h>
#include <arrow/ipc/reader.h>
#include <arrow/ipc/api.h>



namespace fs = arrow::fs;
#define ABORT_ON_FAILURE(expr)                     \
do {                                             \
        arrow::Status status_ = (expr);                \
        if (!status_.ok()) {                           \
            std::cerr << status_.message() << std::endl; \
            abort();                                     \
    }                                              \
} while (0);

#define ArrowGet(var, id, call, msg) \
auto id = call; if (!id.ok()) { qDebug() << msg; return -1; } auto var = id.ValueOrDie();


int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QCommandLineParser parser;
    parser.addPositionalArgument("files", "Feather file with list of files to upload", "[files...]");

    parser.addOption(QCommandLineOption("threads", "The number of parallel threads to use for uploading.", "threads", "8"));
    parser.addOption(QCommandLineOption("help", "Show help message"));


    parser.process(app);
    if (parser.isSet("help")) parser.showHelp(0);

    const QStringList files = parser.positionalArguments();
    if (files.isEmpty()) {
        std::cerr << "Mandatory to set the input file list" << std::endl;
        parser.showHelp(1);

    }

    int threads = parser.value("threads").toInt();

    // Read Arrow entries
    QStringList items = { "BaseDirectory", "CloudStorage/provider", "CloudStorage/accessKey",
                         "CloudStorage/secretKey", "CloudStorage/bucket",
                         "CloudStorage/key", "CloudStorage/connectionString",
                         "CloudStorage/container", "CloudStorage/blob" };



    std::string uri = files[0].toStdString(), root_path;
    ArrowGet(fs, r0, fs::FileSystemFromUriOrPath(uri, &root_path), "Arrow File not loading" << files[0]);

    ArrowGet(input, r1, fs->OpenInputFile(uri), "Error opening arrow file" << files[0]);
    arrow::Result<std::shared_ptr<arrow::ipc::RecordBatchFileReader>  > r2 = arrow::ipc::RecordBatchFileReader::Open(input);
    if (!r2.ok()) { qDebug() << "Batch open error"; return -1; }
    std::shared_ptr<arrow::ipc::RecordBatchFileReader> reader = r2.ValueOrDie();


    auto schema = reader->schema();

    for (int record = 0; record < reader->num_record_batches(); ++record)
    {
        ArrowGet(rb, r4, reader->ReadRecordBatch(record), "Error Get Record");

        auto files = std::static_pointer_cast<arrow::StringArray>(rb->GetColumnByName("file"));
        auto dates = std::static_pointer_cast<arrow::Date32Array>(rb->GetColumnByName("uploaded"));

        for (int i = 0; i < files->length(); ++i)
            qDebug() << QString::fromStdString(std::string(files->Value(i)))
                ;
        //        << (dates->Value(i) == 0 ? "No data" : QDateTime::fromSecsSinceEpoch(dates->Value(i)))

    }


    auto meta = schema->metadata();
    auto provider = meta->Get("CloudStorage/provider").ValueOr("");
    if (provider=="")
        return -1;

    if (provider == "AWS")
    {
        qDebug() << "Let's go AWS!!!";
        Aws::SDKOptions options;

        Aws::InitAPI(options);
        {
            QThreadPool::globalInstance()->setMaxThreadCount(threads);

            //        QWidget window;
            //        QVBoxLayout* layout = new QVBoxLayout(&window);
            //        layout->setContentsMargins(0,0,0,0);
            //        window.setLayout(layout);

            //        for (const QString& file : files) {
            //            AwsFileUploader* uploader = new AwsFileUploader(file, bucketName, keyName);
            //            ProgressBar* progressBar = new ProgressBar(uploader);
            //            layout->addWidget(progressBar);
            //            uploader->start();
            //        }

            //        window.show();
            //        app.exec();
        }

        Aws::ShutdownAPI(options);
    }
    return 0;
}

AwsFileUploader::AwsFileUploader(const QString &filePath, const QString &bucketName, const QString &keyName, const QString &secretKey)
    : m_filePath(filePath), m_bucketName(bucketName), m_keyName(keyName), m_secretKey(secretKey)
{
}

void AwsFileUploader::start()
{
    auto future = QtConcurrent::run([this]() { this->upload(); } );
}

static const char * ALLOCATION_TAG = "PhenoLink"; // your allocation tag

void AwsFileUploader::upload()
{

    using namespace Aws::Auth;
    using namespace Aws::Http;
    using namespace Aws::Client;
    using namespace Aws::S3;
    using namespace Aws::S3::Model;
    using namespace Aws::Utils;
//    using namespace Aws::Transfer;

    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit finished();
        return;
    }

    Aws::Client::ClientConfiguration config;

    std::shared_ptr<S3Client> s3Client = Aws::MakeShared<S3Client>(
        ALLOCATION_TAG,
        AWSCredentials(Aws::String(m_keyName.toStdString()), Aws::String(m_secretKey.toStdString()))
//        , config
    );

    Aws::S3::Model::PutObjectRequest objectRequest;
    objectRequest.SetBucket(m_bucketName.toStdString());
    objectRequest.SetKey(m_keyName.toStdString());

    std::shared_ptr<Aws::IOStream> inputData = Aws::MakeShared<Aws::FStream>("PutObjectInputStream",
                                                                             m_filePath.toStdString().c_str(), std::ios_base::in | std::ios_base::binary);

    objectRequest.SetBody(inputData);
    objectRequest.SetContentLength(static_cast<long>(file.size()));
    //    objectRequest.SetDataSentEventHandler(); // Put a Lambda here to get transfert feedback

    auto putObjectOutcome = s3Client->PutObject(objectRequest);
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
    auto future = QtConcurrent::run([this](){ this->upload(); } );
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
