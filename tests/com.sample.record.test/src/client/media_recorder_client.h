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
    std::string mRecordPath;
    std::string mCapturePath;

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

    std::string &getRecordPath() { return mRecordPath; }
    std::string &getCapturePath() { return mCapturePath; }

    RecordingState state = RecordingState::Stopped;
};

#endif // __MEDIA_RECORDER_CLIENT__
