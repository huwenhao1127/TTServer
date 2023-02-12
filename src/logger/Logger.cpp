/*
 * @Author: huwenhao 1398744100@qq.com
 * @Date: 2023-02-04 17:22:50
 * @LastEditors: huwenhao 1398744100@qq.com
 * @LastEditTime: 2023-02-05 12:09:04
 * @FilePath: /TTServer/src/Logger/Logger.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "Logger.h"
#include "Formatter.h"

namespace tts
{

Logger::Logger(const std::string& sLoggerName = "root", EnmLoggerLevel eLevel = ENM_LOGGER_LEVEL_NONE) :
    m_sName(sLoggerName), m_eLevel(eLevel)
{
    return;
}

void Logger::Record(EnmLoggerLevel eLevel, const STLogRecord& stOneRecord)
{
    // 不输出低等级日志
    if (eLevel < m_eLevel)
    {
        return;
    }
    
    // 内容通过AppderMgr输出
    for (auto ptrAppender : m_listAppender)
    {
        ptrAppender->Append(eLevel, stOneRecord);
    }
    
    return;
}

void Logger::Debug(const STLogRecord& stOneRecord)
{
    Record(ENM_LOGGER_LEVEL_DEBUG, stOneRecord);
}

void Logger::Info(const STLogRecord& stOneRecord)
{
    Record(ENM_LOGGER_LEVEL_INFO, stOneRecord);
}

void Logger::Warn(const STLogRecord& stOneRecord)
{
    Record(ENM_LOGGER_LEVEL_WARN, stOneRecord);
}

void Logger::Error(const STLogRecord& stOneRecord)
{   
    Record(ENM_LOGGER_LEVEL_ERROR, stOneRecord);
}

void Logger::Fatal(const STLogRecord& stOneRecord)
{
    Record(ENM_LOGGER_LEVEL_FATAL, stOneRecord);
}

void Logger::DelAppender(BaseAppender::ptr oAppender)
{
    for (auto iter = m_listAppender.begin(); iter != m_listAppender.end(); ++iter)
    {
        if (*iter = oAppender)
        {
            m_listAppender.erase(iter);
            break;
        }
    }
}




}