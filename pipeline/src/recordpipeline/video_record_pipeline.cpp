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
        std::string socketPath = "/tmp/" + video_src_;
        std::string filePath   = createRecordFileName(path_);

        pipeline_desc =
            "shmsrc socket-path=" + socketPath + " is-live=true do-timestamp=true name=videoSrc";

        pipeline_desc += " ! video/x-raw, width=" + std::to_string(mVideoFormat.width) +
                         ", height=" + std::to_string(mVideoFormat.height) +
                         ", format=RGB16, framerate=0/1, colorimetry=1:1:5:1";
        pipeline_desc +=
            " ! v4l2h264enc ! capsfilter name=videoEnc ! h264parse ! queue ! qtmux name=mux";
        pipeline_desc += " ! filesink sync=true location=" + filePath;

        // for audio
        pipeline_desc += " pulsesrc do_timestamp=false ! queue";

        std::string element =
            ElementFactory::GetPreferredElementName(pipelineType, "audio-converter");
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

        pipeline_desc += " ! mux.";
    }

    LOGI("pipeline : %s", pipeline_desc.c_str());

    pipeline_ = gst_parse_launch(pipeline_desc.c_str(), NULL);
    if (pipeline_ == NULL)
    {
        LOGI("Error. Pipeline is NULL");
        return false;
    }

    // 2. Setup encoder
    auto caps_h264 = gst_bin_get_by_name(GST_BIN(pipeline_), "videoEnc");
    if (caps_h264)
    {
        auto caps = gst_caps_new_simple("video/x-h264", "level", G_TYPE_STRING, "4", nullptr);

        g_object_set(caps_h264, "caps", caps, nullptr);
    }

    // 3. Setup audio capsfilter
    auto audio_caps = gst_bin_get_by_name(GST_BIN(pipeline_), "audioCaps");
    if (audio_caps)
    {
        auto caps = gst_caps_new_simple("audio/x-raw", "rate", G_TYPE_INT, mAudioFormat.sampleRate,
                                        "channels", G_TYPE_INT, mAudioFormat.channels, nullptr);
        g_object_set(audio_caps, "caps", caps, nullptr);
    }

    // 4. Setup audio encoder
    auto audio_enc = gst_bin_get_by_name(GST_BIN(pipeline_), "audioEnc");
    if (audio_enc)
    {
        g_object_set(audio_enc, "bitrate", mAudioFormat.bitRate, nullptr);
    }

    LOGI("end");
    return true;
}

bool VideoRecordPipeline::Pause()
{
    LOGI("start");

    bool ret = BaseRecordPipeline::Pause();

    // Reconfigure shmsrc
    auto src = gst_bin_get_by_name(GST_BIN(pipeline_), "videoSrc");
    gst_element_set_state(src, GST_STATE_PAUSED);

    gst_element_set_state(src, GST_STATE_NULL);
    gst_element_set_state(src, GST_STATE_PAUSED);

    LOGI("end");
    return ret;
}

bool VideoRecordPipeline::Unload()
{
    LOGI("start");

    GstState state;
    gst_element_get_state(pipeline_, &state, nullptr, GST_CLOCK_TIME_NONE);
    LOGI("state = %s", gst_element_state_get_name(state));
    if (state == GST_STATE_PAUSED)
    {
        Play();
    }

    bool ret = BaseRecordPipeline::Unload();

    LOGI("end");
    return ret;
}
