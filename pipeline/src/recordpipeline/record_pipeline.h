#ifndef _RECORD_PIPELINE_H_
#define _RECORD_PIPELINE_H_

#include <functional>
#include <gst/gst.h>
#include <string>

using CALLBACK_T =
    std::function<void(const gint type, const gint64 numValue, const gchar *strValue, void *udata)>;

class RecordPipeline
{
public:
    virtual bool Load(const std::string &msg)       = 0;
    virtual bool Unload()                           = 0;
    virtual bool Play()                             = 0;
    virtual bool Pause()                            = 0;
    virtual void RegisterCbFunction(CALLBACK_T cbf) = 0;
};

#endif // _RECORD_PIPELINE_H_
