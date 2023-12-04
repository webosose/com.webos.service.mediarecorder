#pragma once

#include <string>

class Error
{
public:
    Error(int code, const std::string &message);

    int getCode() const;
    const std::string &getMessage() const;

private:
    int code;
    std::string message;
};

enum ErrorCode
{
    ERR_NONE                             = 0,
    ERR_JSON_PARSING                     = 100,
    ERR_JSON_TYPE                        = 110,
    ERR_SOURCE_NOT_SPECIFIED             = 200,
    ERR_NOT_SUPPORT_AUDIO_ONLY_RECORDING = 210,
    ERR_RECORDER_ID_NOT_SPECIFIED        = 300,
    ERR_INVALID_RECORDER_ID              = 310,
    ERR_PATH_NOT_SPECIFIED               = 400,
    ERR_FORMAT_NOT_SPECIFIED             = 500,
    ERR_UNSUPPORTED_FORMAT               = 510,
    ERR_CAMERA_OPEN_FAIL                 = 520,
    ERR_FAILED_TO_START_RECORDING        = 600,
    ERR_FAILED_TO_STOP_RECORDING         = 610,
    ERR_SNAPSHOT_CAPTURE_FAILED          = 620,
    ERR_FAILED_TO_PAUSE                  = 630,
    ERR_FAILED_TO_RESUME                 = 640,
    ERR_OPEN_FAIL                        = 700,
    ERR_CLOSE_FAIL                       = 710,
    ERR_INVALID_STATE                    = 800,
    ERR_CANNOT_WRITE                     = 900,
    ERR_LIST_END                         = 1000,
};
