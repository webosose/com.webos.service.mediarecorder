#ifndef PIPELINE_FACTORY_H_
#define PIPELINE_FACTORY_H_

#include <memory>
#include <pbnjson.hpp>

class RecordPipeline;
class PipelineFactory
{
public:
    static std::shared_ptr<RecordPipeline> CreateRecorder(const pbnjson::JValue &parsed);
};
#endif // PIPELINE_FACTORY_H_
