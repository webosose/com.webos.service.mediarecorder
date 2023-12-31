#ifndef __MEDIA_CLIENT__
#define __MEDIA_CLIENT__

#include "client.h"
#include <nlohmann/json.hpp>

using namespace nlohmann;

class MediaClient : public Client
{
public:
    MediaClient(std::string name = "media");
    ~MediaClient();

    bool load(const std::string &windowId, const std::string &video_name);
    bool load(const json &option);
    bool play();
    bool pause();
    bool unload();
    bool subscribe();
    bool unsubscribe();
    bool seek(int pos);

    // camera
    bool takeCameraSnapshot();
    bool startCameraRecord();
    bool stopCameraRecord();

    std::string mediaId;

    enum State
    {
        STOP,
        PAUSE,
        PLAY
    } state{STOP};
};

#endif // __MEDIA_CLIENT__
