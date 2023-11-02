#ifndef IMAGE_H
#define IMAGE_H

#include <GLES2/gl2.h>

class Image
{
    GLuint textureId;

public:
    Image();
    ~Image();

    void createJpegTexture(const char *file);
    void createYuvTexture(const char *file, int width, int height);
    void draw();
    void deleteTexture();

    int x, y, w, h;
};

#endif
