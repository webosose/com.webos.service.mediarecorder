#ifndef _LOG_H_
#define _LOG_H_

#include <PmLog.h>
#include <unistd.h>

static PmLogContext getPmLogContext()
{
    static PmLogContext s_context = 0;
    if (0 == s_context)
    {
        PmLogGetContext("CAMTest", &s_context);
    }
    return s_context;
}

#define INFO_LOG_(FORMAT__, ...)                                                                   \
    do                                                                                             \
    {                                                                                              \
        PmLogInfo(getPmLogContext(), "MAIN", 0, FORMAT__, ##__VA_ARGS__);                          \
    } while (0)

#define INFO_LOG(FORMAT__, ...)                                                                    \
    INFO_LOG_("[%d:%d][%s] " FORMAT__, getpid(), gettid(), __func__, ##__VA_ARGS__)

#define DEBUG_LOG_(FORMAT__, ...)                                                                  \
    do                                                                                             \
    {                                                                                              \
        printf(FORMAT__, ##__VA_ARGS__);                                                           \
        printf("\n");                                                                              \
        PmLogInfo(getPmLogContext(), "MAIN", 0, FORMAT__, ##__VA_ARGS__);                          \
    } while (0)

#define DEBUG_LOG(FORMAT__, ...)                                                                   \
    DEBUG_LOG_("[%d:%d][%s] " FORMAT__, getpid(), gettid(), __func__, ##__VA_ARGS__)

#define ERROR_LOG_(FORMAT__, ...)                                                                  \
    do                                                                                             \
    {                                                                                              \
        printf(FORMAT__, ##__VA_ARGS__);                                                           \
        printf("\n");                                                                              \
        PmLogError(getPmLogContext(), "MAIN", 0, FORMAT__, ##__VA_ARGS__);                         \
    } while (0)

#define ERROR_LOG(FORMAT__, ...)                                                                   \
    ERROR_LOG_("[%d:%d][%s] " FORMAT__, getpid(), gettid(), __func__, ##__VA_ARGS__)

#define WARNING_LOG(FORMAT__, ...)                                                                 \
    PmLogWarning(getPmLogContext(), "MAIN", 0, FORMAT__, ##__VA_ARGS__);

//#define SHM_SYNC_TEST

#endif //_LOG_H_
