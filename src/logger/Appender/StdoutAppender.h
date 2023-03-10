/*
 * @Author: huwenhao 1398744100@qq.com
 * @Date: 2023-02-04 21:49:05
 * @LastEditors: huwenhao 1398744100@qq.com
 * @LastEditTime: 2023-02-05 12:09:08
 * @FilePath: /TTServer/src/logger/Appender/StdoutAppender.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#pragma once
#include "BaseAppender.h"

namespace tts {

class StdoutAppender : public BaseAppender
{
public:
    typedef std::shared_ptr<StdoutAppender> ptr;
    StdoutAppender(const std::string& sPattern = DEFAULT_FORMATTER_PATTERN)
        : BaseAppender(sPattern)
    { 
    }
    ~StdoutAppender() {}
public:
    /**
     * 输出内容到终端
    */
    virtual void Append(EnmLoggerLevel eLevel, Logger& stLogger, const STLogRecord& stRecord) override;
};

}