// Copyright (c) 2023 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#define LOG_TAG "MediaRecorder"
#include "media_recorder.h"
#include "json_utils.h"
#include "log.h"
#include "ls_connector.h"
#include <nlohmann/json.hpp>
#include <random>
#include <sys/time.h>

using namespace nlohmann;

const char *const mediaIdStr     = "mediaId";
const char *const returnValueStr = "returnValue";

const std::string mp4Format = "MP4";
const std::string m4aFormat = "M4A";

#define LUNA_CALLBACK(NAME)                                                                        \
    +[](const char *m, void *c) -> bool { return ((MediaRecorder *)c)->NAME(m); }

// Get random number between 1000 and 9999
static int getRandomNumber()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<int> dist(1000, 9999);

    return dist(gen);
}

static bool isSupportedVideoFileFormat(const std::string &input)
{
    std::vector<std::string> videoFileTypes = {
        mp4Format
        // You can add additional file formats here.
    };

    for (const std::string &fileType : videoFileTypes)
    {
        if (input == fileType)
        {
            return true;
        }
    }

    return false;
}

static bool isSupportedAudioFileFormat(const std::string &input)
{
    std::vector<std::string> audioFileTypes = {
        m4aFormat
        // You can add additional file formats here.
    };

    for (const std::string &fileType : audioFileTypes)
    {
        if (input == fileType)
        {
            return true;
        }
    }

    return false;
}

static bool isSupportedImageFileFormat(const std::string &input)
{
    std::vector<std::string> imageFileTypes = {
        "JPEG"
        // Additional image file types can be added here.
    };

    for (const std::string &fileType : imageFileTypes)
    {
        if (input == fileType)
        {
            return true;
        }
    }

    return false;
}

static bool isSupportedVideoCodec(const std::string &input)
{
    std::vector<std::string> videoCodecTypes = {
        "H264"
        // Additional video codec types can be added here.
    };

    for (const std::string &codecType : videoCodecTypes)
    {
        if (input == codecType)
        {
            return true;
        }
    }

    return false;
}

static ErrorCode isSupportedAudioFormat(const std::string &codec, const unsigned int sampleRate,
                                        const unsigned int channels, const unsigned int bitRate)
{
    std::vector<audio_support_list_t> audioSupportTypes = {
        // Additional audio format types can be added here.
        // example
        //{"AAC_check_bitrate", {32000, 44100, 48000}, {1, 2, 3, 4, 5}, {0,
        // 64000,128000,192000,256000}},
        {"AAC",
         {7350, 8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000, 64000, 88200, 96000},
         {1, 2, 3, 4, 5},
         {}}};

    auto pos = std::find_if(
        audioSupportTypes.begin(), audioSupportTypes.end(),
        [&](audio_support_list_t &i)
        {
            if (i.codec == "AAC")
            {
                auto samplePos =
                    std::find_if(i.sampleRate.begin(), i.sampleRate.end(),
                                 [&sampleRate](unsigned int n) { return n == sampleRate; });

                auto channelPos =
                    std::find_if(i.channels.begin(), i.channels.end(),
                                 [&channels](unsigned int n) { return n == channels; });

                if ((i.codec == codec) && samplePos != i.sampleRate.end() &&
                    channelPos != i.channels.end())
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }
            else if (i.codec == "AAC_check_bitrate")
            {
                auto samplePos =
                    std::find_if(i.sampleRate.begin(), i.sampleRate.end(),
                                 [&sampleRate](unsigned int n) { return n == sampleRate; });

                auto channelPos =
                    std::find_if(i.channels.begin(), i.channels.end(),
                                 [&channels](unsigned int n) { return n == channels; });

                auto bitRatePos = std::find_if(i.bitRate.begin(), i.bitRate.end(),
                                               [&bitRate](unsigned int n) { return n == bitRate; });

                if ((i.codec == codec) && samplePos != i.sampleRate.end() &&
                    channelPos != i.channels.end() && bitRatePos != i.bitRate.end())
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
        });

    if (pos != audioSupportTypes.end())
    {
        return ERR_NONE;
    }
    else
    {
        return ERR_UNSUPPORTED_AUDIO_FORMAT;
    }
}

static bool isInTargetFolders(const std::string &path)
{
    std::vector<std::string> targetFolders = {
        "/media/internal/", "/tmp/"
        // Additional target folders can be added here.
    };

    for (const std::string &folder : targetFolders)
    {
        if (path.substr(0, folder.size()) == folder)
        {
            return true;
        }
    }
    return false;
}

MediaRecorder::MediaRecorder() { PLOGI(""); }

MediaRecorder::~MediaRecorder()
{
    PLOGI("");
    if (state != CLOSE)
    {
        close();
    }
}

ErrorCode MediaRecorder::open(std::string &video_src, bool audio_src)
{
    PLOGI("");

    if (video_src.empty() && !audio_src)
    {
        PLOGE("Source must be specified");
        return ERR_SOURCE_NOT_SPECIFIED;
    }

    videoSrc   = video_src;
    audioSrc   = audio_src;
    recorderId = getRandomNumber();

    // set default audio format
    if (audioSrc)
    {
        mAudioFormat = mAudioFormatDefault;

        PLOGI("mAudioFormat: %s, %u, %u, %u", mAudioFormat.codec.c_str(), mAudioFormat.sampleRate,
              mAudioFormat.channels, mAudioFormat.bitRate);
    }
    std::string service_name = "com.webos.service.mediarecorder-" + std::to_string(recorderId);
    record_client            = std::make_unique<LSConnector>(service_name, "record");

    state = OPEN;
    return ERR_NONE;
}

ErrorCode MediaRecorder::close()
{
    PLOGI("");
    if (state != OPEN)
    {
        PLOGE("Invalid state %d", state);
        return ERR_INVALID_STATE;
    }

    state = CLOSE;
    return ERR_NONE;
}

ErrorCode MediaRecorder::setOutputFile(std::string &path)
{
    PLOGI("");
    if (state != OPEN)
    {
        PLOGE("Invalid state %d", state);
        return ERR_INVALID_STATE;
    }

    if (!isInTargetFolders(path))
    {
        return ERR_CANNOT_WRITE;
    }

    mRecordBasePath = path;
    return ERR_NONE;
}

ErrorCode MediaRecorder::setOutputFormat(std::string &format)
{
    PLOGI("");
    if (state != OPEN)
    {
        PLOGE("Invalid state %d", state);
        return ERR_INVALID_STATE;
    }

    mFormat = format;

    if (!videoSrc.empty())
    {
        return isSupportedVideoFileFormat(format) ? ERR_NONE : ERR_UNSUPPORTED_FORMAT;
    }
    else
    {
        return isSupportedAudioFileFormat(format) ? ERR_NONE : ERR_UNSUPPORTED_FORMAT;
    }
}

ErrorCode MediaRecorder::start()
{
    PLOGI("");
    if (state != OPEN)
    {
        PLOGE("Invalid state %d", state);
        return ERR_INVALID_STATE;
    }

    auto json_obj     = json::object();
    json_obj["appId"] = "com.webos.app.mediaevents-test";

    if (!videoSrc.empty())
    {
        if (!getCameraFormat())
        {
            return ERR_CAMERA_OPEN_FAIL;
        }

        auto video        = json::object();
        video["videoSrc"] = videoSrc;
        video["width"]    = mVideoFormat.width;
        video["height"]   = mVideoFormat.height;
        video["codec"]    = mVideoFormat.codec;
        video["fps"]      = mVideoFormat.fps;
        video["bitRate"]  = mVideoFormat.bitRate;
        json_obj["video"] = video;

        PLOGI("Video Format: codec=%s, width=%d, height=%d, fps=%d, bitRate=%d",
              mVideoFormat.codec.c_str(), mVideoFormat.width, mVideoFormat.height, mVideoFormat.fps,
              mVideoFormat.bitRate);
    }

    if (audioSrc)
    {
        auto audio            = json::object();
        audio["codec"]        = mAudioFormat.codec;
        audio["sampleRate"]   = mAudioFormat.sampleRate;
        audio["channelCount"] = mAudioFormat.channels;
        audio["bitRate"]      = mAudioFormat.bitRate;
        json_obj["audio"]     = audio;

        PLOGI("Audio Format: codec=%s, sampleRate=%d, channels=%d, bitRate=%d",
              mAudioFormat.codec.c_str(), mAudioFormat.sampleRate, mAudioFormat.channels,
              mAudioFormat.bitRate);
    }

    // If setOutputFormat method is not invoked
    if (!videoSrc.empty())
    {
        if (mFormat.empty())
            mFormat = mp4Format;
        else if (!isSupportedVideoFileFormat(mFormat))
            return ERR_UNSUPPORTED_VIDEO_FORMAT;
    }
    else if (audioSrc)
    {
        if (mFormat.empty())
            mFormat = m4aFormat;
        else if (!isSupportedAudioFileFormat(mFormat))
            return ERR_UNSUPPORTED_AUDIO_FORMAT;
    }

    // If setOutputFile method is not invoked
    if (mRecordBasePath.empty())
    {
        PLOGI("Using default path : /media/internal/");
        mRecordBasePath = "/media/internal/";
    }
    if (!videoSrc.empty())
    {
        mRecordPath = createRecordFileName(mRecordBasePath, "Record");
    }
    else if (audioSrc)
    {
        mRecordPath = createRecordFileName(mRecordBasePath, "Audio");
    }

    json_obj["format"] = mFormat;
    json_obj["path"]   = mRecordPath;

    auto json_obj_option      = json::object();
    json_obj_option["option"] = json_obj;

    json j;
    j["uri"]     = "record://com.webos.service.mediarecorder";
    j["payload"] = json_obj_option;
    j["type"]    = "record";

    // send message for load
    std::string uri = "luna://com.webos.media/load";
    PLOGI("%s '%s'", uri.c_str(), to_string(j).c_str());

    std::string resp;
    record_client->callSync(uri.c_str(), to_string(j).c_str(), &resp);
    PLOGI("resp %s", resp.c_str());

    try
    {
        json jOut = json::parse(resp);
        if (get_optional<bool>(jOut, returnValueStr).value_or(false))
        {
            mMediaId = get_optional<std::string>(jOut, mediaIdStr).value_or("");

            // send message for play
            if (mMediaId.empty())
            {
                throw std::invalid_argument("mMediaId is empty");
            }

            json j;
            j[mediaIdStr]   = mMediaId;
            std::string uri = "luna://com.webos.media/play";
            std::string resp;
            record_client->callSync(uri.c_str(), to_string(j).c_str(), &resp);
            PLOGI("resp %s", resp.c_str());

            json jOut = json::parse(resp);
            if (get_optional<bool>(jOut, returnValueStr).value_or(false))
            {
                state = RECORDING;
                return ERR_NONE;
            }
        }
    }
    catch (const json::exception &e)
    {
        PLOGE("Error occurred: %s", e.what());
    }

    return ERR_FAILED_TO_START_RECORDING;
}

ErrorCode MediaRecorder::stop()
{
    PLOGI("");
    if (state != RECORDING && state != PAUSE)
    {
        PLOGE("Invalid state %d", state);
        return ERR_INVALID_STATE;
    }

    // send message
    std::string uri = "luna://com.webos.media/unload";

    json j;
    j[mediaIdStr] = mMediaId;
    PLOGI("%s '%s'", uri.c_str(), to_string(j).c_str());

    std::string resp;
    record_client->callSync(uri.c_str(), to_string(j).c_str(), &resp);
    PLOGI("resp %s", resp.c_str());

    try
    {
        json jOut = json::parse(resp);
        if (get_optional<bool>(jOut, returnValueStr).value_or(false))
        {
            state = OPEN;
            return ERR_NONE;
        }
    }
    catch (const json::exception &e)
    {
        PLOGE("Error occurred: %s", e.what());
    }

    return ERR_FAILED_TO_STOP_RECORDING;
}

ErrorCode MediaRecorder::takeSnapshot(std::string &path, std::string &format)
{
    // ToDo : New Implementation required
    PLOGI("");
    if (state != RECORDING)
    {
        PLOGE("Invalid state %d", state);
        return ERR_INVALID_STATE;
    }

    if (!isInTargetFolders(path))
    {
        return ERR_CANNOT_WRITE;
    }

    if (!isSupportedImageFileFormat(format))
    {
        return ERR_UNSUPPORTED_FORMAT;
    }

    auto json_obj     = json::object();
    json_obj["appId"] = "com.webos.app.mediaevents-test";

    if (!videoSrc.empty())
    {
        auto image        = json::object();
        image["videoSrc"] = videoSrc;
        image["width"]    = mVideoFormat.width;
        image["height"]   = mVideoFormat.height;
        image["codec"]    = format;
        image["quality"]  = 90;
        json_obj["image"] = image;
    }

    mCapturePath     = createRecordFileName(path, "Capture");
    json_obj["path"] = mCapturePath;

    auto json_obj_option      = json::object();
    json_obj_option["option"] = json_obj;

    json j;
    j["uri"]     = "record://com.webos.service.mediarecorder";
    j["payload"] = json_obj_option;
    j["type"]    = "record";

    // send message for load
    std::string uri = "luna://com.webos.media/load";
    PLOGI("%s '%s'", uri.c_str(), to_string(j).c_str());

    if (snapshot_client == nullptr)
    {
        std::string service_name =
            "com.webos.service.mediarecorder-" + std::to_string(recorderId) + "-snapshot";
        snapshot_client = std::make_unique<LSConnector>(service_name, "snapshot");
    }

    std::string resp;
    snapshot_client->callSync(uri.c_str(), to_string(j).c_str(), &resp);
    PLOGI("resp %s", resp.c_str());

    try
    {
        json jOut = json::parse(resp);
        if (get_optional<bool>(jOut, returnValueStr).value_or(false))
        {
            // send message for subscribe
            json j;
            j[mediaIdStr] = get_optional<std::string>(jOut, mediaIdStr).value_or("");

            std::string uri = "luna://com.webos.media/subscribe";
            bool retVal     = snapshot_client->subscribe(uri.c_str(), j.dump().c_str(),
                                                         LUNA_CALLBACK(snapshotCb), this);
            PLOGI("%s '%s'", uri.c_str(), to_string(j).c_str());
            if (!retVal)
            {
                PLOGE("%s fail to subscribe", __func__);
            }

            // send message for play
            uri = "luna://com.webos.media/play";
            PLOGI("%s '%s'", uri.c_str(), to_string(j).c_str());

            std::string resp;
            snapshot_client->callSync(uri.c_str(), to_string(j).c_str(), &resp);
            PLOGI("resp %s", resp.c_str());

            json jOut = json::parse(resp);
            if (get_optional<bool>(jOut, returnValueStr).value_or(false))
            {
                mEos = false;
                {
                    int cnt = 0;
                    while (!mEos && cnt < 10000) // 10s
                    {
                        g_usleep(1000);
                        cnt++;
                    }
                    PLOGI("capture done : %d ms", cnt);
                }

                return ERR_NONE;
            }
        }
    }
    catch (const json::exception &e)
    {
        PLOGE("Error occurred: %s", e.what());
    }

    return ERR_SNAPSHOT_CAPTURE_FAILED;
}

bool MediaRecorder::snapshotCb(const char *message)
{
    PLOGI("payload : %s", message);

    json j = json::parse(message, nullptr, false);
    if (j.is_discarded())
    {
        PLOGE("payload parsing fail!");
        return false;
    }

    if (j.contains("endOfStream"))
    {
        mEos = true;

        // send message for unsubscribe
        bool retVal = snapshot_client->unsubscribe();
        if (!retVal)
        {
            PLOGE("%s fail to subscribe", __func__);
        }
    }

    return true;
}

ErrorCode MediaRecorder::setAudioFormat(std::string &audioCodec, uint32_t sampleRate,
                                        uint32_t channels, uint32_t bitRate)
{
    PLOGI("");
    if (state != OPEN)
    {
        PLOGE("Invalid state %d", state);
        return ERR_INVALID_STATE;
    }

    if (!audioSrc)
    {
        PLOGE("audio is not opened");
        return ERR_AUDIO_NOT_OPENED;
    }

    if (isSupportedAudioFormat(audioCodec, sampleRate, channels, bitRate) != ERR_NONE)
    {
        return ERR_UNSUPPORTED_AUDIO_FORMAT;
    }

    mAudioFormat.codec      = audioCodec;
    mAudioFormat.sampleRate = sampleRate;
    mAudioFormat.channels   = channels;

    // prevent pipeline errors
    if (bitRate > 0 && bitRate < channels * 5 && audioCodec == "AAC")
    {
        mAudioFormat.bitRate = mAudioFormat.channels * 5;
    }
    else
    {
        mAudioFormat.bitRate = bitRate;
    }

    PLOGI("mAudioFormat: %s, %u, %u, %u", mAudioFormat.codec.c_str(), mAudioFormat.sampleRate,
          mAudioFormat.channels, mAudioFormat.bitRate);

    return ERR_NONE;
}

ErrorCode MediaRecorder::setVideoFormat(std::string &videoCodec, unsigned int bitRate)
{
    PLOGI("");
    ErrorCode err = ERR_NONE;
    if (state != OPEN)
    {
        PLOGE("Invalid state %d", state);
        return ERR_INVALID_STATE;
    }

    if (videoSrc.empty())
    {
        PLOGE("video is not opened");
        return ERR_VIDEO_NOT_OPENED;
    }

    if (!isSupportedVideoCodec(videoCodec))
    {
        return ERR_UNSUPPORTED_VIDEO_FORMAT;
    }

    if (!videoCodec.empty())
    {
        mVideoFormat.codec = videoCodec;
    }

    if (bitRate >= 25000 && bitRate <= 25000000)
    {
        mVideoFormat.bitRate = bitRate;
    }
    else
    {
        err = ERR_VIDEO_BITRATE_OUT_OF_RANGE;
    }

    PLOGI("mVideoFormat: %s,  %d", mVideoFormat.codec.c_str(), mVideoFormat.bitRate);

    return err;
}

ErrorCode MediaRecorder::pause()
{
    PLOGI("");

    if (state != RECORDING)
    {
        PLOGE("Invalid state %d", state);
        return ERR_INVALID_STATE;
    }

    // send message
    std::string uri = "luna://com.webos.media/pause";

    json j;
    j[mediaIdStr] = mMediaId;
    PLOGI("%s '%s'", uri.c_str(), to_string(j).c_str());

    std::string resp;
    record_client->callSync(uri.c_str(), to_string(j).c_str(), &resp);
    PLOGI("resp %s", resp.c_str());

    try
    {
        json jOut = json::parse(resp);
        if (get_optional<bool>(jOut, returnValueStr).value_or(false))
        {
            state = PAUSE;
            return ERR_NONE;
        }
    }
    catch (const json::exception &e)
    {
        PLOGE("Error occurred: %s", e.what());
    }

    return ERR_FAILED_TO_PAUSE;
}

ErrorCode MediaRecorder::resume()
{
    PLOGI("");

    if (state != PAUSE)
    {
        PLOGE("Invalid state %d", state);
        return ERR_INVALID_STATE;
    }

    // send message
    std::string uri = "luna://com.webos.media/play";

    json j;
    j[mediaIdStr] = mMediaId;
    PLOGI("%s '%s'", uri.c_str(), to_string(j).c_str());

    std::string resp;
    record_client->callSync(uri.c_str(), to_string(j).c_str(), &resp);
    PLOGI("resp %s", resp.c_str());

    try
    {
        json jOut = json::parse(resp);
        if (get_optional<bool>(jOut, returnValueStr).value_or(false))
        {
            state = RECORDING;
            return ERR_NONE;
        }
    }
    catch (const json::exception &e)
    {
        PLOGE("Error occurred: %s", e.what());
    }

    return ERR_FAILED_TO_RESUME;
}

bool MediaRecorder::isSupportedExtension(const std::string &extension) const
{
    std::string lowercaseExtension = extension;
    std::transform(lowercaseExtension.begin(), lowercaseExtension.end(), lowercaseExtension.begin(),
                   ::tolower);

    return (lowercaseExtension == "mp4") || (lowercaseExtension == "m4a") ||
           (lowercaseExtension == "jpg") || (lowercaseExtension == "jpeg");
}

std::string MediaRecorder::createRecordFileName(const std::string &recordpath,
                                                const std::string &prefix) const
{
    auto path = recordpath;
    if (path.empty())
        path = "/media/internal";

    // Find the file extension to check if file name is provided or path is provided
    std::size_t position  = path.find_last_of(".");
    std::string extension = path.substr(position + 1);

    if (!isSupportedExtension(extension))
    {
        // Check if specified location ends with '/' else add
        char ch = path.back();
        if (ch != '/')
            path += "/";

        std::time_t now  = std::time(nullptr);
        std::tm *timePtr = std::localtime(&now);
        if (timePtr == nullptr)
        {
            PLOGE("failed to get local time");
            return "";
        }

        struct timeval tmnow;
        gettimeofday(&tmnow, NULL);

        std::string ext;
        if (prefix == "Record")
        {
            ext = "mp4";
        }
        else if (prefix == "Audio")
        {
            ext = "m4a";
        }
        else if (prefix == "Capture")
        {
            ext = "jpeg";
        }
        else
        {
            PLOGE("Invalid prefix");
            return "";
        }

        // prefix + "DDMMYYYY-HHMMSSss" + "." + ext
        std::ostringstream oss;
        oss << prefix << std::setw(2) << std::setfill('0') << timePtr->tm_mday << std::setw(2)
            << std::setfill('0') << (timePtr->tm_mon) + 1 << std::setw(2) << std::setfill('0')
            << (timePtr->tm_year) + 1900 << "-" << std::setw(2) << std::setfill('0')
            << timePtr->tm_hour << std::setw(2) << std::setfill('0') << timePtr->tm_min
            << std::setw(2) << std::setfill('0') << timePtr->tm_sec << std::setw(2)
            << std::setfill('0') << tmnow.tv_usec / 10000;

        path += oss.str();
        path += "." + ext;
    }

    PLOGI("path : %s", path.c_str());
    return path;
}

bool MediaRecorder::getCameraFormat()
{
    // send message for getFormat
    json j;
    j["id"] = videoSrc;

    std::string uri = "luna://com.webos.service.camera2/getFormat";
    PLOGI("%s '%s'", uri.c_str(), to_string(j).c_str());

    std::string resp;
    record_client->callSync(uri.c_str(), to_string(j).c_str(), &resp, 16000);
    PLOGI("resp %s", resp.c_str());

    json jOut = json::parse(resp);
    if (get_optional<bool>(jOut, returnValueStr).value_or(false))
    {
        if (jOut.contains("params"))
        {
            json params         = jOut["params"];
            mVideoFormat.width  = get_optional<unsigned int>(params, "width").value_or(0);
            mVideoFormat.height = get_optional<unsigned int>(params, "height").value_or(0);
            mVideoFormat.fps    = get_optional<unsigned int>(params, "fps").value_or(0);
            PLOGI("width=%d, height=%d, fps=%d", mVideoFormat.width, mVideoFormat.height,
                  mVideoFormat.fps);

            return true;
        }
    }

    PLOGE("Failed to get camera format");
    return false;
}
