#include "media_client.h"
#include "appParm.h"
#include "json_utils.h"
#include "log_info.h"
#include <nlohmann/json.hpp>

using namespace nlohmann;

const std::string pausedStr("paused");
const std::string playingStr("playing");
const std::string eosStr("endOfStream");
const std::string mediaIdStr("mediaId");
const std::string returnValueStr("returnValue");

static bool funCb(const char *msg, void *data)
{
    DEBUG_LOG("%s", (char *)data);
    json j = json::parse(msg, nullptr, false);
    printf("%s\n", j.dump(4).c_str());
    return true;
}

MediaClient::MediaClient(std::string name) : Client(name)
{
    DEBUG_LOG("start");
    mUri = "luna://com.webos.media/";
}

MediaClient::~MediaClient() { DEBUG_LOG("start"); }

/*
# luna-send -n 1 -f luna://com.webos.media/load '{
   "uri":"file:///media/internal/Record13072023-03343650.mp4",
   "type":"media",
   "payload":{
      "option":{
         "appId":"com.webos.app.enactbrowser",
         "windowId":"_Window_Id_1"
      }
   }
}'
*/
bool MediaClient::load(const std::string &windowId, const std::string &video_name)
{
    bool ret = false;

    // send message
    std::string uri = mUri + __func__;

    json j;
    j["uri"]  = "file:///" + video_name;
    j["type"] = "media";

    json payload;
    payload["option"]["appId"]    = "com.webos.app.enactbrowser";
    payload["option"]["windowId"] = windowId;
    j["payload"]                  = payload;

    DEBUG_LOG("%s '%s'", uri.c_str(), to_string(j).c_str());

    auto loadCb = [](const char *msg, void *data) -> bool
    {
        DEBUG_LOG("loadCb");

        json j = json::parse(msg, nullptr, false);
        printf("%s\n", j.dump(4).c_str());
        if (j.is_discarded())
        {
            DEBUG_LOG("msg parsing error!");
            return false;
        }

        bool return_value = get_optional<bool>(j, returnValueStr.c_str()).value_or(false);
        if (return_value)
        {
            MediaClient *client = (MediaClient *)data;

            client->mediaId = get_optional<std::string>(j, mediaIdStr.c_str()).value_or("");
            DEBUG_LOG("mediaId = %s", client->mediaId.c_str());

            client->play();
            client->subscribe();
        }

        return true;
    };

    ret = luna_client->callAsync(uri.c_str(), to_string(j).c_str(), loadCb, this);

    state = MediaState::PLAY;
    return ret;
}

bool MediaClient::load(const json &option)
{
    bool ret = false;

    // send message
    std::string uri = mUri + __func__;

    json j;
    j["uri"]               = "camera://com.webos.service.camera2/7010";
    j["payload"]["option"] = option;
    j["type"]              = "camera";

    DEBUG_LOG("%s '%s'", uri.c_str(), to_string(j).c_str());

    auto loadCb = [](const char *msg, void *data) -> bool
    {
        DEBUG_LOG("loadCb");

        json j = json::parse(msg, nullptr, false);
        printf("%s\n", j.dump(4).c_str());
        if (j.is_discarded())
        {
            DEBUG_LOG("msg parsing error!");
            return false;
        }

        bool return_value = get_optional<bool>(j, returnValueStr.c_str()).value_or(false);
        if (return_value)
        {
            MediaClient *client = (MediaClient *)data;

            client->mediaId = get_optional<std::string>(j, mediaIdStr.c_str()).value_or("");
            DEBUG_LOG("mediaId = %s", client->mediaId.c_str());

            client->play();
            client->subscribe();
        }

        return true;
    };

    ret = luna_client->callAsync(uri.c_str(), to_string(j).c_str(), loadCb, this);

    state = MediaState::PLAY;
    return ret;
}

bool MediaClient::play()
{
    bool ret = false;

    // send message
    std::string uri = mUri + __func__;

    json j;
    j[mediaIdStr] = mediaId;

    DEBUG_LOG("%s '%s'", uri.c_str(), to_string(j).c_str());
    ret = luna_client->callAsync(uri.c_str(), to_string(j).c_str(), funCb, (void *)__func__);

    return ret;
}

bool MediaClient::pause()
{
    DEBUG_LOG("");

    bool ret = false;

    // send message
    std::string uri = mUri + __func__;

    json j;
    j[mediaIdStr] = mediaId;

    DEBUG_LOG("%s '%s'", uri.c_str(), to_string(j).c_str());
    ret = luna_client->callAsync(uri.c_str(), to_string(j).c_str(), funCb, (void *)__func__);

    return ret;
}

bool MediaClient::unload()
{
    bool ret = false;

    unsubscribe();

    // send message
    std::string uri = mUri + __func__;

    json j;
    j[mediaIdStr] = mediaId;

    DEBUG_LOG("%s '%s'", uri.c_str(), to_string(j).c_str());
    ret = luna_client->callAsync(uri.c_str(), to_string(j).c_str(), funCb, (void *)__func__);

    state = MediaState::STOP;
    return ret;
}

bool MediaClient::subscribe()
{
    bool ret = false;

    // send message
    std::string uri = mUri + __func__;

    json j;
    j[mediaIdStr] = mediaId;

    DEBUG_LOG("%s '%s'", uri.c_str(), to_string(j).c_str());

    startTime = std::chrono::steady_clock::now();

    auto subscribeCb = [](const char *msg, void *data) -> bool
    {
        json j = json::parse(msg, nullptr, false);
        if (j.is_discarded())
        {
            DEBUG_LOG("msg parsing error!");
            return false;
        }

        MediaClient *client = (MediaClient *)data;

        const std::string currentTimeStr("currentTime");
        if (j.contains(currentTimeStr))
        {
            if (client->state == MediaState::PLAY)
            {
                auto currentTime = std::chrono::steady_clock::now();
                auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(
                                       currentTime - client->startTime)
                                       .count();

                if (elapsedTime >= 1)
                {
                    printf("%s: %d\n", currentTimeStr.c_str(), (int)j[currentTimeStr]);
                    client->startTime = currentTime;
                }
            }
        }
        else
        {
            DEBUG_LOG("subscribeCb");
            printf("%s\n", j.dump(4).c_str());
        }

        if (j.contains(eosStr))
        {
            std::string media_id = j[eosStr][mediaIdStr];
            DEBUG_LOG("endOfStream : mediaId = %s", media_id.c_str());
            client->seek(0);
        }
        else if (j.contains(pausedStr))
        {
            client->state = MediaState::PAUSE;
        }
        else if (j.contains(playingStr))
        {
            client->state = MediaState::PLAY;
        }

        return true;
    };

    ret = luna_client->subscribe(uri.c_str(), to_string(j).c_str(), &subscribeKey_, subscribeCb,
                                 this);

    return ret;
}

bool MediaClient::unsubscribe()
{
    bool ret = false;

    // send message
    std::string uri = mUri + __func__;

    json j;
    j[mediaIdStr] = mediaId;

    DEBUG_LOG("%s '%s'", uri.c_str(), to_string(j).c_str());
    ret = luna_client->callAsync(uri.c_str(), to_string(j).c_str(), funCb, (void *)__func__);

    return ret;
}

bool MediaClient::seek(int pos)
{
    bool ret = false;

    // send message
    std::string uri = mUri + __func__;

    json j;
    j[mediaIdStr] = mediaId;
    j["position"] = pos;

    DEBUG_LOG("%s '%s'", uri.c_str(), to_string(j).c_str());
    ret = luna_client->callAsync(uri.c_str(), to_string(j).c_str(), funCb, (void *)__func__);

    return ret;
}

/*
luna-send -n 1 -f luna://com.webos.media/takeCameraSnapshot '{
    "mediaId": "_IqrDWLvVUpKK77",
    "location": "/media/internal/",
    "format": "jpg",
    "width": 1280,
    "height": 720,
    "pictureQuality": 90
}'
*/
bool MediaClient::takeCameraSnapshot()
{
    bool ret = false;

    // send message
    std::string uri = mUri + __func__;

    json j;
    j[mediaIdStr]       = mediaId;
    j["location"]       = "/media/internal/";
    j["width"]          = appParm.width;
    j["height"]         = appParm.height;
    j["format"]         = "jpg";
    j["pictureQuality"] = 90;
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

/*
luna-send -n 1 -f luna://com.webos.media/startCameraRecord '{
   "mediaId" :"_m7D5151u4g3g8f" ,
   "location":"/media/internal/",
   "format" :"MP4",
   "audio":true,
   "audioSrc":"usb_mic0"
}'
*/
bool MediaClient::startCameraRecord()
{
    bool ret = false;

    // send message
    std::string uri = mUri + __func__;

    json j;
    j[mediaIdStr] = mediaId;
    j["location"] = "/media/internal/";
    j["format"]   = "MP4";
    j["audio"]    = appParm.disable_audio ? false : true;
    j["audioSrc"] = "usb_mic0";
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

bool MediaClient::stopCameraRecord()
{
    bool ret = false;

    // send message
    std::string uri = mUri + __func__;

    json j;
    j[mediaIdStr] = mediaId;
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
