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

#ifndef SRC_ResourceFacilitator_REQUESTOR_H_
#define SRC_ResourceFacilitator_REQUESTOR_H_

#include "message.h"
#include <functional>
#include <map>
#include <memory>
#include <resource_calculator.h>
#include <string>

namespace mrc
{
class ResourceCalculator;
}
namespace base
{
struct source_info_t;
}
namespace uMediaServer
{
class ResourceManagerClient;
}

namespace resource
{

struct videoResData_t
{
    VIDEO_CODEC vcodec;
    VIDEO_CODEC vdecode;
    VIDEO_CODEC vencode;

    int width;
    int height;
    int frameRate;
    int escanType;
    int e3DType;
    int parWidth;  // pixel-aspect-ratio width
    int parHeight; // pixel-aspect-ratio height
};

typedef std::function<void()> Functor;
typedef std::function<bool(int32_t)> PlaneIDFunctor;
typedef mrc::ResourceCalculator MRC;
typedef std::multimap<std::string, int> PortResource_t;

class ResourceRequestor
{
public:
    explicit ResourceRequestor(const std::string &appId, const std::string &connectionId = "");
    virtual ~ResourceRequestor();

    const std::string getConnectionId() const { return connectionId_; }
    void registerUMSPolicyActionCallback(Functor callback) { cb_ = std::move(callback); }
    void registerPlaneIdCallback(PlaneIDFunctor callback) { planeIdCb_ = std::move(callback); }
    bool acquireResources(PortResource_t &resourceMMap, const base::source_info_t &sourceInfo,
                          const std::string &display_mode, const int32_t display_path = 0);

    bool releaseResource();

    bool notifyForeground() const;
    bool notifyBackground() const;
    bool notifyActivity() const;
    bool notifyPipelineStatus(const std::string &status) const;
    void allowPolicyAction(const bool allow);
    void setAppId(std::string id);
    const std::string getAcquiredResource() const { return acquiredResource_; }

private:
    bool setSourceInfo(const base::source_info_t &sourceInfo);
    bool policyActionHandler(const char *action, const char *resources, const char *requestorType,
                             const char *requestorName, const char *connectionId);
    void planeIdHandler(int32_t planePortIdx);
    bool parsePortInformation(const std::string &payload, PortResource_t &resourceMMap);
    bool parseResources(const std::string &payload, std::string &resources);

    // translate enum type from omx player to resource calculator
    int translateVideoCodec(const VIDEO_CODEC vcodec) const;
    int translateScanType(const int escanType) const;
    int translate3DType(const int e3DType) const;
    mrc::ResourceListOptions calcVdecResources();
    mrc::ResourceListOptions calcVencResources();
    mrc::ResourceListOptions calcDisplayResource(const std::string &display_mode);

    std::shared_ptr<MRC> rc_;
    std::shared_ptr<uMediaServer::ResourceManagerClient> umsRMC_;

    std::string appId_;
    std::string instanceId_;
    std::string connectionId_;
    Functor cb_;
    PlaneIDFunctor planeIdCb_;
    std::string acquiredResource_;
    videoResData_t videoResData_;

    bool allowPolicy_;
};

} // namespace resource

#endif // SRC_ResourceFacilitator_REQUESTOR_H_
