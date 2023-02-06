#include <QString>
#include <QDir>

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/ListObjectsRequest.h>

#include <google/cloud/storage/client.h>

#include <azure/storage/blobs/blob_client.hpp>

#include <unordered_map>
#include <future>




class S3Dir : public QDir
{
public:
    S3Dir(const QString &path) : QDir(path) {}

    QStringList entryList(QDir::Filters filters = QDir::NoFilter,
                          QDir::SortFlags sort = QDir::NoSort) const
    {
        QStringList list;

        Aws::S3::S3Client s3Client;
        Aws::S3::Model::ListObjectsRequest request;
        request.WithBucket(this->path().toStdString());

        auto outcome = s3Client.ListObjects(request);
        if (outcome.IsSuccess())
        {
            std::unordered_map<QString, std::pair<int64_t, Aws::Utils::DateTime>> fileData; // map to cache file data

            // create a vector to store the futures of the async queries
            std::vector<std::future<void>> futures;

            for (auto &object : outcome.GetResult().GetContents())
            {
                QString fileName = QString::fromStdString(object.GetKey());
                // apply filters
                if ((filters & QDir::Files) && object.GetSize() == 0)
                    continue;
                if ((filters & QDir::Dirs) && object.GetSize() != 0)
                    continue;
//                if ((filters & QDir::NoSymLinks) && QString::fromStdString(object.GetStorageClass()) == "SYMLINK")
//                    continue;
                if ((filters & QDir::NoDot) && (fileName == "." || fileName == ".."))
                    continue;
                if ((filters & QDir::NoDotDot) && fileName == "..")
                    continue;
                if ((filters & QDir::Hidden) && !QString::fromStdString(object.GetKey()).startsWith("."))
                    continue;

                list.append(fileName);
                fileData[fileName] = std::make_pair(object.GetSize(), object.GetLastModified());
            }

            // wait for all async queries to finish
            for (auto &f : futures) f.get();

            // sort the list
            if (sort & QDir::Name)
                std::sort(list.begin(), list.end(), [](const QString &s1, const QString &s2) { return s1 < s2; });
            if (sort & QDir::Time)
                std::sort(list.begin(), list.end(), [&fileData](const QString &s1, const QString &s2) { return fileData[s1].second < fileData[s2].second; });
            if (sort & QDir::Size)
                std::sort(list.begin(), list.end(), [&fileData](const QString &s1, const QString &s2) { return fileData[s1].first < fileData[s2].first; });
        }
        return list;
    }

};



/*
class GCSDir : public QDir
{
public:
    GCSDir(const QString &path) : QDir(path) {}

    QStringList entryList(QDir::Filters filters = QDir::NoFilter,
                          QDir::SortFlags sort = QDir::NoSort) const
    {
        QStringList list;

        google::cloud::storage::Client storageClient;
        auto bucket = storageClient.bucket(path().toStdString());

        std::vector<std::future<void>> futures;
        std::unordered_map<QString, std::pair<std::int64_t, std::string>> fileData;

        for (auto& [object, _] : bucket)
        {
            QString fileName = QString::fromStdString(object.name());
            // apply filters
            if ((filters & QDir::Files) && object.size() == 0)
                continue;
            if ((filters & QDir::Dirs) && object.size() != 0)
                continue;
            if ((filters & QDir::NoSymLinks) && object.metadata().count("x-goog-meta-symlink") != 0)
                continue;
            if ((filters & QDir::NoDot) && (fileName == "." || fileName == ".."))
                continue;
            if ((filters & QDir::NoDotDot) && fileName == "..")
                continue;
            if ((filters & QDir::Hidden) && !fileName.startsWith("."))
                continue;

            list.append(fileName);

            // cache file data asynchronously
            futures.emplace_back(std::async([&storageClient, fileName, &fileData, path = path()]() {
                auto object = storageClient.GetObject(path.toStdString(), fileName.toStdString());
                fileData[fileName] = std::make_pair(object.size(), object.metadata()["updated"]);
            }));
        }

        // wait for all async queries to finish
        for (auto &f : futures) f.get();

        // sort the list
        if (sort & QDir::Name)
            std::sort(list.begin(), list.end(), [](const QString &s1, const QString &s2) { return s1 < s2; });
        if (sort & QDir::Time)
            std::sort(list.begin(), list.end(), [&fileData](const QString &s1, const QString &s2) { return fileData[s1].second < fileData[s2].second; });
        if (sort & QDir::Size)
            std::sort(list.begin(), list.end(), [&fileData](const QString &s1, const QString &s2) { return fileData[s1].first < fileData[s2].first; });

        return list;
    }
};


*/
/*
class AzureBlobDir : public QDir
{
public:
    AzureBlobDir(const QString &path) : QDir(path) {}

    QStringList entryList(QDir::Filters filters = QDir::NoFilter,
                          QDir::SortFlags sort = QDir::NoSort) const
    {
        QStringList list;

        azure::storage_lite::blob_client storageClient(path().toStdString());
        auto container = storageClient.list_blobs();

        std::vector<std::future<void>> futures;
        std::unordered_map<QString, std::pair<std::int64_t, std::string>> fileData;

        for (auto& object : container)
        {
            QString fileName = QString::fromStdString(object.name);
            // apply filters
            if ((filters & QDir::Files) &&
                    object.properties.content_length == 0)
                continue;
            if ((filters & QDir::Dirs) && object.properties.content_length != 0)
                continue;
            if ((filters & QDir::NoSymLinks) && object.metadata.count("x-az-symlink") != 0)
                continue;
            if ((filters & QDir::NoDot) && (fileName == "." || fileName == ".."))
                continue;
            if ((filters & QDir::NoDotDot) && fileName == "..")
                continue;
            if ((filters & QDir::Hidden) && !fileName.startsWith("."))
                continue;
            list.append(fileName);

            // cache file data asynchronously
            futures.emplace_back(std::async([&storageClient, fileName, &fileData]() {
                auto object = storageClient.get_blob_properties(fileName.toStdString());
                fileData[fileName] = std::make_pair(object.properties.content_length, object.properties.last_modified);
            }));
        }

        // wait for all async queries to finish
        for (auto &f : futures) f.get();
        // sort the list
        if (sort & QDir::Name)
            std::sort(list.begin(), list.end(), [](const QString &s1, const QString &s2) { return s1 < s2; });
        if (sort & QDir::Time)
            std::sort(list.begin(), list.end(), [&fileData](const QString &s1, const QString &s2) { return fileData[s1].second < fileData[s2].second; });
        if (sort & QDir::Size)
            std::sort(list.begin(), list.end(), [&fileData](const QString &s1, const QString &s2) { return fileData[s1].first < fileData[s2].first; });

        return list;
    }
};
*/
