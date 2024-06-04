#include "log_info.h"
#include <GLES2/gl2.h>

GLint get_attrib(GLuint program, const char *name)
{
    GLint attribute = glGetAttribLocation(program, name);
    if (attribute == -1)
        printf("Could not bind attribute %s\n", name);
    return attribute;
}

GLint get_uniform(GLuint program, const char *name)
{
    GLint uniform = glGetUniformLocation(program, name);
    if (uniform == -1)
        printf("Could not bind uniform %s\n", name);
    return uniform;
}

// Create a shader object, load the shader source, and
// compile the shader.
GLuint LoadShader(GLenum type, const char *shaderSrc)
{
    GLuint shader;
    GLint compiled;

    // Create the shader object
    shader = glCreateShader(type);

    if (shader == 0)
    {
        return 0;
    }

    // Load the shader source
    glShaderSource(shader, 1, &shaderSrc, NULL);

    // Compile the shader
    glCompileShader(shader);

    // Check the compile status
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

    if (!compiled)
    {
        GLint infoLen = 0;

        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);

        if (infoLen > 1)
        {
            char *infoLog = (char *)malloc(sizeof(char) * infoLen);

            glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
            DEBUG_LOG("Error compiling shader:\n%s\n", infoLog);

            free(infoLog);
        }

        glDeleteShader(shader);
        return 0;
    }

    return shader;
}
