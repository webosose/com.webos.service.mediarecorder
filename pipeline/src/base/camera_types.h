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

#ifndef SRC_CAMERA_TYPES_H_
#define SRC_CAMERA_TYPES_H_

#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include <gst/gst.h>
#include <gst/pbutils/pbutils.h>
#include <gst/player/player.h>

typedef struct
{
    GMainLoop *loop;
    GstElement *source;
    GstElement *sink;
    char *shmpointer;
    int shmemid;
} ProgramData;

typedef enum
{
    GRP_ERROR_NONE,
    GRP_ERROR_STREAM,
    GRP_ERROR_ASYNC,
    GRP_ERROR_RES_ALLOC,
    GRP_ERROR_MAX
} GRP_ERROR_CODE;

typedef enum
{
    GRP_DEFAULT_DISPLAY = 0,
    GRP_PRIMARY_DISPLAY = 0,
    GRP_SECONDARY_DISPLAY,
} GRP_DISPLAY_PATH;

typedef enum
{
    GRP_NOTIFY_LOAD_COMPLETED = 0,
    GRP_NOTIFY_UNLOAD_COMPLETED,
    GRP_NOTIFY_SOURCE_INFO,
    GRP_NOTIFY_END_OF_STREAM,
    GRP_NOTIFY_PLAYING,
    GRP_NOTIFY_PAUSED,
    GRP_NOTIFY_ERROR,
    GRP_NOTIFY_VIDEO_INFO,
    GRP_NOTIFY_ACTIVITY,
    GRP_NOTIFY_ACQUIRE_RESOURCE,
    GRP_NOTIFY_MAX
} GRP_NOTIFY_TYPE_T;

/* player status enum type */
typedef enum
{
    GRP_LOADING_STATE,
    GRP_STOPPED_STATE,
    GRP_PAUSING_STATE,
    GRP_PAUSED_STATE,
    GRP_PLAYING_STATE,
    GRP_PLAYED_STATE,
} GRP_PIPELINE_STATE;

#endif
