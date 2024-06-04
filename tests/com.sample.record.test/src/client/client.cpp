#include "client.h"
#include "log_info.h"
#include <system_error>

Client::Client(const std::string &name)
{
    DEBUG_LOG("start");

    GMainContext *c = g_main_context_new();
    loop_           = g_main_loop_new(c, false);

    try
    {
        loopThread_ = std::make_unique<std::thread>(g_main_loop_run, loop_);
    }
    catch (const std::system_error &e)
    {
        DEBUG_LOG("Caught a system_error with code %d meaning %s", e.code().value(), e.what());
    }

    while (!g_main_loop_is_running(loop_))
    {
    }

    std::string handle_name = name + "_client";
    pthread_setname_np(loopThread_->native_handle(), handle_name.c_str());

    std::string service_name = "com.sample.record.test-" + name;
    luna_client              = std::make_unique<LunaClient>(service_name.c_str(), c);
    g_main_context_unref(c);
}

Client::~Client()
{
    DEBUG_LOG("start");

    g_main_loop_quit(loop_);
    if (loopThread_->joinable())
    {
        try
        {
            loopThread_->join();
        }
        catch (const std::system_error &e)
        {
            DEBUG_LOG("Caught a system_error with code %d meaning %s", e.code().value(), e.what());
        }
    }
    g_main_loop_unref(loop_);
}
