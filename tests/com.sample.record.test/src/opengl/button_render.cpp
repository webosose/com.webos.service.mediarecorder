#include "button_render.h"
#include "appParm.h"
#include "log_info.h"
#include "play_button.h"
#include "record_button.h"

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

ButtonRender::ButtonRender(EventCallback callback)
{
    DEBUG_LOG("start");

    makeButtonProgram();

    callback_ = callback;
    CreateButton();
    useButtonProgram();
}

ButtonRender::~ButtonRender()
{
    DEBUG_LOG("start");

    disableButtonProgram();
    glDeleteProgram(programButton);
}

void ButtonRender::CreateButton()
{
    DEBUG_LOG("start");

    const int PY1 = 200;
    const int PY2 = (PY1 - 180);

    startCameraButton = std::make_unique<Button>(360, PY1, "start_cam.jpg", callback_);
    stopCameraButton  = std::make_unique<Button>(700, PY1, "stop_cam.jpg", callback_);

    recordButton = std::make_unique<RecordButton>(360, PY2, "", callback_);
    playButton   = std::make_unique<PlayButton>(360 + (128 + 40) * 2, PY2, "", callback_);
    stopButton   = std::make_unique<Button>(360 + 128 + 40, PY2, "stop.jpg", callback_);

    startCaptureButton =
        std::make_unique<Button>(360 + (128 + 40) * 3, PY2, "take_picture.jpg", callback_);
    exitButton = std::make_unique<Button>(360 + (128 + 40) * 4, PY2, "exit.jpg", callback_);
    ptzButton  = std::make_unique<Button>(360 + (128 + 40) * 4, PY1 - 4, "ptz.jpg", callback_);
}

void ButtonRender::draw()
{
    // useButtonProgram();

    startCameraButton->draw();
    stopCameraButton->draw();

    recordButton->draw();
    playButton->draw();
    stopButton->draw();

    startCaptureButton->draw();
    exitButton->draw();
    ptzButton->draw();

    // disableButtonProgram();
}

bool ButtonRender::handleInput(int x, int y)
{
    if (startCameraButton->handleInput(x, y))
        return true;
    if (stopCameraButton->handleInput(x, y))
        return true;

    if (recordButton->handleInput(x, y))
        return true;
    if (playButton->handleInput(x, y))
        return true;
    if (stopButton->handleInput(x, y))
    {
        EventType event_type = EVENT_NONE;
        if (recordButton->state != RecordButtonState::Record)
        {
            event_type = EVENT_STOP_RECORD;
            recordButton->setState(RecordButtonState::Record);
        }
        if (playButton->state != PlayButtonState::Play)
        {
            event_type = EVENT_STOP_VIDEO;
            playButton->setState(PlayButtonState::Play);
        }
        callback_(event_type);
        return true;
    }

    if (startCaptureButton->handleInput(x, y))
        return true;
    if (exitButton->handleInput(x, y))
        return true;
    if (ptzButton->handleInput(x, y))
        return true;

    return false;
}

// Initialize the shader and program object
int ButtonRender::makeButtonProgram()
{
    DEBUG_LOG("start");

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
    programButton = glCreateProgram();

    if (programButton == 0)
    {
        ERROR_LOG("Create program fail");
        return 0;
    }

    glAttachShader(programButton, vertexShader);
    glAttachShader(programButton, fragmentShader);

    // Bind vPosition to attribute 0
    // glBindAttribLocation ( programButton, 0, "vPosition" );

    // Link the program
    glLinkProgram(programButton);

    // Check the link status
    glGetProgramiv(programButton, GL_LINK_STATUS, &linked);

    if (!linked)
    {
        GLint infoLen = 0;

        glGetProgramiv(programButton, GL_INFO_LOG_LENGTH, &infoLen);

        if (infoLen > 1)
        {
            char *infoLog = (char *)malloc(sizeof(char) * infoLen);

            glGetProgramInfoLog(programButton, infoLen, NULL, infoLog);
            DEBUG_LOG("Error linking program:\n%s\n", infoLog);

            free(infoLog);
        }

        glDeleteProgram(programButton);
        ERROR_LOG("Link program fail");
        return 0;
    }

    // Get the attribute locations
    positionLoc = glGetAttribLocation(programButton, "a_position");
    texCoordLoc = glGetAttribLocation(programButton, "a_texCoord");

    // Get the sampler location
    samplerLoc = glGetUniformLocation(programButton, "s_texture");

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    ///
    // Draw a triangle using the shader pair created in Init()
    //

    // Use the program object
    glUseProgram(programButton);

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

void ButtonRender::useButtonProgram()
{
    glUseProgram(programButton);

    // Load the vertex position
    glVertexAttribPointer(positionLoc, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), vPositon);
    // Load the texture coordinate
    glVertexAttribPointer(texCoordLoc, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), vTexCoord);

    glEnableVertexAttribArray(positionLoc);
    glEnableVertexAttribArray(texCoordLoc);

    glActiveTexture(GL_TEXTURE0);

    // Set the sampler texture unit to 0
    glUniform1i(samplerLoc, 0);
}

void ButtonRender::disableButtonProgram()
{
    glDisableVertexAttribArray(positionLoc);
    glDisableVertexAttribArray(texCoordLoc);
}
