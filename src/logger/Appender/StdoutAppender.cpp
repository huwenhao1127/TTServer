#include "StdoutAppender.h"

namespace tts
{

void StdoutAppender::Append(EnmLoggerLevel eLevel, const STLogRecord& stRecord)
{
    if (eLevel < m_eLevel)
    {
        return ;
    }
    
    std::cout << m_ptrFormatter->Format(stRecord);
}


}
