#include "${plugin_name}.h"
#include <QFile>
#include <QDir>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>



${plugin_name}::${plugin_name}()
{
    description("${plugin_path}",
                QStringList() << "wiest.daessle@gmail.com",
                "Executes a Cell Profiler pipeline");

    use(&images, "input-image", "Input Time Image")
            .channelsAsVector() // This process does handle all the channels together,
            ;


}



CheckoutProcessPluginInterface *${plugin_name}::clone()
{
    return new ${plugin_name}();
}


void ${plugin_name}::exec()
{
    qDebug() << "Execution...";
    try
    {
    // Use images here

    } catch (cv::Exception& ex)
    {
        qDebug() << ex.what();
    }
}

