#ifndef BUTTON_H
#define BUTTON_H

#include "window/window_manager.h"
#include <GLES2/gl2.h>
#include <functional>
#include <string>

enum EventType
{
    EVENT_START_CAMERA,
    EVENT_STOP_CAMERA,
    EVENT_START_RECORD,
    EVENT_PAUSE_RECORD,
    EVENT_RESUME_RECORD,
    EVENT_STOP_RECORD,
    EVENT_PLAY_VIDEO,
    EVENT_PAUSE_VIDEO,
    EVENT_STOP_VIDEO,
    EVENT_START_CAPTURE,
    EVENT_PTZ,
    EVENT_EXIT,
    EVENT_NONE,
};

class Button
{
    GLuint texID[2];
    bool enable = false;
    int numPic  = 1;

    void createTextures(std::string &name);

protected:
    std::function<void(int)> handler_;
    GLuint texture_id;
    wl_Rect rect;
    EventType et = EVENT_NONE;

    bool InRange(int, int);
    GLuint createTexture(const std::string &name);

public:
    Button(int x, int y, std::string name, std::function<void(int)> handler);
    ~Button();

    void draw();
    bool get() { return enable; };
    void set(bool flag)
    {
        enable     = flag;
        texture_id = texID[enable];
    };
    bool handleInput(int x, int y);
};

#endif
