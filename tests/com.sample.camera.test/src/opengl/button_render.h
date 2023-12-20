#ifndef BUTTON_RENDER_H
#define BUTTON_RENDER_H

#include "button.h"
#include "opengl_utils.h"
#include <functional>
#include <memory>

class RecordButton;
class PlayButton;
class ButtonRender
{
    typedef std::function<void(int)> EventCallback;

    // Handle to a program object
    GLuint programButton;

    // Attribute locations
    GLint positionLoc;
    GLint texCoordLoc;

    // Sampler location
    GLint samplerLoc;

    EventCallback callback_;

    int makeButtonProgram();
    void useButtonProgram();
    void disableButtonProgram();

public:
    std::unique_ptr<RecordButton> recordButton;
    std::unique_ptr<PlayButton> playButton;

    std::unique_ptr<Button> startCameraButton;
    std::unique_ptr<Button> stopCameraButton;
    std::unique_ptr<Button> stopButton;
    std::unique_ptr<Button> startCaptureButton;
    std::unique_ptr<Button> exitButton;
    std::unique_ptr<Button> ptzButton;

    ButtonRender(EventCallback callback);
    ~ButtonRender();

    void CreateButton();
    void draw();
    bool handleInput(int x, int y);
};

#endif
