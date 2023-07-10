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
#include "log.h"
#include <nlohmann/json.hpp>
#include <string>

using namespace nlohmann;

const std::string service = "com.webos.service.mediarecorder";

MediaRecorderManager::MediaRecorderManager() : LS::Handle(LS::registerService(service.c_str()))
{
    LS_CATEGORY_BEGIN(MediaRecorderManager, "/")
    LS_CATEGORY_METHOD(open)
    LS_CATEGORY_METHOD(close)
    LS_CATEGORY_METHOD(setOutputFile)
    LS_CATEGORY_METHOD(setOutputFormat)
    LS_CATEGORY_METHOD(start)
    LS_CATEGORY_METHOD(stop)
    LS_CATEGORY_METHOD(takeSnapshot)
    LS_CATEGORY_END;

    // attach to mainloop and run it
    attachToLoop(main_loop_ptr_.get());

    // run the gmainloop
    g_main_loop_run(main_loop_ptr_.get());
}

bool MediaRecorderManager::open(LSMessage &message)
{
    bool ret      = false;
    auto *payload = LSMessageGetPayload(&message);
    PLOGI("payload %s", payload);

    json resp;
    resp["returnValue"] = ret;

    LS::Message request(&message);
    request.respond(to_string(resp).c_str());

    return true;
}

bool MediaRecorderManager::close(LSMessage &message)
{
    bool ret      = false;
    auto *payload = LSMessageGetPayload(&message);
    PLOGI("payload %s", payload);

    json resp;
    resp["returnValue"] = ret;

    LS::Message request(&message);
    request.respond(to_string(resp).c_str());

    return true;
}

bool MediaRecorderManager::setOutputFile(LSMessage &message)
{
    bool ret      = false;
    auto *payload = LSMessageGetPayload(&message);
    PLOGI("payload %s", payload);

    json resp;
    resp["returnValue"] = ret;

    LS::Message request(&message);
    request.respond(to_string(resp).c_str());

    return true;
}

bool MediaRecorderManager::setOutputFormat(LSMessage &message)
{
    bool ret      = false;
    auto *payload = LSMessageGetPayload(&message);
    PLOGI("payload %s", payload);

    json resp;
    resp["returnValue"] = ret;

    LS::Message request(&message);
    request.respond(to_string(resp).c_str());

    return true;
}

bool MediaRecorderManager::start(LSMessage &message)
{
    bool ret      = false;
    auto *payload = LSMessageGetPayload(&message);
    PLOGI("payload %s", payload);

    json resp;
    resp["returnValue"] = ret;

    LS::Message request(&message);
    request.respond(to_string(resp).c_str());

    return true;
}

bool MediaRecorderManager::stop(LSMessage &message)
{
    bool ret      = false;
    auto *payload = LSMessageGetPayload(&message);
    PLOGI("payload %s", payload);

    json resp;
    resp["returnValue"] = ret;

    LS::Message request(&message);
    request.respond(to_string(resp).c_str());

    return true;
}

bool MediaRecorderManager::takeSnapshot(LSMessage &message)
{
    bool ret      = false;
    auto *payload = LSMessageGetPayload(&message);
    PLOGI("payload %s", payload);

    json resp;
    resp["returnValue"] = ret;

    LS::Message request(&message);
    request.respond(to_string(resp).c_str());

    return true;
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
    return 0;
}
