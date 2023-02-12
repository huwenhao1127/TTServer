/*
 * @Author: huwenhao 1398744100@qq.com
 * @Date: 2023-02-05 11:09:33
 * @LastEditors: huwenhao 1398744100@qq.com
 * @LastEditTime: 2023-02-05 12:09:01
 * @FilePath: /TTServer/src/logger/LoggerCommDef.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#pragma once
#include "CommDef.h"

namespace tts {

#define MAX_LOGGER_CONTENT_SIZE 1024

// 日志记录
struct STLogRecord
{
    time_t m_tTime;             // 时间
    const char* m_szFile;       // 文件名
    uint32_t m_uiLineNum;       // 行号
    uint32_t m_uiThreadID;      // 线程id
    uint32_t m_uiFiberID;       // 协程id
    const char m_szContent[MAX_LOGGER_CONTENT_SIZE];    // 内容
};

enum EnmLoggerLevel
{
    ENM_LOGGER_LEVEL_NONE  = 0,      // 默认所有级别日志都输出
    ENM_LOGGER_LEVEL_DEBUG = 1,
    ENM_LOGGER_LEVEL_INFO  = 2,
    ENM_LOGGER_LEVEL_WARN  = 3,
    ENM_LOGGER_LEVEL_ERROR = 4,
    ENM_LOGGER_LEVEL_FATAL = 5,
};


}