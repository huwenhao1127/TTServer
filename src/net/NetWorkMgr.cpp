#include <vector>
#include <string>
#include <arpa/inet.h>
#include "NetWorkMgr.h"
#include "NetWork.h"
#include "EpollMgr.h"

NetWorkMgr::NetWorkMgr()
{
}

NetWorkMgr::~NetWorkMgr()
{
}

int NetWorkMgr::Init()
{
    
    m_mapNetWork.clear();
    if (UDP_ADDR_USE_ANY_ADDRESS)
    {
        NetWork::ptr pNetWork = CreateNetWork(INADDR_ANY);
        if (nullptr == pNetWork)
        {
            LOG_ERR_FMT(ptrNetLogger, "create network fail using INADDR_ANY");
            return -1;
        }
    }
    else
    {
        // 每个网卡地址创建一个fd, 需要手动指定网卡列表
        std::vector<std::string> vecNIC = {"eth0"};
        for (std::string& sNIC : vecNIC)
        {
            NetWork::ptr pNetWork = CreateNetWork(sNIC);
            if (nullptr == pNetWork)
            {
                LOG_ERR_FMT(ptrNetLogger, "NIC[{}] create network fail.", sNIC);
                return -1;
            }
        }
    }
    LOG_DBG_FMT(ptrNetLogger, "net work init succ.");
    return 0;
}

int NetWorkMgr::Proc()
{
    int iEventNum = EpollMgr::Inst().PollWait();
    if (iEventNum <= 0)
    {
        return 0;
    }

    int iFD = 0;
    int iEvent = 0;
    int iProcNum = 0;
    while(0 == EpollMgr::Inst().PollEvent(&iFD, &iEvent))
    {
        if (m_mapNetWork.find(iFD) == m_mapNetWork.end() || nullptr == m_mapNetWork[iFD])
        {
            LOG_ERR_FMT(ptrNetLogger, "network[{}] not found", iFD);
            EpollMgr::Inst().PollDel(iFD);
            close(iFD);
            continue;
        }
        iProcNum += m_mapNetWork[iFD]->ProcIOEvent(iEvent);
    }

    return iProcNum;
}

NetWork::ptr NetWorkMgr::CreateNetWork(std::string& sNIC)
{
    // 获取网卡ip
    char** arrIp = localIP(sNIC.c_str());
    if (nullptr == arrIp)
    {
        LOG_ERR_FMT(ptrNetLogger, "get NIC[{}] ip fail.", sNIC);
        return nullptr;
    }

    in_addr_t ulAddr = inet_addr(arrIp[0]);
    
    for (auto iter = arrIp; *iter != nullptr; ++iter)
    {
        free(*iter);
    }
    free(arrIp);
    arrIp = nullptr;

    return CreateNetWork(ulAddr);
}

NetWork::ptr NetWorkMgr::CreateNetWork(in_addr_t ulAddr)
{
    if (INADDR_NONE == ulAddr)
    {
        LOG_ERR_FMT(ptrNetLogger, "create network fail. adddr:{}", ulAddr);
        return nullptr;
    }

    // 设置地址与端口
    sockaddr_in stSockAddr;
    ZeroStruct(stSockAddr);
    stSockAddr.sin_family = AF_INET;
    stSockAddr.sin_port = htons(UDP_ADDR_PORT);
    stSockAddr.sin_addr.s_addr = ulAddr;

    // 创建socket
    int iFD = CreateSocket(stSockAddr);
    if (0 > iFD)
    {
        LOG_ERR_FMT(ptrNetLogger, "create network fail. adddr:{}", sock_addr(&stSockAddr));
        return nullptr;
    }
    
    // 将fd加入epoll监听队列
    if (0 != EpollMgr::Inst().PollAdd(iFD, EPOLLIN | EPOLLOUT))
    {
        close(iFD);
        LOG_ERR_FMT(ptrNetLogger, "poll add fail. adddr:{}, fd:{}", sock_addr(&stSockAddr), iFD);
        return nullptr;
    }

    // 创建网络对象
    uint64_t ullNetWorkID = sockaddr_to_id(stSockAddr);
    NetWork::ptr pNetWork(new NetWork(iFD, ullNetWorkID, stSockAddr));
    if (nullptr == pNetWork)
    {
        EpollMgr::Inst().PollDel(iFD);
        close(iFD);
        LOG_ERR_FMT(ptrNetLogger, "create network fail. adddr:{}, fd:{}", sock_addr(&stSockAddr), iFD);
        return nullptr;
    }

    m_mapNetWork[iFD] = pNetWork;
    LOG_DBG_FMT(ptrNetLogger, "create network succ. adddr:{}, fd:{}, id:{}", sock_addr(&stSockAddr), iFD, ullNetWorkID);
    return pNetWork;
}

int NetWorkMgr::CreateSocket(const struct sockaddr_in& stAddr)
{
    int iFD = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (0 > iFD)
    {
        LOG_ERR_FMT(ptrNetLogger, "create socket fail. adddr:{} error:{}", sock_addr(&stAddr), GetSocketError());
        return -1;
    }

    if (0 != sock_reuse(iFD, 1))
    {
        close(iFD);
        LOG_ERR_FMT(ptrNetLogger, "socket reuse fail. adddr:{}", sock_addr(&stAddr));
        return -1;
    }

    if (0 != sock_reuse_port(iFD, 1))
    {
        close(iFD);
        LOG_ERR_FMT(ptrNetLogger, "socket reuse port fail. adddr:{}", sock_addr(&stAddr));
        return -1;
    }

    if (0 != bind(iFD, (const struct sockaddr*)&stAddr, sizeof(stAddr)))
    {
        close(iFD);
        LOG_ERR_FMT(ptrNetLogger, "socket bind fail. adddr:{}, error:{}", sock_addr(&stAddr), GetSocketError());
        return -1;
    }

    // 设置缓冲区大小
    if (0 != sock_wbuf(iFD, SOCKET_SND_BUF_SIZE))
    {
        close(iFD);
        LOG_ERR_FMT(ptrNetLogger, "socket set snd buff fail. adddr:{}, error:{}", sock_addr(&stAddr), GetSocketError());
        return -1;
    }

    if (0 != sock_rbuf(iFD, SOCKET_RCV_BUF_SIZE))
    {
        close(iFD);
        LOG_ERR_FMT(ptrNetLogger, "socket set rcv buff fail. adddr:{}, error:{}", sock_addr(&stAddr), GetSocketError());
        return -1;
    }
    return iFD;
}