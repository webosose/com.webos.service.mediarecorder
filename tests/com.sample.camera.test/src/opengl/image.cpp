#include "image.h"
#include "appParm.h"
#include "image_utils.h"
#include "log_info.h"

Image::Image()
{
    x = appParm.x2;
    y = 360;
    w = 640;
    h = 360;
}

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
    w = h * width / height;

    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

    GLenum myTextureMagFilter = (width < 640) ? GL_NEAREST : GL_LINEAR;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, myTextureMagFilter);

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
    w = h * width / height;

    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb_image);

    GLenum myTextureMagFilter = (width < 640) ? GL_NEAREST : GL_LINEAR;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, myTextureMagFilter);

    delete[] rgb_image;
    delete[] yuy2_image;
}

void Image::draw()
{
    if (glIsTexture(textureId) == false)
        return;

    GLushort indices[] = {0, 1, 2, 0, 2, 3};

    glViewport(x, y, w, h);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
}

void Image::deleteTexture()
{
    if (glIsTexture(textureId) == false)
        return;

    glDeleteTextures(1, &textureId);
    DEBUG_LOG("textureID = %d", textureId);
}
