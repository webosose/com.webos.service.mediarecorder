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

#ifndef __MEDIA_RECORDER__
#define __MEDIA_RECORDER__

#include "error.h"
#include "luna-service2/lunaservice.hpp"
#include <memory>
#include <vector>

class LunaClient;
class MediaRecorder
{
    enum State
    {
        CLOSE,
        OPEN,
        PREPARED,
        RECORDING,
    };

    int recorderId = 0;
    std::string mPath;
    std::string mFormat;
    std::string videoSrc;
    std::string audioSrc;
    State state = CLOSE;

    std::unique_ptr<LunaClient> luna_client{nullptr};
    GMainLoop *loop_{nullptr};
    std::unique_ptr<std::thread> loopThread_;

public:
    MediaRecorder();
    ~MediaRecorder();

    ErrorCode open(std::string &video_src, std::string &audio_src);
    ErrorCode setOutputFile(std::string &path);
    ErrorCode setOutputFormat(std::string &format);
    ErrorCode start();
    ErrorCode stop();
    ErrorCode takeSnapshot(std::string &path, std::string &format);
    ErrorCode close();

    int getRecorderId() { return recorderId; }
};

#endif // __MEDIA_RECORDER__
