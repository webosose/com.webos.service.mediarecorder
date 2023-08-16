#include "camera_client.h"
#include "appParm.h"
#include "json_utils.h"
#include "log_info.h"
#include <nlohmann/json.hpp>

using namespace nlohmann;

const std::string returnValueStr("returnValue");

CameraClient::CameraClient() : Client("camera")
{
    DEBUG_LOG("start");
    mUri = "luna://com.webos.service.camera2/";
}

CameraClient::~CameraClient() { DEBUG_LOG("start"); }

bool CameraClient::getCameraList()
{
    bool ret = false;

    // send message
    std::string uri     = mUri + __func__;
    std::string payload = "{}";
    DEBUG_LOG("%s '%s'", uri.c_str(), payload.c_str());

    std::string resp;
    ret = luna_client->callSync(uri.c_str(), payload.c_str(), &resp);

    json jOut = json::parse(resp);
    printf("%s\n", jOut.dump(4).c_str());
    if (jOut.is_discarded())
    {
        DEBUG_LOG("payload parsing error!");
        return false;
    }

    bool return_value = get_optional<bool>(jOut, returnValueStr.c_str()).value_or(false);
    if (return_value)
    {
        // get the first camera id
        cameraId = get_optional<std::string>(jOut["deviceList"][0], "id").value_or("");
        DEBUG_LOG("CameraId : %s\n", cameraId.c_str());
    }

    return ret;
}

bool CameraClient::open()
{
    bool ret = false;

    // send message
    std::string uri = mUri + __func__;

    json j;
    j["id"] = cameraId;
#ifdef SHM_SYNC_TEST
    j["pid"] = getpid();
#endif
    DEBUG_LOG("%s '%s'", uri.c_str(), to_string(j).c_str());

    std::string resp;
    ret = luna_client->callSync(uri.c_str(), to_string(j).c_str(), &resp);

    json jOut = json::parse(resp);
    printf("%s\n", jOut.dump(4).c_str());
    if (jOut.is_discarded())
    {
        DEBUG_LOG("payload parsing error!");
        return false;
    }

    bool return_value = get_optional<bool>(jOut, returnValueStr.c_str()).value_or(false);
    if (return_value)
    {
        handle = get_optional<int>(jOut, "handle").value_or(0);
        DEBUG_LOG("handle : %d\n", handle);
    }

    return ret;
}

bool CameraClient::setFormat()
{
    bool ret = false;

    // send message
    std::string uri = mUri + __func__;

    json j;
    j["handle"] = handle;

    json params;
    params["width"]  = appParm.width;
    params["height"] = appParm.height;
    params["format"] = appParm.format;
    params["fps"]    = appParm.fps;
    j["params"]      = params;
    DEBUG_LOG("%s '%s'", uri.c_str(), to_string(j).c_str());

    std::string resp;
    ret = luna_client->callSync(uri.c_str(), to_string(j).c_str(), &resp);

    json jOut = json::parse(resp);
    printf("%s\n", jOut.dump(4).c_str());
    if (jOut.is_discarded())
    {
        DEBUG_LOG("payload parsing error!");
        return false;
    }

    bool return_value = get_optional<bool>(jOut, returnValueStr.c_str()).value_or(false);
    if (return_value)
    {
        DEBUG_LOG("ok\n");
    }

    return ret;
}

bool CameraClient::startPreview()
{
    bool ret = false;

    // send message
    std::string uri = mUri + __func__;

    json j;
    j["handle"] = handle;

    json params;
    params["type"]   = "sharedmemory";
    params["source"] = "0";
    j["params"]      = params;
    DEBUG_LOG("%s '%s'", uri.c_str(), to_string(j).c_str());

    std::string resp;
    ret = luna_client->callSync(uri.c_str(), to_string(j).c_str(), &resp);

    json jOut = json::parse(resp);
    printf("%s\n", jOut.dump(4).c_str());
    if (jOut.is_discarded())
    {
        DEBUG_LOG("payload parsing error!");
        return false;
    }

    bool return_value = get_optional<bool>(jOut, returnValueStr.c_str()).value_or(false);
    if (return_value)
    {
        key = get_optional<int>(jOut, "key").value_or(0);
        DEBUG_LOG("key : %d\n", key);
    }

    return ret;
}

bool CameraClient::stopPreview()
{
    bool ret = false;

    // send message
    std::string uri = mUri + __func__;

    json j;
    j["handle"] = handle;
    DEBUG_LOG("%s '%s'", uri.c_str(), to_string(j).c_str());

    std::string resp;
    ret = luna_client->callSync(uri.c_str(), to_string(j).c_str(), &resp);

    json jOut = json::parse(resp);
    printf("%s\n", jOut.dump(4).c_str());
    if (jOut.is_discarded())
    {
        DEBUG_LOG("payload parsing error!");
        return false;
    }

    bool return_value = get_optional<bool>(jOut, returnValueStr.c_str()).value_or(false);
    if (return_value)
    {
        DEBUG_LOG("ok\n");
    }

    return ret;
}

/*
luna-send -n 1 -f luna://com.webos.service.camera2/startCapture '{
     "handle": 2516,
     "params":
      {
         "width": 1280,
         "height": 720,
         "format": "JPEG",
         "mode":"MODE_ONESHOT"
      }
}'
*/
bool CameraClient::startCapture()
{
    bool ret = false;

    // send message
    std::string uri = mUri + __func__;

    json j;
    j["handle"] = handle;

    json params;
    params["width"]  = appParm.width;
    params["height"] = appParm.height;
    params["format"] = appParm.format;
    params["mode"]   = "MODE_ONESHOT";
    j["params"]      = params;
    DEBUG_LOG("%s '%s'", uri.c_str(), to_string(j).c_str());

    std::string resp;

    int64_t startClk = g_get_monotonic_time();
    luna_client->callSync(uri.c_str(), to_string(j).c_str(), &resp);
    int64_t endClk = g_get_monotonic_time();

    json jOut = json::parse(resp);
    printf("%s\n", jOut.dump(4).c_str());
    DEBUG_LOG("Response time %lld ms", (long long int)((endClk - startClk) / 1000));

    if (jOut.is_discarded())
    {
        DEBUG_LOG("payload parsing error!");
        return false;
    }

    ret = get_optional<bool>(jOut, returnValueStr.c_str()).value_or(false);
    return ret;
}

bool CameraClient::stopCapture()
{
    bool ret = false;
    return ret;
}

bool CameraClient::close()
{
    bool ret = false;

    // send message
    std::string uri = mUri + __func__;

    json j;
    j["handle"] = handle;
    DEBUG_LOG("%s '%s'", uri.c_str(), to_string(j).c_str());

    std::string resp;
    ret = luna_client->callSync(uri.c_str(), to_string(j).c_str(), &resp);

    json jOut = json::parse(resp);
    printf("%s\n", jOut.dump(4).c_str());
    if (jOut.is_discarded())
    {
        DEBUG_LOG("payload parsing error!");
        return false;
    }

    bool return_value = get_optional<bool>(jOut, returnValueStr.c_str()).value_or(false);
    if (return_value)
    {
        DEBUG_LOG("ok\n");
        handle = 0;
    }

    return ret;
}
