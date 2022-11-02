// Copyright (c) 2020 LG Electronics, Inc.
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

#include <media_recorder_service.h>

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


void MediaRecorderService::Notify(const gint notification, const gint64 numValue,
        const gchar *strValue, void *payload)
{
    switch (notification)
    {
        case CMP_NOTIFY_SOURCE_INFO:
        {
            PMLOG_DEBUG(CONST_MODULE_MEDIA_RECORDER,"sourceInfo\n");
            break;
        }
        case CMP_NOTIFY_ERROR:
        {
            PMLOG_DEBUG(CONST_MODULE_MEDIA_RECORDER,"error\n");
            break;
        }
        case CMP_NOTIFY_LOAD_COMPLETED:
        {
            PMLOG_DEBUG(CONST_MODULE_MEDIA_RECORDER,"loadCompleted\n");
            break;
        }

        case CMP_NOTIFY_UNLOAD_COMPLETED:
        {
            PMLOG_DEBUG(CONST_MODULE_MEDIA_RECORDER,"unloadCompleted\n");
            break;
        }

        case CMP_NOTIFY_END_OF_STREAM:
        {
            PMLOG_DEBUG(CONST_MODULE_MEDIA_RECORDER,"endOfStream\n");
            break;
        }

        case CMP_NOTIFY_PLAYING:
        {
            PMLOG_DEBUG(CONST_MODULE_MEDIA_RECORDER,"playing\n");
            break;
        }

        case CMP_NOTIFY_PAUSED:
        {
            PMLOG_DEBUG(CONST_MODULE_MEDIA_RECORDER,"paused\n");
            break;
        }
        case CMP_NOTIFY_ACTIVITY: {
            PMLOG_DEBUG(CONST_MODULE_MEDIA_RECORDER,"notifyActivity to resource requestor\n");
            if (media_recorder_client_)
                media_recorder_client_->NotifyActivity();
            break;
        }
        case CMP_NOTIFY_ACQUIRE_RESOURCE: {
            PMLOG_DEBUG(CONST_MODULE_MEDIA_RECORDER,"acquire_resource\n");
            ACQUIRE_RESOURCE_INFO_T* info = static_cast<ACQUIRE_RESOURCE_INFO_T*>(payload);
            info->result = false;
            if (media_recorder_client_)
                info->result = media_recorder_client_->AcquireResources(*(info->sourceInfo), info->displayMode, numValue);
            break;
        }
        default:
        {
            PMLOG_DEBUG(CONST_MODULE_MEDIA_RECORDER,"This notification(%d) can't be handled here!\n", notification);
            break;
        }
    }
}

bool MediaRecorderService::load(LSMessage &message)
{
  bool ret = false;
  auto *payload = LSMessageGetPayload(&message);
  PMLOG_INFO(CONST_MODULE_MEDIA_RECORDER, "payload %s", payload);

  pbnjson::JValue reply = pbnjson::Object();
  if (reply.isNull())
      return false;

  media_recorder_client_ = std::make_unique<cmp::pipeline::MediaRecorderClient>();

  media_recorder_client_->RegisterCallback(
      std::bind(&MediaRecorderService::Notify, this,
      std::placeholders::_1, std::placeholders::_2,
      std::placeholders::_3, std::placeholders::_4));

  ret = media_recorder_client_->Load(payload);

  reply.put("returnValue", ret);

  LS::Message request(&message);
  request.respond(reply.stringify().c_str());

  return true;

}

bool MediaRecorderService::unload(LSMessage &message)
{

  bool ret = false;
  auto *payload = LSMessageGetPayload(&message);
  PMLOG_INFO(CONST_MODULE_MEDIA_RECORDER, "payload %s", payload);

  pbnjson::JValue reply = pbnjson::Object();
  if (reply.isNull())
      return false;

  if (media_recorder_client_)
    ret = media_recorder_client_->Unload();

  reply.put("returnValue", ret);

  LS::Message request(&message);
  request.respond(reply.stringify().c_str());

  return true;


}

bool MediaRecorderService::play(LSMessage &message)
{

  bool ret = false;
  auto *payload = LSMessageGetPayload(&message);
  PMLOG_INFO(CONST_MODULE_MEDIA_RECORDER, "payload %s", payload);

  pbnjson::JValue reply = pbnjson::Object();
  if (reply.isNull())
      return false;

  if (media_recorder_client_)
    ret = media_recorder_client_->Play();

  reply.put("returnValue", ret);

  LS::Message request(&message);
  request.respond(reply.stringify().c_str());

  return true;


}

bool MediaRecorderService::startRecord(LSMessage &message)
{
  bool ret = false;
  auto *payload = LSMessageGetPayload(&message);
  PMLOG_INFO(CONST_MODULE_MEDIA_RECORDER, "payload %s", payload);

  std::string location;
  std::string format;
  bool audio;
  std::string audioSrc;

  pbnjson::JValue parsed = pbnjson::JDomParser::fromString(payload);

  if(parsed.hasKey("location")) {
      location = parsed["location"].asString().c_str();
  }
  else
  {
    location = "/media/internal/";
  }

  if(parsed.hasKey("format")) {
      format = parsed["format"].asString().c_str();
  }
  else
  {
    format = "MP4";
  }

  if(parsed.hasKey("audio")) {
      parsed["audio"].asBool(audio);
  }
  else
  {
    audio = false;
  }

  if(parsed.hasKey("audioSrc")) {
      audioSrc = parsed["audioSrc"].asString().c_str();
  }
  else
  {
    audioSrc = "pcm_input";
  }

  if (media_recorder_client_)
  {
    ret = media_recorder_client_->StartRecord(location, format, audio, audioSrc);
  }

  pbnjson::JValue reply = pbnjson::Object();
  if (reply.isNull())
      return false;

  reply.put("returnValue", ret);
  if(ret == true){
    reply.put("location", location);
    reply.put("format", format);
    reply.put("audio", audio);
    reply.put("audioSrc", audioSrc);
  }
  LS::Message request(&message);
  request.respond(reply.stringify().c_str());

  return true;

}

bool MediaRecorderService::stopRecord(LSMessage &message)
{
  bool ret = false;
  auto *payload = LSMessageGetPayload(&message);
  PMLOG_INFO(CONST_MODULE_MEDIA_RECORDER, "payload %s", payload);

  pbnjson::JValue reply = pbnjson::Object();
  if (reply.isNull())
      return false;

  if (media_recorder_client_)
  {
    ret = media_recorder_client_->StopRecord();
  }

  reply.put("returnValue", ret);

  LS::Message request(&message);
  request.respond(reply.stringify().c_str());

  return true;


}

bool MediaRecorderService::takeSnapshot(LSMessage &message)
{
  bool ret = false;
  auto *payload = LSMessageGetPayload(&message);
  PMLOG_INFO(CONST_MODULE_MEDIA_RECORDER, "payload %s", payload);

  std::string location;

  pbnjson::JValue parsed = pbnjson::JDomParser::fromString(payload);

  if(parsed.hasKey("location")) {
      location = parsed["location"].asString().c_str();
  }
  else
  {
    location = "/media/internal/";
  }


  if (media_recorder_client_)
  {
    ret = media_recorder_client_->TakeSnapshot(location);
  }

  pbnjson::JValue reply = pbnjson::Object();
  if (reply.isNull())
      return false;

  reply.put("returnValue", ret);
  if(ret == true){
    reply.put("location", location);
  }

  LS::Message request(&message);
  request.respond(reply.stringify().c_str());

  return true;


}

int main(int argc, char* argv[])
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

