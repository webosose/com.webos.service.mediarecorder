#include "play_button.h"

PlayButton::PlayButton(int px, int py, std::string name, std::function<void(int)> handler)
    : Button(px, py, name, handler)
{
    play  = createTexture("play.jpg");
    pause = createTexture("pause.jpg");

    setState(PlayButtonState::Play);
}

void PlayButton::setState(PlayButtonState newState)
{
    state = newState;

    switch (state)
    {
    case PlayButtonState::Play:
    case PlayButtonState::Resume:
        texture_id = play;
        break;
    case PlayButtonState::Pause:
        texture_id = pause;
        break;
    }
}

bool PlayButton::handleInput(int x, int y)
{
    if (InRange(x, y))
    {
        switch (state)
        {
        case PlayButtonState::Play:
            et = EVENT_PLAY_VIDEO;
            setState(PlayButtonState::Pause);
            break;
        case PlayButtonState::Pause:
            et = EVENT_PAUSE_VIDEO;
            setState(PlayButtonState::Resume);
            break;
        case PlayButtonState::Resume:
            et = EVENT_PLAY_VIDEO;
            setState(PlayButtonState::Pause);
            break;
        }

        handler_(et);
        return true;
    }

    return false;
}
