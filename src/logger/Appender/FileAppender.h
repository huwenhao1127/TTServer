/*
 * @Author: huwenhao 1398744100@qq.com
 * @Date: 2023-02-04 21:50:07
 * @LastEditors: huwenhao 1398744100@qq.com
 * @LastEditTime: 2023-02-05 12:00:19
 * @FilePath: /TTServer/src/logger/Appender/FileAppender.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#pragma once
#include "BaseAppender.h"

namespace tts {

class FileAppender : public BaseAppender
{
public:
    typedef std::shared_ptr<FileAppender> ptr;
    FileAppender(const std::string& sFileName, const std::string& sPattern = DEFAULT_FORMATTER_PATTERN)
        : BaseAppender(sPattern),
          m_sFileName(sFileName) 
        { 
        }
    ~FileAppender() 
    {
        
    }

public:
    /**
     * 输出内容到文件
    */
    virtual void Append(EnmLoggerLevel eLevel, Logger& stLogger, const STLogRecord& stRecord) override;

private:
    void ReOpenFile();
private:
    std::string m_sFileName;
    std::ofstream m_oFileStream;
};


}