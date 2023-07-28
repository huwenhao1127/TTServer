/**
 * @file BaseThread.h
 * @author huwenhao ()
 * @brief 线程基类
 * @date 2023-07-20
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#pragma once
#include <thread>
#include <sys/prctl.h>
#include <unistd.h> 
#include <sys/prctl.h>

// IDLE时间
#define BASE_THREAD_IDLE_SLEEP_TIME     1000
// TICK周期默认100ms
#define BASE_THREAD_TICK_INTERVAL       100

class BaseThread
{
public:
    BaseThread(const char* szName);
    virtual ~BaseThread();

private:
    // 禁止拷贝
    BaseThread(const BaseThread&);
    BaseThread& operator=(const BaseThread&);

public:
    /**
     * @brief 开始运行
     * 
     * @return int 
     */
    int Start();

    /**
     * @brief 停止运行
     * 
     * @return int 
     */
    int Stop();

    /**
     * @brief join
     * 
     */
    void Join();

    /**
     * @brief detach
     * 
     */
    void Detach();

private:
    /**
     * @brief idle
     * 
     * @return int 
     */
    virtual int Idle() {usleep(m_dwIdleSleepTm); return 0;};

    /**
     * @brief 初始化
     * 
     * @return int 
     */
    virtual int Init() {return 0;} 

    /**
     * @brief 主循环内执行
     * 
     * @return int 
     */
    virtual int Proc() {return 0;} 

    /**
     * @brief 主循环内Tick
     * 
     * @return int 
     */
    virtual int Tick() {return 0;} 

    /**
     * @brief 主循环内1sTick
     * 
     * @return int 
     */
    virtual int Tick1S() {return 0;} 

    /**
     * @brief 主循环内5sTick
     * 
     * @return int 
     */
    virtual int Tick5S() {return 0;} 
    /**
     * @brief 主循环内10sTick
     * 
     * @return int 
     */
    virtual int Tick10S() {return 0;} 

    /**
     * @brief 主循环内20sTick
     * 
     * @return int 
     */
    virtual int Tick20S() {return 0;} 

protected:
    /**
     * @brief 线程回调
     * 
     * @param pCallBack 
     */
    friend void ThreadMainFunc(void *pCallBack);

    /**
     * @brief 线程执行
     * 
     */
    void Run();

protected:
    const char* m_szThreadName;     // 线程名
    std::thread::id m_oThreadID;    // 线程id
    std::thread* m_pThread;         // 线程实例
    bool m_bActive;                 // 是否运行
    uint64_t m_dwIdleSleepTm;
    uint32_t m_dwTickInterval;
    uint64_t m_ullLastTickTime;
    uint64_t m_ullLastTickTime1S;
    uint64_t m_ullLastTickTime5S;
    uint64_t m_ullLastTickTime10S;
    uint64_t m_ullLastTickTime20S;

};
