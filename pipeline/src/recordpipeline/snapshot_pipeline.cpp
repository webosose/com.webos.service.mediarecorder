#include "snapshot_pipeline.h"
#include "element_factory.h"
#include "glog.h"

bool SnapshotPipeline::launch()
{
    LOGI("start");

    // 1. Build pipeline description and launch.
    gchar *content = nullptr;
    std::string pipeline_desc;
    const char *snapshot_pipeline_path = "/etc/g-record-pipeline/snapshot_pipeline";

    if (g_file_get_contents(snapshot_pipeline_path, &content, nullptr, nullptr))
    {
        pipeline_desc = std::string(content);
        g_free(content);
    }
    else
    {
        std::string socketPath = "/tmp/" + video_src_;
        pipeline_desc          = "shmsrc socket-path=" + socketPath + " num-buffers=1";
        pipeline_desc += " ! video/x-raw, width=" + std::to_string(mImageFormat.width) +
                         ", height=" + std::to_string(mImageFormat.height) +
                         ", format=RGB16, framerate=0/1";

        pipeline_desc += " ! videoconvert";
        std::string element =
            ElementFactory::GetPreferredElementName(pipelineType, "snapshot-encoder");
        if (!element.empty())
            pipeline_desc += " ! " + element + " name=encoder";

        element = ElementFactory::GetPreferredElementName(pipelineType, "snapshot-sink");
        if (!element.empty())
            pipeline_desc += " ! " + element + " name=sink";
    }

    LOGI("pipeline : %s", pipeline_desc.c_str());

    pipeline_ = gst_parse_launch(pipeline_desc.c_str(), NULL);
    if (pipeline_ == NULL)
    {
        LOGI("Error. Pipeline is NULL");
        return false;
    }

    // 2. Setup snapshot encoder
    auto encoder = gst_bin_get_by_name(GST_BIN(pipeline_), "encoder");
    if (encoder)
    {
        g_object_set(encoder, "quality", mImageFormat.quality, nullptr);
    }

    // 3. Setup sink
    auto sink = gst_bin_get_by_name(GST_BIN(pipeline_), "sink");
    if (sink)
    {
        g_object_set(sink, "location", path_.c_str(), nullptr);
    }

    LOGI("end");
    return true;
}
