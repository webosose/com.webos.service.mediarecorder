#define LOG_TAG "LSConnector"
#include "ls_connector.h"
#include "log.h"
#include <system_error>

LSConnector::LSConnector(const std::string &service_name, const std::string &thread_name)
{
    PLOGI("start");

    GMainContext *c = g_main_context_new();
    loop_           = g_main_loop_new(c, false);

    try
    {
        loopThread_ = std::make_unique<std::thread>(g_main_loop_run, loop_);
    }
    catch (const std::system_error &e)
    {
        PLOGI("Caught a system_error with code %d meaning %s", e.code().value(), e.what());
    }

    while (!g_main_loop_is_running(loop_))
    {
    }

    pthread_setname_np(loopThread_->native_handle(), thread_name.c_str());

    luna_client = std::make_unique<LunaClient>(service_name.c_str(), c);
    g_main_context_unref(c);
}

LSConnector::~LSConnector()
{
    PLOGI("start");

    g_main_loop_quit(loop_);
    if (loopThread_->joinable())
    {
        try
        {
            loopThread_->join();
        }
        catch (const std::system_error &e)
        {
            PLOGI("Caught a system_error with code %d meaning %s", e.code().value(), e.what());
        }
    }
    g_main_loop_unref(loop_);
}

bool LSConnector::callSync(const char *uri, const char *param, std::string *result, int timeout)
{
    return luna_client->callSync(uri, param, result, timeout);
}

bool LSConnector::subscribe(const char *uri, const char *param, Handler handler, void *data)
{
    bool ret = luna_client->subscribe(uri, param, &subscribeKey_, handler, data);
    PLOGI("subscribeKey %ld", subscribeKey_);
    return ret;
}
bool LSConnector::unsubscribe()
{
    if (subscribeKey_)
    {
        PLOGI("subscribeKey %ld", subscribeKey_);
        return luna_client->unsubscribe(subscribeKey_);
    }

    return true;
}
