#ifndef _CAM_PLAYER_APP_
#define _CAM_PLAYER_APP_

#include <memory>

class CameraClient;
class MediaClient;
class MediaRecorderClient;
class WindowManager;
class Button;
class Image;
class CamPlayerApp
{
    void startCamera();
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

    bool parseOption(int argc, char **argv);
    void printHelp();
    void printOption();

    void CreateButton();
    void CreateImageBox();
    void drawButtons();
    void drawImage();

    bool mDone = false;

    std::unique_ptr<WindowManager> mWindowManager;

    std::unique_ptr<CameraClient> mCameraClient;
    std::unique_ptr<MediaClient> mCameraPlayer;
    std::unique_ptr<MediaClient> mMediaPlayer;
    std::unique_ptr<MediaRecorderClient> mMediaRecorder;

    std::unique_ptr<Button> startCameraButton;
    std::unique_ptr<Button> stopCameraButton;
    std::unique_ptr<Button> startRecordButton;
    std::unique_ptr<Button> stopRecordButton;
    std::unique_ptr<Button> pauseRecordButton;
    std::unique_ptr<Button> resumeRecordButton;
    std::unique_ptr<Button> playVideoButton;
    std::unique_ptr<Button> pauseVideoButton;
    std::unique_ptr<Button> stopVideoButton;
    std::unique_ptr<Button> startCaptureButton;
    std::unique_ptr<Button> exitButton;
    std::unique_ptr<Button> ptzButton;

    std::unique_ptr<Image> imageBox;

public:
    CamPlayerApp(int argc, char **argv);
    ~CamPlayerApp();

    bool initialize();
    bool execute();
    void setExporterRegion(int exporter_number, int x, int y, int w, int h);

    bool mFullScreen = false;
};

#endif // _CAM_PLAYER_APP_
