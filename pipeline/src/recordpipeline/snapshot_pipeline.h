#ifndef _SNAPSHOT_PIPELINE_H_
#define _SNAPSHOT_PIPELINE_H_

#include "base_record_pipeline.h"

class SnapshotPipeline : public BaseRecordPipeline
{
public:
    SnapshotPipeline() { pipelineType = "Snapshot"; }
    bool launch();
};

#endif // _SNAPSHOT_PIPELINE_H_
