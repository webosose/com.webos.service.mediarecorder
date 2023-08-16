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

#include "cam_player_app.h"
#include "log_info.h"
#include <csignal>

CamPlayerApp *miCameraApp;

void my_handler(int s)
{
    ERROR_LOG("Caught signal %d", s);
    if (miCameraApp)
    {
        delete miCameraApp;
    }
    ERROR_LOG("exit 1");
    exit(1);
}

void singal_catcher(void (*handler)(int))
{
    struct sigaction sa;

    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);
}

int main(int argc, char **argv)
{
    DEBUG_LOG("start");

    singal_catcher(my_handler);

    miCameraApp = new CamPlayerApp(argc, argv);

    miCameraApp->initialize();
    miCameraApp->execute();

    delete miCameraApp;

    DEBUG_LOG("end");
    exit(0);
}
