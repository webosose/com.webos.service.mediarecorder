#include "cam_player_app.h"
#include "appParm.h"
#include "client/camera_client.h"
#include "client/media_client.h"
#include "client/media_recorder_client.h"
#include "file_utils.h"
#include "log_info.h"
#include "opengl/button.h"
#include "opengl/image.h"
#include "opengl/init_opengl.h"
#include "window/window_manager.h"
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>

using namespace nlohmann;

CustomData appParm = {
    false,   // use UMS
    "shmem", // memory type (shmem, device)
    "JPEG",  // Image format (JPEG, YUV)
    1280,    // Camera width
    720,     // Camear height
    false,   // disable audio recording
    30,      // Preview fps
    0,       // x1
    1280     // x2
};

CamPlayerApp::CamPlayerApp(int argc, char **argv)
{
    DEBUG_LOG("");
    parseOption(argc, argv);
}

CamPlayerApp::~CamPlayerApp()
{
    DEBUG_LOG("");

    stopVideo();
    stopCamera();

    mWindowManager->finalize();
}

bool CamPlayerApp::initialize()
{
    DEBUG_LOG("");

    mCameraClient  = std::make_unique<CameraClient>();
    mMediaPlayer   = std::make_unique<MediaClient>();
    mCameraPlayer  = std::make_unique<MediaClient>("camera-player");
    mMediaRecorder = std::make_unique<MediaRecorderClient>();

    mWindowManager = std::make_unique<WindowManager>();
    mWindowManager->initialize();

    return true;
}

bool CamPlayerApp::execute()
{
    DEBUG_LOG("");

    startCamera();

    if (!InitOpenGL())
    {
        ERROR_LOG("GL Init error");
        return GL_FALSE;
    }

    CreateButton();
    CreateImageBox();

    DEBUG_LOG("wl_display_dispatch_pending");
    while ((wl_display_dispatch_pending(mWindowManager->foreign.getDisplay()) != -1) &&
           (!mDone)) // Dispatch main queue events without reading from the display fd
    {
        drawButtons();
        drawImage();
        eglSwapBuffers(eglGetCurrentDisplay(), eglGetCurrentSurface(EGL_READ));
    }

    return true;
}

bool CamPlayerApp::parseOption(int argc, char **argv)
{
    for (;;)
    {
        switch (getopt(argc, argv, "m:f:w:h:ud?"))
        {
        case 'm':
            appParm.memType = optarg;
            continue;
        case 'f':
            appParm.format = optarg;
            continue;
        case 'u':
            appParm.use_ums = true;
            continue;

        case 'w':
            appParm.width = atoi(optarg);
            continue;

        case 'h':
            appParm.height = atoi(optarg);
            continue;

        case 'd':
            appParm.disable_audio = true;
            continue;

        case '?':
        default:
            printHelp();
            exit(0);

        case -1:
            break;
        }

        break;
    }

    printOption();

    return 0;
}

void CamPlayerApp::printHelp()
{
    std::cout << "Usage: camera_test [OPTION]..." << std::endl;
    std::cout << "Options" << std::endl;
    std::cout << "  -u          use ums" << std::endl;
    std::cout << "  -m          mode (shmem, device)" << std::endl;
    std::cout << "  -f          format (JPEG, YUV)" << std::endl;
    std::cout << "  -w          width" << std::endl;
    std::cout << "  -h          height" << std::endl;
    std::cout << "  -d          disable audio record" << std::endl;
    std::cout << "  -?          help" << std::endl;
}

void CamPlayerApp::printOption()
{
    std::cout << std::endl;
    std::cout << "use ums : " << std::boolalpha << appParm.use_ums << std::endl;
    std::cout << "mode : " << appParm.memType << std::endl;
    std::cout << "format : " << appParm.format << std::endl;
    std::cout << "width : " << appParm.width << std::endl;
    std::cout << "height : " << appParm.height << std::endl;
    std::cout << "disable audio : " << appParm.disable_audio << std::endl;
    std::cout << std::endl;
}

/*
# luna-send -n 1 -f luna://com.webos.media/load '{
    "uri": "camera://com.webos.service.camera2/7010",
    "payload": {
        "option": {
            "appId": "com.webos.app.mediaevents-test",
            "windowId": "_Window_Id_1",
            "videoDisplayMode": "Textured",
            "width": 640,
            "height": 480,
            "format": "JPEG",
            "frameRate": 30,
            "memType": "shmem",
            "memSrc": "7010"
        }
    },
    "type": "camera"
}'
*/
void CamPlayerApp::startCamera()
{
    DEBUG_LOG("state %d", mCameraPlayer->state);
    if (mCameraPlayer->state == MediaClient::PLAY)
    {
        DEBUG_LOG("invalid state");
        return;
    }

    DEBUG_LOG("start");

    if (appParm.memType.compare("shmem") == 0)
    {
        mCameraClient->getCameraList();
        mCameraClient->open();
        mCameraClient->setFormat();
        mCameraClient->startPreview();
    }

    std::string image_format = appParm.format;
    if (image_format == "YUV")
    {
        image_format = "YUY2";
    }

    std::string mem_src = std::to_string(mCameraClient->key);
    if (appParm.memType == "device")
    {
        mem_src = "/dev/video0"; //[ToDo] Set temporarily.
    }

    json option;
    option["appId"]            = "com.webos.app.mediaevents-test";
    option["windowId"]         = mWindowManager->exporter1.getWindowID();
    option["videoDisplayMode"] = "Textured";
    option["width"]            = appParm.width;
    option["height"]           = appParm.height;
    option["format"]           = image_format;
    option["frameRate"]        = appParm.fps;
    option["memType"]          = appParm.memType;
    option["memSrc"]           = mem_src;

    mCameraPlayer->load(option);
}

void CamPlayerApp::stopCamera()
{
    DEBUG_LOG("state %d", mCameraPlayer->state);
    if (mCameraPlayer->state == MediaClient::STOP)
    {
        DEBUG_LOG("invalid state");
        return;
    }

    DEBUG_LOG("start");

    mCameraPlayer->unload();

    if (appParm.memType.compare("shmem") == 0)
    {
        mCameraClient->stopPreview();
        mCameraClient->close();
    }

    imageBox->deleteTexture();
}

void CamPlayerApp::startRecord()
{
    DEBUG_LOG("start");

    stopVideo();

    deleteAll("/media/internal/", "mp4");

    if (appParm.use_ums)
    {
        mCameraPlayer->startCameraRecord();
    }
    else
    {
        mMediaRecorder->open(mCameraPlayer->mediaId);
        mMediaRecorder->setOutputFile();
        mMediaRecorder->setOutputFormat();
        mMediaRecorder->start();
    }

    isRecording = true;
}

void CamPlayerApp::stopRecord()
{
    DEBUG_LOG("start");

    if (appParm.use_ums)
    {
        mCameraPlayer->stopCameraRecord();
    }
    else
    {
        mMediaRecorder->stop();
        mMediaRecorder->close();
    }

    isRecording = false;
}

void CamPlayerApp::takeCameraSnapshot()
{
    DEBUG_LOG("start");

    bool ret = false;

    if (appParm.use_ums)
    {
        ret = mCameraPlayer->takeCameraSnapshot();
    }
    else
    {
        ret = mMediaRecorder->takeSnapshot();
    }

    if (ret)
    {
        deleteAll("/media/internal/", "jpeg");

        // wait for capture complete
        int i;
        std::string snapshot_file_name;
        for (i = 0; i < 200 && snapshot_file_name.empty(); i++)
        {
            snapshot_file_name = getLastFile("/media/internal/", "jpeg");
            g_usleep(10 * 1000);
        }
        DEBUG_LOG("getFile = %d ms", (i + 1) * 10);

        imageBox->createTexture(snapshot_file_name.c_str());
        deleteFile(snapshot_file_name);
    }

    DEBUG_LOG("end\n");
}

void CamPlayerApp::startCapture()
{
    DEBUG_LOG("start");

    bool ret = mCameraClient->startCapture();
    if (ret)
    {
        deleteAll("/media/internal/", "jpeg");

        std::string capture_file_name;

        if (appParm.format == "JPEG")
        {
            capture_file_name = getLastFile("/tmp/", "jpeg");
            imageBox->createTexture(capture_file_name.c_str());
        }
        else
        {
            capture_file_name = getLastFile("/tmp/", "yuv");
            imageBox->createTexture(capture_file_name.c_str(), appParm.width, appParm.height);
        }

        deleteFile(capture_file_name);
    }

    DEBUG_LOG("end\n");
}

void CamPlayerApp::stopCapture() { mCameraClient->stopCapture(); }

void CamPlayerApp::exitProgram()
{
    DEBUG_LOG("start");
    mDone = true;
}

void CamPlayerApp::playVideo()
{
    DEBUG_LOG("state %d", mMediaPlayer->state);

    if (mMediaPlayer->state == MediaClient::STOP)
    {
        mMediaPlayer->load(mWindowManager->exporter2.getWindowID(),
                           getLastFile("/media/internal/", "mp4"));
    }
    else if (mMediaPlayer->state == MediaClient::PAUSE)
    {
        mMediaPlayer->play();
    }
    else
    {
        DEBUG_LOG("Invalid state : %d", mMediaPlayer->state);
    }

    DEBUG_LOG("end");
}

void CamPlayerApp::pauseVideo()
{
    DEBUG_LOG("state %d", mMediaPlayer->state);

    if (mMediaPlayer->state == MediaClient::PLAY)
    {
        mMediaPlayer->pause();
    }

    DEBUG_LOG("end");
}

void CamPlayerApp::stopVideo()
{
    DEBUG_LOG("state %d", mMediaPlayer->state);

    if (mMediaPlayer->state != MediaClient::STOP)
    {
        mMediaPlayer->unload();
    }

    DEBUG_LOG("end");
}

void CamPlayerApp::CreateButton()
{
    DEBUG_LOG("start");

    const int PY1 = 200;
    const int PY2 = (PY1 - 180);

    startCameraButton = std::make_unique<Button>(360, PY1, "start", [this]() { startCamera(); });
    stopCameraButton  = std::make_unique<Button>(700, PY1, "stop", [this]() { stopCamera(); });

    startRecordButton =
        std::make_unique<Button>(360, PY2, "start_rec", [this]() { startRecord(); });
    stopRecordButton = std::make_unique<Button>(360, PY2, "stop_rec", [this]() { stopRecord(); });

    playVideoButton =
        std::make_unique<Button>(360 + 128 + 40, PY2, "play", [this]() { playVideo(); });
    pauseVideoButton =
        std::make_unique<Button>(360 + 128 + 40, PY2, "pause", [this]() { pauseVideo(); });
    stopVideoButton =
        std::make_unique<Button>(360 + (128 + 40) * 2, PY2, "stop_rec", [this]() { stopVideo(); });

#if 0
    startCaptureButton = std::make_unique<Button>(360 + (128 + 40) * 3, PY2, "take_picture",
                                                  [this]() { startCapture(); });
#else
    startCaptureButton = std::make_unique<Button>(360 + (128 + 40) * 3, PY2, "take_picture",
                                                  [this]() { takeCameraSnapshot(); });
#endif
    /*
        stopCaptureButton = std::make_unique<Button>(360 + (128 + 40) * 4, PY2, "stop_rec",
                                                     [this]() { stopCapture(); });
     */
    exitButton =
        std::make_unique<Button>(360 + (128 + 40) * 4, PY2, "exit", [this]() { exitProgram(); });
}

void CamPlayerApp::CreateImageBox()
{
    DEBUG_LOG("start");
    imageBox = std::make_unique<Image>();
}

void CamPlayerApp::drawButtons()
{
    // Clear the color buffer
    glClear(GL_COLOR_BUFFER_BIT);

    startCameraButton->draw();
    stopCameraButton->draw();

    if (isRecording)
        stopRecordButton->draw();
    else
        startRecordButton->draw();

    if (mMediaPlayer->state == MediaClient::PLAY)
        pauseVideoButton->draw();
    else
        playVideoButton->draw();

    stopVideoButton->draw();

    startCaptureButton->draw();
    // stopCaptureButton->draw();

    exitButton->draw();
}

void CamPlayerApp::drawImage() { imageBox->draw(); }
