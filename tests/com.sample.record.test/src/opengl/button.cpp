#include "button.h"
#include "image_utils.h"

#define H 1080
const std::string res_path = "/usr/palm/applications/com.sample.record.test/";

Button::Button(int px, int py, std::string name, std::function<void(int)> handler)
{
    handler_ = handler;
    rect     = {px, py, 128, 128};
    if (!name.empty())
        createTextures(name);

    if (name.find("start_cam") != std::string::npos)
        et = EVENT_START_CAMERA;
    else if (name.find("stop_cam") != std::string::npos)
        et = EVENT_STOP_CAMERA;
    else if (name.find("take_picture") != std::string::npos)
        et = EVENT_START_CAPTURE;
    else if (name.find("ptz") != std::string::npos)
        et = EVENT_PTZ;
    else if (name.find("exit") != std::string::npos)
        et = EVENT_EXIT;
}

Button::~Button() {}

void Button::createTextures(std::string &name)
{
    std::string path = res_path + name;

    // Use tightly packed data
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Generate a texture object
    glGenTextures(2, texID);

    int width, height, channels;
    unsigned char *data = jpeg_load(path.c_str(), &width, &height, &channels, 0);
    // DEBUG_LOG("%d %d %d %s", width, height, channels, path.c_str());

    if (width < height)
        numPic = height / width;

    for (int i = 0; i < numPic; i++)
    {
        glBindTexture(GL_TEXTURE_2D, texID[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height / numPic, 0, GL_RGB, GL_UNSIGNED_BYTE,
                     data + width * height / numPic * channels * i);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    rect.w     = width;
    rect.h     = height / numPic;
    texture_id = texID[enable];

    delete[] data;
}

GLuint Button::createTexture(const std::string &name)
{
    std::string path = res_path + name;

    // Use tightly packed data
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    GLuint textureID;

    // Generate a texture object
    glGenTextures(1, &textureID);

    int width, height, channels;
    unsigned char *data = jpeg_load(path.c_str(), &width, &height, &channels, 0);
    // DEBUG_LOG("%d %d %d %s", width, height, channels, path.c_str());

    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    rect.w = width;
    rect.h = height;

    delete[] data;

    return textureID;
}

bool Button::InRange(int x, int y)
{
    // printf("%s : %d %d %d %d\n", __func__, rect.x, rect.y, rect.w, rect.h);
    return (x > rect.x && x < (rect.x + rect.w) && y > (H - rect.y - rect.h) && y < (H - rect.y));
}

void Button::draw()
{
    GLushort indices[] = {0, 1, 2, 0, 2, 3};

    glViewport(rect.x, rect.y, rect.w, rect.h);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
}

bool Button::handleInput(int x, int y)
{
    // printf("%s : %d %d\n", __func__, x, y);
    if (InRange(x, y))
    {
        if (numPic > 1)
        {
            enable ^= true;
            texture_id = texID[enable];
            printf("%s Button event=%d enable=%d\n", __func__, et, enable);
        }
        else
            printf("%s Button event=%d\n", __func__, et);

        if (et != EVENT_NONE)
            handler_(et);
        return true;
    }

    return false;
}
