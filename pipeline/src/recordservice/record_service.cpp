// Copyright (c) 2019-2023 LG Electronics, Inc.
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

#define LOG_TAG "RecordService"
#include "record_service.h"
#include "glog.h"
#include <pbnjson.hpp>
#include <string>

#include "base.h"
#include "camera_types.h"
#include "message.h"
#include "parser.h"
#include "pipeline_factory.h"
#include "record_pipeline.h"
#include "resourcefacilitator/requestor.h"
#include "serializer.h"

const char *const SUBSCRIPTION_KEY = "recordService";

RecordService::RecordService(const char *service_name)
    : LS::Handle(LS::registerService(service_name))
{
    LOGI("Start : %s", service_name);

    LS_CATEGORY_BEGIN(RecordService, "/")
    LS_CATEGORY_METHOD(start)
    LS_CATEGORY_METHOD(stop)
    LS_CATEGORY_METHOD(pause)
    LS_CATEGORY_METHOD(resume)
    LS_CATEGORY_METHOD(subscribe)
    LS_CATEGORY_END;

    // attach to mainloop and run it
    attachToLoop(main_loop_ptr_.get());

    // run the gmainloop
    g_main_loop_run(main_loop_ptr_.get());

    LOGI("end");
}

void RecordService::Notify(const gint notification, const gint64 numValue, const gchar *strValue,
                           void *payload)
{
    parser::Composer composer;
    base::media_info_t mediaInfo = {media_id_};
    switch (notification)
    {
    case GRP_NOTIFY_SOURCE_INFO:
    {
        base::source_info_t info = *static_cast<base::source_info_t *>(payload);
        composer.put("sourceInfo", info);
        break;
    }

    case GRP_NOTIFY_VIDEO_INFO:
    {
        base::video_info_t info = *static_cast<base::video_info_t *>(payload);
        composer.put("videoInfo", info);
        LOGI("videoInfo: width %d, height %d", info.width, info.height);
        break;
    }
    case GRP_NOTIFY_ERROR:
    {
        base::error_t error = *static_cast<base::error_t *>(payload);
        error.mediaId       = media_id_;
        composer.put("error", error);

        if (numValue == GRP_ERROR_RES_ALLOC)
        {
            LOGI("policy action occured!");
        }
        break;
    }
    case GRP_NOTIFY_LOAD_COMPLETED:
    {
        composer.put("loadCompleted", mediaInfo);
        break;
    }

    case GRP_NOTIFY_UNLOAD_COMPLETED:
    {
        composer.put("unloadCompleted", mediaInfo);
        LOGI("quit main loop");
        g_main_loop_quit(main_loop_ptr_.get());
        break;
    }

    case GRP_NOTIFY_END_OF_STREAM:
    {
        composer.put("endOfStream", mediaInfo);
        break;
    }

    case GRP_NOTIFY_PLAYING:
    {
        composer.put("playing", mediaInfo);
        break;
    }

    case GRP_NOTIFY_PAUSED:
    {
        composer.put("paused", mediaInfo);
        break;
    }
    case GRP_NOTIFY_ACTIVITY:
    {
        LOGI("notifyActivity to resource requestor");
        if (resourceRequestor_)
            resourceRequestor_->notifyActivity();
        break;
    }
    case GRP_NOTIFY_ACQUIRE_RESOURCE:
    {
        LOGI("Notify, GRP_NOTIFY_ACQUIRE_RESOURCE");
        ACQUIRE_RESOURCE_INFO_T *info = static_cast<ACQUIRE_RESOURCE_INFO_T *>(payload);
        info->result = AcquireResources(*(info->sourceInfo), info->displayMode, numValue);
        break;
    }
    default:
    {
        LOGI("This notification(%d) can't be handled here!", notification);
        break;
    }
    }

    if (!composer.result().empty())
    {
        LOGI("%s", composer.result().c_str());

        unsigned int num_subscribers =
            LSSubscriptionGetHandleSubscribersCount(this->get(), SUBSCRIPTION_KEY);
        if (num_subscribers > 0)
        {
            LOGI("num_subscribers = %u", num_subscribers);

            LSError lserror;
            LSErrorInit(&lserror);

            LOGI("notifying");
            if (!LSSubscriptionReply(this->get(), SUBSCRIPTION_KEY, composer.result().c_str(),
                                     &lserror))
            {
                LSErrorPrint(&lserror, stderr);
                LOGE("subscription reply failed");
            }
            LSErrorFree(&lserror);
        }
    }
}

bool RecordService::start(LSMessage &message)
{
    jvalue_ref json_outobj = jobject_create();
    auto *payload          = LSMessageGetPayload(&message);
    LOGI("payload %s", payload);

    pbnjson::JValue parsed = pbnjson::JDomParser::fromString(payload);

    app_id_            = "com.webos.app.mediaevents-test";
    media_id_          = "";
    resourceRequestor_ = std::make_unique<resource::ResourceRequestor>(app_id_, media_id_);

    recorder_ = PipelineFactory::CreateRecorder(parsed);

    if (!recorder_)
    {
        LOGE("Error: Player not created");
    }
    else
    {
        LoadCommon();

        if (recorder_->Load(parsed.stringify()))
        {
            LOGI("Loaded Player");
            isLoaded_ = true;
        }
        else
        {
            LOGE("Failed to load player");
        }
    }

    jobject_put(json_outobj, J_CSTR_TO_JVAL("returnValue"), jboolean_create(true));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));
    LOGI("response message : %s", jvalue_stringify(json_outobj));

    j_release(&json_outobj);

    return true;
}

bool RecordService::stop(LSMessage &message)
{
    bool ret               = false;
    jvalue_ref json_outobj = jobject_create();
    auto *payload          = LSMessageGetPayload(&message);
    LOGI("payload %s", payload);

    if (!isLoaded_)
    {
        LOGI("already unloaded");
        ret = true;
    }
    else
    {
        if (!recorder_ || !recorder_->Unload())
            LOGE("fails to unload the player");
        else
        {
            isLoaded_ = false;
            ret       = true;
            if (resourceRequestor_)
            {
                resourceRequestor_->releaseResource();
            }
            else
                LOGE("ReleaseResources fails");
        }
    }

    jobject_put(json_outobj, J_CSTR_TO_JVAL("returnValue"), jboolean_create(ret));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));
    LOGI("response message : %s", jvalue_stringify(json_outobj));

    j_release(&json_outobj);

    return true;
}

bool RecordService::pause(LSMessage &message)
{
    jvalue_ref json_outobj = jobject_create();
    auto *payload          = LSMessageGetPayload(&message);
    LOGI("payload %s", payload);

    if (!recorder_ || !isLoaded_)
    {
        LOGE("Invalid recorder state, recorder should be loaded");
        return false;
    }

    bool ret = recorder_->Pause();

    jobject_put(json_outobj, J_CSTR_TO_JVAL("returnValue"), jboolean_create(ret));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));
    LOGI("response message : %s", jvalue_stringify(json_outobj));

    j_release(&json_outobj);

    return true;
}

bool RecordService::resume(LSMessage &message)
{
    jvalue_ref json_outobj = jobject_create();
    auto *payload          = LSMessageGetPayload(&message);
    LOGI("payload %s", payload);

    if (!recorder_ || !isLoaded_)
    {
        LOGE("Invalid recorder state, recorder should be loaded");
        return false;
    }

    bool ret = recorder_->Play();

    jobject_put(json_outobj, J_CSTR_TO_JVAL("returnValue"), jboolean_create(ret));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));
    LOGI("response message : %s", jvalue_stringify(json_outobj));

    j_release(&json_outobj);

    return true;
}

bool RecordService::subscribe(LSMessage &message)
{
    LOGI("start");
    LSError error;
    LSErrorInit(&error);

    bool ret = LSSubscriptionAdd(this->get(), SUBSCRIPTION_KEY, &message, &error);
    LOGI("LSSubscriptionAdd %s", ret ? "ok" : "failed");
    LOGI("cnt %d", LSSubscriptionGetHandleSubscribersCount(this->get(), SUBSCRIPTION_KEY));
    LSErrorFree(&error);

    jvalue_ref json_outobj = jobject_create();
    jobject_put(json_outobj, J_CSTR_TO_JVAL("returnValue"), jboolean_create(ret));
    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));
    LOGI("response message : %s", jvalue_stringify(json_outobj));

    j_release(&json_outobj);

    return ret;
}

void RecordService::LoadCommon()
{
    recorder_->RegisterCbFunction(std::bind(&RecordService::Notify, this, std::placeholders::_1,
                                            std::placeholders::_2, std::placeholders::_3,
                                            std::placeholders::_4));

    if (resourceRequestor_)
    {
        resourceRequestor_->registerUMSPolicyActionCallback(
            [this]()
            {
                base::error_t error;
                error.errorCode = MEDIA_MSG_ERR_POLICY;
                error.errorText = "Policy Action";
                Notify(GRP_NOTIFY_ERROR, GRP_ERROR_RES_ALLOC, nullptr, static_cast<void *>(&error));
            });
    }
}

bool RecordService::AcquireResources(const base::source_info_t &sourceInfo,
                                     const std::string &display_mode, uint32_t display_path)
{
    LOGI("RecordService::AcquireResources");
    resource::PortResource_t resourceMMap;

    if (resourceRequestor_)
    {
        if (!resourceRequestor_->acquireResources(resourceMMap, sourceInfo, display_mode,
                                                  display_path))
        {
            LOGE("resource acquisition failed");
            return false;
        }

        for (const auto &it : resourceMMap)
        {
            LOGI("Resource::[%s]=>index:%d", it.first.c_str(), it.second);
        }
    }

    return true;
}

std::string parseRecordServiceName(int argc, char *argv[]) noexcept
{
    int c;
    std::string serviceName;

    while ((c = getopt(argc, argv, "s:")) != -1)
    {
        switch (c)
        {
        case 's':
            serviceName = optarg ? optarg : "";
            break;

        case '?':
            LOGE("unknown service name");
            break;

        default:
            break;
        }
    }
    if (serviceName.empty())
    {
        LOGE("service name is not specified");
    }
    return serviceName;
}

int main(int argc, char *argv[])
{
    LOGI("start");
    try
    {
        std::string serviceName = parseRecordServiceName(argc, argv);
        if (serviceName.empty())
        {
            return 1;
        }
        RecordService RecordServiceInstance(serviceName.c_str());
    }
    catch (...)
    {
        LOGE("An exception occurred.");
        return 1;
    }
    LOGI("end");
    return 0;
}
