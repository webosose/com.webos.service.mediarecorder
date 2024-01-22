#ifndef __LS_CONNECTOR__
#define __LS_CONNECTOR__

#include "luna_client.h"
#include <glib.h>
#include <string>
#include <thread>

class LSConnector
{
    std::unique_ptr<LunaClient> luna_client{nullptr};
    GMainLoop *loop_{nullptr};
    std::unique_ptr<std::thread> loopThread_;
    unsigned long subscribeKey_{0};

public:
    LSConnector(const std::string &service_name, const std::string &thread_name);
    ~LSConnector();

    bool callSync(const char *uri, const char *param, std::string *result, int timeout = 2000);
    bool subscribe(const char *uri, const char *param, Handler handler, void *data);
    bool unsubscribe();
};

#endif // __LS_CONNECTOR__
