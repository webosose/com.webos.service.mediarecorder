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

#pragma once

#include <cstdint>
#include <vector>

struct stream_format_t
{
    std::string codec = "";
    bool empty() const { return codec.empty(); }
};

struct video_format_t : stream_format_t
{
    unsigned int width   = 0;
    unsigned int height  = 0;
    unsigned int fps     = 0;
    unsigned int bitRate = 0;
};

struct audio_format_t : stream_format_t
{
    uint32_t sampleRate = 0;
    uint32_t channels   = 0;
    uint32_t bitRate    = 0;
};

struct image_format_t : stream_format_t
{
    unsigned int width   = 0;
    unsigned int height  = 0;
    unsigned int quality = 0;
};

struct audio_support_list_t : stream_format_t
{
    std::vector<uint32_t> sampleRate;
    std::vector<uint32_t> channels;
    std::vector<uint32_t> bitRate;
};
