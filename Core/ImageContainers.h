#ifndef IMAGECONTAINERS_H
#define IMAGECONTAINERS_H

#include <Dll.h>
#include <opencv2/highgui.hpp>


// FIXME: Add JSON / read & write context

class DllCoreExport ImageContainer
{
public:
    // Time index access

    ImageContainer();
    virtual ~ImageContainer();

    size_t count();

    cv::Mat& operator[](size_t i);

    cv::Mat& operator()(size_t i);

    virtual void loadFromJSON(QJsonObject data);
    virtual void storeJson(QJsonObject json);

    virtual QString basePath(QJsonObject data);

    virtual cv::Mat getImage(size_t i);

    virtual void  deallocate();

    // To be used by plugin that wish to return results as TimeImage
    // First set the number of expected outputs
    virtual void setCount(size_t s);
    // Set the image in the container
    virtual void setImage(size_t i, cv::Mat mat);

protected:

    QJsonObject _data; // This is of no real use for the plugins but necessary for the handler (may be it should be put as an opaque object)

    std::vector<cv::Mat> images;
    bool _loaded;
    size_t _count;

};

// 2D + t
class DllCoreExport TimeImage: public ImageContainer
{
public:
    virtual void loadFromJSON(QJsonObject data);
    virtual QString basePath(QJsonObject json);

    virtual cv::Mat getImage(size_t i);

};

// 3D
class DllCoreExport StackedImage: public ImageContainer
{
public:
    virtual void loadFromJSON(QJsonObject data);
    virtual cv::Mat getImage(size_t i);
    virtual QString basePath(QJsonObject json);

};

// 3D + t
class DllCoreExport TimeStackedImage
{
public:

    TimeStackedImage();

    size_t count();


    StackedImage& operator[](size_t i);
    virtual void loadFromJSON(QJsonObject data);
    virtual QString basePath(QJsonObject json);
    virtual void storeJson(QJsonObject json);

    StackedImage getImage(size_t i);

    virtual void  deallocate();

protected:
    std::vector<StackedImage> _times;
    size_t                    _count;
    bool _loaded;
    QJsonObject               _data;
};

// 2D + fields
class DllCoreExport ImageXP: public ImageContainer
{
public:
    virtual void loadFromJSON(QJsonObject data);
    virtual QString basePath(QJsonObject json);
    virtual cv::Mat getImage(int i, int c=-1);

};

// 2D + t + fields
// 
class DllCoreExport TimeImageXP
{
public:

    TimeImageXP();
    size_t count();


    TimeImage& operator[](size_t i);

    virtual void loadFromJSON(QJsonObject data);
    virtual QString basePath(QJsonObject json);
    virtual void storeJson(QJsonObject json);
    TimeImage getImage(size_t i);


    virtual void  deallocate();

protected:

    std::vector<TimeImage>    _xp;
    size_t                    _count;
    bool                      _loaded;
    QJsonObject               _data;
};

// 3D + fields
class DllCoreExport StackedImageXP
{
public:

    StackedImageXP();

    size_t count();

    StackedImage& operator[](size_t i);

    virtual void loadFromJSON(QJsonObject data);
    virtual QString basePath(QJsonObject json);
    virtual void storeJson(QJsonObject json);
    StackedImage getImage(size_t i);

    virtual void  deallocate();

protected:
    std::vector<StackedImage> _xp;

    size_t                    _count;
    bool                      _loaded;
    QJsonObject               _data;

};

// 3D + t + fields
class DllCoreExport TimeStackedImageXP
{
public:
    TimeStackedImageXP();
    size_t count();

    TimeStackedImage& operator[](size_t i);

    virtual void loadFromJSON(QJsonObject data);
    virtual QString basePath(QJsonObject json);
    virtual void storeJson(QJsonObject json);

    TimeStackedImage getImage(size_t i );

    virtual void  deallocate();
protected:
    std::vector<TimeStackedImage> _xp;

    size_t                    _count;
    bool                      _loaded;
    QJsonObject               _data;
};


class DllCoreExport WellPlate
{
public:

    size_t countX();

    size_t countY();

    TimeStackedImageXP& operator()(unsigned i, unsigned j);

    virtual void loadFromJSON(QJsonObject data);
    virtual QString basePath(QJsonObject json);
    virtual void  deallocate();
protected:
    QJsonObject _data; // This is of no real use for the plugins but necessary for the handler (may it should be put as an opaque object)
    std::map<unsigned, std::map<unsigned, TimeStackedImageXP> > _plate;

};


namespace cocvMat {

void DllCoreExport loadFromJSON(QJsonObject data, cv::Mat& mat, int image = -1);

}


#endif // IMAGECONTAINERS_H
