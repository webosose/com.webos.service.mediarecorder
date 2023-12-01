#include "audio_record_pipeline.h"
#include "element_factory.h"
#include "glog.h"

bool AudioRecordPipeline::launch()
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
        std::string element = ElementFactory::GetPreferredElementName(pipelineType, "audio-src");
        if (!element.empty())
            pipeline_desc = element + " ! queue";
        else
            pipeline_desc = "pulsesrc ! queue";

        element = ElementFactory::GetPreferredElementName(pipelineType, "audio-converter");
        if (!element.empty())
            pipeline_desc += " ! " + element + " ! capsfilter name=audioCaps";
        else
            pipeline_desc += " ! audioconvert ! capsfilter name=audioCaps";

        if (mAudioFormat.audioCodec == "AAC")
        {
            element = ElementFactory::GetPreferredElementName(pipelineType, "audio-encoder-aac");
            if (!element.empty())
                pipeline_desc += " ! " + element + " name=audioEnc";
            else
                pipeline_desc += " ! avenc_aac name=audioEnc";
        }
        else
        {
            pipeline_desc += " ! avenc_aac name=audioEnc";
        }

        element = ElementFactory::GetPreferredElementName(pipelineType, "audio-mux");
        if (!element.empty())
            pipeline_desc += " ! " + element;
        else
            pipeline_desc += " ! mp4mux";

        element = ElementFactory::GetPreferredElementName(pipelineType, "audio-sink");
        if (!element.empty())
            pipeline_desc += " ! " + element + " name=audioSink";
        else
            pipeline_desc += " ! filesink name=audioSink";
    }

    LOGI("pipeline : %s", pipeline_desc.c_str());

    pipeline_ = gst_parse_launch(pipeline_desc.c_str(), NULL);
    if (pipeline_ == NULL)
    {
        LOGI("Error. Pipeline is NULL");
        return false;
    }

    // 2. Setup capsfilter
    auto audio_caps = gst_bin_get_by_name(GST_BIN(pipeline_), "audioCaps");
    if (audio_caps)
    {
        auto caps = gst_caps_new_simple("audio/x-raw", "rate", G_TYPE_INT, mAudioFormat.sampleRate,
                                        "channels", G_TYPE_INT, mAudioFormat.channels, nullptr);

        g_object_set(audio_caps, "caps", caps, nullptr);
    }

    // 3. Setup encoder
    auto audio_enc = gst_bin_get_by_name(GST_BIN(pipeline_), "audioEnc");
    if (audio_enc)
    {
        g_object_set(audio_enc, "bitrate", mAudioFormat.bitRate, nullptr);
    }

    // 4. Setup sink
    std::string filePath = createRecordFileName(path_);
    auto audio_sink      = gst_bin_get_by_name(GST_BIN(pipeline_), "audioSink");
    if (audio_sink)
    {
        g_object_set(audio_sink, "location", filePath.c_str(), nullptr);
    }

    LOGI("end");
    return true;
}
