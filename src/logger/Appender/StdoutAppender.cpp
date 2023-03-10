#include "StdoutAppender.h"
#include "Logger.h"
#include <iostream>

namespace tts
{

void StdoutAppender::Append(EnmLoggerLevel eLevel, Logger& stLogger, const STLogRecord& stRecord)
{
    if (eLevel > stLogger.GetLoggerLevel())
    {
        return ;
    }
    
    std::cout << m_stFormatter.Format(eLevel, stLogger, stRecord);
}

}
