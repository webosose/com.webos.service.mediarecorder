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
#include "format_utils.h"
#include <memory>
#include <vector>

class LSConnector;
class MediaRecorder
{
    enum State
    {
        CLOSE,
        OPEN,
        RECORDING,
        PAUSE
    };

    int recorderId = 0;
    std::string mPath;
    std::string mFormat;
    std::string videoSrc;
    std::string audioSrc;
    State state = CLOSE;

    std::unique_ptr<LSConnector> record_client{nullptr};
    std::unique_ptr<LSConnector> snapshot_client{nullptr};

    video_format_t mVideoFormat;
    audio_format_t mAudioFormat;
    std::string mMediaId;

public:
    MediaRecorder();
    ~MediaRecorder();

    ErrorCode open(std::string &video_src, std::string &audio_src);
    ErrorCode setOutputFile(std::string &path);
    ErrorCode setOutputFormat(std::string &format);
    ErrorCode setVideoFormat(std::string &videoCodec, unsigned int width, unsigned int height,
                             unsigned int fps, unsigned int bitRate);
    ErrorCode setAudioFormat(std::string &audioCodec, unsigned int sampleRate,
                             unsigned int channels, unsigned int bitRate);
    ErrorCode start();
    ErrorCode stop();
    ErrorCode takeSnapshot(std::string &path, std::string &format);
    ErrorCode close();
    ErrorCode pause();
    ErrorCode resume();

    int getRecorderId() { return recorderId; }
};

#endif // __MEDIA_RECORDER__
