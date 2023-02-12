/*
 * @Author: huwenhao 1398744100@qq.com
 * @Date: 2023-02-04 21:50:16
 * @LastEditors: huwenhao 1398744100@qq.com
 * @LastEditTime: 2023-02-05 12:08:23
 * @FilePath: /TTServer/src/logger/Appender/FileAppender.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "FileAppender.h"

namespace tts
{

void FileAppender::Append(EnmLoggerLevel eLevel, const STLogRecord& stRecord)
{
    if (eLevel < m_eLevel)
    {
        return ;
    }
    ReOpenFile();
    m_oFileStream << m_ptrFormatter->Format(stRecord);
}

void FileAppender::ReOpenFile()
{
    if (m_oFileStream)
    {
        m_oFileStream.close();
    }
    m_oFileStream.open(m_sFileName);
}

}
