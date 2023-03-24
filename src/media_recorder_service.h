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

#include <glib.h>
#include <string>
#include "luna-service2/lunaservice.hpp"
#include <PmLog.h>
#include "PmLogLib.h"
#include <pbnjson.hpp>

#define CONST_MODULE_MEDIA_RECORDER "MediaRecorder"

#define PMLOG_ERROR(module, args...) PmLogMsg(getCameraLunaPmLogContext(), Error, module, 0, ##args)
#define PMLOG_INFO(module, FORMAT__, ...) \
  PmLogInfo(getCameraLunaPmLogContext(), \
  module, 0, "%s():%d " FORMAT__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define PMLOG_DEBUG(FORMAT__, ...) \
  PmLogDebug(getCameraLunaPmLogContext(), \
  "[%s:%d]" FORMAT__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__)

static inline PmLogContext getCameraLunaPmLogContext()
{
  static PmLogContext usLogContext = 0;
  if (0 == usLogContext)
  {
    PmLogGetContext("camera", &usLogContext);
  }
  return usLogContext;
}

class MediaRecorderService : public LS::Handle
{
private:
using mainloop = std::unique_ptr<GMainLoop, void (*)(GMainLoop *)>;
mainloop main_loop_ptr_ = {g_main_loop_new(nullptr, false), g_main_loop_unref};

public:
  MediaRecorderService();

  MediaRecorderService(MediaRecorderService const &) = delete;
  MediaRecorderService(MediaRecorderService &&) = delete;
  MediaRecorderService &operator=(MediaRecorderService const &) = delete;
  MediaRecorderService &operator=(MediaRecorderService &&) = delete;

  bool load(LSMessage &message);
  bool unload(LSMessage &message);
  bool play(LSMessage &message);
  bool startRecord(LSMessage &message);
  bool stopRecord(LSMessage &message);
  bool takeSnapshot(LSMessage &message);

};



