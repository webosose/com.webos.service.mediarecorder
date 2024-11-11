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

#include "buffer_encoder.h"

#include <cmath>
#include <cstring>
#include <gio/gio.h>
#include <glib.h>
#include <map>
#include <memory>
#include <sstream>
#include <stdlib.h>
#include <string.h>

#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include <gst/gst.h>
#include <gst/pbutils/pbutils.h>

#include <pbnjson.hpp>

namespace mrf
{

BufferEncoder::BufferEncoder()
{
    filter_H264_ = nullptr;
    caps_H264_   = nullptr;
    filter_NV12_ = nullptr;
    caps_NV12_   = nullptr;
}

BufferEncoder::~BufferEncoder() { Destroy(); }

bool BufferEncoder::Initialize(const mrf::EncoderConfig *config_data,
                               BufferCallback buffer_callback)
{
    PLOGI(" ");
    buffer_callback_ = std::move(buffer_callback);
    if (!CreatePipeline(config_data))
    {
        PLOGE("CreatePipeline Failed");
        return false;
    }
    bitrate_ = 0;
    return true;
}

void BufferEncoder::Destroy()
{
    if (pipeline_)
    {
        gst_element_set_state(pipeline_, GST_STATE_NULL);
        pipeline_ = nullptr;
    }
}

bool BufferEncoder::EncodeBuffer(const uint8_t *yBuf, size_t ySize, const uint8_t *uBuf,
                                 size_t uSize, const uint8_t *vBuf, size_t vSize,
                                 uint64_t bufferTimestamp, const bool requestKeyFrame)
{
    if (!pipeline_)
    {
        PLOGE("Pipeline is null");
        return false;
    }

    size_t bufferSize  = ySize + uSize * 2;
    guint8 *feedBuffer = (guint8 *)g_malloc(bufferSize);
    if (feedBuffer == NULL)
    {
        PLOGE("memory allocation error!!!!!");
        return false;
    }

    memcpy(feedBuffer, yBuf, ySize);
    memcpy(feedBuffer + ySize, uBuf, uSize);
    memcpy(feedBuffer + ySize + uSize, vBuf, vSize);

    GstBuffer *gstBuffer = gst_buffer_new_wrapped(feedBuffer, bufferSize);
    if (!gstBuffer)
    {
        PLOGE("Buffer wrapping error");
        return false;
    }

    GstFlowReturn gstReturn = gst_app_src_push_buffer((GstAppSrc *)source_, gstBuffer);
    if (gstReturn < GST_FLOW_OK)
    {
        PLOGE("gst_app_src_push_buffer errCode[ %d ]", gstReturn);
        return false;
    }
    return true;
}

bool BufferEncoder::UpdateEncodingParams(uint32_t bitrate, uint32_t framerate)
{
    PLOGD(": bitrate=%d, framerate=%d", bitrate, framerate);

    if (encoder_ && bitrate > 0 && bitrate_ != bitrate)
    {
#if defined(GST_V4L2_ENCODER)
        GstStructure *extraCtrls =
            gst_structure_new("extra-controls", "video_bitrate", G_TYPE_INT, bitrate, NULL);
        g_object_set(G_OBJECT(encoder_), "extra-controls", extraCtrls, NULL);
#else
        g_object_set(G_OBJECT(encoder_), "target-bitrate", bitrate, NULL);
#endif
        bitrate_ = bitrate;
    }
    return true;
}

bool BufferEncoder::CreateEncoder(VideoCodecProfile profile)
{
    PLOGD(" profile: %d", profile);

    if (profile >= H264PROFILE_MIN && profile <= H264PROFILE_MAX)
    {
#if defined(GST_V4L2_ENCODER)
        encoder_ = gst_element_factory_make("v4l2h264enc", "encoder");
        PLOGD("selected. encoder is v4l2h264enc");
#else
        encoder_ = gst_element_factory_make("omxh264enc", "encoder");
        PLOGD("selected. encoder is omxh264enc");
#endif
    }
    else if (profile >= VP8PROFILE_MIN && profile <= VP8PROFILE_MAX)
    {
        encoder_ = gst_element_factory_make("omxvp8enc", "encoder");
        PLOGD("selected. encoder is omxvp8enc");
    }
    else
    {
        PLOGE(": Unsupported Codedc");
        return false;
    }

    if (!encoder_)
    {
        PLOGE("encoder_ element creation failed.");
        return false;
    }
    return true;
}

bool BufferEncoder::CreateSink()
{
    sink_ = gst_element_factory_make("appsink", "sink");
    if (!sink_)
    {
        PLOGE("sink_ element creation failed.");
        return false;
    }
    g_object_set(G_OBJECT(sink_), "emit-signals", TRUE, "sync", FALSE, NULL);
    g_signal_connect(sink_, "new-sample", G_CALLBACK(OnEncodedBuffer), this);

    return true;
}

bool BufferEncoder::LinkElements(const EncoderConfig *configData)
{
    PLOGD(": width: %d, height: %d", configData->width, configData->height);

    filter_YUY2_ = gst_element_factory_make("capsfilter", "filter-YUY2");
    if (!filter_YUY2_)
    {
        PLOGE("filter_YUY2_(%p) Failed", filter_YUY2_);
        return false;
    }

    caps_YUY2_ =
        gst_caps_new_simple("video/x-raw", "width", G_TYPE_INT, configData->width, "height",
                            G_TYPE_INT, configData->height, "framerate", GST_TYPE_FRACTION,
                            configData->frameRate, 1, "format", G_TYPE_STRING, "I420", NULL);
    g_object_set(G_OBJECT(filter_YUY2_), "caps", caps_YUY2_, NULL);

#if defined(GST_V4L2_ENCODER)
    filter_H264_ = gst_element_factory_make("capsfilter", "filter-h264");
    if (!filter_H264_)
    {
        PLOGE("filter_H264_ element creation failed.");
        return false;
    }
    caps_H264_ = gst_caps_new_simple("video/x-h264", "level", G_TYPE_STRING, "4", NULL);
    g_object_set(G_OBJECT(filter_H264_), "caps", caps_H264_, NULL);
#else
#if defined(USE_NV12)
    filter_NV12_ = gst_element_factory_make("capsfilter", "filter-NV");
    if (!filter_NV12_)
    {
        PLOGE("filter_ element creation failed.");
        return false;
    }

    caps_NV12_ = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "NV12", NULL);
    g_object_set(G_OBJECT(filter_NV12_), "caps", caps_NV12_, NULL);
#endif
#endif

    converter_ = gst_element_factory_make("videoconvert", "converted");
    if (!converter_)
    {
        PLOGE("converter_(%p) Failed", converter_);
        return false;
    }

    parse_ = gst_element_factory_make("rawvideoparse", "parser");
    if (!parse_)
    {
        PLOGE("parse_(%p) Failed", parse_);
        return false;
    }

    g_object_set(G_OBJECT(parse_), "width", configData->width, NULL);
    g_object_set(G_OBJECT(parse_), "height", configData->height, NULL);
    if (PIXEL_FORMAT_I420 == configData->pixelFormat)
    {
        g_object_set(G_OBJECT(parse_), "format", 2, NULL);
    }

#if defined(GST_V4L2_ENCODER)
    gst_bin_add_many(GST_BIN(pipeline_), source_, filter_YUY2_, parse_, converter_, encoder_,
                     filter_H264_, sink_, NULL);
#else
#if defined(USE_NV12)
    gst_bin_add_many(GST_BIN(pipeline_), source_, filter_YUY2_, parse_, converter_, filter_NV12_,
                     encoder_, sink_, NULL);
#else
    gst_bin_add_many(GST_BIN(pipeline_), source_, filter_YUY2_, parse_, converter_, encoder_, sink_,
                     NULL);
#endif
#endif

    if (!gst_element_link(source_, filter_YUY2_))
    {
        PLOGE("Linkerror - source_ & filter_YUY2");
        return false;
    }

    if (!gst_element_link(filter_YUY2_, parse_))
    {
        PLOGE("Link error - filter_YUY2 & converter_");
        return false;
    }

    if (!gst_element_link(parse_, converter_))
    {
        PLOGE("Link error - parse_ & converter_");
        return false;
    }

#if defined(GST_V4L2_ENCODER)
    if (!gst_element_link(converter_, encoder_))
    {
        PLOGE("Link error - converter_ & encoder_");
        return false;
    }
    if (!gst_element_link(encoder_, filter_H264_))
    {
        PLOGE("Link error - encoder_ & filter_H264_");
        return false;
    }
#else
#if defined(USE_NV12)
    if (!gst_element_link(converter_, filter_NV12_))
    {
        PLOGE("Link error - converter_ & filter_NV12_");
        return false;
    }

    if (!gst_element_link(filter_NV12_, encoder_))
    {
        PLOGE("Link error - filter_NV12_ & encoder_");
        return false;
    }
#else
    if (!gst_element_link(converter_, encoder_))
    {
        PLOGE("Link error - converter_ & encoder_");
        return false;
    }
#endif
#endif

#if defined(GST_V4L2_ENCODER)
    if (!gst_element_link(filter_H264_, sink_))
    {
        PLOGE("Link error - filter_H264_ & sink_");
        return false;
    }
#else
    if (!gst_element_link(encoder_, sink_))
    {
        PLOGE("Link error - encoder_ & sink_");
        return false;
    }
#endif
    return true;
}

gboolean BufferEncoder::HandleBusMessage(GstBus *bus_, GstMessage *message, gpointer user_data)
{
    GstMessageType messageType = GST_MESSAGE_TYPE(message);
    if (messageType != GST_MESSAGE_QOS && messageType != GST_MESSAGE_TAG)
    {
        PLOGD("Element[ %s ][ %d ][ %s ]", GST_MESSAGE_SRC_NAME(message), messageType,
              gst_message_type_get_name(messageType));
    }
    return true;
}

bool BufferEncoder::CreatePipeline(const EncoderConfig *configData)
{
    SetGstreamerDebug();

    gst_init(NULL, NULL);
    gst_pb_utils_init();
    PLOGI(" ");
    pipeline_ = gst_pipeline_new("video-encoder");
    if (!pipeline_)
    {
        PLOGE("Cannot create encoder pipeline!");
        return false;
    }

    source_ = gst_element_factory_make("appsrc", "app-source");
    if (!source_)
    {
        PLOGE("source_ element creation failed.");
        return false;
    }

    g_object_set(source_, "format", GST_FORMAT_TIME, NULL);
    g_object_set(source_, "do-timestamp", true, NULL);

    if (!CreateEncoder(configData->profile))
    {
        PLOGE("Encoder creation failed !!!");
        return false;
    }

    if (!CreateSink())
    {
        PLOGE("Sink creation failed !!!");
        return false;
    }

    if (!LinkElements(configData))
    {
        PLOGE("element linking failed !!!");
        return false;
    }

    bus_ = gst_pipeline_get_bus(GST_PIPELINE(pipeline_));
    gst_bus_add_watch(bus_, BufferEncoder::HandleBusMessage, this);
    gst_object_unref(bus_);

    return gst_element_set_state(pipeline_, GST_STATE_PLAYING);
}

/* called when the appsink notifies us that there is a new buffer ready for
 * processing */
GstFlowReturn BufferEncoder::OnEncodedBuffer(GstElement *elt, gpointer *data)
{
    BufferEncoder *encoder = reinterpret_cast<BufferEncoder *>(data);
    GstSample *sample;

    /* get the sample from appsink */
    sample = gst_app_sink_pull_sample(GST_APP_SINK(elt));
    if (sample)
    {
        GstMapInfo map_info = {};
        GstBuffer *buffer   = gst_sample_get_buffer(sample);
        gst_buffer_map(buffer, &map_info, GST_MAP_READ);

        if (!map_info.data || map_info.size == 0)
        {
            PLOGD(": Empty buffer received");
            gst_buffer_unmap(buffer, &map_info);
            gst_sample_unref(sample);
            return GST_FLOW_OK;
        }

        bool is_keyframe = false;
        if (!GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT))
        {
            is_keyframe = true;
        }

        if (encoder->start_time_ == std::chrono::time_point<std::chrono::system_clock>())
        {
            encoder->start_time_ = std::chrono::system_clock::now();
        }

        encoder->buffers_per_sec_++;
        auto time_past = std::chrono::system_clock::now() - encoder->start_time_;
        if (time_past >= std::chrono::seconds(1))
        {
            encoder->current_seconds_++;
            PLOGI(": Encoder @ %d secs => %d fps", encoder->current_seconds_,
                  encoder->buffers_per_sec_);
            encoder->start_time_      = std::chrono::system_clock::now();
            encoder->buffers_per_sec_ = 0;
        }

        uint64_t timestamp = GST_BUFFER_TIMESTAMP(buffer);
        PLOGD("OnEncodedBuffer: Buffer ready, calling MCIL::GstVideoEncoder. --#");
        encoder->buffer_callback_(map_info.data, map_info.size, timestamp, is_keyframe);

        gst_buffer_unmap(buffer, &map_info);
        gst_sample_unref(sample);
    }
    return GST_FLOW_OK;
}

void BufferEncoder::SetGstreamerDebug()
{
    pbnjson::JValue parsed = pbnjson::JDomParser::fromFile("/etc/gst-video-encoder/gst_debug.conf");
    if (!parsed.isObject())
    {
        PLOGE("Gst debug file parsing error");
    }

    pbnjson::JValue debug = parsed["gst_debug"];
    int size              = debug.arraySize();
    for (int i = 0; i < size; i++)
    {
        const char *kDebug     = "GST_DEBUG";
        const char *kDebugFile = "GST_DEBUG_FILE";
        const char *kDebugDot  = "GST_DEBUG_DUMP_DOT_DIR";
        if (debug[i].hasKey(kDebug) && !debug[i][kDebug].asString().empty())
            setenv(kDebug, debug[i][kDebug].asString().c_str(), 1);
        if (debug[i].hasKey(kDebugFile) && !debug[i][kDebugFile].asString().empty())
            setenv(kDebugFile, debug[i][kDebugFile].asString().c_str(), 1);
        if (debug[i].hasKey(kDebugDot) && !debug[i][kDebugDot].asString().empty())
            setenv(kDebugDot, debug[i][kDebugDot].asString().c_str(), 1);
    }
}
} // namespace mrf
