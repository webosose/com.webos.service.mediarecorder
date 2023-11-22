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
#include "luna_client.h"
#include <nlohmann/json.hpp>
#include <random>

using namespace nlohmann;

const std::string returnValueStr("returnValue");

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
        "MPEG4"
        // Additional video file types can be added here.
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

ErrorCode MediaRecorder::open(std::string &video_src, std::string &audio_src)
{
    PLOGI("");

    if (video_src.empty() && audio_src.empty())
    {
        PLOGE("Source must be specified");
        return ERR_SOURCE_NOT_SPECIFIED;
    }

    videoSrc   = video_src;
    audioSrc   = audio_src;
    recorderId = getRandomNumber();

    // set default audio format (audioCodec, sampleRate, channels, bitRate)
    mAudioFormat = {"AAC", 44100, 2, 192000};

    PLOGI("mAudioFormat: %s, %d, %d, %d", mAudioFormat.audioCodec.c_str(), mAudioFormat.sampleRate,
          mAudioFormat.channels, mAudioFormat.bitRate);

    // set default vidoe format (width, height, fps, bitRate)
    // ToDo camera service getFormat
    mVideoFormat = {"H264", 1280, 720, 30, 200000};

    PLOGI("mVideoFormat: %s, %d, %d, %d, %d", mVideoFormat.videoCodec.c_str(), mVideoFormat.width,
          mVideoFormat.height, mVideoFormat.fps, mAudioFormat.bitRate);

    GMainContext *c = g_main_context_new();
    loop_           = g_main_loop_new(c, false);

    try
    {
        loopThread_ = std::make_unique<std::thread>(g_main_loop_run, loop_);
    }
    catch (const std::system_error &e)
    {
        PLOGE("Caught a system_error with code %d meaning %s", e.code().value(), e.what());
        return ERR_OPEN_FAIL;
    }

    while (!g_main_loop_is_running(loop_))
    {
    }

    std::string thread_name = "t" + std::to_string(recorderId);
    pthread_setname_np(loopThread_->native_handle(), thread_name.c_str());

    std::string service_name = "com.webos.service.mediarecorder-" + std::to_string(recorderId);
    luna_client              = std::make_unique<LunaClient>(service_name.c_str(), c);
    g_main_context_unref(c);

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

    g_main_loop_quit(loop_);
    if (loopThread_->joinable())
    {
        try
        {
            loopThread_->join();
        }
        catch (const std::system_error &e)
        {
            PLOGE("Caught a system_error with code %d meaning %s", e.code().value(), e.what());
            return ERR_CLOSE_FAIL;
        }
    }
    g_main_loop_unref(loop_);

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

    mPath = path;
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

    // If setOutputFile method is not invoked
    if (mPath.empty())
    {
        PLOGI("Using default path : /media/internal/");
        mPath = "/media/internal/";
    }

    if (isSupportedVideoFileFormat(format))
    {
        if (format == "MPEG4")
        {
            mFormat = "MP4";
        }

        return ERR_NONE;
    }
    else
    {
        return ERR_UNSUPPORTED_FORMAT;
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
        auto video        = json::object();
        video["videoSrc"] = videoSrc;
        video["width"]    = mVideoFormat.width;
        video["height"]   = mVideoFormat.height;
        video["codec"]    = mVideoFormat.videoCodec;
        video["fps"]      = mVideoFormat.fps;
        video["bitRate"]  = mVideoFormat.bitRate;
        json_obj["video"] = video;
    }

    if (!audioSrc.empty())
    {
        auto audio            = json::object();
        audio["codec"]        = mAudioFormat.audioCodec;
        audio["sampleRate"]   = mAudioFormat.sampleRate;
        audio["channelCount"] = mAudioFormat.channels;
        audio["bitRate"]      = mAudioFormat.bitRate;
        json_obj["audio"]     = audio;
    }

    json_obj["path"]   = mPath;
    json_obj["format"] = mFormat;

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
    luna_client->callSync(uri.c_str(), to_string(j).c_str(), &resp);
    PLOGI("resp %s", resp.c_str());

    try
    {
        json jOut = json::parse(resp);
        if (get_optional<bool>(jOut, returnValueStr.c_str()).value_or(false))
        {
            mMediaId = get_optional<std::string>(jOut, "mediaId").value_or("");

            // send message for play
            if (mMediaId.empty())
            {

                throw std::invalid_argument("mMediaId is empty");
            }

            json j;
            j["mediaId"]    = mMediaId;
            std::string uri = "luna://com.webos.media/play";
            std::string resp;
            luna_client->callSync(uri.c_str(), to_string(j).c_str(), &resp);
            PLOGI("resp %s", resp.c_str());

            json jOut = json::parse(resp);
            if (get_optional<bool>(jOut, returnValueStr.c_str()).value_or(false))
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
    j["mediaId"] = mMediaId;
    PLOGI("%s '%s'", uri.c_str(), to_string(j).c_str());

    std::string resp;
    luna_client->callSync(uri.c_str(), to_string(j).c_str(), &resp);
    PLOGI("resp %s", resp.c_str());

    try
    {
        json jOut = json::parse(resp);
        if (get_optional<bool>(jOut, returnValueStr.c_str()).value_or(false))
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

    // send message
    std::string uri = "luna://com.webos.media/takeCameraSnapshot";

    json j;
    j["mediaId"]        = videoSrc;
    j["location"]       = path;
    j["format"]         = format; // [TODO] Required but does not work
    j["width"]          = 1280;   // [TODO] Required but does not work
    j["height"]         = 720;    // [TODO] Required but does not work
    j["pictureQuality"] = 30;     // [TODO] Required but does not work
    PLOGI("%s '%s'", uri.c_str(), to_string(j).c_str());

    std::string resp;
    luna_client->callSync(uri.c_str(), to_string(j).c_str(), &resp);
    PLOGI("resp %s", resp.c_str());

    try
    {
        json jOut = json::parse(resp);
        if (get_optional<bool>(jOut, returnValueStr.c_str()).value_or(false))
        {
            return ERR_NONE;
        }
    }
    catch (const json::exception &e)
    {
        PLOGE("Error occurred: %s", e.what());
    }

    return ERR_SNAPSHOT_CAPTURE_FAILED;
}

ErrorCode MediaRecorder::setAudioFormat(std::string &audioCodec, unsigned int sampleRate,
                                        unsigned int channels, unsigned int bitRate)
{
    PLOGI("");
    if (state != OPEN)
    {
        PLOGE("Invalid state %d", state);
        return ERR_INVALID_STATE;
    }

    if (!audioCodec.empty())
        mAudioFormat.audioCodec = audioCodec;
    if (sampleRate != 0)
        mAudioFormat.sampleRate = sampleRate;
    if (channels != 0)
        mAudioFormat.channels = channels;
    if (bitRate != 0)
        mAudioFormat.bitRate = bitRate;

    PLOGI("mAudioFormat: %s, %d, %d, %d", mAudioFormat.audioCodec.c_str(), mAudioFormat.sampleRate,
          mAudioFormat.channels, mAudioFormat.bitRate);

    return ERR_NONE;
}

ErrorCode MediaRecorder::setVideoFormat(std::string &videoCodec, unsigned int width,
                                        unsigned int height, unsigned int fps, unsigned int bitRate)
{
    PLOGI("");
    if (state != OPEN)
    {
        PLOGE("Invalid state %d", state);
        return ERR_INVALID_STATE;
    }

    if (!videoCodec.empty())
        mVideoFormat.videoCodec = videoCodec;
    if (width != 0)
        mVideoFormat.width = width;
    if (height != 0)
        mVideoFormat.height = height;
    if (fps != 0)
        mVideoFormat.fps = fps;
    if (bitRate != 0)
        mVideoFormat.bitRate = bitRate;

    PLOGI("mAudioFormat: %s, %d, %d, %d", mAudioFormat.audioCodec.c_str(), mAudioFormat.sampleRate,
          mAudioFormat.channels, mAudioFormat.bitRate);

    return ERR_NONE;
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
    j["mediaId"] = mMediaId;
    PLOGI("%s '%s'", uri.c_str(), to_string(j).c_str());

    std::string resp;
    luna_client->callSync(uri.c_str(), to_string(j).c_str(), &resp);
    PLOGI("resp %s", resp.c_str());

    try
    {
        json jOut = json::parse(resp);
        if (get_optional<bool>(jOut, returnValueStr.c_str()).value_or(false))
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
    j["mediaId"] = mMediaId;
    PLOGI("%s '%s'", uri.c_str(), to_string(j).c_str());

    std::string resp;
    luna_client->callSync(uri.c_str(), to_string(j).c_str(), &resp);
    PLOGI("resp %s", resp.c_str());

    try
    {
        json jOut = json::parse(resp);
        if (get_optional<bool>(jOut, returnValueStr.c_str()).value_or(false))
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