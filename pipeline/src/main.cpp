// Copyright (c) 2019-2023 LG Electronics, Inc.
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

#include "glog.h"
#include "record_service.h"
#include <cstdio>
#include <unistd.h>

#define MAX_SERVICE_STRING 120

int main(int argc, char *argv[])
{
    int c;
    char service_name[MAX_SERVICE_STRING + 1] = {
        '\0',
    };
    bool service_name_specified = false;

    while ((c = getopt(argc, argv, "c:s:r:d:v:a:")) != -1)
    {
        switch (c)
        {
        case 's':
            snprintf(service_name, MAX_SERVICE_STRING, "%s", optarg);
            service_name_specified = true;
            break;

        case '?':
            LOGE("unknown service name");
            break;

        default:
            break;
        }
    }

    if (!service_name_specified)
        return 1;

    RecordService *service = RecordService::GetInstance(service_name);

    service->Wait();

    delete service;

    return 0;
}
