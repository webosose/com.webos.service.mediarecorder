#ifndef PLAY_BUTTON_H
#define PLAY_BUTTON_H

#include "button.h"

enum class PlayButtonState
{
    Play,
    Pause,
    Resume,
};

class PlayButton : public Button
{
    GLuint play, pause;

public:
    PlayButtonState state;

    PlayButton(int x, int y, std::string name, std::function<void(int)> handler);
    void setState(PlayButtonState newState);

    bool handleInput(int x, int y);
};

#endif
