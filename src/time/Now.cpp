
#include "Now.h"
#include <time.h>

/**
 * 常用时间函数耗时
 函数	            单次调用时钟周期
 time(NULL)	        98
 gettimeofday	    366
 localtime	        5522
 localtime_r	    1102
 mktime	            2868
 strftime	        4734
**/

thread_local int64_t Now::m_llMillisecond = 0L;
thread_local timeval Now::m_currTimeval = {0, 0};
thread_local char Now::m_timeStr[32] = "\0";

void Now::Update()
{
    time_t tOldTime = TimeStamp();
    gettimeofday(&m_currTimeval, nullptr);
    m_llMillisecond = (int64_t)(m_currTimeval.tv_sec * 1000) + (int64_t)(m_currTimeval.tv_usec / 1000);

    // 字符串精度为秒，同一秒不更新
    if (tOldTime == m_currTimeval.tv_sec)
    {
        return;
    }

    m_timeStr[0] = '\0';
    time_t tCurTime = TimeStamp();
    tm local{};
    if (nullptr == localtime_r(&tCurTime, &local))
    {
        return;
    }

    strftime(m_timeStr, sizeof(m_timeStr), "%F %T", &local);
}