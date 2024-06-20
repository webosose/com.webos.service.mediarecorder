#ifndef _CAM_PLAYER_APP_
#define _CAM_PLAYER_APP_

#include <memory>

class CameraClient;
class MediaClient;
class MediaRecorderClient;
class ButtonRender;
class Image;
class WindowManager;
class MediaPlayer;
class CamPlayerApp
{
    bool startCamera();
    void stopCamera();
    void startCapture();
    void stopCapture();
    void capture();

    void playVideo();
    void pauseVideo();
    void stopVideo();

    void startRecord();
    void stopRecord();
    void pauseRecord();
    void resumeRecord();
    void takeCameraSnapshot();
    void takeSnapshot();

    void exitProgram();
    void setSolutions();

    void printHelp();
    void printOption();

    void CreateImageBox();
    bool isFullScreen();
    void handleEvent(int e);
    void draw();

    bool mDone = false;

    std::unique_ptr<WindowManager> mWindowManager;
    std::unique_ptr<CameraClient> mCameraClient;
    std::unique_ptr<MediaClient> mCameraPlayer;
    std::unique_ptr<MediaRecorderClient> mMediaRecorder;
#ifdef USE_MEDIA_PLAYER
    std::unique_ptr<MediaPlayer> mMediaPlayer;
#else
    std::unique_ptr<MediaClient> mMediaPlayer;
#endif

    std::unique_ptr<ButtonRender> buttonRender;
    std::unique_ptr<Image> imageBox;

public:
    CamPlayerApp();
    ~CamPlayerApp();

    bool parseOption(int argc, char **argv);
    bool initialize();
    bool execute();

    bool mFullScreen = false;
    void handleInput(int x, int y);
};

#endif // _CAM_PLAYER_APP_
