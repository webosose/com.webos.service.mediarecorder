#include "log_info.h"
#include <GLES2/gl2.h>

static GLfloat vPositon[] = {
    -1.0f, 1.0f,  0.0f, // Position 0
    -1.0f, -1.0f, 0.0f, // Position 1
    1.0f,  -1.0f, 0.0f, // Position 2
    1.0f,  1.0f,  0.0f, // Position 3
};

static GLfloat vTexCoord[] = {
    0.0f, 0.0f, // TexCoord 0
    0.0f, 1.0f, // TexCoord 1
    1.0f, 1.0f, // TexCoord 2
    1.0f, 0.0f  // TexCoord 3
};

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

// Initialize the shader and program object
int InitOpenGL()
{
    DEBUG_LOG("start");

    // Handle to a program object
    GLuint programObject;

    // Attribute locations
    GLint positionLoc;
    GLint texCoordLoc;

    // Sampler location
    GLint samplerLoc;

    GLbyte vShaderStr[] = "attribute vec4 a_position;   \n"
                          "attribute vec2 a_texCoord;   \n"
                          "varying vec2 v_texCoord;     \n"
                          "void main()                  \n"
                          "{                            \n"
                          "   gl_Position = a_position; \n"
                          "   v_texCoord = a_texCoord;  \n"
                          "}                            \n";

    GLbyte fShaderStr[] = "precision mediump float;                            \n"
                          "varying vec2 v_texCoord;                            \n"
                          "uniform sampler2D s_texture;                        \n"
                          "void main()                                         \n"
                          "{                                                   \n"
                          "  gl_FragColor = texture2D( s_texture, v_texCoord );\n"
                          "}                                          \n";

    GLuint vertexShader;
    GLuint fragmentShader;
    GLint linked;

    // Load the vertex/fragment shaders
    vertexShader   = LoadShader(GL_VERTEX_SHADER, (const char *)vShaderStr);
    fragmentShader = LoadShader(GL_FRAGMENT_SHADER, (const char *)fShaderStr);

    // Create the program object
    programObject = glCreateProgram();

    if (programObject == 0)
    {
        ERROR_LOG("Create program fail");
        return 0;
    }

    glAttachShader(programObject, vertexShader);
    glAttachShader(programObject, fragmentShader);

    // Bind vPosition to attribute 0
    // glBindAttribLocation ( programObject, 0, "vPosition" );

    // Link the program
    glLinkProgram(programObject);

    // Check the link status
    glGetProgramiv(programObject, GL_LINK_STATUS, &linked);

    if (!linked)
    {
        GLint infoLen = 0;

        glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &infoLen);

        if (infoLen > 1)
        {
            char *infoLog = (char *)malloc(sizeof(char) * infoLen);

            glGetProgramInfoLog(programObject, infoLen, NULL, infoLog);
            DEBUG_LOG("Error linking program:\n%s\n", infoLog);

            free(infoLog);
        }

        glDeleteProgram(programObject);
        ERROR_LOG("Link program fail");
        return 0;
    }

    // Get the attribute locations
    positionLoc = glGetAttribLocation(programObject, "a_position");
    texCoordLoc = glGetAttribLocation(programObject, "a_texCoord");

    // Get the sampler location
    samplerLoc = glGetUniformLocation(programObject, "s_texture");

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    ///
    // Draw a triangle using the shader pair created in Init()
    //

    // Use the program object
    glUseProgram(programObject);

    // Load the vertex position
    glVertexAttribPointer(positionLoc, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), vPositon);
    // Load the texture coordinate
    glVertexAttribPointer(texCoordLoc, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), vTexCoord);

    glEnableVertexAttribArray(positionLoc);
    glEnableVertexAttribArray(texCoordLoc);

    // Bind the texture
    glActiveTexture(GL_TEXTURE0);

    // Set the sampler texture unit to 0
    glUniform1i(samplerLoc, 0);

    return 1;
}
