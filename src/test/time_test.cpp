#include "TimeWheel.h"
#include <string>
#include <iostream>
#include "Logger.h"
using namespace std;

static tts::Logger::ptr ptrLogger(new tts::Logger("LOGGER1", tts::EnmLoggerLevel::ERROR));

void CallBack(const std::string& sInfo)
{
    std::string sTmp = sInfo + std::string(Now::TimeStr());
    LOG_DBG(ptrLogger, sTmp.c_str());
}

int main()
{
    tts::RotateAppender::ptr ptrRotateAppender(new tts::RotateAppender("/home/wenhowhu/TTServer/log", "log", 100 * 1024 * 1024, 10));
    ptrLogger->AddAppender(ptrRotateAppender);
    tts::StdoutAppender::ptr ptrStdoutAppender(new tts::StdoutAppender());
    ptrLogger->AddAppender(ptrStdoutAppender);
    Now::Update();

    TimeWheel::ptr poTimeWheel(new TimeWheel(1000, 1));
    std::string sMsg = "add: " + std::string(Now::TimeStr());
    LOG_DBG(ptrLogger, sMsg.c_str());
    poTimeWheel->AddTimer(1000, std::bind(CallBack, "1s: "), true);
    poTimeWheel->AddTimer(2000, std::bind(CallBack, "2s: "), false);
    poTimeWheel->AddTimer(5000, std::bind(CallBack, "5s: "), true);
    poTimeWheel->AddTimer(10000, std::bind(CallBack, "10s: "), true);

    while(1)
    {
        Now::Update();
        poTimeWheel->Tick();
    }
    return 0;
}