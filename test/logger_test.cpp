#include "Logger.h"
#include "time.h"

int main()
{
    tts::Logger::ptr ptrLogger(new tts::Logger("LOGGER1", 
                                tts::EnmLoggerLevel::ERROR, 
                                LOGGER_APPENDER_ROTATE,
                                nullptr,
                                "log",
                                "/home/wenhowhu/TTServer/log"));

    size_t cnt = 1000000;
    clock_t start, end;
    start = clock();
    for (size_t i = 0; i < cnt; i++)
    {
        LOG_DBG_FMT(ptrLogger, "test: {}", std::to_string(i).c_str());
    }
    end = clock();
    float time = (float)(end - start) / CLOCKS_PER_SEC;
    LOG_DBG_FMT(ptrLogger, "cnt[{}] time[{}] speed[{}]", cnt, time, (cnt / time));
    return 0;
}