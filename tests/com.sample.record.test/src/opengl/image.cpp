#include "image.h"
#include "appParm.h"
#include "image_utils.h"
#include "log_info.h"

Image::Image() { orgRect = {appParm.x2, 360, 640, 360}; }

Image::~Image() {}

// JPEG Image
void Image::createJpegTexture(const char *file)
{
    int width, height, channels;

    // Use tightly packed data
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Generate a texture object
    glGenTextures(1, &textureId);

    unsigned char *data = jpeg_load(file, &width, &height, &channels, 0);
    DEBUG_LOG("%d %d %d %s", width, height, channels, file);

    // adjust image ratio
    orgRect.w = orgRect.h * width / height;
    rect      = orgRect;
    state     = wl_State::Normal;

    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    delete[] data;
}

// YUY2 Image
void Image::createYuvTexture(const char *filename, int width, int height)
{
    // Use tightly packed data
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Generate a texture object
    glGenTextures(1, &textureId);

    FILE *file = fopen(filename, "rb");
    if (file == NULL)
    {
        fprintf(stderr, "Failed to open file: %s\n", filename);
        return;
    }

    unsigned char *yuy2_image = new unsigned char[width * height * 2];
    unsigned char *rgb_image  = new unsigned char[width * height * 3];

    int dataSize = fread(yuy2_image, 1, width * height * 2, file);
    printf("dataSize = %d\n", dataSize);
    fclose(file);

    convertYUY2toRGB(yuy2_image, width, height, rgb_image);

    // adjust image ratio
    orgRect.w = orgRect.h * width / height;
    rect      = orgRect;
    state     = wl_State::Normal;

    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb_image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    delete[] rgb_image;
    delete[] yuy2_image;
}

void Image::draw()
{
    if (glIsTexture(textureId) == false)
        return;

    GLushort indices[] = {0, 1, 2, 0, 2, 3};

    glViewport(rect.x, rect.y, rect.w, rect.h);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
}

void Image::setFullscreen()
{
    printf("%s\n", __func__);

    // adjust video ratio
    rect.h = 1080;
    rect.w = rect.h * appParm.width / appParm.height;
    rect.x = (1920 - rect.w) / 2;
    rect.y = 0;

    state = wl_State::FullScreen;
}

void Image::setNormalSize()
{
    printf("%s\n", __func__);
    rect  = orgRect;
    state = wl_State::Normal;
}

bool Image::handleInput(int x, int y)
{
    // printf("x => %d, y => %d\n", x, y);
    if (state == wl_State::FullScreen)
    {
        if (x > rect.x && x < rect.x + rect.w && y > rect.y && y < rect.y + rect.h)
        {
            setNormalSize();
            return true;
        }
    }
    else if (state == wl_State::Normal)
    {
        if (x > rect.x && x < rect.x + rect.w && y > rect.y && y < rect.y + rect.h)
        {
            setFullscreen();
            return true;
        }
    }

    return false;
}

void Image::deleteTexture()
{
    if (glIsTexture(textureId) == false)
        return;

    glDeleteTextures(1, &textureId);
    DEBUG_LOG("textureID = %d", textureId);

    state = wl_State::Idle;
}
