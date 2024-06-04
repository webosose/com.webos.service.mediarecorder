#ifndef __CAMERA_CLIENT__
#define __CAMERA_CLIENT__

#include "client.h"
#include <vector>

class CameraClient : public Client
{
    bool getSolutionName(std::string &solution_name);

    std::string solutionName;

public:
    CameraClient();
    ~CameraClient();

    bool getCameraList();
    bool open();
    bool setFormat();
    bool getFormat();
    bool startCamera();
    bool stopCamera();
    bool startPreview(const std::string &window_id);
    bool stopPreview();
    bool startCapture();
    bool stopCapture();
    bool capture();
    bool close();
    bool checkCamera(const std::string &camera_id);
    bool setSolutions(bool enable);

    int handle{0};
    int key{0};
    std::vector<std::string> captureFileList;
    int preview_fps = 30;
    std::string mediaId;
    std::string cameraId;

    enum State
    {
        STOP,
        START,
    } state{STOP};
};

#endif // __CAMERA_CLIENT__
