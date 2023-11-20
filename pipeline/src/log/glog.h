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

#ifndef _GLOG_LOG_H_
#define _GLOG_LOG_H_

#include <PmLogLib.h>
#include <assert.h>

PmLogContext GetPmLogContext();

#define LOGI(FORMAT__, ...)                                                                        \
    PmLogInfo(GetPmLogContext(), "grp", 0, "[%s:%d] " FORMAT__, __PRETTY_FUNCTION__, __LINE__,     \
              ##__VA_ARGS__)

#define LOGD(FORMAT__, ...)                                                                        \
    PmLogDebug(GetPmLogContext(), "[%s:%d]" FORMAT__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__)

#define LOGE(FORMAT__, ...)                                                                        \
    PmLogError(GetPmLogContext(), "grp", 0, "[%s:%d] " FORMAT__, __PRETTY_FUNCTION__, __LINE__,    \
               ##__VA_ARGS__)

#define LOGW(FORMAT__, ...)                                                                        \
    PmLogWarning(GetPmLogContext(), "grp", 0, "[%s:%d] " FORMAT__, __PRETTY_FUNCTION__, __LINE__,  \
                 ##__VA_ARGS__)

/* Assert print */
#define GRPASSERT(cond)                                                                            \
    {                                                                                              \
        if (!(cond))                                                                               \
        {                                                                                          \
            LOGE("ASSERT FAILED : %s:%d:%s: %s", __FILE__, __LINE__, __func__, #cond);             \
            assert(0);                                                                             \
        }                                                                                          \
    }

#endif // _GLOG_LOG_H_
