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

// 日志记录
struct STLogRecord
{
    timeval m_valTime;             // 时间
    const char* m_szFile;       // 文件名
    uint32_t m_uiLineNum;       // 行号
    uint32_t m_uiThreadID;      // 线程id
    uint32_t m_uiFiberID;       // 协程id
    const char* m_szThreadName; // 线程名
    const char* m_szFuncName;   // 函数名
    const char* m_szContent;    // 内容
};

enum EnmLoggerLevel
{
    NONE  = 0,      // 默认所有级别日志都输出
    DEBUG = 1,
    INFO  = 2,
    WARN  = 3,
    ERROR = 4,
    FATAL = 5,
};


}