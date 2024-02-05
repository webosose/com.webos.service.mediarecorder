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

include(FindPkgConfig)

pkg_check_modules(GLIB2 REQUIRED glib-2.0)
include_directories(${GLIB2_INCLUDE_DIRS})
link_directories(${GLIB2_LIBRARY_DIRS})

pkg_check_modules(GSTREAMER gstreamer-1.0 REQUIRED)
include_directories(${GSTREAMER_INCLUDE_DIRS})
link_directories(${GSTREAMER_LIBRARY_DIRS})

pkg_check_modules(LUNASERVICE luna-service2 REQUIRED)
include_directories(${LUNASERVICE_INCLUDE_DIRS})
link_directories(${LUNASERVICE_LIBRARY_DIRS})

pkg_check_modules(PBNJSON pbnjson_cpp REQUIRED)
include_directories(${PBNJSON_INCLUDE_DIRS})
link_directories(${PBNJSON_LIBRARY_DIRS})

pkg_check_modules(RESOURCE-CALCULATOR REQUIRED media-resource-calculator)
include_directories(${RESOURCE-CALCULATOR_INCLUDE_DIRS})
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

find_package(Boost REQUIRED COMPONENTS regex)
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

pkg_check_modules(PMLOGLIB QUIET PmLogLib)
if (PMLOGLIB_FOUND)
  set(PMLOG_LIBRARIES PmLogLib)
endif (PMLOGLIB_FOUND)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(.)
include_directories(base)
include_directories(log)
include_directories(parser)
include_directories(recordservice)
include_directories(recordpipeline)
include_directories(pipelinefactory)

set(GSTAPP_LIB gstapp-1.0)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")

set(G-RECORD-PIPELINE_SRC
    main.cpp
    recordpipeline/base_record_pipeline.cpp
    recordpipeline/video_record_pipeline.cpp
    recordpipeline/audio_record_pipeline.cpp
    recordpipeline/snapshot_pipeline.cpp
    pipelinefactory/pipeline_factory.cpp
    pipelinefactory/element_factory.cpp
    parser/parser.cpp
    parser/serializer.cpp
    log/glog.cpp
    recordservice/record_service.cpp
    resourcefacilitator/requestor.cpp
    )

set(G-RECORD-PIPELINE_LIBRARIES
    ums_connector
    ${Boost_REGEX_LIBRARY}
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
    ${RESOURCE-CALCULATOR_LDFLAGS}
    resource_mgr_client
    resource_mgr_client_c
    ${GSTAPP_LIB}
    )

add_executable(g-record-pipeline ${G-RECORD-PIPELINE_SRC})
target_link_libraries(g-record-pipeline
    ${G-RECORD-PIPELINE_LIBRARIES}
    ${RESOURCE-CALCULATOR_LDFLAGS}
    resource_mgr_client
    resource_mgr_client_c
    )

if(USE_RPI)
  message(STATUS "Using config & debug file for raspberrypi")
  install(FILES recordpipeline/raspberrypi/gst_debug.conf DESTINATION /etc/g-record-pipeline)
  install(FILES recordpipeline/raspberrypi/gst_elements.conf DESTINATION /etc/g-record-pipeline)
# emulator or default
else()
  message(STATUS "Using config & debug file for software")
  install(FILES recordpipeline/qemux86/gst_debug.conf DESTINATION /etc/g-record-pipeline)
  install(FILES recordpipeline/qemux86/gst_elements.conf DESTINATION /etc/g-record-pipeline)

endif()

install(TARGETS g-record-pipeline DESTINATION ${WEBOS_INSTALL_SBINDIR})
