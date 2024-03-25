#ifndef _MEDIA_PLAYER_H_
#define _MEDIA_PLAYER_H_

#include "media_state.h"
#include "wayland/camera_window_manager.h"
#include <gst/gst.h>
#include <memory>
#include <string>
#include <thread>

class MediaPlayer
{
    GstElement *pipeline_{nullptr};
    LSM::CameraWindowManager lsm_camera_window_manager_;
    std::string window_id_;
    GMainLoop *loop_{nullptr};
    std::unique_ptr<std::thread> loopThread_;
    uint32_t busId_{0};

public:
    MediaPlayer();
    ~MediaPlayer();

    bool load(const std::string &windowId, const std::string &video_name);
    bool play();
    bool pause();
    bool unload();

    bool attachSurface(bool allow_no_window = false);
    bool detachSurface();

    bool handleBusMessage(GstBus *bus, GstMessage *msg);
    GstBusSyncReply handleBusSyncMessage(GstBus *bus, GstMessage *msg);
    bool addBus();
    bool remBus();
    void SetGstreamerDebug();

    MediaState state = MediaState::STOP;
};

#endif // _MEDIA_PLAYER_H_
