/*
 * @Author: huwenhao 1398744100@qq.com
 * @Date: 2023-02-04 17:22:56
 * @LastEditors: huwenhao 1398744100@qq.com
 * @LastEditTime: 2023-02-05 12:09:03
 * @FilePath: /TTServer/src/Logger/Logger.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#pragma once
#include "Singleton.h"
#include "FileAppender.h"
#include "StdoutAppender.h"
#include "RotateAppender.h"
#include "LoggerCommDef.h"

#define LOG_DBG(logger, msg)        \
do                                  \
{                                   \
    timeval tVal;                   \
    gettimeofday(&tVal, nullptr);   \
    tts::STLogRecord stRecord{tVal, __FILE__, __LINE__, 0, 0, "", __FUNCTION__, msg};   \
    logger->Debug(stRecord);        \
}   while(0)                        \

#define LOG_ERR(logger, msg)        \
do                                  \
{                                   \
    timeval tVal;                   \
    gettimeofday(&tVal, nullptr);   \
    tts::STLogRecord stRecord{tVal, __FILE__, __LINE__, 0, 0, "", __FUNCTION__, msg};   \
    logger->Error(stRecord);        \
}   while(0)                        \

namespace tts {

#define LOGGER_APPENDER_FILE            (1 << 1)
#define LOGGER_APPENDER_ROTATE          (1 << 2)
#define LOGGER_APPENDER_STDOUT          (1 << 3)

class Logger
{
public:
    typedef std::shared_ptr<Logger> ptr;

    Logger(const std::string& sLoggerName = "root", 
           EnmLoggerLevel eLevel = NONE, 
           int iAppender = LOGGER_APPENDER_ROTATE, 
           const char* szPattern = nullptr,
           const char* szFileName = nullptr,
           const char* szLogDir = nullptr,
           uint64_t ullMaxFileSize = 100 * 1024 * 1024, 
           int iMaxFileNum = 10);
    ~Logger() {}

public:
    /**
     * @description: 记录一条日志
     * @param {EnmLoggerLevel} eLevel
     * @param {STLogRecord&} stOneRecord
     * @return {*}
     */    
    void Record(EnmLoggerLevel eLevel, const STLogRecord& stOneRecord);
    void Debug(const STLogRecord& stOneRecord);
    void Info(const STLogRecord& stOneRecord);
    void Warn(const STLogRecord& stOneRecord);
    void Error(const STLogRecord& stOneRecord);
    void Fatal(const STLogRecord& stOneRecord);

    /**
     * @description: 获取日志器等级
     * @return {*}
     */    
    EnmLoggerLevel GetLoggerLevel() const {return m_eLevel;}
    void SetLoggerLevel(EnmLoggerLevel eLevel) {m_eLevel = eLevel;}

    /**
     * @description: 添加Appender
     * @param {EnmAppenderType} eType appender类型
     * @return {*}
     */    
    void AddAppender(BaseAppender::ptr ptrAppender);
    void DelAppender(BaseAppender::ptr ptrAppender);

    const std::string& GetLoggerName() {return m_sName;}
    
private:
    std::string m_sName;
    EnmLoggerLevel m_eLevel;
    std::list<BaseAppender::ptr> m_listAppender;
};

}