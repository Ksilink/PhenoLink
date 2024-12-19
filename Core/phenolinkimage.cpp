
#include <QString>
#include <QByteArray>
#include <QFile>
#include <QDebug>

#include "phenolinkimage.h"


#include "jxl/encode.h"
#include "jxl/encode_cxx.h"

#include "jxl/decode.h"
#include "jxl/decode_cxx.h"
#include "jxl/thread_parallel_runner.h"
#include "jxl/thread_parallel_runner_cxx.h"
#include "jxl/resizable_parallel_runner.h"
#include "jxl/resizable_parallel_runner_cxx.h"

#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include <sys/types.h>

#include <sys/stat.h>

#include <archive.h>
#include <archive_entry.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef CZI
#include "libCZI_Config.h"
#include "CZIReader.h"

#endif


// a jxl decode function


int decode(QByteArray& comp, cv::Mat& reconstructed)
{

    std::vector<uint8_t> icc_profile;
    // Multi-threaded parallel runner.
    auto runner = JxlResizableParallelRunnerMake(nullptr);

    auto dec = JxlDecoderMake(nullptr);
    if (JXL_DEC_SUCCESS !=
        JxlDecoderSubscribeEvents(dec.get(), JXL_DEC_BASIC_INFO |
                                                 JXL_DEC_COLOR_ENCODING |
                                                 JXL_DEC_FULL_IMAGE)) {
        fprintf(stderr, "JxlDecoderSubscribeEvents failed\n");
        return false;
    }

    if (JXL_DEC_SUCCESS != JxlDecoderSetParallelRunner(dec.get(),
                                                       JxlResizableParallelRunner,
                                                       runner.get())) {
        fprintf(stderr, "JxlDecoderSetParallelRunner failed\n");
        return false;
    }

    JxlBasicInfo info;
    JxlPixelFormat format = { 1, JXL_TYPE_UINT16, JXL_NATIVE_ENDIAN, 0 };

    uint8_t* jxl = (uint8_t*)comp.data();

    JxlDecoderSetInput(dec.get(), jxl, comp.size());
    JxlDecoderCloseInput(dec.get());
    int xsize = 0, ysize = 0;
    //    std::vector<unsigned short> pixels;

    for (;;) {
        JxlDecoderStatus status = JxlDecoderProcessInput(dec.get());

        if (status == JXL_DEC_ERROR) {
            fprintf(stderr, "Decoder error\n");
            return false;
        }
        else if (status == JXL_DEC_NEED_MORE_INPUT) {
            fprintf(stderr, "Error, already provided all input\n");
            return false;
        }
        else if (status == JXL_DEC_BASIC_INFO) {
            if (JXL_DEC_SUCCESS != JxlDecoderGetBasicInfo(dec.get(), &info)) {
                fprintf(stderr, "JxlDecoderGetBasicInfo failed\n");
                return false;
            }
            xsize = info.xsize;
            ysize = info.ysize;
            //            std::cout << "Data " << xsize << " " << ysize << std::endl;
            JxlResizableParallelRunnerSetThreads(
                runner.get(),
                JxlResizableParallelRunnerSuggestThreads(info.xsize, info.ysize));
        }
        else if (status == JXL_DEC_COLOR_ENCODING) {
            // Get the ICC color profile of the pixel data
           /*
            size_t icc_size;
            if (JXL_DEC_SUCCESS !=
                JxlDecoderGetICCProfileSize(
                    dec.get(), &format, JXL_COLOR_PROFILE_TARGET_DATA, &icc_size)) {
                fprintf(stderr, "JxlDecoderGetICCProfileSize failed\n");
                return false;
            }
            icc_profile.resize(icc_size);
            if (JXL_DEC_SUCCESS != JxlDecoderGetColorAsICCProfile(
                    dec.get(), &format,
                    JXL_COLOR_PROFILE_TARGET_DATA,
                    icc_profile.data(), icc_profile.size())) {
                fprintf(stderr, "JxlDecoderGetColorAsICCProfile failed\n");
                return false;
            }*/
        }
        else if (status == JXL_DEC_NEED_IMAGE_OUT_BUFFER) {
            size_t buffer_size;
            if (JXL_DEC_SUCCESS !=
                JxlDecoderImageOutBufferSize(dec.get(), &format, &buffer_size)) {
                fprintf(stderr, "JxlDecoderImageOutBufferSize failed\n");
                return false;
            }
            if (buffer_size != size_t(xsize * ysize * 2)) {
                std::cerr << "Invalid out buffer size %" << static_cast<uint64_t>(buffer_size) << " %" << static_cast<uint64_t>(xsize * ysize * 2) << std::endl;
                return false;
            }

            //            pixels.resize(xsize * ysize);
            reconstructed=cv::Mat(ysize, xsize, CV_16U);
            void* pixels_buffer = (void*)reconstructed.data;//pixels.data();
            size_t pixels_buffer_size = reconstructed.total()*2;
            if (JXL_DEC_SUCCESS != JxlDecoderSetImageOutBuffer(dec.get(), &format,
                                                               pixels_buffer,
                                                               pixels_buffer_size)) {
                fprintf(stderr, "JxlDecoderSetImageOutBuffer failed\n");
                return false;
            }
        }
        else if (status == JXL_DEC_FULL_IMAGE) {
            // Nothing to do. Do not yet return. If the image is an animation, more
            // full frames may be decoded. This example only keeps the last one.
        }
        else if (status == JXL_DEC_SUCCESS) {
            // All decoding successfully finished.
            // It's not required to call JxlDecoderReleaseInput(dec.get()) here since
            // the decoder will be destroyed.
            //            cv::Mat(ysize, xsize, CV_16U, pixels.data()).copyTo(reconstructed);
            return true;
        }
        else {
            fprintf(stderr, "Unknown decoder status\n");
            return false;
        }
    }

}


cv::Mat cvFromByteArray(QString& path, QByteArray& b, int flags)
{
    cv::Mat res;
    if (b.size())
    {
        if (path.endsWith(".jxl"))
        {
            decode(b, res);
        }
        else
        {
            cv::Mat m(b.length(), 1, CV_8U, b.data());
            res = cv::imdecode(m, flags);
        }
    }
    else
    {
        qDebug() << "Empty image file, check acquisition";
    }

    return res;
}

void fillByteArray(struct archive* ar, QByteArray& ardata)
{
    const void *buff;
    size_t size;
    la_int64_t offset;
    for (;;)
    {
        int r = archive_read_data_block(ar, &buff, &size, &offset);
        if (r == ARCHIVE_EOF)
            break;
        if (r < ARCHIVE_OK)
        {
            qDebug() << "Error reading archive";
            return;
        }
        ardata.append((char*)buff, size);
    }
}


QMap<QString, int > split_pos; // store temp access file

cv::Mat pl::imread(QString &path, int flags)
{
    cv::Mat res;
    QString lpath = path;

#ifdef CZI

    if (path.endsWith(".czi"))
    {
        auto str = path;
        str.chop(4);

        auto idxdot = str.lastIndexOf(".");
        auto idx = str.mid(idxdot+1).toInt();

        str.chop(str.size()-str.lastIndexOf("."));
        str += ".czi";

        wchar_t* czi_file = (wchar_t*)str.utf16();

        auto stream = libCZI::CreateStreamFromFile(czi_file);
        auto cziReader = libCZI::CreateCZIReader();
        cziReader->Open(stream);


        // use libczi to skip to proper postion

        auto sbBlk = cziReader->ReadSubBlock(idx);
        auto bitmap = sbBlk->CreateBitmap();

        int type = -1;

//        cv::
        if (bitmap->GetPixelType() ==  libCZI::PixelType::Gray8)
            type = CV_8U;

        if (bitmap->GetPixelType() ==  libCZI::PixelType::Gray16)
            type = CV_16U;

        cv::Mat res;
        if (type >= 0)
        {
            auto locked = bitmap->Lock();
            cv::Mat inter =     cv::Mat(bitmap->GetHeight(),
                                        bitmap->GetWidth(),
                                        type, locked.ptrDataRoi);

            inter.copyTo(res);
            bitmap->Unlock();
        }
        return res;
    }

#endif


    // Assuming if file doesn't exist it's a jxl file or tar or czi
    if (!QFile::exists(path))
    {
        QString filename = path; // initial file name to search through archive

        lpath.chop(4);
        QString pth = lpath;


        lpath+=".jxl";


        if (!QFile::exists(lpath))
        { // TODO: Check for .tar file with less char than initial file
            // this allows for single file many channels, single file many time points / channels
            // Many fields in single file etc...
            int idx = filename.lastIndexOf("/");
            QString refname = filename.mid(0, idx);

            int start_pth = split_pos.value(refname, pth.size());

            for (int i = pth.size(); i > 0; --i)
            {
                QString  tp = pth.mid(0, i);
                tp += ".tar";
                if (QFile::exists(tp))
                { // Found the closest matching archive name
                    if (start_pth != pth.size()) // save for later
                        split_pos[refname]=i;

                    QString search_file = filename.mid(idx+1),
                        search_file_jxl = filename.mid(idx+1, filename.size()-idx-5)+".jxl";

//                    qDebug() << tp << search_file << search_file_jxl;

                    QByteArray ardata;

                    // Open archive
                    bool jxl = false;
                    struct archive_entry *entry;
                    int r;


                    struct archive *ar = archive_read_new();
                    archive_read_support_format_tar(ar);

                    r = archive_read_open_filename(ar, tp.toLatin1().data(), 10240);
                    if (r != ARCHIVE_OK) {
                        qDebug() << "Archive opening error";
                        return res;

                    }
                    while (archive_read_next_header(ar, &entry) == ARCHIVE_OK)
                    {
                        auto arch_fname =archive_entry_pathname_utf8(entry);
                        jxl = search_file_jxl==arch_fname;
                        if (search_file==arch_fname || jxl)
                        {
                            fillByteArray(ar, ardata);
                            break;
                        } else
                        {
                           archive_read_data_skip(ar);  // Note 2
                        }
                    }
                    r = archive_read_free(ar);  // Note 3
                    if (r != ARCHIVE_OK)
                    {
                        qDebug() << "Close Archive error";
                    }

                    if (ardata.size() == 0)
                        qDebug() << "Error getting image data from archive";
                    else
                    {
                        auto name = jxl ? QString(".jxl") : QString(".tif");
                        res = cvFromByteArray(name, ardata, flags);
                    }
                    return res;
                }
            }
        }

    }


    QFile f(lpath);
    if (f.open(QFile::ReadOnly))
    {
        QByteArray b = f.readAll();
        res = cvFromByteArray(lpath, b, flags);
    }
    else
    {
        qDebug() << "File opening error" << lpath;
    }
    return res;
}
