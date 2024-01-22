#ifndef RECORD_BUTTON_H
#define RECORD_BUTTON_H

#include "button.h"

enum class RecordButtonState
{
    Record,
    Pause,
    Resume
};

class RecordButton : public Button
{
    GLuint record, pause, resume;

public:
    RecordButtonState state;

    RecordButton(int x, int y, std::string name, std::function<void(int)> handler);
    void setState(RecordButtonState newState);

    bool handleInput(int x, int y);
};

#endif
