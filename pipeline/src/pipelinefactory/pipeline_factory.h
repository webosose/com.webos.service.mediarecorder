#ifndef _PIPELINE_FACTORY_H_
#define _PIPELINE_FACTORY_H_

#include <memory>
#include <pbnjson.hpp>

class RecordPipeline;
class PipelineFactory
{
public:
    static std::shared_ptr<RecordPipeline> CreateRecoder(const pbnjson::JValue &parsed);
};
#endif // _PIPELINE_FACTORY_H_
