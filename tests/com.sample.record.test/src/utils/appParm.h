#ifndef APP_PARM_H
#define APP_PARM_H

#include "cam_player_app.h"
#include <string>

/* Structure to contain all our information */
struct CustomData
{
    bool use_ums;
    std::string memType; // shmem, device
    std::string format;  // JPEG, YUV
    int width;
    int height;
    bool disable_audio;
    int fps;
    int x1, x2;
    bool use_start_camera;
};

extern CustomData appParm;

extern CamPlayerApp *miCameraApp;

#endif // APP_PARM_H
