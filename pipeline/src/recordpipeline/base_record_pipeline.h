#ifndef _BASE_RECORD_PIPELINE_H_
#define _BASE_RECORD_PIPELINE_H_

#include "base.h"
#include "camera_types.h"
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
    bool isSupportedExtension(const std::string &) const;

public:
    BaseRecordPipeline();
    virtual ~BaseRecordPipeline();

    bool Load(const std::string &msg);
    bool Unload();
    bool Play();
    bool Pause();
    void RegisterCbFunction(CALLBACK_T cbf);
    virtual bool launch() = 0;

protected:
    int32_t width_{0}, height_{0}, framerate_{0}, display_path_{GRP_DEFAULT_DISPLAY}, handle_{0};
    std::string uri_, memtype_, memsrc_, format_, camera_id_;

    GstElement *pipeline_{nullptr};
    std::string pipelineType;

    std::string createRecordFileName(const std::string &) const;
};

#endif // _BASE_RECORD_PIPELINE_H_
