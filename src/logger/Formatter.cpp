#include "Formatter.h"
#include <vector>
#include <tuple>
#include <string>
#include <functional>
#include "Logger.h"

namespace tts
{

void Formatter::Init()
{
    // 绑定item与fmt函数
    BindItemAndFmtFunc();

    // 解析格式
    ParseFormatter();
}

void Formatter::ParseFormatter()
{
    // 符号%后第一个字符为模式，{}内字符串为模式附加的字符串
    std::string sTemStr = "";
    for (std::size_t i = 0; i < m_sFormatter.size(); ++i)
    {
        if (m_sFormatter[i] != '%') 
        {
            sTemStr = sTemStr + m_sFormatter[i];
            continue;
        }

        // 模式匹配符, 且后一个字符为模式
        if (m_sFormatter[i] == '%' && 
            i + 1 < m_sFormatter.size() && 
            m_mapItemFmt.find(m_sFormatter[i+1]) != m_mapItemFmt.end())
        {
            if ("" != sTemStr)
            {
                // 存储匹配符前的字符串模式
                m_vecItems.push_back(std::tuple<char, std::string>('s', sTemStr));
                sTemStr.clear();
            }
            
            // 日期模式
            if (m_sFormatter[i+1] == 'd')
            {
                // 解析日期的格式
                std::string strTimeFmt = "";
                if (i + 2 < m_sFormatter.size() && m_sFormatter[i+2] == '{')
                {
                    std::size_t iIdx = i + 3;
                    bool bSucc = false;
                    while (iIdx < m_sFormatter.size() && m_sFormatter[iIdx] != '}')
                    {
                        strTimeFmt = strTimeFmt + m_sFormatter[iIdx];
                        iIdx++;
                        if (m_sFormatter[iIdx] == '}')
                        {
                            bSucc = true;
                        }
                    }
                    // 匹配日期格式失败
                    if (!bSucc)
                    {
                        strTimeFmt = "";
                        i = i+1;          
                    }
                    else
                    {
                        i = iIdx;
                    }
                }
                m_vecItems.push_back(std::tuple<char, std::string>('d', strTimeFmt));
            }
            // 其它模式
            else
            {
                m_vecItems.push_back(std::tuple<char, std::string>(m_sFormatter[i+1], ""));
                i = i+1;
            }
        }
    }
}

std::string Formatter::Format(EnmLoggerLevel eLevel, Logger& stLogger, const STLogRecord& stRecord)
{
    std::stringstream ssFmt;
    for (std::tuple<char, std::string> tupItem : m_vecItems)
    {
        char cItem = std::get<0>(tupItem);
        std::string& sExInfo = std::get<1>(tupItem);

        if (cItem == 'd' && sExInfo == "")
        {
            // 使用默认日期格式
            m_mapItemFmt[cItem](ssFmt, eLevel, stLogger, stRecord, "%Y-%m-%d %H:%M:%S");
        }
        else
        {
            m_mapItemFmt[cItem](ssFmt, eLevel, stLogger, stRecord, sExInfo);
        }
    }
    return ssFmt.str();
}

void Formatter::MessageItemFormat(
    std::ostream& osFmtItem, EnmLoggerLevel eLevel, Logger& stLogger, const STLogRecord& stRecord, const std::string& sFormat /* = "" */)
{
    osFmtItem << stRecord.m_szContent;
}

void Formatter::LevelItemFormat(
    std::ostream& osFmtItem, EnmLoggerLevel eLevel, Logger& stLogger, const STLogRecord& stRecord, const std::string& sFormat /* = "" */)
{
    osFmtItem << Formatter::LevelToString(eLevel);
}

void Formatter::NameItemFormat(
    std::ostream& osFmtItem, EnmLoggerLevel eLevel, Logger& stLogger, const STLogRecord& stRecord, const std::string& sFormat /* = "" */)
{   
    osFmtItem << stLogger.GetLoggerName();
}

void Formatter::ThreadIdItemFormat(
    std::ostream& osFmtItem, EnmLoggerLevel eLevel, Logger& stLogger, const STLogRecord& stRecord, const std::string& sFormat /* = "" */)
{
    osFmtItem << std::to_string(stRecord.m_uiThreadID);
}

void Formatter::NewLineItemFormat(
    std::ostream& osFmtItem, EnmLoggerLevel eLevel, Logger& stLogger, const STLogRecord& stRecord, const std::string& sFormat /* = "" */)
{
    osFmtItem << std::endl;
}

void Formatter::DateTimeItemFormat(
    std::ostream& osFmtItem, EnmLoggerLevel eLevel, Logger& stLogger, const STLogRecord& stRecord, const std::string& sFormat /* = "%Y-%m-%d %H:%M:%S" */)
{
    struct tm tm;
    time_t time = stRecord.m_tTime;
    localtime_r(&time, &tm);
    char szBuf[64];
    strftime(szBuf, sizeof(szBuf), sFormat.c_str(), &tm);
    osFmtItem << szBuf;
}

void Formatter::FilenameItemFormat(
    std::ostream& osFmtItem, EnmLoggerLevel eLevel, Logger& stLogger, const STLogRecord& stRecord, const std::string& sFormat /* = "" */)
{
    osFmtItem << stRecord.m_szFile;
}

void Formatter::LineNumItemFormat(
    std::ostream& osFmtItem, EnmLoggerLevel eLevel, Logger& stLogger, const STLogRecord& stRecord, const std::string& sFormat /* = "" */)
{
    osFmtItem << stRecord.m_uiLineNum;
}

void Formatter::TabItemFormat(
    std::ostream& osFmtItem, EnmLoggerLevel eLevel, Logger& stLogger, const STLogRecord& stRecord, const std::string& sFormat /* = "" */)
{
    osFmtItem << "\t";
}

void Formatter::FiberIdItemFormat(
    std::ostream& osFmtItem, EnmLoggerLevel eLevel, Logger& stLogger, const STLogRecord& stRecord, const std::string& sFormat /* = "" */)
{
    osFmtItem << stRecord.m_uiFiberID;
}

void Formatter::ThreadNameItemFormat(
    std::ostream& osFmtItem, EnmLoggerLevel eLevel, Logger& stLogger, const STLogRecord& stRecord, const std::string& sFormat /* = "" */)
{
    osFmtItem << stRecord.m_szThreadName;
}

void Formatter::FuncNameItemFormat(
    std::ostream& osFmtItem, EnmLoggerLevel eLevel, Logger& stLogger, const STLogRecord& stRecord, const std::string& sFormat /* = "" */)
{
    osFmtItem << stRecord.m_szFuncName;
}

void Formatter::StringItemFormat(
        std::ostream& osFmtItem, EnmLoggerLevel eLevel, Logger& stLogger, const STLogRecord& stRecord, const std::string& sFormat /* = "" */)
{
    osFmtItem << sFormat;
}

const char* Formatter::LevelToString(EnmLoggerLevel eLevel)
{
    switch(eLevel) 
    {

        #define XX(name)            \
        case EnmLoggerLevel::name:  \
            return #name;           \
            break;

        XX(DEBUG);
        XX(INFO);
        XX(WARN);
        XX(ERROR);
        XX(FATAL);

        #undef XX
    default:
        return "UNKNOW";
    }
    return "UNKNOW";
}

void Formatter::BindItemAndFmtFunc()
{
    m_mapItemFmt['m'] = MessageItemFormat;
    m_mapItemFmt['L'] = LevelItemFormat;
    m_mapItemFmt['c'] = NameItemFormat;
    m_mapItemFmt['t'] = ThreadIdItemFormat;
    m_mapItemFmt['n'] = NewLineItemFormat;
    m_mapItemFmt['d'] = DateTimeItemFormat;
    m_mapItemFmt['f'] = FilenameItemFormat;
    m_mapItemFmt['l'] = LineNumItemFormat;
    m_mapItemFmt['T'] = TabItemFormat;
    m_mapItemFmt['F'] = FiberIdItemFormat;
    m_mapItemFmt['N'] = ThreadNameItemFormat;
    m_mapItemFmt['U'] = FuncNameItemFormat;
    m_mapItemFmt['s'] = StringItemFormat;
}




}
