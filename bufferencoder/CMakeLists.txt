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

include(FindPkgConfig)

pkg_check_modules(GLIB2 REQUIRED glib-2.0)
include_directories(${GLIB2_INCLUDE_DIRS})
link_directories(${GLIB2_LIBRARY_DIRS})

pkg_check_modules(GSTPLAYER gstreamer-player-1.0 REQUIRED)
include_directories(${GSTPLAYER_INCLUDE_DIRS})
link_directories(${GSTPLAYER_LIBRARY_DIRS})

pkg_check_modules(GSTAPP gstreamer-app-1.0 REQUIRED)
include_directories(${GSTAPP_INCLUDE_DIRS})
link_directories(${GSTAPP_LIBRARY_DIRS})

pkg_check_modules(GSTREAMER gstreamer-1.0 REQUIRED)
include_directories(${GSTREAMER_INCLUDE_DIRS})
link_directories(${GSTREAMER_LIBRARY_DIRS})

pkg_check_modules(GSTPBUTIL gstreamer-pbutils-1.0 REQUIRED)
include_directories(${GSTPBUTIL_INCLUDE_DIRS})
link_directories(${GSTPBUTIL_LIBRARY_DIRS})

pkg_check_modules(LUNASERVICE luna-service2 REQUIRED)
include_directories(${LUNASERVICE_INCLUDE_DIRS})
link_directories(${LUNASERVICE_LIBRARY_DIRS})

option(WEBOS_CLANG_BUILD "Build with clang" OFF)
if(${WEBOS_CLANG_BUILD})
  pkg_check_modules(PBNJSON pbnjson_cpp_clang REQUIRED)
else()
  pkg_check_modules(PBNJSON pbnjson_cpp REQUIRED)
endif()
include_directories(${PBNJSON_INCLUDE_DIRS})
link_directories(${PBNJSON_LIBRARY_DIRS})

find_package(Boost REQUIRED COMPONENTS regex)
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

pkg_check_modules(PMLOGLIB QUIET PmLogLib)
if (PMLOGLIB_FOUND)
  set(PMLOG_LIBRARIES PmLogLib)
endif (PMLOGLIB_FOUND)

set(GSTAPP_LIB gstapp-1.0)

set(BUFFER_ENCODER_LIB gst-video-encoder)

if(GST_V4L2_ENCODER)
    add_definitions(-DGST_V4L2_ENCODER)
endif()

if(USE_NV12)
    add_definitions(-DUSE_NV12)
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)

set(BUFFER_ENCODER_HEADERS
    buffer_encoder.h
    ${CMAKE_SOURCE_DIR}/include/log.h
)

set(BUFFER_ENCODER_SRC
    buffer_encoder.cpp
)

set(BUFFER_ENCODER_LIBRARIES
    ${GSTPLAYER_LIBRARIES}
    ${GSTREAMER_LIBRARIES}
    ${GSTPBUTIL_LIBRARIES}
    ${GSTAPP_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${GLIB2_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${PBNJSON_LIBRARIES}
    ${PMLOG_LIBRARIES}
    ${LUNASERVICE_LDFLAGS}
    ${GSTAPP_LIB}
)

add_library(${BUFFER_ENCODER_LIB} SHARED ${BUFFER_ENCODER_SRC})
set_target_properties(${BUFFER_ENCODER_LIB} PROPERTIES VERSION 1.0 SOVERSION 1)
target_link_libraries(${BUFFER_ENCODER_LIB} ${BUFFER_ENCODER_LIBRARIES})

install(TARGETS ${BUFFER_ENCODER_LIB} DESTINATION ${WEBOS_INSTALL_LIBDIR})
install(FILES ${BUFFER_ENCODER_HEADERS} DESTINATION ${WEBOS_INSTALL_INCLUDEDIR}/gst-video-encoder)

#install gst-video-encoder.pc file
configure_file(gst-video-encoder.pc.in ${CMAKE_BINARY_DIR}/gst-video-encoder.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/gst-video-encoder.pc DESTINATION share/pkgconfig)

#use gst debug file
message(STATUS "Using gst debug file")
install(FILES gst_debug.conf DESTINATION /etc/gst-video-encoder)

