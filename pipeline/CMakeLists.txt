# Copyright (c) 2019-2023 LG Electronics, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 2.8)

project(g-record-pipeline)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -lrt")

if(NOT DEFINED WEBOS_INSTALL_ROOT)
  set(WEBOS_INSTALL_ROOT /usr/local/webos/)
endif()

if(NOT DEFINED WEBOS_INSTALL_SBINDIR)
  set(WEBOS_INSTALL_SBINDIR /usr/sbin)
endif()

set(CMAKE_INSTALL_PREFIX ${WEBOS_INSTALL_ROOT}usr)

if (NOT DEFINED ENV{PKG_CONFIG_PATH})
  set(ENV{PKG_CONFIG_PATH} ${WEBOS_INSTALL_ROOT}usr/share/pkgconfig)
endif()

file(GLOB_RECURSE HEADERS "*.h*")
add_custom_target(show_headers SOURCES ${HEADERS})

find_package(PkgConfig REQUIRED)

add_subdirectory(src)
add_subdirectory(files)
