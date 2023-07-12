#ifndef APP_PARM_H
#define APP_PARM_H

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
};

extern CustomData appParm;

#endif // APP_PARM_H
