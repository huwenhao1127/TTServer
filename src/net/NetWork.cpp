#include "EpollMgr.h"
#include "NetWork.h"
#include "NetCommDef.h"
#include "NetMgr.h"
#include "Socket.h"
#include "NetConnect.h"
#include "NetConnectMgr.h"
#include "Now.h"
#include "CookieMgr.h"
#include <malloc.h>

char NetWork::s_szRecvBuff[MAX_NET_RECV_BUF_LEN] = {0};

NetWork::NetWork(uint64_t ullID, int iSockFD, const sockaddr_in& stSockAddr)
{
    m_ullID = ullID;
    m_iSockFD = iSockFD;
    m_stSockAddr = stSockAddr;
    m_ulConnID = 0;
    ZeroStruct(m_stEncyptData);
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
    if (m_oSendQueue.IsEmpty())
    {
        return 0;
    }

    int iSendNum = 0;
    size_t ulSendSize = 0;

    while (!m_oSendQueue.IsEmpty())
    {
        SendQueueNode *pstNode = m_oSendQueue.Front();
        CHECK_IF_PARAM_NULL(ptrNetLogger, pstNode, 0);
        STNetPacket *pstPacket = send_queue_entry(pstNode, STNetPacket, stQueNode);
        CHECK_IF_PARAM_NULL(ptrNetLogger, pstPacket, 0);
        
        const sockaddr_in& stClientAddr = pstPacket->stClientAddr;
        int iSize = sendto(m_iSockFD, pstPacket->szBuff, pstPacket->ulBuffLen, 0, (const struct sockaddr*)&stClientAddr, sizeof(stClientAddr));
        if (0 > iSize)
        {
            if (EAGAIN == errno)
            {
                LOG_DBG_FMT(ptrNetLogger, " send to EAGAIN, fd[{}], num[{}], size[{}]", m_iSockFD, iSendNum, ulSendSize);
            }
            else
            {
                // 发生错误,删除当前包
                m_oSendQueue.Pop();
                free(pstPacket);
                LOG_ERR_FMT(ptrNetLogger, " send to {} fail: {}, fd[{}]", sock_addr(&stClientAddr), strerror(errno), m_iSockFD);
            }
            break;
        }

        iSendNum++;
        ulSendSize += iSize;

        // 执行连接对象回调
        if (nullptr != pstPacket->poConn)
        {
            pstPacket->poConn->OnIOWrite(*pstPacket);
        }

        LOG_DBG_FMT(ptrNetLogger, "fd[{}] sendto[{}] succ, num[{}], size[{}], Queue left:{}", m_iSockFD, sock_addr(&stClientAddr), iSendNum, ulSendSize, m_oSendQueue.Size());
        m_oSendQueue.Pop();
        free(pstPacket);
    }
    
    LOG_DBG_FMT(ptrNetLogger, "fd[{}] sendto succ, num[{}], size[{}], Queue left:{}", m_iSockFD, iSendNum, ulSendSize, m_oSendQueue.Size());
    return iSendNum;
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
        case NET_PACKET_HANDSHAKE1_ACK:
        {
            return HandleHandShake1Ack(oMsg, stClientAddr);
        }
        case NET_PACKET_HANDSHAKE2_ACK:
        {
            return HandleHandShake2Ack(oMsg, stClientAddr);
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
    const STHandShakePacket& stPacket = oMsg.Body().stHandShake;
    NetConnect* poConn = NetConnectMgr::Inst().GetConnByAddr(stClientAddr);
    if (nullptr != poConn)
    {
        // 链接已经存在
        LOG_DBG_FMT(ptrNetLogger, " conn already exits, client[{}] connid[{}]", sock_addr(&stClientAddr), poConn->GetConnectID());
        return 0;
    }

    // 第一次握手,生成cookies
    STNetMsgHead stHead = {0};
    stHead.bIsReliable = 0;
    stHead.bType = NET_PACKET_HANDSHAKE1_ACK;

    STHandShakePacket stRsp = {0};
    stRsp.dwTimeStamp = Now::TimeStamp();
    stRsp.bScreteKey = CookieMgr::Inst().GetKeySeq();
    stRsp.ullExtData = stPacket.ullExtData;
    int iRet = CookieMgr::Inst().CreateCookie(stClientAddr, Now::TimeStamp(), stRsp.bScreteKey, stRsp.szCookies, MAX_HANDSHAKE_COOKIES_SIZE);
    if (0 != iRet)
    {
        LOG_ERR_FMT(ptrNetLogger, "client[{}] create cookie fail.", sock_addr(&stClientAddr));
        return -1;
    }

    // 发到发送队列
    iRet = SendHandShakeMsg(stHead, stRsp, stClientAddr);
    if (0 !=  iRet)
    {
        LOG_ERR_FMT(ptrNetLogger, "client[{}] handshake1 fail, send msg fail.", sock_addr(&stClientAddr));
        return -1;
    }

    LOG_DBG_FMT(ptrNetLogger, "client[{}] handshake1 succ.", sock_addr(&stClientAddr));
    return 0;
}

int NetWork::SendHandShakeMsg(STNetMsgHead& stHead, STHandShakePacket& stPacket, const sockaddr_in& stClient)
{
    STNetPacket *pstNetPacket = NewPacket(HANDSHAKE_PACKET_SIZE);
    CHECK_IF_PARAM_NULL(ptrNetLogger, pstNetPacket, -1);

    NetWriter oWriter(pstNetPacket->szBuff, pstNetPacket->ulBuffLen);
    oWriter.Write(stHead);
    oWriter.Write(stPacket);
    pstNetPacket->ulDataLen = oWriter.GetSize();
    pstNetPacket->stClientAddr = stClient;
    return 0;
}

int NetWork::SendHandShakeSucc(const STHandShakePacket& stReq, NetConnect& oConn)
{
    STNetMsgHead stHead = {0};
    stHead.bIsReliable = 0;
    stHead.bType = NET_PACKET_HANDSHAKE2_ACK;

    STHandShakePacket stRsp = {0};
    stRsp.dwTimeStamp = stReq.dwTimeStamp;
    stRsp.bScreteKey  = stReq.bScreteKey;
    stRsp.dwConnId    = oConn.GetConnectID();
    stRsp.ullExtData  = stReq.ullExtData;
    memcpy(stRsp.szCookies, stReq.szCookies, MAX_HANDSHAKE_COOKIES_SIZE);
    const STEncyptData& stEncyptData = oConn.GetEncyptData();

    if (1 == stEncyptData.bIsDHKey)
    {
        stRsp.bIsKey = 0;
        stRsp.bEncryptDataLen = stEncyptData.uiNumBLen;
        memcpy(stRsp.szEncryptData, stEncyptData.szNumB, stEncyptData.uiNumBLen);
    }
    else
    {
        stRsp.bIsKey = 1;
        stRsp.bEncryptDataLen = stEncyptData.uiKeyLen;
        memcpy(stRsp.szEncryptData, stEncyptData.szKey, stEncyptData.uiKeyLen);
    }

    LOG_DBG_FMT(ptrNetLogger, "client[{}] handshake2 succ.", sock_addr(&oConn.GetClientAddr()));
    return SendHandShakeMsg(stHead, stRsp, oConn.GetClientAddr());
}

int NetWork::SendHandShakeFail(const STHandShakePacket& stReq, const sockaddr_in& stClient, int iReason /* = 0 */)
{
    STNetMsgHead stHead = {0};
    stHead.bIsReliable = 0;
    stHead.bType = NET_PACKET_HANDSHAKE2_ACK;

    STHandShakePacket stRsp = {0};
    stRsp.dwTimeStamp = stReq.dwTimeStamp;
    stRsp.bScreteKey  = stReq.bScreteKey;
    stRsp.ullExtData  = stReq.ullExtData;
    stRsp.dwConnId = 0;
    memcpy(stRsp.szCookies, stReq.szCookies, MAX_HANDSHAKE_COOKIES_SIZE);

    LOG_DBG_FMT(ptrNetLogger, "client[{}] handshake2 fail, reason:{}", sock_addr(&stClient), iReason);
    return SendHandShakeMsg(stHead, stRsp, stClient);;
}

int NetWork::HandleHandShake2(const NetMsg& oMsg, const sockaddr_in& stClientAddr)
{
    const STHandShakePacket& stReq = oMsg.Body().stHandShake;
    NetConnect* poConn = NetConnectMgr::Inst().GetConnByAddr(stClientAddr);
    char szTmpCookie[MAX_HANDSHAKE_COOKIES_SIZE] = {0};
    if (nullptr != poConn)
    {
        const uint8_t bCurSeq = CookieMgr::Inst().GetKeySeq();
        if (0 != CookieMgr::Inst().CreateCookie(stClientAddr, stReq.dwTimeStamp, bCurSeq, szTmpCookie, MAX_HANDSHAKE_COOKIES_SIZE))
        {
            LOG_ERR_FMT(ptrNetLogger, "client[{}] create cookie fail.", sock_addr(&stClientAddr));
            return -1;
        }

        // 验证cookie和大数A是否一致
        const char *szConnCookie = poConn->GetCookie();
        const STEncyptData& stEncyptData = poConn->GetEncyptData();
        if (0 == memcmp(szTmpCookie, szConnCookie, MAX_HANDSHAKE_COOKIES_SIZE) &&
            0 == memcmp(szTmpCookie, stReq.szCookies, MAX_HANDSHAKE_COOKIES_SIZE) &&
            0 == memcmp(stEncyptData.szNumA, stReq.szEncryptData, stEncyptData.uiNumALen))
        {
            // 一致说明是丢包导致重复握手,直接回复成功
            SendHandShakeSucc(stReq, *poConn);
            LOG_DBG_FMT(ptrNetLogger, " client[{}] respon ack2 again.", sock_addr(&stClientAddr));
        }
        return 0;
    }

    // 检查序列号是否过期
    if (Now::TimeStamp() > (stReq.dwTimeStamp + MAX_HANDSHAKE_COOKIES_TIME))
    {
        SendHandShakeFail(stReq, stClientAddr, -1);
        LOG_ERR_FMT(ptrNetLogger, "client[{}] handshake2 fail, start time[{}]", sock_addr(&stClientAddr), stReq.dwTimeStamp);
        return -1;
    }

    // 检查cookie
    if (0 != CookieMgr::Inst().CreateCookie(stClientAddr, stReq.dwTimeStamp, stReq.bScreteKey, szTmpCookie, MAX_HANDSHAKE_COOKIES_SIZE))
    {
        SendHandShakeFail(stReq, stClientAddr, -2);
        LOG_ERR_FMT(ptrNetLogger, "client[{}] create cookie fail.", sock_addr(&stClientAddr));
        return -1;
    }

    if (0 != memcmp(szTmpCookie, stReq.szCookies, MAX_HANDSHAKE_COOKIES_SIZE))
    {
        SendHandShakeFail(stReq, stClientAddr, -3);
        LOG_ERR_FMT(ptrNetLogger, "client[{}] handshake2 fail, cookie different", sock_addr(&stClientAddr));
        return -1;
    }

    // 检查加密数据(大数A)
    if (0 >= stReq.bEncryptDataLen)
    {
        SendHandShakeFail(stReq, stClientAddr, -4);
        LOG_ERR_FMT(ptrNetLogger, "client[{}] handshake2 fail, EncryptDataLen: {}", sock_addr(&stClientAddr), (int)stReq.bEncryptDataLen);
        return -1;
    }

    STEncyptData stEncyptData = {0};
    stEncyptData.bIsDHKey = true;
    stEncyptData.uiNumALen = stReq.bEncryptDataLen;
    stEncyptData.uiNumBLen = MAX_ENCRYPT_DATA_LEN;
    memcpy(stEncyptData.szNumA, stReq.szEncryptData, stReq.bEncryptDataLen);

    // 根据大数A,使用DH生成密钥和大数B
    stEncyptData.uiKeyLen = MAX_ENCRYPT_DATA_LEN;
    int iRet = NetSecurityMgr::Inst().HandleHandShake2(
        stEncyptData.szNumA, stEncyptData.uiNumALen, stEncyptData.szNumB, stEncyptData.uiNumBLen, stEncyptData.szKey, stEncyptData.uiKeyLen);
    if (0 != iRet)
    {
        stEncyptData.bIsDHKey = false;
        std::string sKey = "";
        for (int i = 0; i < MAX_ENCRYPT_DATA_LEN; ++i)
        {
            stEncyptData.szKey[i] = (uint8_t)CookieMgr::RandomInt2(0, 255);
        }
        LOG_ERR_FMT(ptrNetLogger, "client[{}] generate DH key fail, use random key: {}", sock_addr(&stClientAddr), sKey);
    }
    NetSecurityMgr::PrintKey(stEncyptData.szKey, stEncyptData.uiKeyLen, "send server key:");

    // 创建新连接
    poConn = NetConnectMgr::Inst().CreateNewConn(this, stClientAddr, stReq.szCookies, stEncyptData);
    if (nullptr == poConn)
    {
        SendHandShakeFail(stReq, stClientAddr, -5);
        LOG_ERR_FMT(ptrNetLogger, "client[{}] handshake2 fail: create conn fail.", sock_addr(&stClientAddr));
        return -1;
    }

    // 握手并成功建立连接
    SendHandShakeSucc(stReq, *poConn);
    NetMgr::Inst().NotifyConnEvent(ENM_CONN_EVENT_TYPE_CONNECT, poConn->GetConnectID(), 0, stReq.ullExtData);
    LOG_DBG_FMT(ptrNetLogger, "client[{}] handshake2 succ, connid:{}, extdata:{}", sock_addr(&stClientAddr), poConn->GetConnectID(), stReq.ullExtData);
    return 0;
}

int NetWork::HandleReconnect(const NetMsg& oMsg, const sockaddr_in& stClientAddr)
{
    const STReconnectPacket& stPacket = oMsg.Body().stReconnect;

    // 检查客户端地址对应的链接是否存在
    NetConnect* poConn = NetConnectMgr::Inst().GetConnByAddr(stClientAddr);
    if (nullptr != poConn)
    {
        if (stPacket.dwConnId != poConn->GetConnectID())
        {
            SendReconnctAck(stClientAddr, nullptr);
            LOG_ERR_FMT(ptrNetLogger, "client[{}] reconnect fail, conn[{} {}] error.", sock_addr(&stClientAddr), stPacket.dwConnId, poConn->GetConnectID());
            return -1;
        }

        // 客户端地址没变且链接ID一致，直接重发ack
        SendReconnctAck(stClientAddr, poConn);
        LOG_DBG_FMT(ptrNetLogger, "client[{}] conn[{}] reconnect ack again.", sock_addr(&stClientAddr), poConn->GetConnectID());
        return 0;
    }

    // 客户端地址改变，用链接ID尝试获取链接对象
    poConn = NetConnectMgr::Inst().GetConnByID(stPacket.dwConnId);
    if (nullptr == poConn)
    {
        SendReconnctAck(stClientAddr, nullptr);
        LOG_ERR_FMT(ptrNetLogger, "client[{}] reconnect fail, not found conn.", sock_addr(&stClientAddr));
        return -1;
    }

    // 检查cookie是否一致
    if (0 != memcmp(stPacket.szCookies, poConn->GetCookie(), MAX_HANDSHAKE_COOKIES_SIZE))
    {
        SendReconnctAck(stClientAddr, nullptr);
        LOG_ERR_FMT(ptrNetLogger, "client[{}] reconnect fail, cookie error.", sock_addr(&stClientAddr));
        return -1;
    }

    // 执行重连
    if (0 != poConn->Reconnect(stClientAddr))
    {
        SendReconnctAck(stClientAddr, nullptr);
        LOG_ERR_FMT(ptrNetLogger, "client[{}] reconnect fail, conn[{}] reconnect fail.", sock_addr(&stClientAddr), poConn->GetConnectID());
        return -1;
    }

    SendReconnctAck(stClientAddr, poConn);
    LOG_DBG_FMT(ptrNetLogger, "client[{}] conn[{}] reconnect succ.", sock_addr(&stClientAddr), poConn->GetConnectID());
    return 0;
}

int NetWork::SendReconnctAck(const sockaddr_in& stClientAddr, NetConnect* poConn)
{
    STNetPacket *pstNetPacket = NewPacket(RECONNECT_PACKET_SIZE);
    CHECK_IF_PARAM_NULL(ptrNetLogger, pstNetPacket, -1);

    STNetMsgHead stHead = {0};
    stHead.bType = NET_PACKET_RECONNECT_ACK;
    stHead.bIsReliable = 0;

    STReconnectPacket stRsp = {0};
    if (nullptr != poConn)
    {
        stRsp.dwConnId = poConn->GetConnectID();
        memcpy(stRsp.szCookies, poConn->GetCookie(), MAX_HANDSHAKE_COOKIES_SIZE);
    }
    
    NetWriter oWriter(pstNetPacket->szBuff, pstNetPacket->ulBuffLen);
    oWriter.Write(stHead);
    oWriter.Write(stRsp);
    pstNetPacket->ulDataLen = oWriter.GetSize();
    pstNetPacket->stClientAddr = stClientAddr;
    return 0;
}

int NetWork::HandleNetData(const NetMsg& oMsg, const sockaddr_in& stClientAddr)
{
    NetConnect *poConn = NetConnectMgr::Inst().GetConnByAddr(stClientAddr);
    if (nullptr == poConn)
    {
        SendRstMsg(CONN_RST_TYPE_UNKNOWN_PEER, 0, stClientAddr);
        LOG_ERR_FMT(ptrNetLogger, "handle net data fail, get conn by addr[{}] fail.", sock_addr(&stClientAddr));
        return -1;
    }

    return poConn->HandleNetMsg(oMsg);
}

int NetWork::SendRstMsg(uint8_t bType, uint32_t ulConnID, const sockaddr_in& stClientAddr)
{
    STNetPacket *pstNetPacket = NewPacket(RST_PACKET_SIZE);
    CHECK_IF_PARAM_NULL(ptrNetLogger, pstNetPacket, -1);
    STNetMsgHead stHead = {NET_PACKET_RST, 0};
    STRstPacket stRst = {ulConnID, bType};

    NetWriter oWriter(pstNetPacket->szBuff, pstNetPacket->ulBuffLen);
    oWriter.Write(stHead);
    oWriter.Write(stRst);
    pstNetPacket->ulDataLen = oWriter.GetSize();
    pstNetPacket->stClientAddr = stClientAddr;
    return 0;
}

int NetWork::SendFinMsg(EnmNetPacketType eType, uint32_t ulConnID, uint8_t bReason, const sockaddr_in& stClientAddr)
{
    if (NET_PACKET_FIN != eType && NET_PACKET_FIN_ACK != eType)
    {
        LOG_ERR_FMT(ptrNetLogger, " send fin smg fail, type[{}] error", (int)eType);
        return -1;
    }

    STNetPacket *pstNetPacket = NewPacket(FIN_PACKET_SIZE);
    CHECK_IF_PARAM_NULL(ptrNetLogger, pstNetPacket, -1);
    STNetMsgHead stHead = {0};
    stHead.bType = eType;
    stHead.bIsReliable = 0;

    STFinPacket stRsp = {0};
    stRsp.dwConnId = ulConnID;
    stRsp.bReason = bReason;

    NetWriter oWriter(pstNetPacket->szBuff, pstNetPacket->ulBuffLen);
    oWriter.Write(stHead);
    oWriter.Write(stRsp);
    pstNetPacket->ulDataLen = oWriter.GetSize();
    pstNetPacket->stClientAddr = stClientAddr;
    return 0;
}

STNetPacket* NetWork::NewPacket(size_t ulBufferSize)
{
    const size_t ulNeedSize = sizeof(STNetPacket) + ulBufferSize;

    STNetPacket *poPacket = (STNetPacket*)malloc(ulNeedSize);
    if (nullptr == poPacket)
    {
        LOG_ERR_FMT(ptrNetLogger, " new packet fail, size:{}", ulBufferSize);
        return nullptr;
    }

    memset(poPacket, 0, ulNeedSize);
    poPacket->ulBuffLen = ulBufferSize;
    m_oSendQueue.Push(&poPacket->stQueNode);
    return poPacket;
}

int NetWork::SendHandShake1Msg(const sockaddr_in& stServerAddr)
{
    STNetMsgHead stHead = {NET_PACKET_HANDSHAKE1, 0};
    STHandShakePacket stPacket = {0};
    return SendHandShakeMsg(stHead, stPacket, stServerAddr);
}

int NetWork::HandleHandShake1Ack(const NetMsg& oMsg, const sockaddr_in& stServerAddr)
{
    const STHandShakePacket& stPacket = oMsg.Body().stHandShake;
    STHandShakePacket stRsp = {0};
    memcpy(&stRsp, &stPacket, sizeof(STHandShakePacket));

    // 生成大数A
    stRsp.bIsKey = 1;
    uint16_t usLen = MAX_ENCRYPT_DATA_LEN;
    m_stEncyptData.uiNumBLen = MAX_ENCRYPT_DATA_LEN;
    int iRet = NetSecurityMgr::Inst().HandleHandShake1ACK((unsigned char*)stRsp.szEncryptData, usLen, m_stEncyptData.szNumB, m_stEncyptData.uiNumBLen);
    if (0 != iRet)
    {
        LOG_ERR_FMT(ptrNetLogger, "client[{}] generate DH key fail", sock_addr(&m_stSockAddr));
        return -1;
    }
    stRsp.bEncryptDataLen = usLen;
    memcpy(m_stEncyptData.szNumA, stRsp.szEncryptData, stRsp.bEncryptDataLen);
    m_stEncyptData.uiNumALen = stRsp.bEncryptDataLen;

    STNetMsgHead stHead = {NET_PACKET_HANDSHAKE2, 0};
    // 发起第二次握手
    return SendHandShakeMsg(stHead, stRsp, stServerAddr);
}

int NetWork::HandleHandShake2Ack(const NetMsg& oMsg, const sockaddr_in& stServerAddr)
{
    if (m_ulConnID > 0)
    {
        LOG_DBG_FMT(ptrNetLogger, "client[{}] conn[{}] exist", sock_addr(&m_stSockAddr), m_ulConnID);
        return 0;
    }

    const STHandShakePacket& stPacket = oMsg.Body().stHandShake;
    if (stPacket.dwConnId == 0)
    {
        LOG_ERR_FMT(ptrNetLogger, "client[{}] handshake fail. server: {}", sock_addr(&m_stSockAddr), sock_addr(&stServerAddr));
        return 0;
    }
    
    if (stPacket.bIsKey)
    {
        // 直接保存密钥
        memcpy(m_stEncyptData.szKey, stPacket.szEncryptData, stPacket.bEncryptDataLen);
        m_stEncyptData.uiKeyLen = stPacket.bEncryptDataLen;
    }
    else
    {
        // 生成密钥
        m_stEncyptData.uiKeyLen = MAX_ENCRYPT_DATA_LEN;
        int iRet = NetSecurityMgr::Inst().HandleHandShake2ACK(
            (const uint8_t*)stPacket.szEncryptData, stPacket.bEncryptDataLen, m_stEncyptData.szNumA, m_stEncyptData.uiNumALen, 
            m_stEncyptData.szNumB, m_stEncyptData.uiNumBLen, m_stEncyptData.szKey, m_stEncyptData.uiKeyLen);
        if (0 != iRet)
        {
            LOG_ERR_FMT(ptrNetLogger, "client[{}] generate DH key fail", sock_addr(&m_stSockAddr));
            return -1;
        }

        NetSecurityMgr::PrintKey(m_stEncyptData.szKey, m_stEncyptData.uiKeyLen, "client key:");
    }

    // 创建新连接
    NetConnect* poConn = NetConnectMgr::Inst().CreateNewConn(this, stServerAddr, stPacket.szCookies, m_stEncyptData);
    if (nullptr == poConn)
    {
        LOG_ERR_FMT(ptrNetLogger, "client[{}] handshake2 fail: create conn fail.", sock_addr(&m_stSockAddr));
        return -1;
    }

    m_ulConnID = poConn->GetConnectID();
    // 握手并成功建立连接
    LOG_DBG_FMT(ptrNetLogger, "client[{}] handshake2 succ, connid:{}", sock_addr(&m_stSockAddr), poConn->GetConnectID());
    return 0;
}

int NetWork::SendHeartBeat(const sockaddr_in& stServerAddr)
{
    STNetMsgHead stHead = {NET_PACKET_HEARTBEAT, 0};
    STHeartBeatPacket stPacket = {0};
    stPacket.dwConnId = m_ulConnID;
    stPacket.ullClientTimeMs = Now::TimeStamp();

    STNetPacket *pstNetPacket = NewPacket(HEARTBEAT_PACKET_SIZE);
    CHECK_IF_PARAM_NULL(ptrNetLogger, pstNetPacket, -1);

    NetWriter oWriter(pstNetPacket->szBuff, pstNetPacket->ulBuffLen);
    oWriter.Write(stHead);
    oWriter.Write(stPacket);
    pstNetPacket->ulDataLen = oWriter.GetSize();
    pstNetPacket->stClientAddr = stServerAddr;

    NetConnect* pConn = NetConnectMgr::Inst().GetConnByID(m_ulConnID);
    if (nullptr != pConn)
    {
        pConn->UpdateHeartBeat(stPacket.ullClientTimeMs);
    }
    return 0;
}