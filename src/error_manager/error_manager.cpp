#include "error_manager.h"

ErrorManager::ErrorManager() { registerErrors(); }

ErrorManager &ErrorManager::getInstance()
{
    static ErrorManager instance;
    return instance;
}

void ErrorManager::addError(int code, const std::string &message)
{
    errors.push_back({code, message});
}

const std::string &ErrorManager::getErrorMessage(int code) const
{
    for (auto it = errors.begin(); it != errors.end(); ++it)
    {
        if (it->getCode() == code)
        {
            return it->getMessage();
        }
    }

    static const std::string defaultError = "Unknown error";
    return defaultError;
}

Error ErrorManager::getError(int code) const { return Error(code, getErrorMessage(code)); }

void ErrorManager::registerErrors()
{
    // 100
    addError(ERR_JSON_PARSING, "JSON parsing error");
    addError(ERR_JSON_TYPE, "JSON type error");

    // 200
    addError(ERR_SOURCE_NOT_SPECIFIED, "Source must be specified");

    // 300
    addError(ERR_RECORDER_ID_NOT_SPECIFIED, "Recorder ID must be specified");
    addError(ERR_INVALID_RECORDER_ID, "Recorder ID is invalid");

    // 400
    addError(ERR_PATH_NOT_SPECIFIED, "Path must be specified");

    // 500
    addError(ERR_FORMAT_NOT_SPECIFIED, "Format must be specified");
    addError(ERR_UNSUPPORTED_FORMAT, "Unsupported format");
    addError(ERR_CAMERA_OPEN_FAIL, "Failed to open camera");
    addError(ERR_VIDEO_BITRATE_OUT_OF_RANGE, "Video bitrate is out of range");

    // 600
    addError(ERR_FAILED_TO_START_RECORDING, "Failed to start recording");
    addError(ERR_FAILED_TO_STOP_RECORDING, "Failed to stop recording");
    addError(ERR_SNAPSHOT_CAPTURE_FAILED, "Snapshot capture failed");
    addError(ERR_FAILED_TO_PAUSE, "Failed to pause");
    addError(ERR_FAILED_TO_RESUME, "Failed to resume");

    // 700
    addError(ERR_OPEN_FAIL, "Failed to open recorder");
    addError(ERR_CLOSE_FAIL, "Failed to close recorder");
    addError(ERR_VIDEO_NOT_OPENED, "Video is not opened");
    addError(ERR_AUDIO_NOT_OPENED, "Audio is not opened");

    // 800
    addError(ERR_INVALID_STATE, "Invalid state");

    // 900
    addError(ERR_CANNOT_WRITE, "Cannot write at specified location");
}
