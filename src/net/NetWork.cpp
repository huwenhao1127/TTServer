#include "EpollMgr.h"
#include "NetWork.h"
#include "NetCommDef.h"
#include "NetMgr.h"
#include "Socket.h"

char NetWork::s_szRecvBuff[MAX_NET_RECV_BUF_LEN] = {0};

NetWork::NetWork(uint64_t ullID, int iSockFD, const sockaddr_in& stSockAddr)
{
    m_ullID = ullID;
    m_iSockFD = iSockFD;
    m_stSockAddr = stSockAddr;
}

NetWork::~NetWork()
{
}

int NetWork::ProcIOEvent(int iEvent)
{
    int iProcNum = 0;
    if (iEvent & EPOLLIN)
    {
        iProcNum += DoIORead();
    }

    if (iEvent & EPOLLOUT)
    {
        iProcNum += DoIOWrite();
    }

    if (iEvent & EPOLLERR)
    {
        LOG_ERR_FMT(ptrNetLogger, " fd[{}] io event error, {}", m_iSockFD, GetSocketError());
    }
    return iProcNum;
}

int NetWork::DoIOWrite()
{

    return 0;
}

int NetWork::DoIORead()
{
    // 读网络包
    int iRecvNum = 0;
    uint64_t ullRecvSize = 0;
    while (1000 > iRecvNum)
    {
        sockaddr_in stClientAddr;
        socklen_t stAddrLen;
        int iCurRecvSize = recvfrom(m_iSockFD, s_szRecvBuff, sizeof(s_szRecvBuff), 0, (struct sockaddr*)&stClientAddr, &stAddrLen);
        
        if (iCurRecvSize <= 0)
        {
            if (EAGAIN != errno)
            {
                LOG_ERR_FMT(ptrNetLogger, " fd[{}] recvform[{}] error:{}", m_iSockFD, sock_addr(&stClientAddr), errno);
            }
            LOG_DBG_FMT(ptrNetLogger, " fd[{}] recvform size: {}", m_iSockFD, iCurRecvSize);
            break;
        }

        ++iRecvNum;
        ullRecvSize += iCurRecvSize;
        // 解包
        m_oMsg.Init();
        if (0 != m_oMsg.DecodeMsg(s_szRecvBuff, iCurRecvSize))
        {
            LOG_ERR_FMT(ptrNetLogger, " msg decode fail, recvform[{}] len:{}", sock_addr(&stClientAddr), iCurRecvSize);
            continue;
        }

        // 处理消息
        HandleNetMsg(m_oMsg, stClientAddr);
    }

    LOG_DBG_FMT(ptrNetLogger, " network id[{}] recv num:{} recv size:{}", m_ullID, iRecvNum, ullRecvSize);
    return iRecvNum;
}

int NetWork::HandleNetMsg(const NetMsg& oMsg, const sockaddr_in& stClientAddr)
{
    switch (oMsg.Head().bType)
    {
        case NET_PACKET_HANDSHAKE1:
        {
            return HandleHandShake1(oMsg, stClientAddr);
        }
        case NET_PACKET_HANDSHAKE2:
        {
            return HandleHandShake2(oMsg, stClientAddr);
        }
        case NET_PACKET_RECONNECT:
        {
            return HandleReconnect(oMsg, stClientAddr);
        }
        default:
        {
            return HandleNetData(oMsg, stClientAddr);
        }
    }
    return 0;
}

int NetWork::HandleHandShake1(const NetMsg& oMsg, const sockaddr_in& stClientAddr)
{
    // 第一次握手,生成cookies
    
    return 0;
}

int NetWork::HandleHandShake2(const NetMsg& oMsg, const sockaddr_in& stClientAddr)
{

    return 0;
}

int NetWork::HandleReconnect(const NetMsg& oMsg, const sockaddr_in& stClientAddr)
{

    return 0;
}

int NetWork::HandleNetData(const NetMsg& oMsg, const sockaddr_in& stClientAddr)
{

    return 0;
}

int NetWork::SendFinMsg(EnmNetPacketType eType, uint32_t ulConnID, uint8_t bReason, const sockaddr_in& stClientAddr)
{
    return 0;
}