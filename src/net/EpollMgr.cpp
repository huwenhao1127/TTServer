#include "EpollMgr.h"
#include "NetCommDef.h"
#include "Socket.h"
#include "Now.h"

int EpollMgr::Init()
{
    m_iMaxFDNum = MAX_NIC_NUM;
    m_iEventIdx = 0;
    m_iEventNum = 0;
    m_llEpollWaitTime = 0; 

    // 创建epoll
    m_iFD = epoll_create(m_iMaxFDNum);
    if (0 >= m_iFD)
    {
        LOG_ERR_FMT(ptrNetLogger, "epoll mgr init fail, {}", GetSocketError());
        return -1;
    }

    // 创建就绪时间列表
    m_pstEpollEvent = new epoll_event[MAX_NIC_NUM];
    if (nullptr == m_pstEpollEvent)
    {
        LOG_ERR_FMT(ptrNetLogger, "create epoll event list fail.");
        return -1;
    }

    LOG_DBG_FMT(ptrNetLogger, "epoll mgr init succ. fd:{}", m_iFD);
    return 0;
}

int EpollMgr::Reclaim()
{
    close(m_iFD);
    if (nullptr != m_pstEpollEvent)
    {
        delete[] m_pstEpollEvent;
        m_pstEpollEvent = nullptr;
    }
    return 0;
}

int EpollMgr::PollAdd(int iFD, int iMode)
{
    struct epoll_event stEvent = {0};
    stEvent.data.fd = iFD;
    stEvent.events = iMode;
    int iRet = epoll_ctl(m_iFD, EPOLL_CTL_ADD, iFD, &stEvent);
    if (0 != iRet)
    {
        LOG_ERR_FMT(ptrNetLogger, "epoll mgr add fd[{} {}] fail, {}", iFD, iMode, GetSocketError());
        return iRet;
    }
    LOG_DBG_FMT(ptrNetLogger, "epoll mgr add fd[{} {}] succ.", iFD, iMode);
    return 0;
}

int EpollMgr::PollMode(int iFD, int iMode)
{
    struct epoll_event stEvent = {0};
    stEvent.data.fd = iFD;
    stEvent.events = iMode;
    int iRet = epoll_ctl(m_iFD, EPOLL_CTL_MOD, iFD, &stEvent);
    if (0 != iRet)
    {
        LOG_ERR_FMT(ptrNetLogger, "epoll mgr mod fd[{} {}] fail, {}", iFD, iMode, GetSocketError());
        return iRet;
    }
    LOG_DBG_FMT(ptrNetLogger, "epoll mgr mod fd[{} {}] succ.", iFD, iMode);
    return 0;
}

int EpollMgr::PollDel(int iFD)
{
    struct epoll_event stEvent = {0};
    stEvent.data.fd = iFD;
    stEvent.events = 0;
    int iRet = epoll_ctl(m_iFD, EPOLL_CTL_DEL, iFD, &stEvent);
    if (0 != iRet)
    {
        LOG_ERR_FMT(ptrNetLogger, "epoll mgr del fd[{}] fail, {}", iFD, GetSocketError());
        return iRet;
    }
    LOG_DBG_FMT(ptrNetLogger, "epoll mgr del fd[{}] succ.", iFD);
    return 0;
}

int EpollMgr::PollWait()
{
    int64_t tNowMs = Now::TimeStampMS();
    if (m_llEpollWaitTime + EPOLL_WAIT_INTERVAL > tNowMs)
    {
        return 0;
    }

    m_llEpollWaitTime = tNowMs;
    m_iEventIdx = 0;
    m_iEventNum = epoll_wait(m_iFD, m_pstEpollEvent, m_iMaxFDNum, 0);
    if (0 > m_iEventNum)
    {
        LOG_ERR_FMT(ptrNetLogger, "epoll[{}] wait fail, {}", m_iFD, GetSocketError());
        return -1;
    }

    return m_iEventNum;
}

int EpollMgr::PollEvent(int* fd, int* events)
{
    if (m_iEventIdx >= m_iEventNum)
    {
        return -1;
    }

    *fd = m_pstEpollEvent[m_iEventIdx].data.fd;
    *events = m_pstEpollEvent[m_iEventIdx].events;
    m_iEventIdx++;

    return 0;
}