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

#define LOG_TAG "MediaRecorderManager"
#include "media_recorder_manager.h"
#include "error.h"
#include "error_manager.h"
#include "json_utils.h"
#include "log.h"
#include "media_recorder.h"
#include <nlohmann/json.hpp>
#include <string>

using namespace nlohmann;

const std::string service = "com.webos.service.mediarecorder";

void handleJsonException(const std::exception &e, ErrorCode &error_code)
{
    if (dynamic_cast<const json::parse_error *>(&e) != nullptr)
    {
        error_code = ERR_JSON_PARSING;
        PLOGE("JSON parsing error: %s", e.what());
    }
    else if (dynamic_cast<const json::type_error *>(&e) != nullptr)
    {
        error_code = ERR_JSON_TYPE;
        PLOGE("JSON type error: %s", e.what());
    }
    else
    {
        PLOGE("Error occurred: %s", e.what());
    }
}

MediaRecorderManager::MediaRecorderManager() : LS::Handle(LS::registerService(service.c_str()))
{
    LS_CATEGORY_BEGIN(MediaRecorderManager, "/")
    LS_CATEGORY_METHOD(open)
    LS_CATEGORY_METHOD(close)
    LS_CATEGORY_METHOD(setOutputFile)
    LS_CATEGORY_METHOD(setOutputFormat)
    LS_CATEGORY_METHOD(setVideoFormat)
    LS_CATEGORY_METHOD(setAudioFormat)
    LS_CATEGORY_METHOD(start)
    LS_CATEGORY_METHOD(stop)
    LS_CATEGORY_METHOD(takeSnapshot)
    LS_CATEGORY_METHOD(pause)
    LS_CATEGORY_METHOD(resume)
    LS_CATEGORY_END;

    // attach to mainloop and run it
    attachToLoop(main_loop_ptr_.get());

    // run the gmainloop
    g_main_loop_run(main_loop_ptr_.get());
}

bool MediaRecorderManager::open(LSMessage &message)
{
    ErrorCode error_code = ERR_LIST_END;
    int recorder_id      = 0;
    auto *payload        = LSMessageGetPayload(&message);
    PLOGI("payload %s", payload);

    try
    {
        json j = json::parse(payload);

        std::string video_src = get_optional<std::string>(j, "video").value_or("");
        bool audio_src        = get_optional<bool>(j, "audio").value_or(false);

        std::unique_ptr<MediaRecorder> recorder = std::make_unique<MediaRecorder>();
        error_code                              = recorder->open(video_src, audio_src);
        if (error_code == ERR_NONE)
        {
            recorder_id            = recorder->getRecorderId();
            recorders[recorder_id] = std::move(recorder);
            printRecorders();
        }
    }
    catch (const std::exception &e)
    {
        handleJsonException(e, error_code);
    }

    json resp;
    if (error_code == ERR_NONE)
    {
        resp["returnValue"] = true;
        resp["recorderId"]  = recorder_id;
    }
    else
    {
        resp["returnValue"] = false;

        Error error       = ErrorManager::getInstance().getError(error_code);
        resp["errorCode"] = error.getCode();
        resp["errorText"] = error.getMessage();

        PLOGE("%d %s", error.getCode(), error.getMessage().c_str());
    }

    std::string respStr = to_string(resp);
    PLOGI("reply %s", respStr.c_str());

    LS::Message request(&message);
    request.respond(respStr.c_str());

    return true;
}

bool MediaRecorderManager::close(LSMessage &message)
{
    ErrorCode error_code = ERR_LIST_END;
    auto *payload        = LSMessageGetPayload(&message);
    PLOGI("payload %s", payload);

    try
    {
        json j = json::parse(payload);

        int recorder_id = 0;
        if (auto value = get_optional<int>(j, "recorderId"))
        {
            recorder_id = *value;
        }
        else
        {
            error_code = ERR_RECORDER_ID_NOT_SPECIFIED;
            throw std::invalid_argument("Parameter is missing");
        }

        if (recorders.find(recorder_id) == recorders.end())
        {
            error_code = ERR_INVALID_RECORDER_ID;
            throw std::invalid_argument("Parameter is invalid");
        }

        error_code = recorders[recorder_id]->close();
        if (error_code == ERR_NONE)
        {
            recorders.erase(recorder_id);
            printRecorders();
        }
    }
    catch (const std::exception &e)
    {
        handleJsonException(e, error_code);
    }

    json resp;
    if (error_code == ERR_NONE)
    {
        resp["returnValue"] = true;
    }
    else
    {
        resp["returnValue"] = false;

        Error error       = ErrorManager::getInstance().getError(error_code);
        resp["errorCode"] = error.getCode();
        resp["errorText"] = error.getMessage();

        PLOGE("%d %s", error.getCode(), error.getMessage().c_str());
    }

    std::string respStr = to_string(resp);
    PLOGI("reply %s", respStr.c_str());

    LS::Message request(&message);
    request.respond(respStr.c_str());

    return true;
}

bool MediaRecorderManager::setOutputFile(LSMessage &message)
{
    ErrorCode error_code = ERR_LIST_END;
    auto *payload        = LSMessageGetPayload(&message);
    PLOGI("payload %s", payload);

    try
    {
        json j = json::parse(payload);

        int recorder_id = 0;
        if (auto value = get_optional<int>(j, "recorderId"))
        {
            recorder_id = *value;
        }
        else
        {
            error_code = ERR_RECORDER_ID_NOT_SPECIFIED;
            throw std::invalid_argument("Parameter is missing");
        }

        if (recorders.find(recorder_id) == recorders.end())
        {
            error_code = ERR_INVALID_RECORDER_ID;
            throw std::invalid_argument("Parameter is invalid");
        }

        // File path
        if (auto value = get_optional<std::string>(j, "path"))
        {
            error_code = recorders[recorder_id]->setOutputFile(*value);
        }
        else
        {
            error_code = ERR_PATH_NOT_SPECIFIED;
            throw std::invalid_argument("Parameter is missing");
        }
    }
    catch (const std::exception &e)
    {
        handleJsonException(e, error_code);
    }

    json resp;
    if (error_code == ERR_NONE)
    {
        resp["returnValue"] = true;
    }
    else
    {
        resp["returnValue"] = false;

        Error error       = ErrorManager::getInstance().getError(error_code);
        resp["errorCode"] = error.getCode();
        resp["errorText"] = error.getMessage();

        PLOGE("%d %s", error.getCode(), error.getMessage().c_str());
    }

    std::string respStr = to_string(resp);
    PLOGI("reply %s", respStr.c_str());

    LS::Message request(&message);
    request.respond(respStr.c_str());

    return true;
}

bool MediaRecorderManager::setOutputFormat(LSMessage &message)
{
    ErrorCode error_code = ERR_LIST_END;
    auto *payload        = LSMessageGetPayload(&message);
    PLOGI("payload %s", payload);

    try
    {
        json j = json::parse(payload);

        int recorder_id = 0;
        if (auto value = get_optional<int>(j, "recorderId"))
        {
            recorder_id = *value;
        }
        else
        {
            error_code = ERR_RECORDER_ID_NOT_SPECIFIED;
            throw std::invalid_argument("Parameter is missing");
        }

        if (recorders.find(recorder_id) == recorders.end())
        {
            error_code = ERR_INVALID_RECORDER_ID;
            throw std::invalid_argument("Parameter is invalid");
        }

        // Video file format
        if (auto value = get_optional<std::string>(j, "format"))
        {
            error_code = recorders[recorder_id]->setOutputFormat(*value);
        }
        else
        {
            error_code = ERR_FORMAT_NOT_SPECIFIED;
            throw std::invalid_argument("Parameter is missing");
        }
    }
    catch (const std::exception &e)
    {
        handleJsonException(e, error_code);
    }

    json resp;
    if (error_code == ERR_NONE)
    {
        resp["returnValue"] = true;
    }
    else
    {
        resp["returnValue"] = false;

        Error error       = ErrorManager::getInstance().getError(error_code);
        resp["errorCode"] = error.getCode();
        resp["errorText"] = error.getMessage();

        PLOGE("%d %s", error.getCode(), error.getMessage().c_str());
    }

    std::string respStr = to_string(resp);
    PLOGI("reply %s", respStr.c_str());

    LS::Message request(&message);
    request.respond(respStr.c_str());

    return true;
}

bool MediaRecorderManager::setVideoFormat(LSMessage &message)
{
    ErrorCode error_code = ERR_LIST_END;
    auto *payload        = LSMessageGetPayload(&message);
    PLOGI("payload %s", payload);

    try
    {
        json j = json::parse(payload);

        int recorder_id = 0;
        if (auto value = get_optional<int>(j, "recorderId"))
        {
            recorder_id = *value;
        }
        else
        {
            error_code = ERR_RECORDER_ID_NOT_SPECIFIED;
            throw std::invalid_argument("Parameter is missing");
        }

        if (recorders.find(recorder_id) == recorders.end())
        {
            error_code = ERR_INVALID_RECORDER_ID;
            throw std::invalid_argument("Parameter is invalid");
        }

        // Video format
        std::string videoCodec = get_optional<std::string>(j, "codec").value_or("H264");
        unsigned int bitRate   = get_optional<unsigned int>(j, "bitRate").value_or(10000000);

        error_code = recorders[recorder_id]->setVideoFormat(videoCodec, bitRate);
    }
    catch (const std::exception &e)
    {
        handleJsonException(e, error_code);
    }

    json resp;
    if (error_code == ERR_NONE)
    {
        resp["returnValue"] = true;
    }
    else
    {
        resp["returnValue"] = false;

        Error error       = ErrorManager::getInstance().getError(error_code);
        resp["errorCode"] = error.getCode();
        resp["errorText"] = error.getMessage();

        PLOGE("%d %s", error.getCode(), error.getMessage().c_str());
    }

    std::string respStr = to_string(resp);
    PLOGI("reply %s", respStr.c_str());

    LS::Message request(&message);
    request.respond(respStr.c_str());

    return true;
}

bool MediaRecorderManager::setAudioFormat(LSMessage &message)
{
    ErrorCode error_code = ERR_LIST_END;
    auto *payload        = LSMessageGetPayload(&message);
    PLOGI("payload %s", payload);

    try
    {
        json j = json::parse(payload);

        int recorder_id = 0;
        if (auto value = get_optional<int>(j, "recorderId"))
        {
            recorder_id = *value;
        }
        else
        {
            error_code = ERR_RECORDER_ID_NOT_SPECIFIED;
            throw std::invalid_argument("Parameter is missing");
        }

        if (recorders.find(recorder_id) == recorders.end())
        {
            error_code = ERR_INVALID_RECORDER_ID;
            throw std::invalid_argument("Parameter is invalid");
        }

        // audio format
        std::string audioCodec = get_optional<std::string>(j, "codec")
                                     .value_or(recorders[recorder_id]->mAudioFormatDefault.codec);
        uint32_t sampleRate = get_optional<uint32_t>(j, "sampleRate")
                                  .value_or(recorders[recorder_id]->mAudioFormatDefault.sampleRate);
        uint32_t audioChannel = get_optional<uint32_t>(j, "channelCount")
                                    .value_or(recorders[recorder_id]->mAudioFormatDefault.channels);
        uint32_t bitRate = get_optional<uint32_t>(j, "bitRate")
                               .value_or(recorders[recorder_id]->mAudioFormatDefault.bitRate);

        error_code =
            recorders[recorder_id]->setAudioFormat(audioCodec, sampleRate, audioChannel, bitRate);
    }
    catch (const std::exception &e)
    {
        handleJsonException(e, error_code);
    }

    json resp;
    if (error_code == ERR_NONE)
    {
        resp["returnValue"] = true;
    }
    else
    {
        resp["returnValue"] = false;

        Error error       = ErrorManager::getInstance().getError(error_code);
        resp["errorCode"] = error.getCode();
        resp["errorText"] = error.getMessage();

        PLOGE("%d %s", error.getCode(), error.getMessage().c_str());
    }

    std::string respStr = to_string(resp);
    PLOGI("reply %s", respStr.c_str());

    LS::Message request(&message);
    request.respond(respStr.c_str());

    return true;
}

bool MediaRecorderManager::start(LSMessage &message)
{
    ErrorCode error_code = ERR_LIST_END;
    auto *payload        = LSMessageGetPayload(&message);
    PLOGI("payload %s", payload);

    try
    {
        json j = json::parse(payload);

        int recorder_id = 0;
        if (auto value = get_optional<int>(j, "recorderId"))
        {
            recorder_id = *value;
        }
        else
        {
            error_code = ERR_RECORDER_ID_NOT_SPECIFIED;
            throw std::invalid_argument("Parameter is missing");
        }

        if (recorders.find(recorder_id) == recorders.end())
        {
            error_code = ERR_INVALID_RECORDER_ID;
            throw std::invalid_argument("Parameter is invalid");
        }

        error_code = recorders[recorder_id]->start();
    }
    catch (const std::exception &e)
    {
        handleJsonException(e, error_code);
    }

    json resp;
    if (error_code == ERR_NONE)
    {
        resp["returnValue"] = true;
    }
    else
    {
        resp["returnValue"] = false;

        Error error       = ErrorManager::getInstance().getError(error_code);
        resp["errorCode"] = error.getCode();
        resp["errorText"] = error.getMessage();

        PLOGE("%d %s", error.getCode(), error.getMessage().c_str());
    }

    std::string respStr = to_string(resp);
    PLOGI("reply %s", respStr.c_str());

    LS::Message request(&message);
    request.respond(respStr.c_str());

    return true;
}

bool MediaRecorderManager::stop(LSMessage &message)
{
    ErrorCode error_code = ERR_LIST_END;
    auto *payload        = LSMessageGetPayload(&message);
    PLOGI("payload %s", payload);

    int recorder_id = 0;

    try
    {
        json j = json::parse(payload);
        if (auto value = get_optional<int>(j, "recorderId"))
        {
            recorder_id = *value;
        }
        else
        {
            error_code = ERR_RECORDER_ID_NOT_SPECIFIED;
            throw std::invalid_argument("Parameter is missing");
        }

        if (recorders.find(recorder_id) == recorders.end())
        {
            error_code = ERR_INVALID_RECORDER_ID;
            throw std::invalid_argument("Parameter is invalid");
        }

        error_code = recorders[recorder_id]->stop();
    }
    catch (const std::exception &e)
    {
        handleJsonException(e, error_code);
    }

    json resp;
    if (error_code == ERR_NONE)
    {
        resp["returnValue"] = true;
        resp["path"]        = recorders[recorder_id]->getRecordPath();
    }
    else
    {
        resp["returnValue"] = false;

        Error error       = ErrorManager::getInstance().getError(error_code);
        resp["errorCode"] = error.getCode();
        resp["errorText"] = error.getMessage();

        PLOGE("%d %s", error.getCode(), error.getMessage().c_str());
    }

    std::string respStr = to_string(resp);
    PLOGI("reply %s", respStr.c_str());

    LS::Message request(&message);
    request.respond(respStr.c_str());

    return true;
}

bool MediaRecorderManager::takeSnapshot(LSMessage &message)
{
    ErrorCode error_code = ERR_LIST_END;
    auto *payload        = LSMessageGetPayload(&message);
    PLOGI("payload %s", payload);

    int recorder_id = 0;

    try
    {
        json j = json::parse(payload);
        if (auto value = get_optional<int>(j, "recorderId"))
        {
            recorder_id = *value;
        }
        else
        {
            error_code = ERR_RECORDER_ID_NOT_SPECIFIED;
            throw std::invalid_argument("Parameter is missing");
        }

        if (recorders.find(recorder_id) == recorders.end())
        {
            error_code = ERR_INVALID_RECORDER_ID;
            throw std::invalid_argument("Parameter is invalid");
        }

        // File path
        std::string path;
        if (auto value = get_optional<std::string>(j, "path"))
        {
            path = *value;
        }
        else
        {
            error_code = ERR_PATH_NOT_SPECIFIED;
            throw std::invalid_argument("Parameter is missing");
        }

        // Image file format
        if (auto value = get_optional<std::string>(j, "format"))
        {
            error_code = recorders[recorder_id]->takeSnapshot(path, *value);
        }
        else
        {
            error_code = ERR_FORMAT_NOT_SPECIFIED;
            throw std::invalid_argument("Parameter is missing");
        }
    }
    catch (const std::exception &e)
    {
        handleJsonException(e, error_code);
    }

    json resp;
    if (error_code == ERR_NONE)
    {
        resp["returnValue"] = true;
        resp["path"]        = recorders[recorder_id]->getCapturePath();
    }
    else
    {
        resp["returnValue"] = false;

        Error error       = ErrorManager::getInstance().getError(error_code);
        resp["errorCode"] = error.getCode();
        resp["errorText"] = error.getMessage();

        PLOGE("%d %s", error.getCode(), error.getMessage().c_str());
    }

    std::string respStr = to_string(resp);
    PLOGI("reply %s", respStr.c_str());

    LS::Message request(&message);
    request.respond(respStr.c_str());

    return true;
}

bool MediaRecorderManager::pause(LSMessage &message)
{
    ErrorCode error_code = ERR_LIST_END;
    auto *payload        = LSMessageGetPayload(&message);
    PLOGI("payload %s", payload);

    try
    {
        json j = json::parse(payload);

        int recorder_id = 0;
        if (auto value = get_optional<int>(j, "recorderId"))
        {
            recorder_id = *value;
        }
        else
        {
            error_code = ERR_RECORDER_ID_NOT_SPECIFIED;
            throw std::invalid_argument("Parameter is missing");
        }

        if (recorders.find(recorder_id) == recorders.end())
        {
            error_code = ERR_INVALID_RECORDER_ID;
            throw std::invalid_argument("Parameter is invalid");
        }

        error_code = recorders[recorder_id]->pause();
    }
    catch (const std::exception &e)
    {
        handleJsonException(e, error_code);
    }

    json resp;
    if (error_code == ERR_NONE)
    {
        resp["returnValue"] = true;
    }
    else
    {
        resp["returnValue"] = false;

        Error error       = ErrorManager::getInstance().getError(error_code);
        resp["errorCode"] = error.getCode();
        resp["errorText"] = error.getMessage();

        PLOGE("%d %s", error.getCode(), error.getMessage().c_str());
    }

    std::string respStr = to_string(resp);
    PLOGI("reply %s", respStr.c_str());

    LS::Message request(&message);
    request.respond(respStr.c_str());

    return true;
}

bool MediaRecorderManager::resume(LSMessage &message)
{
    ErrorCode error_code = ERR_LIST_END;
    auto *payload        = LSMessageGetPayload(&message);
    PLOGI("payload %s", payload);

    try
    {
        json j = json::parse(payload);

        int recorder_id = 0;
        if (auto value = get_optional<int>(j, "recorderId"))
        {
            recorder_id = *value;
        }
        else
        {
            error_code = ERR_RECORDER_ID_NOT_SPECIFIED;
            throw std::invalid_argument("Parameter is missing");
        }

        if (recorders.find(recorder_id) == recorders.end())
        {
            error_code = ERR_INVALID_RECORDER_ID;
            throw std::invalid_argument("Parameter is invalid");
        }

        error_code = recorders[recorder_id]->resume();
    }
    catch (const std::exception &e)
    {
        handleJsonException(e, error_code);
    }

    json resp;
    if (error_code == ERR_NONE)
    {
        resp["returnValue"] = true;
    }
    else
    {
        resp["returnValue"] = false;

        Error error       = ErrorManager::getInstance().getError(error_code);
        resp["errorCode"] = error.getCode();
        resp["errorText"] = error.getMessage();

        PLOGE("%d %s", error.getCode(), error.getMessage().c_str());
    }

    std::string respStr = to_string(resp);
    PLOGI("reply %s", respStr.c_str());

    LS::Message request(&message);
    request.respond(respStr.c_str());

    return true;
}

void MediaRecorderManager::printRecorders()
{
    int index = 0;
    PLOGI("recorders size %zd", recorders.size());
    for (const auto &r : recorders)
    {
        PLOGI("[%d] recorder %d", index, r.first);
        index++;
    }
}

int main(int argc, char *argv[])
{
    try
    {
        MediaRecorderManager mediaRecordService;
    }
    catch (LS::Error &err)
    {
        LSErrorPrint(err, stdout);
        return 1;
    }
    catch (...)
    {
        std::cerr << "An unknown exception occurred." << std::endl;
        return 1;
    }

    return 0;
}
