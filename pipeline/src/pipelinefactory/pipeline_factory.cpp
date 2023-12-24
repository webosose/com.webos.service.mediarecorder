#include "pipeline_factory.h"
#include "audio_record_pipeline.h"
#include "glog.h"
#include "snapshot_pipeline.h"
#include "video_record_pipeline.h"

std::shared_ptr<RecordPipeline> PipelineFactory::CreateRecorder(const pbnjson::JValue &parsed)
{
    LOGI("start");

    const pbnjson::JValue &video = parsed["options"]["option"]["video"];
    if (video.isObject())
    {

        LOGI("Create Video Recorder");
        return std::make_shared<VideoRecordPipeline>();
    }
    else
    {
        const pbnjson::JValue &audio = parsed["options"]["option"]["audio"];
        if (audio.isObject())
        {
            LOGI("Create Audio Recorder");
            return std::make_shared<AudioRecordPipeline>();
        }

        const pbnjson::JValue &image = parsed["options"]["option"]["image"];
        if (image.isObject())
        {
            LOGI("Create Snapshot");
            return std::make_shared<SnapshotPipeline>();
        }
    }

    LOGE("Cant not create recoder");
    return nullptr;
}
