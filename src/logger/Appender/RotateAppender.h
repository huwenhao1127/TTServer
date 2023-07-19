#pragma once
#include "BaseAppender.h"
#include <stdio.h>

namespace tts {

class RotateAppender : public BaseAppender
{
public:
    typedef std::shared_ptr<RotateAppender> ptr;

    RotateAppender(const std::string& sDirName,
         const std::string& sLogName, uint64_t ullMaxFileSize, int iMaxFileNum, const std::string& sPattern = DEFAULT_FORMATTER_PATTERN)
        : BaseAppender(sPattern),
          m_sDirName(sDirName),
          m_sLogName(sLogName),
          m_ullMaxFileSize(ullMaxFileSize),
          m_iMaxFileNum(iMaxFileNum),
          m_iCurFileID(1),
          m_sCurFileName(m_sDirName + "/" + m_sLogName + "." + std::to_string(m_iCurFileID)),
          pstFile(nullptr),
          m_iFlush(0)
          {}

    virtual ~RotateAppender() override;

public:
    /**
     * 输出内容到文件
    */
    virtual void Append(EnmLoggerLevel eLevel, Logger& stLogger, const STLogRecord& stRecord) override;

private:
    void OpenFile();

private:
    std::string m_sDirName;         // 目录名
    std::string m_sLogName;         // 日志名
    uint64_t m_ullMaxFileSize;      // 单文件最大字节数
    int m_iMaxFileNum;              // 最大文件数
    int m_iCurFileID;               // 当前操作的文件idx
    std::string m_sCurFileName;     // 当前操作的文件名
    FILE* pstFile;                  // 当前操作的文件

    int m_iFlush;                   // 
};


}