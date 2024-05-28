#ifndef SNAPSHOT_PIPELINE_H_
#define SNAPSHOT_PIPELINE_H_

#include "base_record_pipeline.h"

class SnapshotPipeline : public BaseRecordPipeline
{
public:
    SnapshotPipeline() { pipelineType = "Snapshot"; }
    bool launch() override;
};

#endif // SNAPSHOT_PIPELINE_H_
