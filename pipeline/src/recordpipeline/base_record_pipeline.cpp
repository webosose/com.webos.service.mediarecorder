#include "base_record_pipeline.h"
#include "glog.h"
#include "message.h"
#include <iomanip>
#include <pbnjson.hpp>
#include <system_error>

BaseRecordPipeline::BaseRecordPipeline()
{
    LOGI("start");

    GMainContext *c = g_main_context_new();
    loop_           = g_main_loop_new(c, false);

    try
    {
        loopThread_ = std::make_shared<std::thread>(g_main_loop_run, loop_);
    }
    catch (const std::system_error &e)
    {
        LOGE("Caught a system_error with code %d meaning %s", e.code().value(), e.what());
    }

    pthread_setname_np(loopThread_->native_handle(), "record-pipeline");

    while (!g_main_loop_is_running(loop_))
    {
    }
    g_main_context_unref(c);

    LOGI("end");
}

BaseRecordPipeline::~BaseRecordPipeline()
{
    LOGI("start");

    Unload();

    g_main_loop_quit(loop_);
    if (loopThread_->joinable())
    {
        try
        {
            loopThread_->join();
        }
        catch (const std::system_error &e)
        {
            LOGE("Caught a system_error with code %d meaning %s", e.code().value(), e.what());
        }
    }
    g_main_loop_unref(loop_);

    LOGI("end");
}

bool BaseRecordPipeline::Load(const std::string &msg)
{
    GRPASSERT(!msg.empty());
    LOGI("start: %s", msg.c_str());

    SetGstreamerDebug();
    gst_init(NULL, NULL);

    ParseOptionString(msg);

    if (GetSourceInfo())
    {
        if (!acquireResource())
        {
            LOGE("resouce acquire failed!");
            return false;
        }

        NotifySourceInfo();
    }

    // 0. Check sanity.
    if (pipeline_ != nullptr)
    {
        LOGE("Error. pipeline already exists.");
        return false;
    }

    // 1. Build pipeline and launch.
    if (!launch())
    {
        LOGE("Pipeline launch fail");
        return false;
    }

    // 2. Get Bus.
    if (!addBus())
    {
        Unload();
        return false;
    }

    // 3. Ready for record.
    BaseRecordPipeline::Pause();

    LOGI("end");
    return true;
}

bool BaseRecordPipeline::Unload()
{
    LOGI("start");

    if (pipeline_ == nullptr)
    {
        LOGW("Pipeline is not loaded.");
        return false;
    }

    auto bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline_));

    if (pipelineType != "Snapshot")
    {
        GstState state;
        gst_element_get_state(pipeline_, &state, nullptr, GST_CLOCK_TIME_NONE);
        LOGI("state = %s", gst_element_state_get_name(state));
        if (state == GST_STATE_PAUSED)
        {
            Play();
        }

        LOGI("Send EOS");
        gst_element_send_event(pipeline_, gst_event_new_eos());

        if (bus != nullptr)
        {
            LOGI("Wait for EOS");
            auto msg = gst_bus_timed_pop_filtered(
                bus, GST_CLOCK_TIME_NONE,
                static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
            if (msg != NULL)
            {
                gst_message_unref(msg);
            }
        }
    }

    LOGI("Unload pipeline");
    bool ret = gst_element_set_state(pipeline_, GST_STATE_NULL);
    if (ret == GST_STATE_CHANGE_FAILURE)
    {
        LOGW("Failed to change pipeline state to NULL");
        LOGW("Keep doing rest of unload procedure.");
    }

    GstState state;
    gst_element_get_state(pipeline_, &state, nullptr, GST_CLOCK_TIME_NONE);
    LOGI("state = %s", gst_element_state_get_name(state));
    if (state == GST_STATE_NULL)
    {
        LOGI("Unload completed");
    }

    if (remBus())
    {
        if (bus != nullptr)
        {
            gst_bus_remove_watch(bus);
        }
    }

    gst_object_unref(bus);

    LOGI("Delete pipeline");
    gst_object_unref(pipeline_);
    pipeline_ = nullptr;

    LOGI("end");
    return true;
}

bool BaseRecordPipeline::Play()
{
    LOGI("start");
    if (pipeline_ != nullptr &&
        gst_element_set_state(pipeline_, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE)
    {
        LOGE("Failed to change pipeline state to PLAYING");
        return false;
    }

    LOGI("end");
    return true;
}

bool BaseRecordPipeline::Pause()
{
    LOGI("start");
    if (pipeline_ != nullptr &&
        gst_element_set_state(pipeline_, GST_STATE_PAUSED) == GST_STATE_CHANGE_FAILURE)
    {
        LOGE("Failed to change pipeline state to PAUSED");
        return false;
    }

    LOGI("end");
    return true;
}

void BaseRecordPipeline::RegisterCbFunction(CALLBACK_T cbf)
{
    LOGI("start");
    cbFunction_ = std::move(cbf);
}

bool BaseRecordPipeline::addBus()
{
    if (pipeline_ == nullptr)
        return false;
    LOGI("start");

    auto bus = gst_element_get_bus(pipeline_);
    if (!bus)
    {
        LOGE("Error. Fail gst_elememt_get_bus!");
        return false;
    }

    auto s = gst_bus_create_watch(bus);
    g_source_set_callback(
        s,
        (GSourceFunc) + [](GstBus *bus, GstMessage *msg, gpointer data) -> gboolean
        {
            BaseRecordPipeline *p = static_cast<BaseRecordPipeline *>(data);
            return p->handleBusMessage(bus, msg);
        },
        this, nullptr);
    g_source_attach(s, g_main_loop_get_context(loop_));
    busId_ = g_source_get_id(s);
    g_source_unref(s);

    gst_object_unref(bus);

    LOGI("end");
    return true;
}

bool BaseRecordPipeline::remBus()
{
    if (busId_ == 0)
        return false;
    LOGI("start");

    GSource *s = g_main_context_find_source_by_id(g_main_loop_get_context(loop_), busId_);
    if (s != nullptr)
        g_source_destroy(s);

    busId_ = 0;

    LOGI("end");
    return true;
}

bool BaseRecordPipeline::acquireResource()
{
    LOGI("start");
    ACQUIRE_RESOURCE_INFO_T resource_info;
    resource_info.sourceInfo  = &source_info_;
    resource_info.displayMode = const_cast<char *>(display_mode_.c_str());
    resource_info.result      = true;

    if (cbFunction_)
        cbFunction_(GRP_NOTIFY_ACQUIRE_RESOURCE, display_path_, nullptr,
                    static_cast<void *>(&resource_info));

    if (!resource_info.result)
    {
        LOGE("resouce acquire fail!");
        return false;
    }

    return true;
}

bool BaseRecordPipeline::GetSourceInfo()
{
    base::video_info_t video_stream_info = {};

    if (!mVideoFormat.empty())
    {
        video_stream_info.width          = mVideoFormat.width;
        video_stream_info.height         = mVideoFormat.height;
        video_stream_info.encode         = VIDEO_CODEC_H264;
        video_stream_info.frame_rate.num = mVideoFormat.fps;
        video_stream_info.frame_rate.den = 1;
    }
    else if (!mImageFormat.empty())
    {
        video_stream_info.width          = mImageFormat.width;
        video_stream_info.height         = mImageFormat.height;
        video_stream_info.encode         = VIDEO_CODEC_MJPEG;
        video_stream_info.frame_rate.num = 1;
        video_stream_info.frame_rate.den = 1;
    }
    else
    {
        //[ToDo] No audio info provided currently.
        return false;
    }

    LOGI("[video info] width: %d, height: %d, frameRate: %d/%d", video_stream_info.width,
         video_stream_info.height, video_stream_info.frame_rate.num,
         video_stream_info.frame_rate.den);

    base::program_info_t program;
    program.video_stream = 1;
    source_info_.programs.push_back(program);

    source_info_.video_streams.push_back(video_stream_info);

    return true;
}

void BaseRecordPipeline::NotifySourceInfo()
{
    LOGI("start");

    // TODO(anonymous): Support multiple video/audio stream case
    if (cbFunction_)
        cbFunction_(GRP_NOTIFY_SOURCE_INFO, 0, nullptr, &source_info_);
}

bool BaseRecordPipeline::handleBusMessage(GstBus *bus, GstMessage *msg)
{
    auto msgType = GST_MESSAGE_TYPE(msg);
    if (msgType != GST_MESSAGE_QOS && msgType != GST_MESSAGE_TAG)
    {
        LOGI("Element[ %s ][ %d ][ %s ]", GST_MESSAGE_SRC_NAME(msg), msgType,
             gst_message_type_get_name(msgType));
    }

    switch (GST_MESSAGE_TYPE(msg))
    {
    case GST_MESSAGE_ERROR:
    {
        LOGE("Got Error");
        base::error_t error = HandleErrorMessage(msg);
        if (cbFunction_)
            cbFunction_(GRP_NOTIFY_ERROR, 0, nullptr, &error);
        GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline_), GST_DEBUG_GRAPH_SHOW_VERBOSE, "grp_error");
        break;
    }
    case GST_MESSAGE_EOS:
    {
        LOGI("Got EOS");
        if (cbFunction_)
            cbFunction_(GRP_NOTIFY_END_OF_STREAM, 0, nullptr, nullptr);
        break;
    }
    case GST_MESSAGE_ASYNC_DONE:
    {
        LOGI("Got AsyncDone");
        break;
    }
    case GST_MESSAGE_STATE_CHANGED:
    {
        if (GST_MESSAGE_SRC(msg) != GST_OBJECT_CAST(pipeline_))
            break;

        GstState oldState = GST_STATE_NULL;
        GstState newState = GST_STATE_NULL;
        gst_message_parse_state_changed(msg, &oldState, &newState, nullptr);
        LOGI("Element[%s] State changed ...%s -> %s", GST_MESSAGE_SRC_NAME(msg),
             gst_element_state_get_name(oldState), gst_element_state_get_name(newState));

        if (newState == GST_STATE_PAUSED && oldState < GST_STATE_PAUSED)
        {
            LOGI("post loadcompleted event");
            if (cbFunction_)
                cbFunction_(GRP_NOTIFY_LOAD_COMPLETED, 0, nullptr, nullptr);
        }
        else if (newState == GST_STATE_PLAYING)
        {
            LOGI("post playing event");
            if (cbFunction_)
                cbFunction_(GRP_NOTIFY_PLAYING, 0, nullptr, nullptr);
            GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline_), GST_DEBUG_GRAPH_SHOW_VERBOSE, "grp_play");
        }
        else if (newState == GST_STATE_PAUSED && oldState == GST_STATE_PLAYING)
        {
            LOGI("post paused event");
            if (cbFunction_)
                cbFunction_(GRP_NOTIFY_PAUSED, 0, nullptr, nullptr);
        }
        //[TODO] Can not post this becaus UMS kills the process first.
        else if (newState == GST_STATE_NULL && oldState >= GST_STATE_PAUSED)
        {
            LOGI("post unloadcompleted event");
            if (cbFunction_)
                cbFunction_(GRP_NOTIFY_UNLOAD_COMPLETED, 0, nullptr, nullptr);
        }
        break;
    }
    case GST_MESSAGE_APPLICATION:
    {
        const GstStructure *gStruct = gst_message_get_structure(msg);

        /* video-info message comes from sink element */
        if (gst_structure_has_name(gStruct, "video-info"))
        {
            LOGI("got video-info message");
            base::video_info_t video_info;
            memset(&video_info, 0, sizeof(base::video_info_t));
            gint width, height, fps_n, fps_d, par_n = -1, par_d = -1;
            gst_structure_get_int(gStruct, "width", &width);
            gst_structure_get_int(gStruct, "height", &height);
            gst_structure_get_fraction(gStruct, "framerate", &fps_n, &fps_d);
            gst_structure_get_int(gStruct, "par_n", &par_n);
            gst_structure_get_int(gStruct, "par_d", &par_d);

            LOGI("width[%d], height[%d], framerate[%d/%d],"
                 "pixel_aspect_ratio[%d/%d]",
                 width, height, fps_n, fps_d, par_n, par_d);

            video_info.width          = width;
            video_info.height         = height;
            video_info.frame_rate.num = fps_n;
            video_info.frame_rate.den = fps_d;
            // TODO: we already know this info. but it's not used now.
            video_info.bit_rate = 0;
            video_info.codec    = 0;

            if (cbFunction_)
                cbFunction_(GRP_NOTIFY_VIDEO_INFO, 0, nullptr, &video_info);
        }
        else if (gst_structure_has_name(gStruct, "request-resource"))
        {
            LOGI("got request-resource message");
        }
        break;
    }
    default:
        break;
    }

    return true;
}

void BaseRecordPipeline::ParseOptionString(const std::string &options)
{
    LOGI("option string: %s", options.c_str());
    pbnjson::JDomParser jdparser;
    if (!jdparser.parse(options, pbnjson::JSchema::AllSchema()))
    {
        LOGE("ERROR JDomParser.parse. msg: %s ", options.c_str());
        return;
    }
    pbnjson::JValue parsed = jdparser.getDom();

    if (parsed.hasKey("uri"))
    {
        uri_ = parsed["uri"].asString();
    }
    else
    {
        LOGI("UMS_INTERNAL_API_VERSION is not version 2.");
        LOGI("Please check the UMS_INTERNAL_API_VERSION in ums.");
        GRPASSERT(0);
    }

    if (parsed["options"]["option"].hasKey("displayPath"))
    {
        int32_t display_path = parsed["options"]["option"]["displayPath"].asNumber<int32_t>();
        display_path_        = (display_path > GRP_SECONDARY_DISPLAY ? 0 : display_path);
    }
    if (parsed["options"]["option"].hasKey("videoDisplayMode"))
    {
        display_mode_ = parsed["options"]["option"]["videoDisplayMode"].asString();
    }
    if (parsed["options"]["option"].hasKey("format"))
    {
        format_ = parsed["options"]["option"]["format"].asString();
    }
    if (parsed["options"]["option"].hasKey("path"))
    {
        path_ = parsed["options"]["option"]["path"].asString();
    }

    pbnjson::JValue video = parsed["options"]["option"]["video"];
    if (video.isObject())
    {
        video_src_ = video["videoSrc"].asString();

        mVideoFormat         = {"H264", 1280, 720, 30, 200000};
        mVideoFormat.codec   = video["codec"].asString();
        mVideoFormat.width   = video["width"].asNumber<int>();
        mVideoFormat.height  = video["height"].asNumber<int>();
        mVideoFormat.fps     = video["fps"].asNumber<int>();
        mVideoFormat.bitRate = video["bitRate"].asNumber<int>();

        LOGI("=== video ===");
        LOGI("videoSrc : %s", video_src_.c_str());
        LOGI("width : %d", mVideoFormat.width);
        LOGI("height : %d", mVideoFormat.height);
        LOGI("codec : %s", mVideoFormat.codec.c_str());
        LOGI("fps: %d", mVideoFormat.fps);
        LOGI("bitRate : %d", mVideoFormat.bitRate);
    }

    pbnjson::JValue audio = parsed["options"]["option"]["audio"];
    if (audio.isObject())
    {
        mAudioFormat            = {"AAC", 44100, 2, 0};
        mAudioFormat.codec      = audio["codec"].asString();
        mAudioFormat.sampleRate = audio["sampleRate"].asNumber<int>();
        mAudioFormat.channels   = audio["channelCount"].asNumber<int>();
        mAudioFormat.bitRate    = audio["bitRate"].asNumber<int>();

        LOGI("=== audio ===");
        LOGI("codec : %s", mAudioFormat.codec.c_str());
        LOGI("sampleRate : %d", mAudioFormat.sampleRate);
        LOGI("channelCount : %d", mAudioFormat.channels);
        LOGI("bitRate : %d", mAudioFormat.bitRate);
    }

    pbnjson::JValue image = parsed["options"]["option"]["image"];
    if (image.isObject())
    {
        video_src_ = image["videoSrc"].asString();

        mImageFormat         = {"JPEG", 1280, 720, 90};
        mImageFormat.codec   = image["codec"].asString();
        mImageFormat.width   = image["width"].asNumber<int>();
        mImageFormat.height  = image["height"].asNumber<int>();
        mImageFormat.quality = image["quality"].asNumber<int>();

        LOGI("=== image ===");
        LOGI("videoSrc : %s", video_src_.c_str());
        LOGI("codec : %s", mImageFormat.codec.c_str());
        LOGI("width : %d", mImageFormat.width);
        LOGI("height : %d", mImageFormat.height);
        LOGI("quality: %d", mImageFormat.quality);
    }

    LOGI("=== file ===");
    if (!format_.empty())
        LOGI("format : %s", format_.c_str());
    LOGI("path : %s", path_.c_str());
}

void BaseRecordPipeline::SetGstreamerDebug()
{
    pbnjson::JValue parsed = pbnjson::JDomParser::fromFile("/etc/g-record-pipeline/gst_debug.conf");
    if (!parsed.isObject())
    {
        LOGE("Gst debug file parsing error");
    }

    pbnjson::JValue debug = parsed["gst_debug"];
    int size              = debug.arraySize();
    for (int i = 0; i < size; i++)
    {
        const char *kDebug     = "GST_DEBUG";
        const char *kDebugFile = "GST_DEBUG_FILE";
        const char *kDebugDot  = "GST_DEBUG_DUMP_DOT_DIR";
        if (debug[i].hasKey(kDebug) && !debug[i][kDebug].asString().empty())
            setenv(kDebug, debug[i][kDebug].asString().c_str(), 1);
        if (debug[i].hasKey(kDebugFile) && !debug[i][kDebugFile].asString().empty())
            setenv(kDebugFile, debug[i][kDebugFile].asString().c_str(), 1);
        if (debug[i].hasKey(kDebugDot) && !debug[i][kDebugDot].asString().empty())
            setenv(kDebugDot, debug[i][kDebugDot].asString().c_str(), 1);
    }
}

int32_t BaseRecordPipeline::ConvertErrorCode(GQuark domain, gint code)
{
    int32_t converted = MEDIA_MSG_ERR_PLAYING;

    if (GST_CORE_ERROR == domain)
    {
        switch (code)
        {
        case MEDIA_MSG_GST_CORE_ERROR_EVENT:
            converted = MEDIA_MSG_GST_CORE_ERROR_EVENT;
            break;
        default:
            break;
        }
    }
    else if (GST_LIBRARY_ERROR == domain)
    {
        // do nothing
    }
    else if (GST_RESOURCE_ERROR == domain)
    {
        switch (code)
        {
        case GST_RESOURCE_ERROR_SETTINGS:
            converted = MEDIA_MSG_GST_RESOURCE_ERROR_SETTINGS;
            break;
        case GST_RESOURCE_ERROR_NOT_FOUND:
            converted = MEDIA_MSG_GST_RESOURCE_ERROR_NOT_FOUND;
            break;
        case GST_RESOURCE_ERROR_OPEN_READ:
            converted = MEDIA_MSG_GST_RESOURCE_ERROR_OPEN_READ;
            break;
        case GST_RESOURCE_ERROR_READ:
            converted = MEDIA_MSG_GST_RESOURCE_ERROR_READ;
            break;
        default:
            break;
        }
    }
    else if (GST_STREAM_ERROR == domain)
    {
        switch (code)
        {
        case GST_STREAM_ERROR_TYPE_NOT_FOUND:
            converted = MEDIA_MSG_GST_STREAM_ERROR_TYPE_NOT_FOUND;
            break;
        case GST_STREAM_ERROR_DEMUX:
            converted = MEDIA_MSG_GST_STREAM_ERROR_DEMUX;
            break;
        default:
            break;
        }
    }
    return converted;
}

base::error_t BaseRecordPipeline::HandleErrorMessage(GstMessage *message)
{
    GError *err = NULL;
    gchar *debug_info;
    gst_message_parse_error(message, &err, &debug_info);
    GQuark domain = err->domain;

    base::error_t error;
    error.errorCode = ConvertErrorCode(domain, (gint)err->code);
    error.errorText = g_strdup(err->message) ? g_strdup(err->message) : "";

    LOGI("[GST_MESSAGE_ERROR][domain:%s][from:%s][code:%d]"
         "[converted:%d][msg:%s]",
         g_quark_to_string(domain), (GST_OBJECT_NAME(GST_MESSAGE_SRC(message))), err->code,
         error.errorCode, err->message);
    LOGI("Debug information: %s", debug_info ? debug_info : "none");

    g_clear_error(&err);
    g_free(debug_info);

    return error;
}
