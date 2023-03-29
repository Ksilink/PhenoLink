#ifndef PHENOLINKIMAGE_H
#define PHENOLINKIMAGE_H

#include <Dll.h>
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>

namespace pl {

cv::Mat DllCoreExport imread(QString& path, int flags = cv::IMREAD_COLOR);

};

#endif // PHENOLINKIMAGE_H
