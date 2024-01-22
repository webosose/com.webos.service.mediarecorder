#ifndef _VIDEO_RECORD_PIPELINE_H_
#define _VIDEO_RECORD_PIPELINE_H_

#include "base_record_pipeline.h"

class VideoRecordPipeline : public BaseRecordPipeline
{
public:
    VideoRecordPipeline() { pipelineType = "VideoRecord"; }
    bool launch();
    bool Pause();
};

#endif // _VIDEO_RECORD_PIPELINE_H_
