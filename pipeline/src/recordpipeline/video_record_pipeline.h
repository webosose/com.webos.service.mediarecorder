#ifndef VIDEO_RECORD_PIPELINE_H_
#define VIDEO_RECORD_PIPELINE_H_

#include "base_record_pipeline.h"

class VideoRecordPipeline : public BaseRecordPipeline
{
public:
    VideoRecordPipeline() { pipelineType = "VideoRecord"; }
    bool launch() override;
    bool Pause() override;
};

#endif // VIDEO_RECORD_PIPELINE_H_
