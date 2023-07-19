/*
 * @Author: huwenhao 1398744100@qq.com
 * @Date: 2023-02-04 17:25:25
 * @LastEditors: huwenhao 1398744100@qq.com
 * @LastEditTime: 2023-02-05 12:07:39
 * @FilePath: /TTServer/src/logger/Appender/BaseAppender.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#pragma once
#include "LoggerCommDef.h"
#include "Formatter.h"

namespace tts {

#define MAX_FMT_CONTENT_SIZE 128
class Logger;

class BaseAppender
{
public:
    typedef std::shared_ptr<BaseAppender> ptr;
    BaseAppender(const std::string& sPattern = DEFAULT_FORMATTER_PATTERN)
        : m_stFormatter(sPattern)
    {
        m_stFormatter.Init();
        return;
    }
    virtual ~BaseAppender() {}

public:
    /**
     * 输出内容
    */
    virtual void Append(EnmLoggerLevel eLevel, Logger& stLogger, const STLogRecord& stRecord) = 0;

    /**
     * @description: 设置输出格式
     * @param {ptr} ptrFormatter
     * @return {*}
     */    
    Formatter& GetFormatter()  {return m_stFormatter;} 

protected:
    Formatter m_stFormatter;                        // 使用的Formatter
    char szFmtRes[MAX_FMT_CONTENT_SIZE + 1];        // 存放格式化字符串
};

}