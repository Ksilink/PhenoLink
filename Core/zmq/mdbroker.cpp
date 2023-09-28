
#include "mdbroker.hpp"


// With this one we can have non homogeneous servers
// e.g: not loading some plugin (like a plugin requiring GPU, which may not be available on the computer
// so this worker will not try to call the plugin

inline service::~service()
{
    for(size_t i = 0; i < m_requests.size(); i++) {
        delete m_requests[i];
    }
}

inline bool service::add_worker(worker *wrk)
{

    auto pl_time = QDateTime::fromString(wrk->m_plugins[m_name]["PluginVersion"].toString().mid(8,19), "yyyy-MM-dd hh:mm:ss");

    //    qDebug() << m_name << "plugin_time" << pl_time;

    bool added = false;

    if (m_process.isEmpty())
    {
        m_process.push_back(wrk);
        added = true;
    }
    else
        for (auto w : m_process)
        {
            auto newtime = QDateTime::fromString(w->m_plugins[m_name]["PluginVersion"].toString().mid(8,19), "yyyy-MM-dd hh:mm:ss");
            if (pl_time < newtime) // if all plugins in the list are older than the new clear
                m_process.clear();

            if ( pl_time <= newtime) // if the plugin time older or equal add new :D (since older would already have been cleared by previous stage
            {
                m_process.push_back(wrk);
                added = true;
            }
        }

    return added;
}
