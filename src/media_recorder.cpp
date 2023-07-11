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

    if (video_src.empty())
    {
        if (audio_src.empty())
        {
            PLOGE("Source must be specified");
            return ERR_SOURCE_NOT_SPECIFIED;
        }
        else
        {
            PLOGE("Audio only recording is not supported");
            return ERR_NOT_SUPPORT_AUDIO_ONLY_RECORDING;
        }
    }

    videoSrc   = video_src;
    audioSrc   = audio_src;
    recorderId = getRandomNumber();

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
    if (state != OPEN && state != PREPARED)
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

        state = PREPARED;
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
    if (state != PREPARED)
    {
        PLOGE("Invalid state %d", state);
        return ERR_INVALID_STATE;
    }

    // send message
    std::string uri = "luna://com.webos.media/startCameraRecord";

    json j;
    j["mediaId"]  = videoSrc;
    j["location"] = mPath;
    j["format"]   = mFormat;
    j["audio"]    = audioSrc.empty() ? false : true;
    j["audioSrc"] = audioSrc;
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

    return ERR_FAILED_TO_START_RECORDING;
}

ErrorCode MediaRecorder::stop()
{
    PLOGI("");
    if (state != RECORDING)
    {
        PLOGE("Invalid state %d", state);
        return ERR_INVALID_STATE;
    }

    // send message
    std::string uri = "luna://com.webos.media/stopCameraRecord";

    json j;
    j["mediaId"] = videoSrc;
    PLOGI("%s '%s'", uri.c_str(), to_string(j).c_str());

    std::string resp;
    luna_client->callSync(uri.c_str(), to_string(j).c_str(), &resp);
    PLOGI("resp %s", resp.c_str());

    try
    {
        json jOut = json::parse(resp);
        if (get_optional<bool>(jOut, returnValueStr.c_str()).value_or(false))
        {
            state = PREPARED;
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
