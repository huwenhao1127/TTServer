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
#include "BaseAppender.h"
#include "LoggerCommDef.h"

namespace tts {

class Logger
{
public:
    typedef std::shared_ptr<Logger> ptr;

    Logger(const std::string& sLoggerName = "root", EnmLoggerLevel eLevel = ENM_LOGGER_LEVEL_NONE);
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
    void AddAppender(BaseAppender::ptr oAppender) {m_listAppender.push_back(oAppender);}
    void DelAppender(BaseAppender::ptr oAppender);
private:
    std::string m_sName;
    EnmLoggerLevel m_eLevel;
    std::list<BaseAppender::ptr> m_listAppender;
};

}