#ifndef BUTTON_H
#define BUTTON_H

#include <GLES2/gl2.h>
#include <functional>
#include <string>

class Button
{
    typedef void (*Callback)();

    GLuint normal;
    GLuint yellow;
    GLuint hover;
    int pressed;
    Callback funcPtr;
    std::function<void()> handler_;
    GLuint texID[3];
    bool enable = false;
    int numPic;

    void createTexture(std::string name);
    int InRange();
    GLuint getId();
    GLuint getIdFromValue();

public:
    Button(int x, int y, std::string name, void *cb);
    Button(int x, int y, std::string name, std::function<void()> handler);
    ~Button();

    void draw();
    bool get() { return enable; };

    int x, y, w, h;
};

#endif
