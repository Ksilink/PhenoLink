
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
    std::vector<unsigned short> pixels;

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
            }
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

            pixels.resize(xsize * ysize);
            void* pixels_buffer = (void*)pixels.data();
            size_t pixels_buffer_size = pixels.size();
            if (JXL_DEC_SUCCESS != JxlDecoderSetImageOutBuffer(dec.get(), &format,
                pixels_buffer,
                pixels_buffer_size * 2)) {
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
            cv::Mat(ysize, xsize, CV_16U, pixels.data()).copyTo(reconstructed);
            return true;
        }
        else {
            fprintf(stderr, "Unknown decoder status\n");
            return false;
        }
    }

}



cv::Mat pl::imread(QString &path, int flags)
{
    cv::Mat res;

        QFile f(path);
        if (f.open(QFile::ReadOnly))
        {
            QByteArray b = f.readAll();
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
            qDebug() << "File openning error" << path;
        }
    return res;
}
