/*
 * @Author: huwenhao 1398744100@qq.com
 * @Date: 2023-02-04 17:22:50
 * @LastEditors: huwenhao 1398744100@qq.com
 * @LastEditTime: 2023-02-05 12:09:04
 * @FilePath: /TTServer/src/Logger/Logger.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "Logger.h"

namespace tts
{

void Logger::Record(EnmLoggerLevel eLevel, const STLogRecord& stOneRecord)
{
    // 不输出低等级日志
    if (eLevel > m_eLevel)
    {
        return;
    }
    
    // 内容通过AppderMgr输出
    for (auto ptrAppender : m_listAppender)
    {
        ptrAppender->Append(eLevel, *this, stOneRecord);
    }
    
    return;
}

void Logger::Debug(const STLogRecord& stOneRecord)
{
    Record(DEBUG, stOneRecord);
}

void Logger::Info(const STLogRecord& stOneRecord)
{
    Record(INFO, stOneRecord);
}

void Logger::Warn(const STLogRecord& stOneRecord)
{
    Record(WARN, stOneRecord);
}

void Logger::Error(const STLogRecord& stOneRecord)
{   
    Record(ERROR, stOneRecord);
}

void Logger::Fatal(const STLogRecord& stOneRecord)
{
    Record(FATAL, stOneRecord);
}

void Logger::DelAppender(BaseAppender::ptr ptrAppender)
{
    for (auto iter = m_listAppender.begin(); iter != m_listAppender.end(); ++iter)
    {
        if (*iter = ptrAppender)
        {
            m_listAppender.erase(iter);
            break;
        }
    }
}

void Logger::AddAppender(BaseAppender::ptr ptrAppender) 
{
    m_listAppender.push_back(ptrAppender);
}




}