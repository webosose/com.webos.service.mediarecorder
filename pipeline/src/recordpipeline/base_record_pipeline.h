#ifndef BASE_RECORD_PIPELINE_H_
#define BASE_RECORD_PIPELINE_H_

#include "base.h"
#include "camera_types.h"
#include "format_utils.h"
#include "record_pipeline.h"
#include <memory>
#include <thread>

static const std::string record_pipeline_path = "/etc/g-record-pipeline/record_pipeline";

class BaseRecordPipeline : public RecordPipeline
{
    GMainLoop *loop_{nullptr};
    std::shared_ptr<std::thread> loopThread_;
    uint32_t busId_{0};
    CALLBACK_T cbFunction_{nullptr};

    std::string display_mode_;
    std::string window_id_;
    base::source_info_t source_info_;
    bool isEos = false;

    bool acquireResource();
    bool GetSourceInfo();
    void NotifySourceInfo();
    void ParseOptionString(const std::string &options);
    void SetGstreamerDebug();
    int32_t ConvertErrorCode(GQuark domain, gint code);
    base::error_t HandleErrorMessage(GstMessage *message);
    bool handleBusMessage(GstBus *bus, GstMessage *msg);
    bool addBus();
    bool remBus();
    bool unloadImpl();
    bool playImpl();
    void sendEos();

public:
    BaseRecordPipeline();
    virtual ~BaseRecordPipeline();

    bool Load(const std::string &msg) override;
    bool Unload() override;
    bool Play() override;
    bool Pause() override;
    void RegisterCbFunction(CALLBACK_T cbf) override;
    virtual bool launch() = 0;

protected:
    int32_t display_path_{GRP_DEFAULT_DISPLAY};
    std::string format_, video_src_, path_;

    GstElement *pipeline_{nullptr};
    std::string pipelineType;

    video_format_t mVideoFormat;
    audio_format_t mAudioFormat;
    image_format_t mImageFormat;
};

#endif // BASE_RECORD_PIPELINE_H_
