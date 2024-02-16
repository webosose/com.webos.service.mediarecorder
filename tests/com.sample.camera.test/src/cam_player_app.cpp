#include "cam_player_app.h"
#include "appParm.h"
#include "client/camera_client.h"
#include "client/media_client.h"
#include "client/media_recorder_client.h"
#include "file_utils.h"
#include "log_info.h"
#include "opengl/button_render.h"
#include "opengl/image.h"
#include "window/window_manager.h"
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>

using namespace nlohmann;

CustomData appParm = {
    false,   // use UMS
    "shmem", // memory type (shmem, posixshm, device)
    "JPEG",  // Image format (JPEG, YUV)
    1280,    // Camera width
    720,     // Camear height
    false,   // disable audio recording
    30,      // Preview fps
    0,       // x1
    1280,    // x2
    false,   // use startCamera
};

CamPlayerApp::CamPlayerApp() { INFO_LOG(""); }

CamPlayerApp::~CamPlayerApp()
{
    INFO_LOG("");

    stopVideo();
    stopRecord();
    stopCamera();

    if (mWindowManager)
    {
        mWindowManager->finalize();
    }
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

    buttonRender = std::make_unique<ButtonRender>([&](int e) { handleEvent(e); });

    return true;
}

bool CamPlayerApp::execute()
{
    DEBUG_LOG("");

    startCamera();
    CreateImageBox();

    DEBUG_LOG("wl_display_dispatch_pending");
    while ((wl_display_dispatch_pending(mWindowManager->foreign.getDisplay()) != -1) &&
           (!mDone)) // Dispatch main queue events without reading from the display fd
    {
        draw();
        g_usleep(100 * 1000); // 100 ms
        eglSwapBuffers(eglGetCurrentDisplay(), eglGetCurrentSurface(EGL_READ));
    }

    return true;
}

bool CamPlayerApp::parseOption(int argc, char **argv)
{
    for (;;)
    {
        switch (getopt(argc, argv, "m:f:w:h:p:udo?"))
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

        case 'p':
            appParm.fps = atoi(optarg);
            continue;

        case 'd':
            appParm.disable_audio = true;
            continue;

        case 'o':
            appParm.use_start_camera = true;
            continue;

        case '?':
        default:
            printHelp();
            return 0;

        case -1:
            break;
        }

        break;
    }

    printOption();

    return 1;
}

void CamPlayerApp::printHelp()
{
    std::cout << "Usage: camera_test [OPTION]..." << std::endl;
    std::cout << "Options" << std::endl;
    std::cout << "  -u          use ums" << std::endl;
    std::cout << "  -m          mode (shmem, posixshm, device)" << std::endl;
    std::cout << "  -f          format (JPEG, YUV)" << std::endl;
    std::cout << "  -w          width" << std::endl;
    std::cout << "  -h          height" << std::endl;
    std::cout << "  -p          preview fps" << std::endl;
    std::cout << "  -d          disable audio record" << std::endl;
    std::cout << "  -o          use startCamera" << std::endl;
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
    std::cout << "fps    : " << appParm.fps << std::endl;
    std::cout << "disable audio : " << appParm.disable_audio << std::endl;
    std::cout << "use startCamera : " << appParm.use_start_camera << std::endl;
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

    if (mCameraClient->state == CameraClient::START)
    {
        DEBUG_LOG("invalid state");
        return;
    }

    DEBUG_LOG("start");

    if (appParm.memType == "shmem" || appParm.memType == "posixshm")
    {
        mCameraClient->getCameraList();
        mCameraClient->open();
        mCameraClient->setFormat();

        if (appParm.use_start_camera)
        {
            mCameraClient->startCamera();
        }
        else
        {
            mCameraClient->startPreview(mWindowManager->exporter[0].getWindowID());
            mWindowManager->setRect(0);
            return;
        }
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

    mCameraClient->getFormat();

    json option;
    option["appId"]            = "com.webos.app.mediaevents-test";
    option["windowId"]         = mWindowManager->exporter[0].getWindowID();
    option["videoDisplayMode"] = "Textured";
    option["width"]            = appParm.width;
    option["height"]           = appParm.height;
    option["format"]           = image_format;
    option["frameRate"]        = mCameraClient->preview_fps;
    option["memType"]          = appParm.memType;
    option["memSrc"]           = mem_src;

    if (appParm.memType == "posixshm")
    {
        option["handle"] = mCameraClient->handle;
    }

    mCameraPlayer->load(option);
    mWindowManager->setRect(0);
}

void CamPlayerApp::stopCamera()
{
    if (mCameraPlayer == nullptr)
        return;

    DEBUG_LOG("start");

    if (appParm.use_start_camera)
    {
        DEBUG_LOG("state %d", mCameraPlayer->state);
        if (mCameraPlayer->state == MediaClient::STOP)
        {
            DEBUG_LOG("invalid state");
            return;
        }

        DEBUG_LOG("start");

        mCameraPlayer->unload();

        if (appParm.memType == "shmem" || appParm.memType == "posixshm")
        {
            mCameraClient->stopCamera();
            mCameraClient->close();
        }
    }
    else
    {

        if (mCameraClient->state == CameraClient::STOP)
        {
            DEBUG_LOG("invalid state");
            return;
        }

        if (appParm.memType == "shmem" || appParm.memType == "posixshm")
        {
            mCameraClient->stopPreview();
            mCameraClient->close();
        }
    }

    mWindowManager->clearRect(0);

    imageBox->deleteTexture();
}

void CamPlayerApp::startRecord()
{
    DEBUG_LOG("start");

    stopVideo();

    if (appParm.use_ums)
    {
        mCameraPlayer->startCameraRecord();
    }
    else
    {
        std::string &videoSrc =
            (appParm.use_start_camera) ? mCameraPlayer->mediaId : mCameraClient->cameraId;
        mMediaRecorder->open(videoSrc);
        mMediaRecorder->setOutputFile();
        mMediaRecorder->setOutputFormat();
        mMediaRecorder->start();
    }
}

void CamPlayerApp::stopRecord()
{
    if (mMediaRecorder == nullptr)
        return;

    DEBUG_LOG("start");
    if (mMediaRecorder->state == RecordingState::Stopped)
    {
        DEBUG_LOG("Already stopped");
        return;
    }

    if (appParm.use_ums)
    {
        mCameraPlayer->stopCameraRecord();
    }
    else
    {
        mMediaRecorder->stop();
        mMediaRecorder->close();
    }
}

void CamPlayerApp::pauseRecord()
{
    DEBUG_LOG("start");
    mMediaRecorder->pause();
}

void CamPlayerApp::resumeRecord()
{
    DEBUG_LOG("start");
    mMediaRecorder->resume();
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

        imageBox->createJpegTexture(snapshot_file_name.c_str());
        deleteFile(snapshot_file_name);
    }

    DEBUG_LOG("end\n");
}

void CamPlayerApp::takeSnapshot()
{
    DEBUG_LOG("start");

    auto start = std::chrono::high_resolution_clock::now();
    if (mMediaRecorder->takeSnapshot())
    {
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration = end - start;
        DEBUG_LOG("%d ms", static_cast<int>(duration.count()));

        std::string snapshot_file_name = mMediaRecorder->getCapturePath();
        imageBox->createJpegTexture(snapshot_file_name.c_str());
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
            imageBox->createJpegTexture(capture_file_name.c_str());
        }
        else
        {
            capture_file_name = getLastFile("/tmp/", "yuv");
            imageBox->createYuvTexture(capture_file_name.c_str(), appParm.width, appParm.height);
        }

        deleteFile(capture_file_name);
    }

    DEBUG_LOG("end\n");
}

void CamPlayerApp::stopCapture() { mCameraClient->stopCapture(); }

void CamPlayerApp::capture()
{
    DEBUG_LOG("start");

    bool ret = mCameraClient->capture();
    if (ret)
    {
        std::string capture_file_name = mCameraClient->captureFileList[0].c_str();

        if (appParm.format == "JPEG")
        {
            imageBox->createJpegTexture(capture_file_name.c_str());
        }
        else
        {
            imageBox->createYuvTexture(capture_file_name.c_str(), appParm.width, appParm.height);
        }
    }

    DEBUG_LOG("end\n");
}

void CamPlayerApp::exitProgram()
{
    DEBUG_LOG("start");
    mDone = true;
}

void CamPlayerApp::setSolutions()
{
    bool enable = buttonRender->ptzButton->get();
    DEBUG_LOG("ptz %d", enable);
    mCameraClient->setSolutions(buttonRender->ptzButton->get());
}

void CamPlayerApp::playVideo()
{
    DEBUG_LOG("state %d", mMediaPlayer->state);

    if (mMediaPlayer->state == MediaClient::STOP)
    {
        std::string video_name = mMediaRecorder->getRecordPath();
        if (video_name.empty())
            video_name = getLastFile("/media/internal/", "mp4");

        if (video_name.empty())
        {
            DEBUG_LOG("no video");
        }
        else
        {
            mMediaPlayer->load(mWindowManager->exporter[1].getWindowID(), video_name);
            mWindowManager->setRect(1);
        }
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
    if (mMediaPlayer == nullptr)
        return;

    DEBUG_LOG("state %d", mMediaPlayer->state);

    if (mMediaPlayer->state != MediaClient::STOP)
    {
        mMediaPlayer->unload();
        mWindowManager->clearRect(1);
    }

    DEBUG_LOG("end");
}

void CamPlayerApp::CreateImageBox()
{
    DEBUG_LOG("start");
    imageBox = std::make_unique<Image>();
}

bool CamPlayerApp::isFullScreen() { return mWindowManager->isFullScreen(); }

void CamPlayerApp::draw()
{
    // Clear the color buffer
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (!isFullScreen())
    {
        buttonRender->draw();
        imageBox->draw();
    }
}

void CamPlayerApp::handleInput(int x, int y)
{
    if (!isFullScreen())
    {
        if (imageBox->handleInput(x, y))
            return;
        if (buttonRender->handleInput(x, y))
            return;
    }

    mWindowManager->handleInput(x, y);
}

void CamPlayerApp::handleEvent(int eventType)
{
    // DEBUG_LOG("%s %d", __func__, eventType);

    switch (eventType)
    {
    case EVENT_START_CAMERA:
        DEBUG_LOG("EVENT_START_CAMERA");
        startCamera();
        break;
    case EVENT_STOP_CAMERA:
        DEBUG_LOG("EVENT_STOP_CAMERA");
        stopCamera();
        break;
    case EVENT_START_RECORD:
        DEBUG_LOG("EVENT_START_RECORD");
        startRecord();
        break;
    case EVENT_PAUSE_RECORD:
        DEBUG_LOG("EVENT_PAUSE_RECORD");
        pauseRecord();
        break;
    case EVENT_RESUME_RECORD:
        DEBUG_LOG("EVENT_RESUME_RECORD");
        resumeRecord();
        break;
    case EVENT_STOP_RECORD:
        DEBUG_LOG("EVENT_STOP_RECORD");
        stopRecord();
        break;
    case EVENT_PLAY_VIDEO:
        DEBUG_LOG("EVENT_PLAY_VIDEO");
        playVideo();
        break;
    case EVENT_PAUSE_VIDEO:
        DEBUG_LOG("EVENT_PAUSE_VIDEO");
        pauseVideo();
        break;
    case EVENT_STOP_VIDEO:
        DEBUG_LOG("EVENT_STOP_VIDEO");
        stopVideo();
        break;
    case EVENT_START_CAPTURE:
        DEBUG_LOG("EVENT_START_CAPTURE");
        if (mMediaRecorder->state == RecordingState::Recording)
        {
            if (appParm.use_start_camera)
            {
                takeCameraSnapshot();
            }
            else
            {
                takeSnapshot();
            }
        }
        else
        {
            capture();
        }
        break;
    case EVENT_PTZ:
        DEBUG_LOG("EVENT_PTZ");
        setSolutions();
        break;
    case EVENT_EXIT:
        DEBUG_LOG("EVENT_EXIT");
        exitProgram();
        break;
    default:
        DEBUG_LOG("Unknown Event Type");
        break;
    }
}
