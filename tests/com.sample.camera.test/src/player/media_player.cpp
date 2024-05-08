#include "media_player.h"
#include "base.h"
#include "camera_types.h"
#include "log_info.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <system_error>

using namespace nlohmann;

MediaPlayer::MediaPlayer()
{
    DEBUG_LOG("start");

    GMainContext *c = g_main_context_new();
    loop_           = g_main_loop_new(c, false);

    try
    {
        loopThread_ = std::make_unique<std::thread>(g_main_loop_run, loop_);
    }
    catch (const std::system_error &e)
    {
        ERROR_LOG("Caught a system_error with code %d meaning %s", e.code().value(), e.what());
    }

    pthread_setname_np(loopThread_->native_handle(), "media-player");

    while (!g_main_loop_is_running(loop_))
    {
    }
    g_main_context_unref(c);
}

MediaPlayer::~MediaPlayer()
{
    DEBUG_LOG("start");

    unload();

    g_main_loop_quit(loop_);
    if (loopThread_->joinable())
    {
        try
        {
            loopThread_->join();
        }
        catch (const std::system_error &e)
        {
            ERROR_LOG("Caught a system_error with code %d meaning %s", e.code().value(), e.what());
        }
    }
    g_main_loop_unref(loop_);
}

// gst-launch-1.0 playbin uri=file:///media/internal/Record22032024-02023395.mp4
// video-sink="waylandsink"
bool MediaPlayer::load(const std::string &windowId, const std::string &video_name)
{
    DEBUG_LOG("start: %s", video_name.c_str());

    SetGstreamerDebug();
    gst_init(NULL, NULL);

    window_id_ = windowId;
    if (!attachSurface(true))
    {
        ERROR_LOG("attachSurface() failed");
        return false;
    }

    // 1. Build pipeline description and launch.
    std::string pipeline_desc;
    pipeline_desc = "playbin uri=file://" + video_name;
    pipeline_desc += " video-sink=\"waylandsink\"";

    DEBUG_LOG("pipeline : %s", pipeline_desc.c_str());

    pipeline_ = gst_parse_launch(pipeline_desc.c_str(), NULL);
    if (pipeline_ == NULL)
    {
        ERROR_LOG("Error. Pipeline is NULL");
        return false;
    }

    // Get Bus.
    if (!addBus())
    {
        unload();
        return false;
    }

    auto bus = gst_element_get_bus(pipeline_);
    gst_bus_set_sync_handler(
        bus,
        (GstBusSyncHandler) + [](GstBus *bus, GstMessage *message, gpointer data) -> GstBusSyncReply
        {
            MediaPlayer *p = static_cast<MediaPlayer *>(data);
            return p->handleBusSyncMessage(bus, message);
        },
        this, nullptr);

    gst_object_unref(bus);

    // Ready for playback.
    pause();

    DEBUG_LOG("end");
    return true;
}

bool MediaPlayer::play()
{
    DEBUG_LOG("start");
    if (pipeline_ != nullptr &&
        gst_element_set_state(pipeline_, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE)
    {
        ERROR_LOG("Failed to change pipeline state to PLAYING");
        return false;
    }

    state = MediaState::PLAY;
    return true;
}

bool MediaPlayer::pause()
{
    DEBUG_LOG("start");
    if (pipeline_ != nullptr &&
        gst_element_set_state(pipeline_, GST_STATE_PAUSED) == GST_STATE_CHANGE_FAILURE)
    {
        ERROR_LOG("Failed to change pipeline state to PLAYING");
        return false;
    }

    state = MediaState::PAUSE;
    return true;
}

bool MediaPlayer::unload()
{
    DEBUG_LOG("start");

    if (pipeline_ == nullptr)
    {
        WARNING_LOG("Pipeline is not loaded.");
        return false;
    }

    DEBUG_LOG("Unload pipeline");
    bool ret = gst_element_set_state(pipeline_, GST_STATE_NULL);
    if (ret == GST_STATE_CHANGE_FAILURE)
    {
        DEBUG_LOG("Failed to change pipeline state to NULL");
        DEBUG_LOG("Keep doing rest of unload procedure.");
    }

    GstState pipeline_state;
    gst_element_get_state(pipeline_, &pipeline_state, nullptr, GST_CLOCK_TIME_NONE);
    DEBUG_LOG("pipeline_state = %s", gst_element_state_get_name(pipeline_state));
    if (pipeline_state == GST_STATE_NULL)
    {
        DEBUG_LOG("Unload completed");
    }

    auto bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline_));
    if (bus != nullptr)
    {
        gst_bus_set_sync_handler(bus, nullptr, nullptr, nullptr);

        if (remBus())
            gst_bus_remove_watch(bus);

        gst_object_unref(bus);
    }

    state = MediaState::STOP;

    DEBUG_LOG("Delete pipeline");
    gst_object_unref(pipeline_);
    pipeline_ = nullptr;

    if (!detachSurface())
    {
        ERROR_LOG("detachSurface() failed");
        return false;
    }

    DEBUG_LOG("end");
    return true;
}

bool MediaPlayer::attachSurface(bool allow_no_window)
{
    DEBUG_LOG("start");

    if (!window_id_.empty())
    {
        if (!lsm_camera_window_manager_.registerID(window_id_.c_str(), NULL))
        {
            ERROR_LOG("register id to LSM failed!");
            return false;
        }
        if (!lsm_camera_window_manager_.attachSurface())
        {
            ERROR_LOG("attach surface to LSM failed!");
            return false;
        }
        DEBUG_LOG("end");
        return true;
    }
    else
    {
        ERROR_LOG("window id is empty!");
        bool ret = allow_no_window ? true : false;
        return ret;
    }
}

bool MediaPlayer::detachSurface()
{
    DEBUG_LOG("start");

    if (!window_id_.empty())
    {
        if (!lsm_camera_window_manager_.detachSurface())
        {
            ERROR_LOG("detach surface to LSM failed!");
            return false;
        }
        if (!lsm_camera_window_manager_.unregisterID())
        {
            ERROR_LOG("unregister id to LSM failed!");
            return false;
        }
    }
    else
    {
        ERROR_LOG("window id is empty!");
    }

    return true;
}

bool MediaPlayer::addBus()
{
    if (pipeline_ == nullptr)
        return false;
    DEBUG_LOG("start");

    auto bus = gst_element_get_bus(pipeline_);
    if (!bus)
    {
        ERROR_LOG("Error. Fail gst_elememt_get_bus!");
        return false;
    }

    auto s = gst_bus_create_watch(bus);
    g_source_set_callback(
        s,
        (GSourceFunc) + [](GstBus *bus, GstMessage *msg, gpointer data) -> gboolean
        {
            MediaPlayer *p = static_cast<MediaPlayer *>(data);
            return p->handleBusMessage(bus, msg);
        },
        this, nullptr);
    g_source_attach(s, g_main_loop_get_context(loop_));
    busId_ = g_source_get_id(s);
    g_source_unref(s);

    gst_object_unref(bus);

    return true;
}

bool MediaPlayer::remBus()
{
    if (busId_ == 0)
        return false;
    DEBUG_LOG("start");

    GSource *s = g_main_context_find_source_by_id(g_main_loop_get_context(loop_), busId_);
    if (s != nullptr)
        g_source_destroy(s);

    busId_ = 0;
    return true;
}

bool MediaPlayer::handleBusMessage(GstBus *bus, GstMessage *msg)
{
    // auto msgType = GST_MESSAGE_TYPE(msg);
    // if (msgType != GST_MESSAGE_QOS && msgType != GST_MESSAGE_TAG)
    //{
    //     DEBUG_LOG("Element[ %s ][ %d ][ %s ]", GST_MESSAGE_SRC_NAME(msg), msgType,
    //           gst_message_type_get_name(msgType));
    // }

    switch (GST_MESSAGE_TYPE(msg))
    {
    case GST_MESSAGE_ERROR:
    {
        ERROR_LOG("Got Error");
        break;
    }
    case GST_MESSAGE_EOS:
    {
        DEBUG_LOG("Got EOS");
        gst_element_seek(pipeline_, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET, 0,
                         GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
        break;
    }
    case GST_MESSAGE_STATE_CHANGED:
    {
        if (GST_MESSAGE_SRC(msg) != GST_OBJECT_CAST(pipeline_))
            break;

        GstState oldState = GST_STATE_NULL;
        GstState newState = GST_STATE_NULL;
        gst_message_parse_state_changed(msg, &oldState, &newState, nullptr);
        // DEBUG_LOG("Element[%s] State changed ...%s -> %s", GST_MESSAGE_SRC_NAME(msg),
        //       gst_element_state_get_name(oldState), gst_element_state_get_name(newState));

        if (newState == GST_STATE_PAUSED && oldState < GST_STATE_PAUSED)
        {
            DEBUG_LOG("post loadcompleted event");
            play();
        }
        else if (newState == GST_STATE_PLAYING)
        {
            DEBUG_LOG("post playing event");
        }
        else if (newState == GST_STATE_PAUSED && oldState == GST_STATE_PLAYING)
        {
            DEBUG_LOG("post paused event");
        }
        //[TODO] Can not post this becaus UMS kills the process first.
        else if (newState == GST_STATE_NULL && oldState >= GST_STATE_PAUSED)
        {
            DEBUG_LOG("post unloadcompleted event");
        }
        break;
    }
    case GST_MESSAGE_APPLICATION:
    {
        const GstStructure *gStruct = gst_message_get_structure(msg);

        /* video-info message comes from sink element */
        if (gst_structure_has_name(gStruct, "video-info"))
        {
            DEBUG_LOG("got video-info message");
            base::video_info_t video_info;
            memset(&video_info, 0, sizeof(base::video_info_t));
            gint width, height, fps_n, fps_d, par_n = -1, par_d = -1;
            gst_structure_get_int(gStruct, "width", &width);
            gst_structure_get_int(gStruct, "height", &height);
            gst_structure_get_fraction(gStruct, "framerate", &fps_n, &fps_d);
            gst_structure_get_int(gStruct, "par_n", &par_n);
            gst_structure_get_int(gStruct, "par_d", &par_d);

            DEBUG_LOG("width[%d], height[%d], framerate[%d/%d],"
                      "pixel_aspect_ratio[%d/%d]",
                      width, height, fps_n, fps_d, par_n, par_d);

            video_info.width          = width;
            video_info.height         = height;
            video_info.frame_rate.num = fps_n;
            video_info.frame_rate.den = fps_d;
            // TODO: we already know this info. but it's not used now.
            video_info.bit_rate = 0;
            video_info.codec    = 0;
        }
        else if (gst_structure_has_name(gStruct, "request-resource"))
        {
            DEBUG_LOG("got request-resource message");
        }
        break;
    }
    default:
        break;
    }

    return true;
}

GstBusSyncReply MediaPlayer::handleBusSyncMessage(GstBus *bus, GstMessage *msg)
{
    // This handler will be invoked synchronously, don't process any application
    // message handling here

    static constexpr char const *waylandDisplayHandleContextType =
        "GstWaylandDisplayHandleContextType";

    switch (GST_MESSAGE_TYPE(msg))
    {
    case GST_MESSAGE_NEED_CONTEXT:
    {
        const gchar *type = nullptr;
        gst_message_parse_context_type(msg, &type);
        if (g_strcmp0(type, waylandDisplayHandleContextType) != 0)
        {
            break;
        }
        DEBUG_LOG("Set a wayland display handle : %p", lsm_camera_window_manager_.getDisplay());
        if (lsm_camera_window_manager_.getDisplay())
        {
            GstContext *context = gst_context_new(waylandDisplayHandleContextType, TRUE);
            gst_structure_set(gst_context_writable_structure(context), "handle", G_TYPE_POINTER,
                              lsm_camera_window_manager_.getDisplay(), nullptr);
            gst_element_set_context(GST_ELEMENT(GST_MESSAGE_SRC(msg)), context);
        }
        goto drop;
    }
    case GST_MESSAGE_ELEMENT:
    {
        if (!gst_is_video_overlay_prepare_window_handle_message(msg))
        {
            break;
        }
        DEBUG_LOG("Set wayland window handle : %p", lsm_camera_window_manager_.getSurface());
        if (lsm_camera_window_manager_.getSurface())
        {
            GstVideoOverlay *videoOverlay = GST_VIDEO_OVERLAY(GST_MESSAGE_SRC(msg));
            gst_video_overlay_set_window_handle(
                videoOverlay, (guintptr)(lsm_camera_window_manager_.getSurface()));

            gint video_disp_height = 0;
            gint video_disp_width  = 0;
            lsm_camera_window_manager_.getVideoSize(video_disp_width, video_disp_height);
            if (video_disp_width && video_disp_height)
            {
                gint display_x = (1920 - video_disp_width) / 2;
                gint display_y = (1080 - video_disp_height) / 2;
                DEBUG_LOG("Set render rectangle :(%d, %d, %d, %d)", display_x, display_y,
                          video_disp_width, video_disp_height);
                gst_video_overlay_set_render_rectangle(videoOverlay, display_x, display_y,
                                                       video_disp_width, video_disp_height);

                gst_video_overlay_expose(videoOverlay);
            }
        }
        goto drop;
    }
    default:
        break;
    }

    return GST_BUS_PASS;

drop:
    gst_message_unref(msg);
    return GST_BUS_DROP;
}

void MediaPlayer::SetGstreamerDebug()
{
    const std::string filename = "/etc/com.sample.camera.test/gst_debug.conf";
    DEBUG_LOG("Read: %s", filename.c_str());

    std::ifstream i(filename);
    json j;
    i >> j;

    if (j.is_object() && j.contains("gst_debug"))
    {
        for (auto &element : j["gst_debug"])
        {

            const char *kDebugKeys[] = {"GST_DEBUG", "GST_DEBUG_FILE", "GST_DEBUG_DUMP_DOT_DIR"};
            for (const char *key : kDebugKeys)
            {
                if (element.contains(key))
                {
                    DEBUG_LOG("Key: %s, Value: %s", key, element[key].get<std::string>().c_str());
                    setenv(key, element[key].get<std::string>().c_str(), 1);
                }
            }
        }
    }
    else
    {
        ERROR_LOG("Gst debug file parsing error");
    }
}
