
#pragma once
#include <stdint.h>
#include <time.h>
#include <list>
#include <vector>
#include <functional>
#include <memory>
#include <iostream>
#include "Now.h"

// 语法糖
typedef std::function<void(void)> TimerCallback;

class Timer
{
public:
    Timer(int iRotations, int iSlotIndex, TimerCallback m_fnCallBack, bool m_bRepeated, int64_t llTimeout) :
        m_iRotations(iRotations), 
        m_iSlotIndex(iSlotIndex), 
        m_llTimeout(llTimeout),
        m_fnCallBack(m_fnCallBack), 
        m_bRepeated(m_bRepeated)
    {}
    ~Timer(){ std::cout << " timer clear" << std::endl;}
    typedef std::shared_ptr<Timer> ptr;

    /**
     * @brief 获取任务触发圈数
     * 
     * @return int 
     */
    inline int GetRotations() {return m_iRotations;}
    inline void SetRotations(int iRotations) {m_iRotations = iRotations;}

    /**
     * @brief 获取所属slot索引
     * 
     * @return int 
     */
    inline int GetSlot() {return m_iSlotIndex;}
    inline void SetSlot(int iSlotIndex) {m_iSlotIndex = iSlotIndex;}

    /**
     * @brief 触发圈数减1
     * 
     */
    inline void DecreaseRotations() {m_iRotations--;}

    /**
     * @brief 执行任务
     * 
     */
    inline void ExcuteTask() {m_fnCallBack();}

    /**
     * @brief 是否重复执行
     * 
     * @return true 
     * @return false 
     */
    inline bool IsRepeated() {return m_bRepeated;}

    /**
     * @brief 获取超时时间
     * 
     * @return int64_t 
     */
    inline int64_t GetTimeout() {return m_llTimeout;}

private:
    int m_iRotations;               // 任务触发圈数
    int m_iSlotIndex;               // Slot索引
    int64_t m_llTimeout;            // 定时时间
    TimerCallback m_fnCallBack;     // 任务回调
    bool m_bRepeated;               // Timer是否重复执行
};

class TimeWheel
{
public:
    TimeWheel(int iSlotNum, int iPrecision) : 
        m_iSlotNum(iSlotNum),
        m_iCurSlot(0),
        m_iPrecision(iPrecision),
        m_tLastTime(Now::TimeStampMS()),
        m_vecSlots(iSlotNum, std::list<Timer::ptr>())
    {}
    ~TimeWheel() {}
    typedef std::shared_ptr<TimeWheel> ptr;

    /**
     * @brief 1ms tick驱动
     * 
     */
    void Tick();

    /**
     * @brief 添加计时器
     * 
     * @param llTimeout 
     * @param fnCallBack 
     * @param bRepeated 
     */
    Timer::ptr AddTimer(int64_t llTimeout, TimerCallback fnCallBack, bool bRepeated);

    /**
     * @brief 删除计时器
     * 
     * @param pstTimer 
     */
    void DelTimer(Timer::ptr poTimer);

private:

    /**
     * @brief 添加计时器
     * 
     * @param poTimer 
     */
    void AddTimer(Timer::ptr poTimer);

    /**
     * @brief 执行当前slot的任务链
     * 
     */
    void ExcuteCurSlotTask();

private:
    int m_iSlotNum;                   // 时间轮Slots总数
    int m_iCurSlot;                   // 当前指向的Slot
    int m_iPrecision;                 // 每个Slot的时间精度（ms）
    time_t m_tLastTime;               // 最后触发时间
    std::vector<std::list<Timer::ptr>> m_vecSlots;     // 每个Slot挂着存放任务的双向链表
};