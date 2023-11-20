#include "pipeline_factory.h"
#include "audio_record_pipeline.h"
#include "glog.h"
#include "snapshot_pipeline.h"
#include "video_record_pipeline.h"

std::shared_ptr<RecordPipeline> PipelineFactory::CreateRecoder(const pbnjson::JValue &parsed)
{
    LOGI("start");

    std::string camera_id, format;
    if (parsed["options"]["option"].hasKey("cameraId"))
    {
        camera_id = parsed["options"]["option"]["cameraId"].asString();
    }
    if (parsed["options"]["option"].hasKey("format"))
    {
        format = parsed["options"]["option"]["format"].asString();
    }

    return std::make_shared<VideoRecordPipeline>();
}
