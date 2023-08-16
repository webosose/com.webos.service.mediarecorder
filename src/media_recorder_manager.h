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

#ifndef __MEDIA_RECORDER_MANAGER__
#define __MEDIA_RECORDER_MANAGER__

#include "luna-service2/lunaservice.hpp"
#include <glib.h>
#include <map>
#include <memory>

class MediaRecorder;
class MediaRecorderManager : public LS::Handle
{
    using mainloop          = std::unique_ptr<GMainLoop, void (*)(GMainLoop *)>;
    mainloop main_loop_ptr_ = {g_main_loop_new(nullptr, false), g_main_loop_unref};

    std::map<int, std::unique_ptr<MediaRecorder>> recorders;

public:
    MediaRecorderManager();

    MediaRecorderManager(MediaRecorderManager const &)            = delete;
    MediaRecorderManager(MediaRecorderManager &&)                 = delete;
    MediaRecorderManager &operator=(MediaRecorderManager const &) = delete;
    MediaRecorderManager &operator=(MediaRecorderManager &&)      = delete;

    bool open(LSMessage &message);
    bool close(LSMessage &message);
    bool setOutputFile(LSMessage &message);
    bool setOutputFormat(LSMessage &message);
    bool start(LSMessage &message);
    bool stop(LSMessage &message);
    bool takeSnapshot(LSMessage &message);

    void printRecorders(); //[TODO] Remove this for debugging purpose.
};

#endif // __MEDIA_RECORDER_MANAGER__
