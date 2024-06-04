#ifndef __CLIENT__
#define __CLIENT__

#include "luna_client.h"
#include <glib.h>
#include <string>
#include <thread>

class Client
{
protected:
    std::unique_ptr<LunaClient> luna_client{nullptr};

    GMainLoop *loop_{nullptr};
    std::unique_ptr<std::thread> loopThread_;
    unsigned long subscribeKey_{0};

    std::string mUri;

public:
    Client(const std::string &name);
    virtual ~Client();
};

#endif // __CLIENT__
