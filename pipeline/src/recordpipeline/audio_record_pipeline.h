#ifndef _AUDIO_RECORD_PIPELINE_H_
#define _AUDIO_RECORD_PIPELINE_H_

#include "base_record_pipeline.h"

class AudioRecordPipeline : public BaseRecordPipeline
{
public:
    AudioRecordPipeline() { pipelineType = "AudioRecord"; }
    bool launch();
};

#endif // _AUDIO_RECORD_PIPELINE_H_
