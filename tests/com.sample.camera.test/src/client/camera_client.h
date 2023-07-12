#ifndef __CAMERA_CLIENT__
#define __CAMERA_CLIENT__

#include "client.h"

class CameraClient : public Client
{
    std::string cameraId;
    int handle{0};

public:
    CameraClient();
    ~CameraClient();

    bool getCameraList();
    bool open();
    bool setFormat();
    bool startPreview();
    bool stopPreview();
    bool startCapture();
    bool stopCapture();
    bool close();

    int key;
};

#endif // __CAMERA_CLIENT__
