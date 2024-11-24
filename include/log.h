/*
 * Copyright (C) 2023 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MEDIARECORDER_SERVICE_LOG_MESSAGES_H_
#define MEDIARECORDER_SERVICE_LOG_MESSAGES_H_

#include "PmLogLib.h"

static inline PmLogContext getRecorderLunaPmLogContext()
{
    static PmLogContext usLogContext = 0;
    if (0 == usLogContext)
    {
        PmLogGetContext("mediarecorder", &usLogContext);
    }
    return usLogContext;
}

/*
 * This is the local tag used for the following simplified
 * logging macros.  You can change this preprocessor definition
 * before using the other macros to change the tag.
 */
#ifndef LOG_TAG
#define LOG_TAG "MediaRecorder"
#endif

/*
 * Simplified macro to send a info log message using the current LOG_TAG.
 */
#ifndef PLOGI
#define PLOGI(FORMAT__, ...)                                                                       \
    PmLogInfo(getRecorderLunaPmLogContext(), LOG_TAG, 0, "[%d:%d][%s():%d] " FORMAT__, getpid(),   \
              gettid(), __FUNCTION__, __LINE__, ##__VA_ARGS__)
#endif

/*
 * Simplified macro to send a warning log message using the current LOG_TAG.
 */
#ifndef PLOGW
#define PLOGW(FORMAT__, ...)                                                                       \
    PmLogWarning(getRecorderLunaPmLogContext(), LOG_TAG, 0, "[%d:%d][%s():%d] " FORMAT__,          \
                 getpid(), gettid(), __FUNCTION__, __LINE__, ##__VA_ARGS__)
#endif

/*
 * Simplified macro to send an error log message using the current LOG_TAG.
 */
#ifndef PLOGE
#define PLOGE(FORMAT__, ...)                                                                       \
    PmLogError(getRecorderLunaPmLogContext(), LOG_TAG, 0, "[%d:%d][%s():%d] " FORMAT__, getpid(),  \
               gettid(), __FUNCTION__, __LINE__, ##__VA_ARGS__)
#endif

/*
 * Macro to send a debug log message.
 */
#ifndef PLOGD
#define PLOGD(FORMAT__, ...)                                                                       \
    PmLogDebug(getRecorderLunaPmLogContext(), "[%d:%d][%s():%d] " FORMAT__, getpid(), gettid(),    \
               __FUNCTION__, __LINE__, ##__VA_ARGS__)
#endif

#endif /* MEDIARECORDER_SERVICE_LOG_MESSAGES_H_ */
