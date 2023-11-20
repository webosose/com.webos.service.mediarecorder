#include "base_record_pipeline.h"
#include "glog.h"
#include "message.h"
#include <iomanip>
#include <pbnjson.hpp>
#include <sys/time.h>
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
    LOGI("format_ : %s", format_.c_str());
    LOGI("width_ : %d", width_);
    LOGI("height_ : %d", height_);
    LOGI("framerate_: %d", framerate_);
    LOGI("memtype_ : %s", memtype_.c_str());
    LOGI("memsrc_ : %s", memsrc_.c_str());
    LOGI("camera_id_ : %s", camera_id_.c_str());

    if (!GetSourceInfo())
    {
        LOGI("get source information failed!");
        return false;
    }

    if (!acquireResource())
    {
        LOGE("resouce acquire failed!");
        return false;
    }

    NotifySourceInfo();

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
    Pause();

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

    LOGI("Send EOS");
    gst_element_send_event(pipeline_, gst_event_new_eos());

    auto bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline_));
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
        gst_bus_remove_watch(bus);

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

    video_stream_info.width          = width_;
    video_stream_info.height         = height_;
    video_stream_info.decode         = VIDEO_CODEC_MJPEG;
    video_stream_info.encode         = VIDEO_CODEC_H264;
    video_stream_info.frame_rate.num = framerate_;
    video_stream_info.frame_rate.den = 1;
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
    if (parsed["options"]["option"].hasKey("windowId"))
    {
        window_id_ = parsed["options"]["option"]["windowId"].asString();
    }
    if (parsed["options"]["option"].hasKey("handle"))
    {
        handle_ = parsed["options"]["option"]["handle"].asNumber<int>();
    }
    if (parsed["options"]["option"].hasKey("videoDisplayMode"))
    {
        display_mode_ = parsed["options"]["option"]["videoDisplayMode"].asString();
    }
    if (parsed["options"]["option"].hasKey("format"))
    {
        format_ = parsed["options"]["option"]["format"].asString();
    }
    if (parsed["options"]["option"].hasKey("width"))
    {
        width_ = parsed["options"]["option"]["width"].asNumber<int>();
    }
    if (parsed["options"]["option"].hasKey("height"))
    {
        height_ = parsed["options"]["option"]["height"].asNumber<int>();
    }
    if (parsed["options"]["option"].hasKey("frameRate"))
    {
        framerate_ = parsed["options"]["option"]["frameRate"].asNumber<int>();
    }
    if (parsed["options"]["option"].hasKey("memType"))
    {
        memtype_ = parsed["options"]["option"]["memType"].asString();
    }
    if (parsed["options"]["option"].hasKey("memSrc"))
    {
        memsrc_ = parsed["options"]["option"]["memSrc"].asString();
    }
    if (parsed["options"]["option"].hasKey("cameraId"))
    {
        camera_id_ = parsed["options"]["option"]["cameraId"].asString();
    }

    LOGI("uri: %s, display-path: %d, window_id: %s, display_mode: %s", uri_.c_str(), display_path_,
         window_id_.c_str(), display_mode_.c_str());
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

bool BaseRecordPipeline::isSupportedExtension(const std::string &extension) const
{
    std::string lowercaseExtension = extension;
    std::transform(lowercaseExtension.begin(), lowercaseExtension.end(), lowercaseExtension.begin(),
                   ::tolower);

    return (lowercaseExtension == "mp4") || (lowercaseExtension == "m4a") ||
           (lowercaseExtension == "jpg") || (lowercaseExtension == "jpeg");
}

std::string BaseRecordPipeline::createRecordFileName(const std::string &recordpath) const
{
    auto path = recordpath;
    if (path.empty())
        path = "/media/internal";

    // Find the file extension to check if file name is provided or path is provided
    std::size_t position  = path.find_last_of(".");
    std::string extension = path.substr(position + 1);

    if (!isSupportedExtension(extension))
    {
        // Check if specified location ends with '/' else add
        char ch = path.back();
        if (ch != '/')
            path += "/";

        std::time_t now  = std::time(nullptr);
        std::tm *timePtr = std::localtime(&now);
        if (timePtr == nullptr)
        {
            LOGE("failed to get local time");
            return "";
        }

        struct timeval tmnow;
        gettimeofday(&tmnow, NULL);

        std::string prefix, ext;
        if (pipelineType == "VideoRecord")
        {
            prefix = "Record";
            ext    = "mp4";
        }
        else if (pipelineType == "AudioRecord")
        {
            prefix = "Audio";
            ext    = "m4a";
        }
        else if (pipelineType == "Snapshot")
        {
            prefix = "Capture";
            ext    = "jpeg";
        }
        else
        {
            LOGE("Invalid pipeline type");
            return "";
        }

        // prefix + "DDMMYYYY-HHMMSSss" + "." + ext
        std::ostringstream oss;
        oss << prefix << std::setw(2) << std::setfill('0') << timePtr->tm_mday << std::setw(2)
            << std::setfill('0') << (timePtr->tm_mon) + 1 << std::setw(2) << std::setfill('0')
            << (timePtr->tm_year) + 1900 << "-" << std::setw(2) << std::setfill('0')
            << timePtr->tm_hour << std::setw(2) << std::setfill('0') << timePtr->tm_min
            << std::setw(2) << std::setfill('0') << timePtr->tm_sec << std::setw(2)
            << std::setfill('0') << tmnow.tv_usec / 10000;

        path += oss.str();
        path += "." + ext;
    }

    LOGI("path : %s", path.c_str());
    return path;
}
