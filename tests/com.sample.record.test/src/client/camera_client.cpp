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

bool checkResolution(const json &data, const std::string &format)
{
    if (data.contains("resolution"))
    {
        const json &resolution = data["resolution"];
        if (resolution.contains(format))
        {
            return true;
        }
    }
    return false;
}

bool CameraClient::checkCamera(const std::string &camera_id)
{
    // send message
    std::string uri = mUri + "getInfo";

    json j;
    j["id"] = camera_id;

    DEBUG_LOG("%s '%s'", uri.c_str(), to_string(j).c_str());

    std::string resp;
    luna_client->callSync(uri.c_str(), to_string(j).c_str(), &resp);

    json jOut = json::parse(resp);
    // printf("%s\n", jOut.dump(4).c_str());
    if (jOut.is_discarded())
    {
        DEBUG_LOG("payload parsing error!");
        return false;
    }

    bool return_value = get_optional<bool>(jOut, returnValueStr.c_str()).value_or(false);
    if (return_value)
    {
        bool hasYUV  = checkResolution(jOut["info"], "YUV");
        bool hasJPEG = checkResolution(jOut["info"], "JPEG");
        DEBUG_LOG("%s: hasYUV=%d hasJPEG=%d", camera_id.c_str(), hasYUV, hasJPEG);
        return hasYUV | hasJPEG;
    }

    return false;
}

bool CameraClient::getCameraList()
{
    bool ret = false;

    // send message
    std::string uri     = mUri + __func__;
    std::string payload = "{}";
    DEBUG_LOG("%s '%s'", uri.c_str(), payload.c_str());

    std::string resp;
    ret = luna_client->callSync(uri.c_str(), payload.c_str(), &resp);

    DEBUG_LOG("ret=%d", ret);
    if (!ret)
    {
        return false;
    }

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
        // get the first camera id supporting YUV or JPEG
        for (const auto &device : jOut["deviceList"])
        {
            if (checkCamera(device["id"]))
            {
                cameraId = device["id"];
                break;
            }
        }

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

bool CameraClient::getFormat()
{
    bool ret = false;

    // send message
    std::string uri = mUri + __func__;

    json j;
    j["id"] = cameraId;
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
        preview_fps = jOut["params"]["fps"];
        DEBUG_LOG("preview_fps : %d\n", preview_fps);
    }

    return ret;
}

bool CameraClient::startCamera()
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
        state = START;
    }

    return ret;
}

bool CameraClient::stopCamera()
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
        state = STOP;
    }

    return ret;
}

bool CameraClient::startPreview(const std::string &window_id)
{
    bool ret = false;

    // send message
    std::string uri = mUri + __func__;

    json j;
    j["handle"]   = handle;
    j["windowId"] = window_id;
    DEBUG_LOG("%s '%s'", uri.c_str(), to_string(j).c_str());

    std::string resp;
    ret = luna_client->callSync(uri.c_str(), to_string(j).c_str(), &resp, 9000);

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
        state = START;
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
        state = STOP;
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

bool CameraClient::capture()
{
    bool ret = false;
    captureFileList.clear();

    // send message
    std::string uri = mUri + __func__;

    json j;
    j["handle"] = handle;

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
    if (ret)
    {
        for (const auto &path : jOut["path"])
        {
            captureFileList.push_back(path);
        }
    }
    return ret;
}

bool CameraClient::close()
{
    if (!handle)
    {
        DEBUG_LOG("Already closed");
        return true;
    }

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

bool CameraClient::setSolutions(bool enable)
{
    bool ret = false;

    // send message
    std::string uri = mUri + __func__;

    json j;
    j["id"]        = cameraId;
    j["solutions"] = json::array();

    if (solutionName.empty())
    {
        getSolutionName(solutionName);
    }

    json solution;
    solution["name"]             = solutionName;
    solution["params"]["enable"] = enable;
    j["solutions"].push_back(solution);

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
        DEBUG_LOG("ok");
    }

    return ret;
}

bool CameraClient::getSolutionName(std::string &solution_name)
{
    bool ret = false;

    // send message
    std::string uri = mUri + "getSolutions";

    json j;
    j["id"] = cameraId;

    DEBUG_LOG("%s '%s'", uri.c_str(), to_string(j).c_str());

    std::string resp;
    luna_client->callSync(uri.c_str(), to_string(j).c_str(), &resp);

    json jOut = json::parse(resp);
    printf("%s\n", jOut.dump(4).c_str());
    if (jOut.is_discarded())
    {
        DEBUG_LOG("payload parsing error!");
        return ret;
    }

    bool return_value = get_optional<bool>(jOut, returnValueStr.c_str()).value_or(false);
    if (return_value)
    {
        DEBUG_LOG("ok");
        for (const auto &solution : jOut["solutions"])
        {
            std::string name = solution["name"];
            if (name.find("FaceDetection") != std::string::npos)
            {
                solution_name = name;
                break;
            }
        }
    }

    return ret;
}
