/**
 * @file Now.h
 * @author your name (you@domain.com)
 * @brief 时间戳相关接口
 * @version 0.1
 * @date 2023-07-18
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once
#include <sys/time.h>
#include <stdint.h>
#include <time.h>

class Now
{
public:
    /**
     * @brief 更新时间戳（进程内更新）
     * 
     */
    static void Update();

    /**
     * @brief 获取秒级时间戳
     * 
     * @return time_t 
     */
    inline static time_t TimeStamp() {return m_currTimeval.tv_sec;}

    /**
     * @brief 获取毫秒级时间戳
     * 
     * @return int64_t 
     */
    inline static int64_t TimeStampMS() {return m_llMillisecond;}

    /**
     * @brief 获取微秒级时间戳
     * 
     * @return int64_t 
     */
    inline static int64_t TimeStampMicroSec() 
    {
        return (int64_t)m_currTimeval.tv_usec + 
               (int64_t)m_currTimeval.tv_sec * 1000 * 1000;
    }

    inline static timeval TiemVal() {return m_currTimeval;}

    /**
     * @brief 获取时间字符串 YYYY-MM-DD HH:MM:SS
     * 
     * @return const char* 
     */
    inline static const char* TimeStr() {return m_timeStr;}

private:
    thread_local static int64_t  m_llMillisecond;
    thread_local static timeval  m_currTimeval;
    thread_local static char     m_timeStr[32];
};