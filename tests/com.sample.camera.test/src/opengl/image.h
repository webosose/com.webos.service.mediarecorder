#ifndef IMAGE_H
#define IMAGE_H

#include "window/window_manager.h"
#include <GLES2/gl2.h>

class Image
{
    void setFullscreen();
    void setNormalSize();

    GLuint textureId;
    wl_Rect orgRect, rect;
    wl_State state = wl_State::Idle;

public:
    Image();
    ~Image();

    void createJpegTexture(const char *file);
    void createYuvTexture(const char *file, int width, int height);
    void draw();
    void deleteTexture();
    bool handleInput(int x, int y);
};

#endif
