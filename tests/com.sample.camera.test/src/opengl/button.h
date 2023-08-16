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

    void createTexture(std::string name);
    int InRange();
    GLuint getId();

public:
    Button(int x, int y, std::string name, void *cb);
    Button(int x, int y, std::string name, std::function<void()> handler);
    ~Button();

    void draw();

    int x, y, w, h;
};

#endif
