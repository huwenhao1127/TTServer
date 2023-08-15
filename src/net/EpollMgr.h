#pragma once
#include <sys/epoll.h>
#include "Singleton.h"

class EpollMgr : public Singleton<EpollMgr>
{
public:
    EpollMgr() {}
    ~EpollMgr() {}
    
    /**
     * @brief 初始化
     * 
     */
    int Init();

    /**
     * @brief 关闭epoll并回收内存
     * 
     * @return int 
     */
    int Reclaim();

    /**
     * @brief 添加监听fd
     * 
     * @param iFD 
     * @param iMode 监听类型 EPOLLIN, EPOLLOUT, EPOLLERR
     * @return int 
     */
    int PollAdd(int iFD, int iMode);

    /**
     * @brief 更新fd监听类型
     * 
     * @param iFD 
     * @param iMode 
     * @return int 
     */
    int PollMode(int iFD, int iMode);

    /**
     * @brief 移除监听fd
     * 
     * @param iFD 
     * @param iMode 
     * @return int 
     */
    int PollDel(int iFD);

    /**
     * @brief Epoll Wait
     * 
     * @return int 
     */
    int PollWait();

    /**
     * @brief 从epoll就绪事件列表获取一个事件
     * 
     * @param iFD       [out]
     * @param iEvents   [out]
     * @return int 
     */
    int PollEvent(int* fd, int* events);

private:
    int m_iFD;                  // epoll fd
    int m_iMaxFDNum;            // 最大套接字数量
    int m_iEventIdx;            // epoll事件迭代
    int m_iEventNum;            // epoll事件数量
    int64_t m_llEpollWaitTime;  // epoll wait时间
    struct epoll_event* m_pstEpollEvent;    // epoll事件列表
};