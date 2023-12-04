# Copyright (c) 2023 LG Electronics, Inc.
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

cmake_minimum_required(VERSION 2.8.7)
project(camera_test C CXX)

include_directories(src)
include_directories(src/utils)

include(FindPkgConfig)

pkg_check_modules(WLCLIENT REQUIRED wayland-webos-client)
include_directories(${WLCLIENT_INCLUDE_DIRS})

pkg_check_modules(WLEGL REQUIRED wayland-egl)
include_directories(${WLEGL_INCLUDE_DIRS})

pkg_check_modules(GLESV2 REQUIRED glesv2)
include_directories(${GLESV2_INCLUDE_DIRS})

pkg_check_modules(PMLOG REQUIRED PmLogLib)
include_directories(${PMLOG_INCLUDE_DIRS})

# -- check for glib 2.0
pkg_check_modules(GLIB2 REQUIRED glib-2.0)
include_directories(${GLIB2_INCLUDE_DIRS})

pkg_check_modules(LS2 REQUIRED luna-service2)
include_directories(${LS2_INCLUDE_DIRS})

pkg_check_modules(JPEG REQUIRED libjpeg)
include_directories(${JPEG_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${JPEG_CFLAGS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -g -lrt -Wno-unused-function")

set(BIN_NAME camera_test)

set(SRC_LIST
    src/main.cpp
    src/cam_player_app.cpp
    src/client/client.cpp
    src/client/media_client.cpp
    src/client/camera_client.cpp
    src/client/media_recorder_client.cpp
    ${CMAKE_SOURCE_DIR}/src/ls_connector/luna_client.cpp
    src/opengl/button.cpp
    src/opengl/image.cpp
    src/opengl/init_opengl.cpp
    src/utils/file_utils.cpp
    src/utils/image_utils.cpp
    src/window/window_manager.cpp
    src/window/wayland_exporter.cpp
    src/window/wayland_foreign.cpp
    #src/window/wayland_surface.cpp
)

add_executable(${BIN_NAME} ${SRC_LIST})
set_target_properties(${BIN_NAME} PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries (${BIN_NAME}
    ${WLCLIENT_LDFLAGS}
    ${WLEGL_LDFLAGS}
    ${GLESV2_LDFLAGS}
    ${LS2_LDFLAGS}
    ${GLIB2_LDFLAGS}
    -lEGL
    ${PMLOG_LDFLAGS}
    ${JPEG_LDFLAGS}
)

INSTALL ( TARGETS ${BIN_NAME}
    DESTINATION /usr/palm/applications/com.sample.camera.test
)

install(FILES appinfo.json DESTINATION palm/applications/com.sample.camera.test)
install(FILES icon.png DESTINATION palm/applications/com.sample.camera.test)
install(FILES res/start_cam.jpg DESTINATION palm/applications/com.sample.camera.test)
install(FILES res/stop_cam.jpg DESTINATION palm/applications/com.sample.camera.test)
install(FILES res/record.jpg DESTINATION palm/applications/com.sample.camera.test)
install(FILES res/play.jpg DESTINATION palm/applications/com.sample.camera.test)
install(FILES res/pause.jpg DESTINATION palm/applications/com.sample.camera.test)
install(FILES res/stop.jpg DESTINATION palm/applications/com.sample.camera.test)
install(FILES res/exit.jpg DESTINATION palm/applications/com.sample.camera.test)
install(FILES res/take_picture.jpg DESTINATION palm/applications/com.sample.camera.test)
install(FILES res/ptz.jpg DESTINATION palm/applications/com.sample.camera.test)

add_subdirectory(files)
