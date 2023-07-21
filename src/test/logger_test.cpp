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
        LOG_DBG(ptrLogger, std::to_string(i).c_str());
    }
    end = clock();
    float time = (float)(end - start) / CLOCKS_PER_SEC;
    std::cout << "cnt: " << cnt << std::endl;
    std::cout << "time(s): " << time << std::endl;
    std::cout << "speed: " << cnt / time << std::endl;
    return 0;
}