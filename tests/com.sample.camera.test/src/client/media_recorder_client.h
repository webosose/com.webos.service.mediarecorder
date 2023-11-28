#ifndef __MEDIA_RECORDER_CLIENT__
#define __MEDIA_RECORDER_CLIENT__

#include "client.h"
#include <nlohmann/json.hpp>

using namespace nlohmann;

enum class RecordingState
{
    Stopped,
    Recording,
    Paused
};

class MediaRecorderClient : public Client
{
    int recorderId = 0;

public:
    MediaRecorderClient(std::string name = "media-recorder");
    ~MediaRecorderClient();

    bool open(std::string &video_src);
    bool setOutputFile();
    bool setOutputFormat();
    bool start();
    bool stop();
    bool close();
    bool takeSnapshot();
    bool pause();
    bool resume();

    RecordingState state = RecordingState::Stopped;
};

#endif // __MEDIA_RECORDER_CLIENT__
