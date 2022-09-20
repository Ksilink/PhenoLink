#include "${plugin_name}.h"
#include <QFile>
#include <QDir>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>



${plugin_name}::${plugin_name}()
{
    description("${plugin_path}",
                QStringList() << "who@youremail" << "and_someoneelse@where",
                "Your Comment here");

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

