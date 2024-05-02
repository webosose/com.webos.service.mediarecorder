# Copyright (c) 2024 LG Electronics, Inc.
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

cmake_minimum_required(VERSION 3.14)
project(com.webos.service.mediarecorder CXX)
set(CMAKE_BUILD_TYPE Debug)

include(webOS/webOS)
webos_modules_init(1 6 3)
webos_component(1 0 0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
webos_add_compiler_flags(ALL -Wall -funwind-tables)
webos_add_compiler_flags(ALL -Wall -rdynamic)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lrt -Werror -Wno-psabi")

include(FindPkgConfig)

pkg_check_modules(GLIB2 REQUIRED glib-2.0)
include_directories(${GLIB2_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${GLIB2_CFLAGS_OTHER})

pkg_check_modules(LUNASERVICE2 REQUIRED luna-service2)
include_directories(${LUNASERVICE2_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${LUNASERVICE2_CFLAGS_OTHER})

pkg_check_modules(PMLOG REQUIRED PmLogLib)
include_directories(${PMLOG_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${PMLOG_CFLAGS_OTHER})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

if(USE_G_RECORD)
    add_definitions(-DUSE_TYPE_G_RECORD)
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src/ls_connector)
include_directories(${CMAKE_SOURCE_DIR}/src/error_manager)

set(SRCS
    ${CMAKE_SOURCE_DIR}/src/media_recorder_manager.cpp
    ${CMAKE_SOURCE_DIR}/src/media_recorder.cpp
    ${CMAKE_SOURCE_DIR}/src/ls_connector/luna_client.cpp
    ${CMAKE_SOURCE_DIR}/src/ls_connector/ls_connector.cpp
    ${CMAKE_SOURCE_DIR}/src/error_manager/error.cpp
    ${CMAKE_SOURCE_DIR}/src/error_manager/error_manager.cpp
)

add_executable(${PROJECT_NAME} ${SRCS})

SET (EXT_LIBS
    ${GLIB2_LDFLAGS}
    ${LUNASERVICE2_LDFLAGS}
    ${PMLOG_LDFLAGS}
)

target_link_libraries(${PROJECT_NAME} ${EXT_LIBS})

install(TARGETS ${PROJECT_NAME} DESTINATION ${WEBOS_INSTALL_SBINDIR})

if(USE_NEW_ACG)
    message("USE_NEW_ACG is ${USE_NEW_ACG}")
    webos_build_system_bus_files(files/sysbus_new)
else()
    webos_build_system_bus_files(files/sysbus)
endif()

add_subdirectory(tests)
add_subdirectory(pipeline)
