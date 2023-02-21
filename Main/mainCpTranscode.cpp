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



#include "jxl/encode.h"
#include "jxl/encode_cxx.h"

#include "jxl/decode.h"
#include "jxl/decode_cxx.h"
#include "jxl/thread_parallel_runner.h"
#include "jxl/thread_parallel_runner_cxx.h"
#include "jxl/resizable_parallel_runner.h"
#include "jxl/resizable_parallel_runner_cxx.h"


struct Data {

    QString outdir;
    QString indir;
    bool dry_run = false, inplace = false;
    int effort;

    std::queue<QString> folderQueue;
    std::atomic<int> folderOver = 1;
    QMutex folder_mut;
    // First x threads will recursively seek folders and dig into them


    std::queue<QString> fileQueue;
    QMutex inputQueueMutex;
    std::atomic<long long> memusage = 0;

    std::atomic<long long> copy_count=0, write_count=0;
    QMutex console;

    std::queue<std::pair<QString, QByteArray>> compressQueue;
    std::queue<std::pair<QString, QByteArray>> writeQueue;

    QMutex compressQueueMutex;
    QMutex writeQueueMutex;

    long long max_data;
    std::atomic<long long> prepared = 0, readed = 0, writen = 0;

    QDateTime ts;

    std::atomic<long long> group0 = 0, group1 = 0, group2 = 0, group3 = 0;
    QMutex mgroup0, mgroup1, mgroup2, mgroup3;


    void adjust_mem(int mem)
    {
        //        qDebug() << "Pre Mem usage" << memusage << mem;
        memusage.fetch_add(mem);
        //        qDebug() << "Post Mem usage" << memusage << mem;

    }



    bool isrunning(){
        return (folderOver != 0
                || ! folderQueue.empty()
                || !fileQueue.empty()
                || !compressQueue.empty()
                || !writeQueue.empty()
                ||  group0 > 0
                ||  group1 > 0
                ||  group2 > 0
                ||  group3 > 0
                );
    }

};

class InputProcessor : public QRunnable {
public:
    InputProcessor(Data &data) : data(data) {}

    void run() override {
//        qDebug() << "Start scan";
        data.folder_mut.lock();
        while (data.isrunning())
        {

            if (data.folderQueue.empty())
            {
                data.folder_mut.unlock();
                QThread::msleep(100);
                data.folder_mut.lock();
                {
                    QMutexLocker lock(&data.mgroup0);
                    if (data.group0 == 0)
                    {
                        //                        qDebug() << "Releasing scan thread";
                        break;
                    }
                }
                continue;
            }

            {
                QMutexLocker lock(&data.mgroup0);
                data.group0 ++;
            }

            auto rec = data.folderQueue.front();
            data.folderQueue.pop();
            data.folder_mut.unlock();

            QDirIterator it(rec);
            int add = 0;
            while (it.hasNext())
            {
                QString file = it.next();
                if (file.endsWith(".") || file.endsWith(".."))
                    continue;

                auto finfo = it.fileInfo();
                if (finfo.isDir())
                {
//                    qDebug() << "Directorrrryyy" << finfo.absoluteFilePath();
                    data.folder_mut.lock();
                    data.folderOver ++;
                    data.folderQueue.push(finfo.absoluteFilePath());
                    data.folder_mut.unlock();
                }
                else if (finfo.isFile() && finfo.lastModified() > data.ts)
                {
 //                   qDebug() << "File" << finfo.absoluteFilePath();
                    QMutexLocker lock(&data.inputQueueMutex);
                    data.fileQueue.push(finfo.absoluteFilePath().mid(data.indir.length()));
                }
                add++;
            }
//            qDebug() << "Scan thread finished" << add;


            data.folder_mut.lock();
            data.prepared+=add;
            data.folderOver--;
            {
                QMutexLocker lock(&data.mgroup0);
                data.group0 --;
                if (data.group0 == 0 && data.folderQueue.empty())
                {
                    //                    qDebug() << "Scan Done";
                    break; // scan is over
                }
            }

        }
        data.folder_mut.unlock();
    }

private:
    Data &data;
};



class FileProcessor : public QRunnable {
public:
    FileProcessor(Data &data) : data(data) {}

    void run() override {

        data.inputQueueMutex.lock();
        while (data.isrunning())
        {

            {// memory consumption checks
                //                QMutexLocker lock(&data.compressQueueMutex);
                if (data.memusage > data.max_data)
                {
                    //                    qDebug() << "Above Memory" << data.memusage << "/"<< data.max_data;
                    data.inputQueueMutex.unlock();

                    QThread::msleep(200); // wait for memory consumtion

                    data.inputQueueMutex.lock();
                    continue;
                }
            }
            if (data.fileQueue.empty())
            {
                data.inputQueueMutex.unlock();

                QThread::msleep(200); // wait for memory consumtion

                data.inputQueueMutex.lock();
                continue;
            }
            {
                QMutexLocker lock(&data.mgroup1);
                data.group1 ++;
            }

            auto infile = data.fileQueue.front();
            data.fileQueue.pop();
            data.inputQueueMutex.unlock();

            // check if the file is absent from destination !!!
            QString jxl = infile;

            if (jxl.endsWith(".tif"))
            {
                jxl.chop(4);
                jxl += ".jxl";
            }
//            qDebug()  << "jxl" << QString("%1/%2").arg(data.outdir, jxl);
            if (!QFileInfo::exists(QString("%1/%2").arg(data.outdir, jxl)))
            {
                QString file(QString("%1/%2").arg(data.indir, infile));
//                qDebug() << "Loading file for copy" << file;
                data.copy_count++;
                data.console.lock();

                //                std::cout << "\r" << "read" << data.prepared << "=>"<< data.write_count << "/" << data.copy_count << " " <<data.fileQueue.size() << " " << data.write_count / (float)data.copy_count << " " << data.memusage << "                                         \n";

                std::cout << "\r" << data.prepared << "=>"<< data.write_count << "/" << data.copy_count << " " << data.write_count / (float)data.copy_count << "                                         ";
                std::flush(std::cout);
                data.console.unlock();


                //                data.inputSemaphore.acquire(); // allow a controlled concurrent access to hard drive
                QFile reader(file);
                if (reader.open(QIODevice::ReadOnly))
                {
                    QByteArray q = reader.readAll();
                    if (q.size() > 0)
                    {
                        data.adjust_mem(q.size());

                        if (file.endsWith(".tif") )
                        {
                            QMutexLocker locker(&data.compressQueueMutex);
                            data.compressQueue.push(std::make_pair(infile, q));
                        }
                        else
                        {
                            QMutexLocker lock(&data.writeQueueMutex);
                            if (!data.dry_run)
                                data.writeQueue.push(std::make_pair(infile, q));
                        }
                        data.inputQueueMutex.lock();
                        data.readed += q.size();
                        data.inputQueueMutex.unlock();
                    }
                }

            }

            data.inputQueueMutex.lock();
            {
                QMutexLocker lock(&data.mgroup1);
                data.group1 --;
            }

        }
        data.inputQueueMutex.unlock();
    }

private:
    Data &data;
};



class CompressProcessor : public QRunnable {
public:
    CompressProcessor(Data &data)
        : data(data) {}



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




    void run() override {
        // Here we compress the tif files if any :)
        // otherwise direct rewrite of the files
        data.compressQueueMutex.lock();
        while (data.isrunning())
        {
            {
                if (data.compressQueue.empty())
                {
                    data.compressQueueMutex.unlock();
                    QThread::msleep(200); // wait for memory consumtion
                    data.compressQueueMutex.lock();
                    continue;
                }
            }

            {
                QMutexLocker lock(&data.mgroup2);
                data.group2 ++;
            }

            auto comp = data.compressQueue.front();
            data.compressQueue.pop();
            data.compressQueueMutex.unlock();

            //                qDebug() << "jxl'ify" << comp.first;
            QString jxl=comp.first; jxl.replace(".tif", ".jxl");

            // generate the jxl file :)


            unsigned char* png;
            unsigned w, h;
            size_t nb_chans = 1, bitdepth = 16;
            int effort = 32;


            auto parallel_runner = [](void* , void* opaque,
                    void fun(void*, size_t), size_t count){
                for (size_t i = 0; i < count; ++i)
                    fun(opaque, i);
            };


            cv::Mat m(comp.second.length(), 1, CV_8U, comp.second.data());
            auto im =  cv::imdecode(m, -1);
            w = im.cols;
            h = im.rows;
            png = (unsigned char*)im.ptr();
            size_t stride = w * nb_chans * (bitdepth > 8 ? 2 : 1);

            size_t num_threads=1;
            int encoded_size = 0;
            QByteArray compressed;
            if (effort <= 0)
            {
                unsigned char * encoded = nullptr;

                encoded_size = JxlFastLosslessEncode(png,
                                                     w, stride, h,
                                                     nb_chans,
                                                     bitdepth, /*big_endian=*/false,
                                                     effort, &encoded, &num_threads, +parallel_runner);

                 compressed = QByteArray((const char*)encoded, encoded_size);
                 delete encoded;

            }
            else
            {
                if (0 != compress(im, compressed))
                {
                    qDebug() << "Compression error" << comp.first << jxl;
                    exit(-1);
                }
            }
//            QByteArray compressed((const char*)encoded, encoded_size);

            data.writeQueueMutex.lock();
            if (!data.dry_run)
                data.writeQueue.push(std::make_pair(jxl, compressed));



            data.adjust_mem(-comp.second.length() );
            data.writeQueueMutex.unlock();

            comp.second.clear();
            data.compressQueueMutex.lock();
            {
                QMutexLocker lock(&data.mgroup2);
                data.group2 --;
            }

        }
        data.compressQueueMutex.unlock();
    }

private:
    Data &data;
};

class WriteProcessor : public QRunnable {
public:
    WriteProcessor(Data &data)
        : data(data) {}

    void run() override {
        data.writeQueueMutex.lock();
        while (data.isrunning())
        {
            {// memory consumption checks
                if (data.writeQueue.empty())
                {
                    data.writeQueueMutex.unlock();
                    QThread::msleep(200); // wait for memory consumtion
                    data.writeQueueMutex.lock();
                    continue;
                }
            }
            {
                QMutexLocker lock(&data.mgroup3);
                data.group3 ++;
            }


            auto comp = data.writeQueue.front();
            data.writeQueue.pop();
            data.writeQueueMutex.unlock();

            {
                QStringList folders = comp.first.replace("\\", "/").split("/");
                folders.pop_back();
                QDir d;
                d.mkpath(data.outdir+"/"+folders.join("/"));
            }


            QString file = data.outdir + "/" + comp.first;
            //            qDebug() << "Writing" << file;
            QFile res(file);
            if (res.open(QIODevice::WriteOnly))
            {
                res.write(comp.second);
            }
            else
            {

                qDebug() << "Write Thread Error:" << res.errorString();
                qDebug() << file << "(" << data.outdir << comp.first << ")";
                exit(0);
            }

            data.write_count++;
            data.console.lock();
            //            std::cout << "\r" << data.prepared << "=>"<< data.write_count << "/" << data.copy_count << " " <<data.fileQueue.size() << " " << data.write_count / (float)data.copy_count << " " << data.memusage << "                                         \n";

            std::cout << "\r" << data.prepared << "=>"<< data.write_count << "/" << data.copy_count << " " << data.write_count / (float)data.copy_count << "                                         ";
            std::flush(std::cout);
            data.console.unlock();
            data.writeQueueMutex.lock();
            data.writen += comp.second.length();

            {
                QMutexLocker lock(&data.mgroup3);
                data.group3--;
            }

            if (!file.endsWith(".jxl"))
            {
                data.adjust_mem(-comp.second.length());
            }
            else
            {
                if (data.inplace)
                {
                    QString tif = file; tif.chop(4); tif += ".tif";
                    QFile::remove(tif);
                }
            }

            comp.second.clear();
        }
        data.writeQueueMutex.unlock();
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

long long fromSi(QString value)
{
    value = value.trimmed().toUpper();
    std::map<QString, int> facts = {
        {"B", 1},
        {"O", 1},
        {"K", 1024},
        {"M", 1024*1024},
        {"G", 1024*1024*1024}
    };


    long long factor = 1;
    for (auto item = facts.begin(), end = facts.end(); item != end; ++item)
    {
        if (value.contains(item->first))
        {
            value.replace(item->first, "");
            factor *= item->second;
        }
    }

    value=value.trimmed();


    long long res = value.toLongLong();

    return res*factor;
}



int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);
    QCommandLineParser parser;
    parser.addPositionalArgument("indir", "input directory");
    parser.addPositionalArgument("outdir", "output directory");

    QSettings settings;

    QMap<QString, QString> opts = {{ "scan-threads", "2" },
                                   {"input-threads", "4" },
                                   {"compress-threads", "8"},
                                   {"write-threads", "4"},
                                   {"max-memory", "2Go"}};
    for (auto it = opts.begin(), end = opts.end(); it != end; ++it)
    {

        QString value = settings.value(it.key(), it.value()).toString();
        parser.addOption(QCommandLineOption(it.key(),QString("number of %1 (default: %2)").arg(it.key(), value), it.key(), value));
    }

    parser.addOption(QCommandLineOption("rescan", "force a full rescan of the folder"));
    parser.addOption(QCommandLineOption("dry-run", "Skip the writing at the end"));
    parser.addOption(QCommandLineOption("effort", "Set the compression effort (default: -1)", "effort", "-1"));

    parser.process(a);


    QStringList positionalArguments = parser.positionalArguments();

    if (positionalArguments.size() != 2) {
        qCritical() << "indir and outdir arguments are required";
        std::cerr <<   parser.helpText().toStdString();

        return 1;
    }

    Data data;

    data.inplace = (data.indir == data.outdir);

    if (data.inplace)
        qDebug() << "In place processing";


    data.folderQueue.push(positionalArguments.at(0)); // Set the first directory to parse !!!
    data.indir = positionalArguments.at(0);
    data.outdir = positionalArguments.at(1);


    int scanThreads = parser.value("scan-threads").toInt();
    int inputThreads = parser.value("input-threads").toInt();
    int compressThreads = parser.value("compress-threads").toInt();
    int writeThreads = parser.value("write-threads").toInt();


    //    data.inputSemaphore.acquire(inputThreads);
    //    data.compressSemaphore.acquire(compressThreads);
    //    data.writeSemaphore.acquire(writeThreads);

    data.dry_run = parser.isSet("dry-run");
    data.max_data = fromSi(parser.value("max-memory"));
    qDebug() << data.indir << data.outdir;// << "Max mem" << data.max_data;

    QThreadPool::globalInstance()->setMaxThreadCount(scanThreads + inputThreads + compressThreads + writeThreads);



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
    // Start compress processing threads
    for (int i = 0; i < compressThreads; ++i) {
        QThreadPool::globalInstance()->start(new CompressProcessor(data));
    }

    // Start write processing threads
    if (!data.dry_run)
        for (int i = 0; i < writeThreads; ++i) {
            QThreadPool::globalInstance()->start(new WriteProcessor(data));
        }

    // Wait for all threads to finish
    QThreadPool::globalInstance()->waitForDone();


    QTime t(0,0);
    t = t.addMSecs(timer.elapsed());

    qDebug() << "\nSynchronisation of" << si(data.readed) << "in" << t.toString("hh:mm:ss.zzz");
    qDebug() << "End file compression " << si(data.writen);



    return 0;
}
