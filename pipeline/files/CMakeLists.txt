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

configure_file(sysbus/g-record-pipeline.manifest.json.in sysbus/g-record-pipeline.manifest.json @ONLY)
configure_file(sysbus/com.webos.g-record-pipeline.role.json.in sysbus/com.webos.g-record-pipeline.role.json @ONLY)
configure_file(sysbus/com.webos.g-record-pipeline.service.in sysbus/com.webos.g-record-pipeline.service @ONLY)
configure_file(sysbus/com.webos.g-record-pipeline.api.json sysbus/com.webos.g-record-pipeline.api.json @ONLY)
configure_file(sysbus/com.webos.g-record-pipeline.perm.json sysbus/com.webos.g-record-pipeline.perm.json @ONLY)
configure_file(sysbus/com.webos.g-record-pipeline.groups.json sysbus/com.webos.g-record-pipeline.groups.json @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/sysbus/g-record-pipeline.manifest.json
    DESTINATION /usr/share/luna-service2/manifests.d/)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/sysbus/com.webos.g-record-pipeline.role.json
    DESTINATION /usr/share/luna-service2/roles.d/)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/sysbus/com.webos.g-record-pipeline.service
    DESTINATION /usr/share/luna-service2/services.d/)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/sysbus/com.webos.g-record-pipeline.api.json
    DESTINATION /usr/share/luna-service2/api-permissions.d/)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/sysbus/com.webos.g-record-pipeline.perm.json
    DESTINATION /usr/share/luna-service2/client-permissions.d/)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/sysbus/com.webos.g-record-pipeline.groups.json
    DESTINATION /usr/share/luna-service2/groups.d/)
