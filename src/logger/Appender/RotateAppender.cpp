#include "RotateAppender.h"
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <string.h>

namespace tts
{

RotateAppender::~RotateAppender()
{
    if (nullptr != pstFile)
    {
        fclose(pstFile);
        pstFile = nullptr;
    }
} 

void RotateAppender::Append(EnmLoggerLevel eLevel, Logger& stLogger, const STLogRecord& stRecord)
{
    OpenFile();
    std::string strRecord = m_stFormatter.Format(eLevel, stLogger, stRecord);
    strncpy(m_szFmtRes, strRecord.c_str(), MAX_FMT_CONTENT_SIZE);
    int iLen = strRecord.size() >= MAX_FMT_CONTENT_SIZE ? MAX_FMT_CONTENT_SIZE : strRecord.size();

    if (nullptr == pstFile)
    {
        std::cout << "file error" << std::endl;
        return;
    }

    size_t sztRet = fwrite(m_szFmtRes, 1, iLen, pstFile);
    if (sztRet != (size_t)iLen)
    {
        std::cout << "write error: " << sztRet << std::endl;
    }

    fflush(pstFile);
}

void RotateAppender::OpenFile()
{
    // 检查
    if (m_iCurFileID > m_iMaxFileNum)
    {
        return;
    }   
    // 目录
    if (access(m_sDirName.c_str(), 0) != 0)
    {
        mode_t modeOld = umask(0);
        mkdir(m_sDirName.c_str(), S_IRUSR | S_IRWXG);
        umask(modeOld);
    }
    if (nullptr == pstFile)
    {
        // 更新追加方式打开文件
        pstFile = fopen(m_sCurFileName.c_str(), "ab+");
        if (nullptr == pstFile)
        {
            // 不存在就新建
            pstFile = fopen(m_sCurFileName.c_str(), "w");
            if (nullptr == pstFile)
            {
                return;
            }
        }
    }
    // 检查文件大小
    struct stat stStat;
    stat(m_sCurFileName.c_str(), &stStat);
    if (stStat.st_size >= (off_t)m_ullMaxFileSize)
    {
        m_iCurFileID = (m_iCurFileID + 1) % (m_iMaxFileNum + 1);
        m_iCurFileID = (m_iCurFileID == 0) ? 1 : m_iCurFileID;
        m_sCurFileName = m_sDirName + "/" + m_sLogName + "." + std::to_string(m_iCurFileID);

        // 重新打开
        fclose(pstFile);
        pstFile = fopen(m_sCurFileName.c_str(), "wb+");
        if (nullptr == pstFile)
        {
            return;
        }
    }
}

}
