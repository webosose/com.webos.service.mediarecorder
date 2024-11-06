// Copyright (c) 2024 LG Electronics, Inc.
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

#ifndef BUFFER_ENCODER_H_
#define BUFFER_ENCODER_H_

#define LOG_TAG "BufferEncoder"
#include "log.h"
#include <chrono>
#include <functional>
#include <gst/gst.h>

using namespace std;

namespace mrf
{

/**
 * Video Codec Type enum
 */
enum VideoCodec
{
    VIDEO_CODEC_NONE        = 0,
    VIDEO_CODEC_H264        = 1,
    VIDEO_CODEC_VC1         = 2,
    VIDEO_CODEC_MPEG2       = 3,
    VIDEO_CODEC_MPEG4       = 4,
    VIDEO_CODEC_THEORA      = 5,
    VIDEO_CODEC_VP8         = 6,
    VIDEO_CODEC_VP9         = 7,
    VIDEO_CODEC_HEVC        = 8,
    VIDEO_CODEC_DOLBYVISION = 9,
    VIDEO_CODEC_AV1         = 10,
    VIDEO_CODEC_MAX         = VIDEO_CODEC_AV1,
};

/**
 * Video Pixel Format enum
 */
enum VideoPixelFormat
{
    PIXEL_FORMAT_UNKNOWN   = 0,
    PIXEL_FORMAT_I420      = 1,
    PIXEL_FORMAT_YV12      = 2,
    PIXEL_FORMAT_I422      = 3,
    PIXEL_FORMAT_I420A     = 4,
    PIXEL_FORMAT_I444      = 5,
    PIXEL_FORMAT_NV12      = 6,
    PIXEL_FORMAT_NV21      = 7,
    PIXEL_FORMAT_UYVY      = 8,
    PIXEL_FORMAT_YUY2      = 9,
    PIXEL_FORMAT_ARGB      = 10,
    PIXEL_FORMAT_XRGB      = 11,
    PIXEL_FORMAT_RGB24     = 12,
    PIXEL_FORMAT_MJPEG     = 14,
    PIXEL_FORMAT_YUV420P9  = 16,
    PIXEL_FORMAT_YUV420P10 = 17,
    PIXEL_FORMAT_YUV422P9  = 18,
    PIXEL_FORMAT_YUV422P10 = 19,
    PIXEL_FORMAT_YUV444P9  = 20,
    PIXEL_FORMAT_YUV444P10 = 21,
    PIXEL_FORMAT_YUV420P12 = 22,
    PIXEL_FORMAT_YUV422P12 = 23,
    PIXEL_FORMAT_YUV444P12 = 24,
    PIXEL_FORMAT_Y16       = 26,
    PIXEL_FORMAT_ABGR      = 27,
    PIXEL_FORMAT_XBGR      = 28,
    PIXEL_FORMAT_P016LE    = 29,
    PIXEL_FORMAT_XR30      = 30,
    PIXEL_FORMAT_XB30      = 31,
    PIXEL_FORMAT_BGRA      = 32,
    PIXEL_FORMAT_RGBAF16   = 33,
    PIXEL_FORMAT_MAX       = PIXEL_FORMAT_RGBAF16,
};

/**
 * Video Codec Profile enum
 */
enum VideoCodecProfile
{
    VIDEO_CODEC_PROFILE_UNKNOWN          = -1,
    VIDEO_CODEC_PROFILE_MIN              = VIDEO_CODEC_PROFILE_UNKNOWN,
    H264PROFILE_MIN                      = 0,
    H264PROFILE_BASELINE                 = H264PROFILE_MIN,
    H264PROFILE_MAIN                     = 1,
    H264PROFILE_EXTENDED                 = 2,
    H264PROFILE_HIGH                     = 3,
    H264PROFILE_HIGH10PROFILE            = 4,
    H264PROFILE_HIGH422PROFILE           = 5,
    H264PROFILE_HIGH444PREDICTIVEPROFILE = 6,
    H264PROFILE_SCALABLEBASELINE         = 7,
    H264PROFILE_SCALABLEHIGH             = 8,
    H264PROFILE_STEREOHIGH               = 9,
    H264PROFILE_MULTIVIEWHIGH            = 10,
    H264PROFILE_MAX                      = H264PROFILE_MULTIVIEWHIGH,
    VP8PROFILE_MIN                       = 11,
    VP8PROFILE_ANY                       = VP8PROFILE_MIN,
    VP8PROFILE_MAX                       = VP8PROFILE_ANY,
    VP9PROFILE_MIN                       = 12,
    VP9PROFILE_PROFILE0                  = VP9PROFILE_MIN,
    VP9PROFILE_PROFILE1                  = 13,
    VP9PROFILE_PROFILE2                  = 14,
    VP9PROFILE_PROFILE3                  = 15,
    VP9PROFILE_MAX                       = VP9PROFILE_PROFILE3,
    HEVCPROFILE_MIN                      = 16,
    HEVCPROFILE_MAIN                     = HEVCPROFILE_MIN,
    HEVCPROFILE_MAIN10                   = 17,
    HEVCPROFILE_MAIN_STILL_PICTURE       = 18,
    HEVCPROFILE_MAX                      = HEVCPROFILE_MAIN_STILL_PICTURE,
    DOLBYVISION_PROFILE0                 = 19,
    DOLBYVISION_PROFILE4                 = 20,
    DOLBYVISION_PROFILE5                 = 21,
    DOLBYVISION_PROFILE7                 = 22,
    THEORAPROFILE_MIN                    = 23,
    THEORAPROFILE_ANY                    = THEORAPROFILE_MIN,
    THEORAPROFILE_MAX                    = THEORAPROFILE_ANY,
    AV1PROFILE_MIN                       = 24,
    AV1PROFILE_PROFILE_MAIN              = AV1PROFILE_MIN,
    AV1PROFILE_PROFILE_HIGH              = 25,
    AV1PROFILE_PROFILE_PRO               = 26,
    AV1PROFILE_MAX                       = AV1PROFILE_PROFILE_PRO,
    DOLBYVISION_PROFILE8                 = 27,
    DOLBYVISION_PROFILE9                 = 28,
    VIDEO_CODEC_PROFILE_MAX              = DOLBYVISION_PROFILE9,
};

class EncoderConfig
{
public:
    EncoderConfig()  = default;
    ~EncoderConfig() = default;

    uint32_t frameRate;
    uint32_t bitRate;
    int width;
    int height;
    VideoPixelFormat pixelFormat;
    size_t outputBufferSize;
    uint8_t h264OutputLevel;
    uint32_t gopLength;
    VideoCodecProfile profile;
};

class BufferEncoder
{
public:
    BufferEncoder();
    ~BufferEncoder();

    using BufferCallback =
        std::function<void(const uint8_t *data, size_t size, uint64_t timestamp, bool is_keyframe)>;

    bool Initialize(const mrf::EncoderConfig *config_data, BufferCallback buffer_callback);
    void Destroy();
    bool EncodeBuffer(const uint8_t *yBuf, size_t ySize, const uint8_t *uBuf, size_t uSize,
                      const uint8_t *vBuf, size_t vSize, uint64_t bufferTimestamp,
                      const bool requestKeyFrame);
    bool UpdateEncodingParams(uint32_t bitrate, uint32_t framerate);

    static GstFlowReturn OnEncodedBuffer(GstElement *elt, gpointer *data);

private:
    bool CreatePipeline(const EncoderConfig *configData);
    bool CreateEncoder(VideoCodecProfile profile);
    bool CreateSink();
    bool LinkElements(const EncoderConfig *configData);

    void SetGstreamerDebug();
    static gboolean HandleBusMessage(GstBus *bus_, GstMessage *message, gpointer user_data);

    GstBus *bus_             = nullptr;
    GstElement *pipeline_    = nullptr;
    GstElement *source_      = nullptr;
    GstElement *filter_YUY2_ = nullptr;
    GstElement *filter_H264_ = nullptr;
    GstElement *parse_       = nullptr;
    GstElement *converter_   = nullptr;
    GstElement *filter_NV12_ = nullptr;
    GstElement *encoder_     = nullptr;
    GstElement *sink_        = nullptr;
    GstCaps *caps_YUY2_      = nullptr;
    GstCaps *caps_NV12_      = nullptr;
    GstCaps *caps_H264_      = nullptr;
    uint32_t bitrate_        = 0;

    std::chrono::time_point<std::chrono::system_clock> start_time_;
    uint32_t current_seconds_ = 0;
    uint32_t buffers_per_sec_ = 0;

    BufferCallback buffer_callback_;
};

} // namespace mrf

#endif // BUFFER_ENCODER_H_
