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

#define LOG_TAG "MediaRecorderService"
#include "media_recorder_service.h"
#include "log.h"
#include <nlohmann/json.hpp>
#include <string>

using namespace nlohmann;

const std::string service = "com.webos.service.mediarecorder";

MediaRecorderService::MediaRecorderService() : LS::Handle(LS::registerService(service.c_str()))
{
    LS_CATEGORY_BEGIN(MediaRecorderService, "/")
    LS_CATEGORY_METHOD(load)
    LS_CATEGORY_METHOD(unload)
    LS_CATEGORY_METHOD(play)
    LS_CATEGORY_METHOD(startRecord)
    LS_CATEGORY_METHOD(stopRecord)
    LS_CATEGORY_METHOD(takeSnapshot)
    LS_CATEGORY_END;

    // attach to mainloop and run it
    attachToLoop(main_loop_ptr_.get());

    // run the gmainloop
    g_main_loop_run(main_loop_ptr_.get());
}

bool MediaRecorderService::load(LSMessage &message)
{
    bool ret      = false;
    auto *payload = LSMessageGetPayload(&message);
    PLOGI("payload %s", payload);

    json jin;
    jin["returnValue"] = ret;

    LS::Message request(&message);
    request.respond(to_string(jin).c_str());

    return true;
}

bool MediaRecorderService::unload(LSMessage &message)
{
    bool ret      = false;
    auto *payload = LSMessageGetPayload(&message);
    PLOGI("payload %s", payload);

    json jin;
    jin["returnValue"] = ret;

    LS::Message request(&message);
    request.respond(to_string(jin).c_str());

    return true;
}

bool MediaRecorderService::play(LSMessage &message)
{
    bool ret      = false;
    auto *payload = LSMessageGetPayload(&message);
    PLOGI("payload %s", payload);

    json jin;
    jin["returnValue"] = ret;

    LS::Message request(&message);
    request.respond(to_string(jin).c_str());

    return true;
}

bool MediaRecorderService::startRecord(LSMessage &message)
{
    bool ret      = false;
    auto *payload = LSMessageGetPayload(&message);
    PLOGI("payload %s", payload);

    json jin;
    jin["returnValue"] = ret;

    LS::Message request(&message);
    request.respond(to_string(jin).c_str());

    return true;
}

bool MediaRecorderService::stopRecord(LSMessage &message)
{
    bool ret      = false;
    auto *payload = LSMessageGetPayload(&message);
    PLOGI("payload %s", payload);

    json jin;
    jin["returnValue"] = ret;

    LS::Message request(&message);
    request.respond(to_string(jin).c_str());

    return true;
}

bool MediaRecorderService::takeSnapshot(LSMessage &message)
{
    bool ret      = false;
    auto *payload = LSMessageGetPayload(&message);
    PLOGI("payload %s", payload);

    json jin;
    jin["returnValue"] = ret;

    LS::Message request(&message);
    request.respond(to_string(jin).c_str());

    return true;
}

int main(int argc, char *argv[])
{
    try
    {
        MediaRecorderService mediaRecordService;
    }
    catch (LS::Error &err)
    {
        LSErrorPrint(err, stdout);
        return 1;
    }
    return 0;
}
