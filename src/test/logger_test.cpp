#include "Logger.h"

int main()
{
    tts::Logger::ptr ptrLogger(new tts::Logger("LOGGER1", tts::EnmLoggerLevel::ERROR));

    tts::FileAppender::ptr ptrFileAppender(new tts::FileAppender("../log/log"));
    ptrLogger->AddAppender(ptrFileAppender);

    tts::StdoutAppender::ptr ptrStdOutAppender(new tts::StdoutAppender());
    ptrLogger->AddAppender(ptrStdOutAppender);

    for (int i = 0; i < 5; i++)
    {
        LOG_DBG(ptrLogger, std::to_string(i).c_str());
    }

    for (int i = 0; i < 5; i++)
    {
        LOG_ERR(ptrLogger, std::to_string(i).c_str());
    }

    return 0;
}