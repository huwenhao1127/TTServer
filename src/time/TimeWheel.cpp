#include "TimeWheel.h"

void TimeWheel::ExcuteCurSlotTask()
{
    for (auto it = m_vecSlots[m_iCurSlot].begin(); it != m_vecSlots[m_iCurSlot].end();)
    {
        if ((*it)->GetRotations() > 0)
        {
            (*it)->DecreaseRotations();
            ++it;
        }
        else
        {
            (*it)->ExcuteTask();
            if ((*it)->IsRepeated())
            {
                AddTimer((*it));
            }
            it = m_vecSlots[m_iCurSlot].erase(it);
        }
    }
    m_iCurSlot = (m_iCurSlot + 1) % m_iSlotNum;
}

void TimeWheel::Tick()
{
    int64_t llCurTime = Now::TimeStampMS();
    // 本次tick跨越的格子数
    int64_t llTickSlotNum = (llCurTime - m_tLastTime) / (int64_t)m_iPrecision;
    if (llTickSlotNum > 0)
    {
        for (int i = 0; i < llTickSlotNum; i++)
        {
            ExcuteCurSlotTask();
        }
        m_tLastTime = llCurTime;
    }
}

Timer::ptr TimeWheel::AddTimer(int64_t llTimeout, TimerCallback fnCallBack, bool bRepeated)
{
    if (llTimeout < 0)
    {
        return nullptr;
    }
    int iRotation = llTimeout / (m_iSlotNum * m_iPrecision);
    // 除以m_iPrecision取整，精度不够时提前触发
    int iAddSlotNum = (llTimeout % (m_iSlotNum * m_iPrecision)) % m_iPrecision;
    int iSlotIndex = (m_iCurSlot + iAddSlotNum) % m_iSlotNum;
    Timer::ptr poTimer(new Timer(iRotation, iSlotIndex, fnCallBack, bRepeated, llTimeout));
    m_vecSlots[iSlotIndex].push_back(poTimer);
    return poTimer;
}

void TimeWheel::AddTimer(Timer::ptr poTimer)
{
    int iRotation = poTimer->GetTimeout() / (m_iSlotNum * m_iPrecision);
    int iAddSlotNum = (poTimer->GetTimeout() % (m_iSlotNum * m_iPrecision)) % m_iPrecision;
    int iSlotIndex = (m_iCurSlot + iAddSlotNum) % m_iSlotNum;
    poTimer->SetSlot(iSlotIndex);
    poTimer->SetRotations(iRotation);
    m_vecSlots[iSlotIndex].push_back(poTimer);
}

void TimeWheel::DelTimer(Timer::ptr poTimer)
{
    if (poTimer == nullptr)
    {
        return;
    }
    for (auto it = m_vecSlots[poTimer->GetSlot()].begin(); it != m_vecSlots[poTimer->GetSlot()].end(); it++)
    {
        if (*it == poTimer)
        {
            m_vecSlots[poTimer->GetSlot()].erase(it);
            return;
        }
    }
}