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

    virtual void loadFromJSON(QJsonObject data, QString base_path = QString());
    virtual void storeJson(QJsonObject json);

    virtual QString basePath(QJsonObject data);

    virtual cv::Mat getImage(size_t channelId, QString base_path = QString());

    virtual void  deallocate();
    virtual size_t getChannelCount();
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
    virtual void loadFromJSON(QJsonObject data, QString bp = QString());
    virtual QString basePath(QJsonObject json);

    virtual cv::Mat getImage(size_t i, QString base_path = QString());

};

// 3D
class DllCoreExport StackedImage: public ImageContainer
{
public:
    virtual void loadFromJSON(QJsonObject data, QString base_path = QString());
    virtual cv::Mat getImage(size_t i, size_t chann, QString base_path = QString());
    virtual QString basePath(QJsonObject json);
    virtual size_t getChannelCount();


};

// 3D + t
class DllCoreExport TimeStackedImage
{
public:

    TimeStackedImage();

    size_t count();


    StackedImage& operator[](size_t i);
    virtual void loadFromJSON(QJsonObject data, QString base_path = QString());
    virtual QString basePath(QJsonObject json);
    virtual void storeJson(QJsonObject json);

    StackedImage getImage(size_t i, QString base_path = QString());

    virtual void  deallocate();
    inline StackedImage& addOne() {
        int l = (int)_times.size();
        _times.resize(l+1);
        return _times[l];
    }


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
    virtual void loadFromJSON(QJsonObject data, QString base_path = QString());
    virtual QString basePath(QJsonObject json);
    virtual size_t getChannelCount();
    virtual cv::Mat getImage(int i, int c=-1, QString base_path = QString());

};

// 2D + t + fields
//
class DllCoreExport TimeImageXP
{
public:

    TimeImageXP();
    size_t count();


    TimeImage& operator[](size_t i);

    virtual void loadFromJSON(QJsonObject data, QString base_path = QString());
    virtual QString basePath(QJsonObject json);
    virtual void storeJson(QJsonObject json);
    TimeImage getImage(size_t i, QString base_path = QString());


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

    virtual void loadFromJSON(QJsonObject data, QString base_path = QString());
    virtual QString basePath(QJsonObject json);
    virtual void storeJson(QJsonObject json);
    StackedImage getImage(size_t i, QString base_path = QString());
    virtual size_t getChannelCount();
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

    virtual void loadFromJSON(QJsonObject data, QString base_path = QString());
    virtual QString basePath(QJsonObject json);
    virtual void storeJson(QJsonObject json);

    TimeStackedImage getImage(size_t i, QString base_path = QString() );

    virtual void  deallocate();
    inline TimeStackedImage& addOne(){
        int l = (int)_xp.size() ;
        _xp.resize(l + 1);
        return _xp[l];
    }


protected:
    std::vector<TimeStackedImage> _xp;

    size_t                    _count;
    bool                      _loaded;
    QJsonObject               _data;
};



class DllCoreExport WellPlate
{
public:
    WellPlate():     _propagated(false)
        {}
    size_t countX();

    size_t countY();

    bool exists(size_t i, size_t j);

    TimeStackedImage& operator()(size_t i, size_t j);

    inline TimeStackedImage& addOne(size_t i, size_t j)
    { return _plate[i][j]; }



    size_t getChannelCount();

    virtual void storeJson(QJsonObject json);
    virtual void loadFromJSON(QJsonObject data, QString base_path = QString());
    virtual QString basePath(QJsonObject json);
    virtual void  deallocate();
protected:
    QJsonObject _data; // This is of no real use for the plugins but necessary for the handler (may it should be put as an opaque object)
    QMap<size_t, QMap<size_t, TimeStackedImage> > _plate;

    bool _propagated;

};




class DllCoreExport WellPlateXP
{
public:

    WellPlateXP():     _propagated(false)
    {}

    size_t countX();

    size_t countY();

    bool exists(size_t i, size_t j);

    TimeStackedImageXP& operator()(size_t i, size_t j);

    inline TimeStackedImageXP& addOne(size_t i, size_t j)
    { return _plate[i][j]; }



    size_t getChannelCount();

    virtual void storeJson(QJsonObject json);
    virtual void loadFromJSON(QJsonObject data, QString base_path = QString());
    virtual QString basePath(QJsonObject json);
    virtual void  deallocate();
protected:
    QJsonObject _data; // This is of no real use for the plugins but necessary for the handler (may it should be put as an opaque object)
    QMap<size_t, QMap<size_t, TimeStackedImageXP> > _plate;
    bool _propagated;

};


namespace cocvMat {

void DllCoreExport loadFromJSON(QJsonObject data, cv::Mat& mat, int image = -1, QString base_path=QString());

}


#endif // IMAGECONTAINERS_H
