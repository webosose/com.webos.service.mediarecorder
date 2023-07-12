#include "button.h"
#include "image_utils.h"
#include "log_info.h"

#define H 1080
extern int mSx, mSy, mState;

Button::Button(int px, int py, std::string name, void *cb)
{
    x       = px;
    y       = py;
    w       = 300;
    h       = 120;
    pressed = 0;
    funcPtr = (Callback)cb;

    createTexture(name);
}

Button::Button(int px, int py, std::string name, std::function<void()> handler)
{
    x        = px;
    y        = py;
    w        = 300;
    h        = 120;
    pressed  = 0;
    handler_ = handler;

    createTexture(name);
}

Button::~Button() {}

void Button::createTexture(std::string name)
{
    char file_name[128];
    int width, height, channels;

    sprintf(file_name, "%s%s.jpg", "/usr/palm/applications/com.sample.camera.test/", name.c_str());

    // Texture object handle
    GLuint textureId[3];

    // Use tightly packed data
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Generate a texture object
    glGenTextures(3, textureId);

    unsigned char *data = jpeg_load(file_name, &width, &height, &channels, 0);
    DEBUG_LOG("%d %d %d %s", width, height, channels, file_name);

    for (int i = 0; i < 3; i++)
    {
        glBindTexture(GL_TEXTURE_2D, textureId[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height / 3, 0, GL_RGB, GL_UNSIGNED_BYTE,
                     data + width * height / 3 * channels * i);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    normal = textureId[0];
    hover  = textureId[1];
    yellow = textureId[2];

    w = width;
    h = height / 3;

    delete[] data;
}

int Button::InRange() { return (mSx > x && mSx < (x + w) && mSy > (H - y - h) && mSy < (H - y)); }

GLuint Button::getId()
{
    GLuint id = normal;

    if (InRange())
    {
        if (mState)
            pressed = 1;
        else
        {
            if (pressed)
            {
                handler_();
            }
            pressed = 0;
        }

        id = pressed ? yellow : hover;
    }
    else
    {
        if (pressed)
        {
            if (mState)
                id = yellow;
            else
            {
                id      = normal;
                pressed = 0;
            }
        }
        else
            id = normal;
    }

    return id;
}

void Button::draw()
{
    GLushort indices[] = {0, 1, 2, 0, 2, 3};

    glViewport(x, y, w, h);
    glBindTexture(GL_TEXTURE_2D, getId());
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
}
