#ifndef __OPEN_GL_UTIL__
#define __OPEN_GL_UTIL__

#include <GLES2/gl2.h>

struct point
{
    GLfloat x;
    GLfloat y;
    GLfloat s;
    GLfloat t;
};

struct vec2
{
    float x, y;
};

struct ivec2
{
    int x;
    int y;
};

GLint get_attrib(GLuint program, const char *name);
GLint get_uniform(GLuint program, const char *name);
GLuint LoadShader(GLenum type, const char *shaderSrc);

#endif // __OPEN_GL_UTIL__
