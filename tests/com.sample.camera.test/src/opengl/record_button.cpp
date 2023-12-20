#include "record_button.h"

RecordButton::RecordButton(int px, int py, std::string name, std::function<void(int)> handler)
    : Button(px, py, name, handler)
{
    record = createTexture("record.jpg");
    pause  = createTexture("pause.jpg");
    resume = createTexture("play.jpg");

    setState(RecordButtonState::Record);
}

void RecordButton::setState(RecordButtonState newState)
{
    state = newState;

    switch (state)
    {
    case RecordButtonState::Record:
        texture_id = record;
        break;
    case RecordButtonState::Pause:
        texture_id = pause;
        break;
    case RecordButtonState::Resume:
        texture_id = resume;
        break;
    }
}

bool RecordButton::handleInput(int x, int y)
{
    if (InRange(x, y))
    {
        switch (state)
        {
        case RecordButtonState::Record:
            et = EVENT_START_RECORD;
            setState(RecordButtonState::Pause);
            break;
        case RecordButtonState::Pause:
            et = EVENT_PAUSE_RECORD;
            setState(RecordButtonState::Resume);
            break;
        case RecordButtonState::Resume:
            et = EVENT_RESUME_RECORD;
            setState(RecordButtonState::Pause);
            break;
        }

        handler_(et);
        return true;
    }

    return false;
}
