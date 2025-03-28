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

#ifndef MEDIA_RECORDER_
#define MEDIA_RECORDER_

#include "error.h"
#include "format_utils.h"
#include <memory>
#include <vector>

class LSConnector;
class Process;
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
    std::string mRecordBasePath;
    std::string mRecordPath;
    std::string mCapturePath;
    std::string mFormat;
    std::string videoSrc;
    bool audioSrc = false;
    State state   = CLOSE;

    std::unique_ptr<LSConnector> record_client{nullptr};
    std::unique_ptr<LSConnector> snapshot_client{nullptr};
    std::unique_ptr<Process> record_process{nullptr};
    std::string record_uri;

    video_format_t mVideoFormat{
        "H264", 1280, 720, 30,
        0}; // default vidoe format (video codec, width, height, fps, bitRate)

    audio_format_t mAudioFormat;
    std::string mMediaId;
    bool mEos{false};

    bool isSupportedExtension(const std::string &) const;
    std::string createRecordFileName(const std::string &, const std::string &) const;
    bool getCameraFormat();

public:
    MediaRecorder();
    ~MediaRecorder();

    ErrorCode open(std::string &video_src, bool audio_src);
    ErrorCode setOutputFile(std::string &path);
    ErrorCode setOutputFormat(std::string &format);
    ErrorCode setVideoFormat(std::string &videoCodec, unsigned int bitRate);
    ErrorCode setAudioFormat(std::string &audioCodec, unsigned int sampleRate,
                             unsigned int channels, unsigned int bitRate);
    ErrorCode start();
    ErrorCode stop();
    ErrorCode takeSnapshot(std::string &path, std::string &format);
    ErrorCode close();
    ErrorCode pause();
    ErrorCode resume();

    int getRecorderId() { return recorderId; }
    std::string &getRecordPath() { return mRecordPath; }
    std::string &getCapturePath() { return mCapturePath; }
    bool snapshotCb(const char *message);

    audio_format_t const mAudioFormatDefault = {
        "AAC", 44100, 2, 0}; // default audio format (audio codec, sampleRate, channels, bitRate)
};

#endif // MEDIA_RECORDER_
