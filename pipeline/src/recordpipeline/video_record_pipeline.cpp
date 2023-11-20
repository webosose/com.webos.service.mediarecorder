#include "video_record_pipeline.h"
#include "element_factory.h"
#include "glog.h"

bool VideoRecordPipeline::launch()
{
    LOGI("start");

    // 1. Build pipeline description and launch.
    gchar *content = nullptr;
    std::string pipeline_desc;
    if (g_file_get_contents(record_pipeline_path.c_str(), &content, nullptr, nullptr))
    {
        pipeline_desc = std::string(content);
        g_free(content);
    }
    else
    {
        std::string element = ElementFactory::GetPreferredElementName(pipelineType, "record-src");
        if (!element.empty())
            pipeline_desc = element + " name=src";

        pipeline_desc += " ! waylandsink";
    }

    LOGI("pipeline : %s", pipeline_desc.c_str());

    pipeline_ = gst_parse_launch(pipeline_desc.c_str(), NULL);
    if (pipeline_ == NULL)
    {
        LOGI("Error. Pipeline is NULL");
        return false;
    }

    // 2. Setup src
    auto src = gst_bin_get_by_name(GST_BIN(pipeline_), "src");
    if (src)
    {
        ElementFactory::SetProperties(pipelineType, src, "record-src");
    }

    LOGI("end");
    return true;
}
