#include <QCoreApplication>
#include <QCommandLineParser>

#include <QDir>
#include <QDirIterator>

#include <QSemaphore>
#include <QFileInfo>
#include <QDateTime>
#include <QSettings>
#include <QThreadPool>
#include <QMutex>
#include <QMutexLocker>
#include <QDebug>

#include <QElapsedTimer>
#include <QTime>

#include <queue>
#include <utility>

#include <opencv2/opencv.hpp>
#include <enc_fast_lossless.h>

#include "mutex_queue.h"

#include "jxl/encode.h"
#include "jxl/encode_cxx.h"

#include "jxl/decode.h"
#include "jxl/decode_cxx.h"
#include "jxl/thread_parallel_runner.h"
#include "jxl/thread_parallel_runner_cxx.h"
#include "jxl/resizable_parallel_runner.h"
#include "jxl/resizable_parallel_runner_cxx.h"



#include <archive.h>
#include <archive_entry.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



struct Data {

    QString outdir;
    QString indir;
    bool dry_run = false, inplace = false;
    int effort;
    int tar = -1;


    mutable std::mutex queue_mut;
    QMap<QString, QStringList> fileFolderQueue;
    QSet<QString> ongoingfolder;

    //    std::queue<QString> folderQueue;

    ThreadsafeQueue<QString> folderQueue;

    std::atomic<int> folderOver = 1;
    QMutex folder_mut;
    // First x threads will recursively seek folders and dig into them


    //    ThreadsafeQueue<QString> fileQueue;
    std::atomic<long long> copy_count=0, write_count=0;
    QMutex console;

    std::atomic<long long> prepared = 0, readed = 0, writen = 0;

    QDateTime ts;
    QDateTime older;

    std::atomic<long long> group0 = 0, group1 = 0;
    QMutex mgroup0, mgroup1;


    QMap<QString, QMap<QString, struct archive *> > tarobjects; // Folder + Tar file name
    QMap<QString,  QStringList > tar_remove; // Folder + Tar file name + List of objects
    QMutex tar_lock;
    QMap<struct archive*, int > tar_close;




    bool isrunning(){
        return (folderOver != 0
                ||  folderQueue.size() != 0
                //                || fileQueue.size() != 0
                ||  fileFolderQueue.size() != 0
                ||  group0 > 0
                ||  group1 > 0
                );
    }

    bool scanFolder()
    {
        return (folderOver != 0
                ||  folderQueue.size() != 0
                ||   group0 > 0)
            ;
    }

    bool hasFile()
    {
        folder_mut.lock();
        //          data.folder_mut.lock();
        auto res = (scanFolder() ||
                    fileFolderQueue.size() != 0);

        folder_mut.unlock();
        return res;
            //fileQueue.size() != 0 );

    }


};

class InputProcessor : public QRunnable {
public:
    InputProcessor(Data &data) : data(data) {}

    void run() override {

        while (data.scanFolder())
        {
            auto rec = data.folderQueue.pop();
            if (rec)
            {
                {
                    QMutexLocker lock(&data.mgroup0);
                    data.group0 ++;
                }

                QDirIterator it(*rec);
                int add = 0;
                while (it.hasNext())
                {
                    QString file = it.next();
                    if (file.endsWith(".") || file.endsWith(".."))
                        continue;

                    data.folder_mut.lock();
                    data.ongoingfolder.insert(*rec);
                    data.folder_mut.unlock();

                    auto finfo = it.fileInfo();
                    if (finfo.isDir() && (!data.older.isNull() && finfo.lastModified() < data.older))
                    {
                        data.folderQueue.push(finfo.absoluteFilePath());
                        data.folderOver++;
                    }
                    else if (finfo.isFile() && finfo.lastModified() > data.ts)
                    {
                        data.folder_mut.lock();
                        QString filepath = finfo.absoluteFilePath();
                        if (filepath.endsWith(".tif") && data.tar > 0)
                        { // advanced mode for tif / tar handling consider the tar as a folder
                            data.fileFolderQueue[filepath.mid(0, filepath.size() -data.tar - 4)].push_back(filepath.mid(data.indir.length()));
                        }
                        else // basic mode process folder by folder
                            data.fileFolderQueue[*rec].push_back(filepath.mid(data.indir.length()));
                        data.folder_mut.unlock();

                        //                        data.fileQueue.push(finfo.absoluteFilePath().mid(data.indir.length()));
                    }
                    add++;
                }
                data.folder_mut.lock();
                data.ongoingfolder.remove(*rec);
                data.folder_mut.unlock();

                data.prepared+=add;
                {
                    QMutexLocker lock(&data.mgroup0);
                    data.group0 --;
                    data.folderOver--;
                }

            }
        }

    }

private:
    Data &data;
};



class FileProcessor : public QRunnable {
public:
    FileProcessor(Data &data) : data(data) {}


    int compress(const cv::Mat& im, QByteArray& compressed)
    {

        auto enc = JxlEncoderMake(nullptr);
        auto runner = JxlThreadParallelRunnerMake(
            /*memory_manager=*/nullptr,
            JxlThreadParallelRunnerDefaultNumWorkerThreads());

        if (JXL_ENC_SUCCESS != JxlEncoderSetParallelRunner(enc.get(),
                                                           JxlThreadParallelRunner,
                                                           runner.get())) {
            std::cerr << "JxlEncoderSetParallelRunner failed" << std::endl;
            return -1;
        }

        JxlPixelFormat pixel_format = { 1, JXL_TYPE_UINT16, JXL_NATIVE_ENDIAN, 0 };
        JxlBasicInfo basic_info;
        basic_info.uses_original_profile = JXL_TRUE;
        JxlEncoderInitBasicInfo(&basic_info);
        basic_info.xsize = im.cols;
        basic_info.ysize = im.rows;

        basic_info.bits_per_sample = 16;
        // //    basic_info.exponent_bits_per_sample = 8;
        //    basic_info.uses_original_profile = JXL_FALSE; // W
        basic_info.num_color_channels = 1;
        basic_info.uses_original_profile = JXL_TRUE;

        if (JXL_ENC_SUCCESS != JxlEncoderSetBasicInfo(enc.get(), &basic_info)) {
            std::cerr << "JxlEncoderSetBasicInfo failed" << std::endl;
            return -1;
        }



        JxlColorEncoding color_encoding = { };

        JxlColorEncodingSetToLinearSRGB(&color_encoding, true);
        color_encoding.color_space = JXL_COLOR_SPACE_GRAY;
        if (JXL_ENC_SUCCESS !=
            JxlEncoderSetColorEncoding(enc.get(), &color_encoding)) {
            std::cerr << "JxlEncoderSetColorEncoding failed" << std::endl;
            return -1;
        }

        //color_encoding.u


        JxlEncoderFrameSettings* frame_settings =
            JxlEncoderFrameSettingsCreate(enc.get(), nullptr);


        JxlEncoderFrameSettingsSetOption(frame_settings, JXL_ENC_FRAME_SETTING_EFFORT, 6);

        if (JXL_ENC_SUCCESS !=
            JxlEncoderSetFrameLossless(frame_settings, true))
            std::cerr << "Warning Lossless mode not set!" << std::endl;

        JxlEncoderFrameSettingsSetOption(frame_settings, JXL_ENC_FRAME_SETTING_MODULAR, 1); // force lossless
        JxlEncoderFrameSettingsSetOption(frame_settings, JXL_ENC_FRAME_SETTING_KEEP_INVISIBLE, 1);
        ///JxlEncoderSetFrameDistance(frame_settings,  1e-9);



        if (JXL_ENC_SUCCESS !=
            JxlEncoderAddImageFrame(frame_settings, &pixel_format,
                                    (void*)im.ptr(), im.total() * sizeof(unsigned short int))) {
            std::cerr << "JxlEncoderAddImageFrame failed" << std::endl;
            return -1;
        }
        JxlEncoderCloseInput(enc.get());


        compressed.resize(64);
        uint8_t* next_out = (uint8_t*)compressed.data();
        size_t avail_out = compressed.size() - (next_out - (uint8_t*)compressed.data());
        JxlEncoderStatus process_result = JXL_ENC_NEED_MORE_OUTPUT;
        while (process_result == JXL_ENC_NEED_MORE_OUTPUT) {
            process_result = JxlEncoderProcessOutput(enc.get(), &next_out, &avail_out);
            if (process_result == JXL_ENC_NEED_MORE_OUTPUT) {
                size_t offset = next_out - (uint8_t*)compressed.data();
                compressed.resize(compressed.size() * 2);
                next_out = (uint8_t*)compressed.data() + offset;
                avail_out = compressed.size() - offset;
            }
        }
        compressed.resize(next_out - (uint8_t*)compressed.data());
        if (JXL_ENC_SUCCESS != process_result) {
            std::cerr << "JxlEncoderProcessOutput failed" << std::endl;
            return -1;
        }
        return 0;
    }


    void writeFile(QString infile, QByteArray q)
    {
        { // Check folder path
            QStringList folders = (infile).replace("\\", "/").split("/");
            folders.pop_back();
            QDir d;
            d.mkpath(data.outdir+"/"+folders.join("/"));
        }

        QString file = data.outdir + "/" + (infile);
        //            qDebug() << "Writing" << file;
        QFile res(file);
        if (res.open(QIODevice::WriteOnly))
        {
            res.write(q);
        }
        else
        {

            qDebug() << "Write Thread Error:" << res.errorString();
            qDebug() << file << "(" << this->data.outdir << (infile) << ")";
            exit(0);
        }
        res.close();

        data.writen += q.size();
    }

    void run() override {
        //        qDebug() << QThread::currentThreadId() << "File Started";

        while (data.hasFile())
        {
            data.folder_mut.lock();
            if (data.fileFolderQueue.size() == 0) { data.folder_mut.unlock(); QThread::msleep(50); continue; }
            auto folder = data.fileFolderQueue.firstKey();
            if (data.fileFolderQueue.first().size() == 0) { data.folder_mut.unlock(); QThread::msleep(50); continue; }
            auto infile = data.fileFolderQueue.first().takeFirst();
            bool finished_folder = data.fileFolderQueue.first().empty() && ! data.ongoingfolder.contains(folder);
            if (finished_folder) data.fileFolderQueue.remove(folder);
            data.folder_mut.unlock();

            if (finished_folder)
            {
                data.console.lock();
                qDebug() << "Finished Folder" << folder;
                data.console.unlock();
            }

            //            auto infile = data.fileQueue.pop();
            if (!infile.isEmpty())
            {
                {
                    QMutexLocker lock(&data.mgroup1);
                    data.group1 ++;
                }
                // check if the file is absent from destination !!!
                QString jxl = infile;
                QString subname = infile.split("/").last();


                if (jxl.endsWith(".tif"))
                {
                    jxl.chop(4);
                    jxl += ".jxl";
                }
                //            qDebug()  << "jxl" << QString("%1/%2").arg(data.outdir, jxl);

                if (data.tar  > 0 && data.tar < subname.size() && infile.endsWith(".tif")) // handling tar file for transcoded data
                { // We will only put transcoded images in the tar archive
                    auto tarfile = data.outdir + infile.mid(0, infile.size() - data.tar - 4 ) + ".tar";
                    struct archive *ar = nullptr;
                    data.tar_lock.lock();
                    if (data.tarobjects[folder].contains(tarfile))
                        ar = data.tarobjects[folder][tarfile];
                    else
                    {
//                        qDebug() << "Creating tar: " << tarfile << "(" << infile << ")";
                        ar = archive_write_new();
                        archive_write_set_format(ar, ARCHIVE_FORMAT_TAR);
                        int r = archive_write_open_filename(ar, tarfile.toLatin1().data());
                        if (r != ARCHIVE_OK)
                        {
                            qDebug()  << archive_error_string(ar);
                        }
                        data.tarobjects[folder][tarfile] = ar;
                    }
                    data.tar_close[ar]++;
                    data.tar_lock.unlock();

                    // now do our stuff with the data
                    QString file(QString("%1/%2").arg(data.indir, infile));
                    QFile reader(file);
                    if (reader.open(QIODevice::ReadOnly))
                    {
                        QByteArray q = reader.readAll();
                        reader.close();
                        data.readed += q.size();
                        if (q.size() > 0)
                        {
                            unsigned char* png;
                            unsigned w, h;
                            size_t nb_chans = 1, bitdepth = 16;

                            auto parallel_runner = [](void* , void* opaque,
                                                      void fun(void*, size_t), size_t count){
                                for (size_t i = 0; i < count; ++i)
                                    fun(opaque, i);
                            };

                            cv::Mat m(q.length(), 1, CV_8U, q.data());

                            auto im =  cv::imdecode(m, -1);
                            m.release();

                            if (im.cols != 0 || im.rows != 0)
                            {
                                w = im.cols;
                                h = im.rows;
                                png = (unsigned char*)im.ptr();
                                size_t stride = w * nb_chans * (bitdepth > 8 ? 2 : 1);

                                size_t num_threads=1;
                                int encoded_size = 0;
                                QByteArray compressed;
                                if (data.effort <= 0)
                                {
                                    unsigned char * encoded = nullptr;

                                    encoded_size = JxlFastLosslessEncode(png,
                                                                         w, stride, h,
                                                                         nb_chans,
                                                                         bitdepth, /*big_endian=*/false,
                                                                         32, &encoded, &num_threads, +parallel_runner);

                                    compressed = QByteArray((const char*)encoded, encoded_size);
                                    free(encoded);
                                }
                                else
                                {
                                    if (0 != compress(im, compressed))
                                    {
                                        qDebug() << "Compression error" << (infile) << jxl;
                                        exit(-1);
                                    }
                                }
                                jxl = jxl.split("/").last();

                                struct archive_entry *entry = archive_entry_new();
                                int r;
                                archive_entry_set_pathname(entry, jxl.toLatin1().data());
                                archive_entry_set_size(entry, compressed.size());
                                archive_entry_set_filetype(entry, AE_IFREG);

                                data.tar_lock.lock();

                                data.tar_remove[folder] << infile;

                                r = archive_write_header(ar, entry);
                                if (r != ARCHIVE_OK)
                                {
                                    qDebug()  <<  archive_error_string(ar);
                                }

                                r = archive_write_data(ar, compressed.data(), compressed.size());
                                if (r != compressed.size())
                                {
                                    qDebug() << archive_error_string(ar);
                                }


                                data.tar_close[ar]--;

                                data.tar_lock.unlock();
                                data.writen += compressed.size();

                                archive_entry_free(entry);
                            }
                        }
                    }


                }
                else if (!QFileInfo::exists(QString("%1/%2").arg(data.outdir, jxl)) || data.effort > 0)
                {
                    QString file(QString("%1/%2").arg(data.indir, infile));
                    //                qDebug() << "Loading file for copy" << file;
                    data.copy_count++;
                    data.console.lock();

                    //                std::cout << "\r" << "read" << data.prepared << "=>"<< data.write_count << "/" << data.copy_count << " " <<data.fileQueue.size() << " " << data.write_count / (float)data.copy_count << " " << data.memusage << "                                         \n";

                    std::cout << "\r" << data.prepared << "=>"<< data.write_count << "/" << data.copy_count << " " << data.write_count / (float)data.copy_count
                              << "                                         ";
                    std::flush(std::cout);
                    data.console.unlock();


                    //                data.inputSemaphore.acquire(); // allow a controlled concurrent access to hard drive
                    QFile reader(file);
                    if (! data.dry_run && reader.open(QIODevice::ReadOnly))
                    {
                        QByteArray q = reader.readAll();
                        reader.close();
                        data.readed += q.size();
                        if (q.size() > 0)
                        {
                            if (file.endsWith(".tif") )
                            {
                                QString jxl=(infile); jxl.replace(".tif", ".jxl");


                                unsigned char* png;
                                unsigned w, h;
                                size_t nb_chans = 1, bitdepth = 16;

                                auto parallel_runner = [](void* , void* opaque,
                                                          void fun(void*, size_t), size_t count){
                                    for (size_t i = 0; i < count; ++i)
                                        fun(opaque, i);
                                };

                                cv::Mat m(q.length(), 1, CV_8U, q.data());

                                auto im =  cv::imdecode(m, -1);
                                m.release();

                                if (im.cols != 0 || im.rows != 0)
                                {
                                    w = im.cols;
                                    h = im.rows;
                                    png = (unsigned char*)im.ptr();
                                    size_t stride = w * nb_chans * (bitdepth > 8 ? 2 : 1);

                                    size_t num_threads=1;
                                    int encoded_size = 0;
                                    QByteArray compressed;
                                    if (data.effort <= 0)
                                    {
                                        unsigned char * encoded = nullptr;

                                        encoded_size = JxlFastLosslessEncode(png,
                                                                             w, stride, h,
                                                                             nb_chans,
                                                                             bitdepth, /*big_endian=*/false,
                                                                             32, &encoded, &num_threads, +parallel_runner);

                                        compressed = QByteArray((const char*)encoded, encoded_size);
                                        free(encoded);
                                    }
                                    else
                                    {
                                        if (0 != compress(im, compressed))
                                        {
                                            qDebug() << "Compression error" << (infile) << jxl;
                                            exit(-1);
                                        }
                                    }

                                    writeFile(jxl, compressed);
                                    im.release();
                                    if (data.inplace)
                                    {
                                        auto res = QFile::remove(file);
                                        if (!res)
                                            qDebug() << "File not removed" << file;
                                    }
                                }
                            }
                            else
                            {
                                writeFile(infile, q);
                            }
                        }

                    }

                    {
                        QMutexLocker lock(&data.mgroup1);
                        data.group1 --;
                    }
                }
            }


            if (finished_folder && data.tar > 0)
            { // close all the open archives
//                qDebug() << "Closing tars";
                for (auto& tarf: data.tarobjects[folder])
                {
                    while (data.tar_close[tarf] !=0)
                        QThread::msleep(10);

                    archive_write_close(tarf);
                    archive_write_free(tarf);
                }
                data.tarobjects.remove(folder);
                if (data.inplace)
                    for (auto rm_file: data.tar_remove[folder])
                        if (!QFile::remove(rm_file))
                            qDebug() << "File not removed" << rm_file;

                data.tar_remove.remove(folder);
            }
        }
    }

private:
    Data &data;
};


#include <cmath>

QString si(size_t value)
{
    QVector<QString> units= {"TB" , "GB", "MB", "kB",  "B"};
    for (int i = 0; i < units.length(); ++i)
    {
        float pw = std::pow(1000, units.length()-i-1);
        if (value > pw)
        {
            return QString("%1 %2").arg(value/pw, 6).arg(units[i]);
        }
    }

    return QString("%1").arg(value);
}

QDateTime parseDate(QString older)
{
    QDateTime date = QDateTime::currentDateTime();
    bool conv;
    int days = older.toInt(&conv);

    if (conv)
    {
        date=date.addDays(-days);
    }
    else
    {
        int start = 0;
        for (int i = 0; i < older.length(); ++i)
        {
            if (older.at(i) == 'y')
            {
                date=date.addYears(-older.sliced(start, i-start).toInt());
                qDebug() << "Year" << date << older.sliced(start, i-start);
                start = i+1;
            }
            if (older.at(i) == 'm')
            {
                date=date.addMonths(-older.sliced(start, i-start).toInt());
                qDebug() << "Month" << date << older.sliced(start, i-start);
                start = i+1;
            }
            if (older.at(i) == 'd')
            {
                date=date.addDays(-older.sliced(start, i-start).toInt());
                qDebug() << "Day" << date << older.sliced(start, i-start);
                start = i+1;
            }

        }
    }
    return date;
}

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);
    QCommandLineParser parser;
    parser.addPositionalArgument("indir", "input directory");
    parser.addPositionalArgument("outdir", "output directory");

    QSettings settings;

    QMap<QString, QString> opts = {{ "scan-threads", "2" },
                                   {"input-threads", "16" },
                                   {"older", "0M"} };

    for (auto it = opts.begin(), end = opts.end(); it != end; ++it)
    {

        QString value = settings.value(it.key(), it.value()).toString();
        parser.addOption(QCommandLineOption(it.key(),QString("number of %1 (default: %2)").arg(it.key(), value), it.key(), value));
    }

    parser.addOption(QCommandLineOption("rescan", "force a full rescan of the folder"));
    parser.addOption(QCommandLineOption("dry-run", "Skip the writing at the end"));
    parser.addOption(QCommandLineOption("effort", "Set the compression effort (default: -1)", "effort", "-1"));

    parser.addOption(QCommandLineOption("tar", "Generate tar files after removing n char from original file name", "tar", "22"));

    parser.addOption(QCommandLineOption("help", "Display the usage help"));



    parser.process(a);

    if (parser.isSet("help"))
    {
        std::cerr <<   parser.helpText().toStdString();
        return 0;
    }



    QStringList positionalArguments = parser.positionalArguments();

    // 1 or 2 args
    if (positionalArguments.size() > 2 || positionalArguments.size() == 0) {
        qCritical() << "indir and/or outdir arguments are required";
        std::cerr <<   parser.helpText().toStdString();

        return 1;
    }

    Data data;

    if (positionalArguments.size() == 2)
    {
        data.folderQueue.push(positionalArguments.at(0)); // Set the first directory to parse !!!
        data.indir = positionalArguments.at(0);
        data.outdir = positionalArguments.at(1);
    }
    else
    {
        data.folderQueue.push(positionalArguments.at(0)); // Set the first directory to parse !!!
        data.indir = positionalArguments.at(0);
        data.outdir = positionalArguments.at(0);
    }


    if (parser.isSet("tar"))
    {
        std::cout << "Tar mode on, will remove " << parser.value("tar").toInt() << " character after extension to create tar file";
        data.tar =  parser.value("tar").toInt() ;
    }


    data.indir.replace("\\", "/");
    data.outdir.replace("\\", "/");


    data.inplace = (data.indir == data.outdir);

    if (data.inplace) qDebug() << "In place processing";

    int scanThreads = parser.value("scan-threads").toInt();
    int inputThreads = parser.value("input-threads").toInt();

    if (parser.isSet("older"))
    {
        QString older = parser.value("older").toLower();

        data.older = parseDate(older);
        qDebug() << "Only taking folder into accont if they are older than " << data.older.toString();
        //        qDebug() << older.toInt();

    }
    else
        data.older = QDateTime::currentDateTime();

    data.dry_run = parser.isSet("dry-run");
    qDebug() << data.indir << data.outdir;

    QThreadPool::globalInstance()->setMaxThreadCount(scanThreads + inputThreads );



    // back forward 1 day to ensure we are re-investiating folder that had remaining acq pending
    data.ts = settings.value("timestamp", QDateTime::fromSecsSinceEpoch(0)).toDateTime().addDays(-1);
    // if rescan is required just get back to epoch :p
    if (parser.isSet("rescan"))
        data.ts = QDateTime::fromSecsSinceEpoch(0);
    data.effort = parser.value("effort").toInt();

    // wath ever happens now just store the timestamp
    settings.setValue("timestamp", QDateTime::currentDateTime());

    for (auto it = opts.begin(), end = opts.end(); it != end; ++it)
    {
        settings.setValue(it.key(), parser.value(it.key()));
    }


    //    QDateTime
    QElapsedTimer timer;

    timer.start();

    // Start input processing threads to scan files
    for (int i = 0; i < scanThreads; ++i) {
        QThreadPool::globalInstance()->start(new InputProcessor(data));
    }

    // Start file loader threads  to pool data
    for (int i = 0; i < inputThreads; ++i) {
        QThreadPool::globalInstance()->start(new FileProcessor(data));
    }

    // Wait for all threads to finish
    QThreadPool::globalInstance()->waitForDone();


    QTime t(0,0);
    t = t.addMSecs(timer.elapsed());

    qDebug() << "\nSynchronisation of" << si(data.readed) << "in" << t.toString("hh:mm:ss.zzz");
    if (data.readed != 0)
        qDebug() << "End file compression " << si(data.writen) << QString("%1%").arg(100.*(data.writen/(float)data.readed),5,'f',2,'0');

    return 0;
}
