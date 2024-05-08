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

install(FILES sysbus/perm/com.sample.camera.test.app.json DESTINATION /usr/share/luna-service2/client-permissions.d/)
install(FILES sysbus/com.sample.camera.test.manifest.json DESTINATION /usr/share/luna-service2/manifests.d/)
install(FILES sysbus/role/com.sample.camera.test.app.json DESTINATION /usr/share/luna-service2/roles.d/)

install(FILES gst_debug.conf DESTINATION /etc/com.sample.camera.test)

install(FILES camera_test DESTINATION ${WEBOS_INSTALL_SBINDIR})
