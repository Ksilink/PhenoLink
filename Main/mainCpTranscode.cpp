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


struct Data {

    QString outdir;
    QString indir;
    bool dry_run = false, inplace = false;
    int effort;

    //    std::queue<QString> folderQueue;

    ThreadsafeQueue<QString> folderQueue;

    std::atomic<int> folderOver = 1;
    QMutex folder_mut;
    // First x threads will recursively seek folders and dig into them


    ThreadsafeQueue<QString> fileQueue;
    std::atomic<long long> copy_count=0, write_count=0;
    QMutex console;

    std::atomic<long long> prepared = 0, readed = 0, writen = 0;

    QDateTime ts;
    QDateTime older;

    std::atomic<long long> group0 = 0, group1 = 0;
    QMutex mgroup0, mgroup1;





    bool isrunning(){
        return (folderOver != 0
                ||  folderQueue.size() != 0
                || fileQueue.size() != 0
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
        return (scanFolder() || fileQueue.size() != 0 );

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

                    auto finfo = it.fileInfo();
                    if (finfo.isDir() && (!data.older.isNull() && finfo.lastModified() < data.older))
                    {
                        data.folderQueue.push(finfo.absoluteFilePath());
                        data.folderOver++;
                    }
                    else if (finfo.isFile() && finfo.lastModified() > data.ts)
                    {
                        data.fileQueue.push(finfo.absoluteFilePath().mid(data.indir.length()));
                    }
                    add++;
                }

                data.prepared+=add;
                {
                    QMutexLocker lock(&data.mgroup0);
                    data.group0 --;
                    data.folderOver--;
                }

            }
//            else
//            {
//                QThread::msleep(200); // to release the mutex burden if too many access
//            }
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

        data.readed += q.size();
    }

    void run() override {
        //        qDebug() << QThread::currentThreadId() << "File Started";

        while (data.hasFile())
        {
            auto infile = data.fileQueue.pop();
            if (!infile)
            {
                QThread::msleep(200); // file queue is empty wait
            }
            else
            {
                {
                    QMutexLocker lock(&data.mgroup1);
                    data.group1 ++;
                }
                // check if the file is absent from destination !!!
                QString jxl = *infile;

                if (jxl.endsWith(".tif"))
                {
                    jxl.chop(4);
                    jxl += ".jxl";
                }
                //            qDebug()  << "jxl" << QString("%1/%2").arg(data.outdir, jxl);
                if (!QFileInfo::exists(QString("%1/%2").arg(data.outdir, jxl)))
                {
                    QString file(QString("%1/%2").arg(data.indir, *infile));
                    //                qDebug() << "Loading file for copy" << file;
                    data.copy_count++;
                    data.console.lock();

                    //                std::cout << "\r" << "read" << data.prepared << "=>"<< data.write_count << "/" << data.copy_count << " " <<data.fileQueue.size() << " " << data.write_count / (float)data.copy_count << " " << data.memusage << "                                         \n";

                    std::cout << "\r" << data.prepared << "=>"<< data.write_count << "/" << data.copy_count << " " << data.write_count / (float)data.copy_count << "                                         ";
                    std::flush(std::cout);
                    data.console.unlock();


                    //                data.inputSemaphore.acquire(); // allow a controlled concurrent access to hard drive
                    QFile reader(file);
                    if (! data.dry_run && reader.open(QIODevice::ReadOnly))
                    {
                        QByteArray q = reader.readAll();
                        if (q.size() > 0)
                        {
                            if (file.endsWith(".tif") )
                            {
                                QString jxl=(*infile); jxl.replace(".tif", ".jxl");


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
                                            qDebug() << "Compression error" << (*infile) << jxl;
                                            exit(-1);
                                        }
                                    }

                                    writeFile(jxl, compressed);
                                    im.release();
                                    if (data.inplace)
                                    {
                                        QFile::remove(file);
                                    }
                                }




                            }
                            else
                            {
                                writeFile(*infile, q);
                            }
                        }
                        reader.close();

                    }

                    {
                        QMutexLocker lock(&data.mgroup1);
                        data.group1 --;
                    }
                }
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

QDateTime parseDate(QStringView older)
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
                start = i+1;
            }
            if (older.at(i) == 'm')
            {
                date=date.addMonths(-older.sliced(start, i-start).toInt());
                start = i+1;
            }
            if (older.at(i) == 'd')
            {
                date=date.addDays(-older.sliced(start, i-start).toInt());
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

    data.indir.replace("\\", "/");
    data.outdir.replace("\\", "/");


    data.inplace = (data.indir == data.outdir);

    if (data.inplace)
        qDebug() << "In place processing";




    int scanThreads = parser.value("scan-threads").toInt();
    int inputThreads = parser.value("input-threads").toInt();

    if (parser.isSet("older"))
    {
        QStringView older = parser.value("older").toLower();

        data.older = parseDate(older);
        qDebug() << "Only taking folder into accont if they are older than " << data.older.toString();
        //        qDebug() << older.toInt();

    }
    else
        data.older = QDateTime::currentDateTime();

    //    data.inputSemaphore.acquire(inputThreads);
    //    data.compressSemaphore.acquire(compressThreads);
    //    data.writeSemaphore.acquire(writeThreads);

    data.dry_run = parser.isSet("dry-run");
    qDebug() << data.indir << data.outdir;// << "Max mem" << data.max_data;

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
    qDebug() << "End file compression " << si(data.writen);

    return 0;
}
