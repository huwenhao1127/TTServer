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
#include "format.h"

#define CHECK_IF_PARAM_NULL(logger, _param, _ret)                   \
do                                                                  \
{                                                                   \
    if (nullptr == _param)                                          \
    {                                                               \
        LOG_ERR_FMT(logger, " param is null. return {}", _ret);     \
        return _ret;                                                \
    }                                                               \
} while(0)                                                          \

#define CHECK_IF_PARAM_NULL_VOID(logger, _param)                    \
do                                                                  \
{                                                                   \
    if (nullptr == _param)                                          \
    {                                                               \
        LOG_ERR_FMT(logger, " param is null.");                     \
        return;                                                     \
    }                                                               \
} while(0)                                                          \

#define CHECK_IF_PARAM_NULL_CONTINUE(logger, _param)                \
do                                                                  \
{                                                                   \
    if (nullptr == _param)                                          \
    {                                                               \
        LOG_ERR_FMT(logger, " param is null, continue");            \
        continue;                                                   \
    }                                                               \
} while(0)                                                          \


#define CHECK_PARAM_NOT_ZERO(logger, _param,  _ret)                 \
do                                                                  \
{                                                                   \
    if (0 != _param)                                                \
    {                                                               \
        LOG_ERR_FMT(logger, " param is not zero. return {}", _ret); \
        return _ret;                                                \
    }                                                               \
} while(0)                                                          \


#define LOG_DBG_FMT(logger, _fmt, _args...)        \
do                                  \
{                                   \
    timeval tVal;                   \
    gettimeofday(&tVal, nullptr);   \
    std::string sMsgFmt = fmt::format(FMT_STRING(_fmt), ##_args);                                   \
    tts::STLogRecord stRecord{tVal, __FILE__, __LINE__, 0, 0, "", __FUNCTION__, sMsgFmt.c_str()};   \
    logger->Debug(stRecord);        \
}   while(0)                        \

#define LOG_ERR_FMT(logger, _fmt, _args...)        \
do                                  \
{                                   \
    timeval tVal;                   \
    gettimeofday(&tVal, nullptr);   \
    std::string sMsgFmt = fmt::format(FMT_STRING(_fmt), ##_args);                                   \
    tts::STLogRecord stRecord{tVal, __FILE__, __LINE__, 0, 0, "", __FUNCTION__, sMsgFmt.c_str()};   \
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