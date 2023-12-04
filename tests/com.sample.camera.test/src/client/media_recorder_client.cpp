#include "media_recorder_client.h"
#include "appParm.h"
#include "json_utils.h"
#include "log_info.h"
#include <nlohmann/json.hpp>

using namespace nlohmann;

const std::string returnValueStr("returnValue");

MediaRecorderClient::MediaRecorderClient(std::string name) : Client(name)
{
    DEBUG_LOG("start");
    mUri = "luna://com.webos.service.mediarecorder/";
}

MediaRecorderClient::~MediaRecorderClient() { DEBUG_LOG("start"); }

/*
 Execute the below command to check the audio source
 $ pactl list short sources | grep alsa-source
 http://clm.lge.com/issue/browse/QWO-702
*/
bool MediaRecorderClient::open(std::string &video_src)
{
    bool ret = false;

    // send message
    std::string uri = mUri + __func__;

    json j;
    j["videoSrc"] = video_src;
    if (appParm.disable_audio == false)
    {
        j["audioSrc"] = "usb_mic0";
    }
    DEBUG_LOG("%s '%s'", uri.c_str(), to_string(j).c_str());

    std::string resp;
    luna_client->callSync(uri.c_str(), to_string(j).c_str(), &resp);

    json jOut = json::parse(resp);
    printf("%s\n", jOut.dump(4).c_str());

    if (jOut.is_discarded())
    {
        DEBUG_LOG("payload parsing error!");
        return false;
    }

    ret = get_optional<bool>(jOut, returnValueStr.c_str()).value_or(false);
    if (ret)
    {
        recorderId = get_optional<int>(jOut, "recorderId").value_or(0);
        DEBUG_LOG("recorderId = %d", recorderId);
    }
    return ret;
}

bool MediaRecorderClient::setOutputFile()
{
    bool ret = false;

    // send message
    std::string uri = mUri + __func__;

    json j;
    j["recorderId"] = recorderId;
    j["path"]       = "/media/internal/";
    DEBUG_LOG("%s '%s'", uri.c_str(), to_string(j).c_str());

    std::string resp;
    luna_client->callSync(uri.c_str(), to_string(j).c_str(), &resp);

    json jOut = json::parse(resp);
    printf("%s\n", jOut.dump(4).c_str());

    if (jOut.is_discarded())
    {
        DEBUG_LOG("payload parsing error!");
        return false;
    }

    ret = get_optional<bool>(jOut, returnValueStr.c_str()).value_or(false);
    return ret;
}

bool MediaRecorderClient::setOutputFormat()
{
    bool ret = false;

    // send message
    std::string uri = mUri + __func__;

    json j;
    j["recorderId"] = recorderId;
    j["format"]     = "MPEG4";
    DEBUG_LOG("%s '%s'", uri.c_str(), to_string(j).c_str());

    std::string resp;
    luna_client->callSync(uri.c_str(), to_string(j).c_str(), &resp);

    json jOut = json::parse(resp);
    printf("%s\n", jOut.dump(4).c_str());

    if (jOut.is_discarded())
    {
        DEBUG_LOG("payload parsing error!");
        return false;
    }

    ret = get_optional<bool>(jOut, returnValueStr.c_str()).value_or(false);
    return ret;
}

bool MediaRecorderClient::start()
{
    bool ret = false;

    // send message
    std::string uri = mUri + __func__;

    json j;
    j["recorderId"] = recorderId;
    DEBUG_LOG("%s '%s'", uri.c_str(), to_string(j).c_str());

    std::string resp;
    luna_client->callSync(uri.c_str(), to_string(j).c_str(), &resp);

    json jOut = json::parse(resp);
    printf("%s\n", jOut.dump(4).c_str());

    if (jOut.is_discarded())
    {
        DEBUG_LOG("payload parsing error!");
        return false;
    }

    ret   = get_optional<bool>(jOut, returnValueStr.c_str()).value_or(false);
    state = RecordingState::Recording;
    return ret;
}

bool MediaRecorderClient::stop()
{
    bool ret = false;

    // send message
    std::string uri = mUri + __func__;

    json j;
    j["recorderId"] = recorderId;
    DEBUG_LOG("%s '%s'", uri.c_str(), to_string(j).c_str());

    std::string resp;
    luna_client->callSync(uri.c_str(), to_string(j).c_str(), &resp);

    json jOut = json::parse(resp);
    printf("%s\n", jOut.dump(4).c_str());

    if (jOut.is_discarded())
    {
        DEBUG_LOG("payload parsing error!");
        return false;
    }

    ret = get_optional<bool>(jOut, returnValueStr.c_str()).value_or(false);
    if (ret)
    {
        mRecordPath = get_optional<std::string>(jOut, "path").value_or("");
        DEBUG_LOG("mRecordPath = %s", mRecordPath.c_str());
    }

    state = RecordingState::Stopped;
    return ret;
}

bool MediaRecorderClient::close()
{
    bool ret = false;

    // send message
    std::string uri = mUri + __func__;

    json j;
    j["recorderId"] = recorderId;
    DEBUG_LOG("%s '%s'", uri.c_str(), to_string(j).c_str());

    std::string resp;
    luna_client->callSync(uri.c_str(), to_string(j).c_str(), &resp);

    json jOut = json::parse(resp);
    printf("%s\n", jOut.dump(4).c_str());

    if (jOut.is_discarded())
    {
        DEBUG_LOG("payload parsing error!");
        return false;
    }

    ret = get_optional<bool>(jOut, returnValueStr.c_str()).value_or(false);

    recorderId = 0;
    return ret;
}

bool MediaRecorderClient::takeSnapshot()
{
    bool ret = false;

    // send message
    std::string uri = mUri + __func__;

    json j;
    j["recorderId"] = recorderId;
    j["path"]       = "/media/internal/";
    j["format"]     = "JPEG";
    DEBUG_LOG("%s '%s'", uri.c_str(), to_string(j).c_str());

    std::string resp;
    luna_client->callSync(uri.c_str(), to_string(j).c_str(), &resp, 9000);

    json jOut = json::parse(resp);
    printf("%s\n", jOut.dump(4).c_str());

    if (jOut.is_discarded())
    {
        DEBUG_LOG("payload parsing error!");
        return false;
    }

    ret = get_optional<bool>(jOut, returnValueStr.c_str()).value_or(false);
    if (ret)
    {
        mCapturePath = get_optional<std::string>(jOut, "path").value_or("");
        DEBUG_LOG("mCapturePath = %s", mCapturePath.c_str());
    }
    return ret;
}

bool MediaRecorderClient::pause()
{
    bool ret = false;

    // send message
    std::string uri = mUri + __func__;

    json j;
    j["recorderId"] = recorderId;
    DEBUG_LOG("%s '%s'", uri.c_str(), to_string(j).c_str());

    std::string resp;
    luna_client->callSync(uri.c_str(), to_string(j).c_str(), &resp);

    json jOut = json::parse(resp);
    printf("%s\n", jOut.dump(4).c_str());

    if (jOut.is_discarded())
    {
        DEBUG_LOG("payload parsing error!");
        return false;
    }

    ret   = get_optional<bool>(jOut, returnValueStr.c_str()).value_or(false);
    state = RecordingState::Paused;
    return ret;
}

bool MediaRecorderClient::resume()
{
    bool ret = false;

    // send message
    std::string uri = mUri + __func__;

    json j;
    j["recorderId"] = recorderId;
    DEBUG_LOG("%s '%s'", uri.c_str(), to_string(j).c_str());

    std::string resp;
    luna_client->callSync(uri.c_str(), to_string(j).c_str(), &resp);

    json jOut = json::parse(resp);
    printf("%s\n", jOut.dump(4).c_str());

    if (jOut.is_discarded())
    {
        DEBUG_LOG("payload parsing error!");
        return false;
    }

    ret   = get_optional<bool>(jOut, returnValueStr.c_str()).value_or(false);
    state = RecordingState::Recording;
    return ret;
}
