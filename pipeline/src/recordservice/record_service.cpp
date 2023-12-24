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

#include "record_service.h"
#include "UMSConnector.h"
#include "base.h"
#include "camera_types.h"
#include "glog.h"
#include "message.h"
#include "parser.h"
#include "pipeline_factory.h"
#include "record_pipeline.h"
#include "resourcefacilitator/requestor.h"
#include "serializer.h"

RecordService *RecordService::instance_ = nullptr;

RecordService::RecordService(const char *service_name)
    : media_id_(""), app_id_(""), umc_(nullptr), recorder_(nullptr), resourceRequestor_(nullptr),
      isLoaded_(false)
{
    LOGI(" this[%p]", this);

    umc_ =
        std::make_unique<UMSConnector>(service_name, nullptr, nullptr, UMS_CONNECTOR_PRIVATE_BUS);

    static UMSConnectorEventHandler event_handlers[] = {
        // uMediaserver public API
        {"load", RecordService::LoadEvent},
        {"attach", RecordService::AttachEvent},
        {"unload", RecordService::UnloadEvent},

        // media operations
        {"play", RecordService::PlayEvent},
        {"pause", RecordService::PauseEvent},
        {"stateChange", RecordService::StateChangeEvent},
        {"unsubscribe", RecordService::UnsubscribeEvent},
        {"setUri", RecordService::SetUriEvent},
        {"setPlane", RecordService::SetPlaneEvent},

        // Resource Manager API
        {"getPipelineState", RecordService::GetPipelineStateEvent},
        {"logPipelineState", RecordService::LogPipelineStateEvent},
        {"getActivePipelines", RecordService::GetActivePipelinesEvent},
        {"setPipelineDebugState", RecordService::SetPipelineDebugStateEvent},

        // exit
        {"exit", RecordService::ExitEvent},
        {NULL, NULL}};

    umc_->addEventHandlers(reinterpret_cast<UMSConnectorEventHandler *>(event_handlers));
}

RecordService *RecordService::GetInstance(const char *service_name)
{
    if (!instance_)
        instance_ = new RecordService(service_name);
    return instance_;
}

RecordService::~RecordService()
{
    if (isLoaded_)
    {
        LOGI("Unload() should be called if it is still loaded");
        recorder_->Unload();
    }
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
        umc_->sendChangeNotificationJsonString(composer.result());
}

bool RecordService::Wait() { return umc_->wait(); }

bool RecordService::Stop() { return umc_->stop(); }

// uMediaserver public API
bool RecordService::LoadEvent(UMSConnectorHandle *handle, UMSConnectorMessage *message, void *ctxt)
{
    std::string msg = instance_->umc_->getMessageText(message);
    LOGI("message : %s", msg.c_str());

    pbnjson::JDomParser jsonparser;
    if (!jsonparser.parse(msg, pbnjson::JSchema::AllSchema()))
    {
        LOGE("ERROR : JDomParser.parse Failed!!!");
        return false;
    }

    pbnjson::JValue parsed = jsonparser.getDom();
    if (!parsed.hasKey("id") && parsed["id"].isString())
    {
        LOGE("id is invalid");
        return false;
    }

    instance_->media_id_ = parsed["id"].asString();
    instance_->app_id_   = parsed["options"]["option"]["appId"].asString();

    LOGI("media_id_ : %s", instance_->media_id_.c_str());
    LOGI("app_id_ : %s", instance_->app_id_.c_str());

    if (instance_->app_id_.empty())
    {
        LOGW("appId is empty! resourceRequestor is not created");
        instance_->app_id_ = "EmptyAppId_" + instance_->media_id_;
    }
    else
        instance_->resourceRequestor_ =
            std::make_unique<resource::ResourceRequestor>(instance_->app_id_, instance_->media_id_);

    instance_->recorder_ = PipelineFactory::CreateRecorder(parsed);

    if (!instance_->recorder_)
    {
        LOGE("Error: Player not created");
    }
    else
    {
        instance_->LoadCommon();

        if (instance_->recorder_->Load(msg))
        {
            LOGI("Loaded Player");
            instance_->isLoaded_ = true;
            return true;
        }
        else
        {
            LOGE("Failed to load player");
        }
    }

    base::error_t error;
    error.errorCode = MEDIA_MSG_ERR_LOAD;
    error.errorText = "Load Failed";
    instance_->Notify(GRP_NOTIFY_ERROR, 0, nullptr, static_cast<void *>(&error));

    return false;
}

bool RecordService::AttachEvent(UMSConnectorHandle *handle, UMSConnectorMessage *message,
                                void *ctxt)
{
    LOGI("AttachEvent");
    return true;
}

bool RecordService::UnloadEvent(UMSConnectorHandle *handle, UMSConnectorMessage *message,
                                void *ctxt)
{
    bool ret = false;
    base::error_t error;
    std::string msg = instance_->umc_->getMessageText(message);
    LOGI("%s", msg.c_str());

    if (!instance_->isLoaded_)
    {
        LOGI("already unloaded");
        ret = true;
    }
    else
    {
        if (!instance_->recorder_ || !instance_->recorder_->Unload())
            LOGE("fails to unload the player");
        else
        {
            instance_->isLoaded_ = false;
            ret                  = true;
            if (instance_->resourceRequestor_)
            {
                instance_->resourceRequestor_->notifyBackground();
                instance_->resourceRequestor_->releaseResource();
            }
            else
                LOGE("NotifyBackground & ReleaseResources fails");
        }
    }

    if (!ret)
    {
        base::error_t error;
        error.errorCode = MEDIA_MSG_ERR_LOAD;
        error.errorText = "Unload Failed";
        error.mediaId   = instance_->media_id_;
        instance_->Notify(GRP_NOTIFY_ERROR, 0, nullptr, static_cast<void *>(&error));
    }

    instance_->recorder_.reset();
    instance_->Notify(GRP_NOTIFY_UNLOAD_COMPLETED, 0, nullptr, nullptr);

    LOGI("UnloadEvent Done");
    return ret;
}

// media operations
bool RecordService::PlayEvent(UMSConnectorHandle *handle, UMSConnectorMessage *message, void *ctxt)
{
    std::string msg = instance_->umc_->getMessageText(message);
    LOGI("message : %s", msg.c_str());

    if (!instance_->recorder_ || !instance_->isLoaded_)
    {
        LOGE("Invalid recorder state, recorder should be loaded");
        return false;
    }

    return instance_->recorder_->Play();
}

bool RecordService::PauseEvent(UMSConnectorHandle *handle, UMSConnectorMessage *message, void *ctxt)
{
    std::string msg = instance_->umc_->getMessageText(message);
    LOGI("message : %s", msg.c_str());

    if (!instance_->recorder_ || !instance_->isLoaded_)
    {
        LOGE("Invalid recorder state, recorder should be loaded");
        return false;
    }

    return instance_->recorder_->Pause();
}

bool RecordService::StateChangeEvent(UMSConnectorHandle *handle, UMSConnectorMessage *message,
                                     void *ctxt)
{
    return instance_->umc_->addSubscriber(handle, message);
}

bool RecordService::UnsubscribeEvent(UMSConnectorHandle *handle, UMSConnectorMessage *message,
                                     void *ctxt)
{
    return true;
}

bool RecordService::SetUriEvent(UMSConnectorHandle *handle, UMSConnectorMessage *message,
                                void *ctxt)
{
    return true;
}

bool RecordService::SetPlaneEvent(UMSConnectorHandle *handle, UMSConnectorMessage *message,
                                  void *ctxt)
{
    return true;
}

// Resource Manager API
bool RecordService::GetPipelineStateEvent(UMSConnectorHandle *handle, UMSConnectorMessage *message,
                                          void *ctxt)
{
    return true;
}

bool RecordService::LogPipelineStateEvent(UMSConnectorHandle *handle, UMSConnectorMessage *message,
                                          void *ctxt)
{
    return true;
}

bool RecordService::GetActivePipelinesEvent(UMSConnectorHandle *handle,
                                            UMSConnectorMessage *message, void *ctxt)
{
    return true;
}

bool RecordService::SetPipelineDebugStateEvent(UMSConnectorHandle *handle,
                                               UMSConnectorMessage *message, void *ctxt)
{
    return true;
}
// exit
bool RecordService::ExitEvent(UMSConnectorHandle *handle, UMSConnectorMessage *message, void *ctxt)
{
    return instance_->umc_->stop();
}

void RecordService::LoadCommon()
{
    if (!resourceRequestor_)
        LOGE("NotifyForeground fails");
    else
        resourceRequestor_->notifyForeground();

    recorder_->RegisterCbFunction(std::bind(&RecordService::Notify, instance_,
                                            std::placeholders::_1, std::placeholders::_2,
                                            std::placeholders::_3, std::placeholders::_4));

    if (resourceRequestor_)
    {
        resourceRequestor_->registerUMSPolicyActionCallback(
            [this]()
            {
                base::error_t error;
                error.errorCode = MEDIA_MSG_ERR_POLICY;
                error.errorText = "Policy Action";
                Notify(GRP_NOTIFY_ERROR, GRP_ERROR_RES_ALLOC, nullptr, static_cast<void *>(&error));
                if (!resourceRequestor_)
                    LOGE("notifyBackground fails");
                else
                    resourceRequestor_->notifyBackground();
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
