#include "NetConnect.h"
#include "Now.h"
#include "NetMgr.h"
#include "NetConnectMgr.h"

NetConnect::NetConnect() : m_oKcp(*this)
{
    m_ulConnID = 0;
    m_eState = CONN_STATE_CLOSE;    
    m_tCreateTime = 0;
    m_ullBindData = 0;
    ZeroStruct(m_stClientAddr);
    ZeroStruct(m_sStatData);
    m_poNetWork = nullptr;
    m_ullNetWorkID = 0;

    m_poMergePacket = nullptr;
    m_ulCurMergeOffset = 0;
    ZeroStruct(m_szMergePacketBuffer);
    ZeroStruct(m_stMergeState);
    m_bEnableMerge = false;
    ZeroStruct(m_stEncyptData);
    ZeroStruct(m_szCookies);

    m_tLastHeartBeat = 0;
    m_tDisconnect = 0;
    m_tFinBegin = 0;
    m_tSendFin = 0;
    m_tTimeWait = 0;
    m_tClose = 0;
    m_tNetWorkSyn = 0;
    m_uiCloseReason = 0;
}

NetConnect::~NetConnect() 
{
}

void NetConnect::Tick1S()
{
    time_t tNow = Now::TimeStamp();
    // 状态切换
    switch (m_eState)
    {
        case CONN_STATE_ESTABLISHED:
        {
            // 检查心跳
            if (tNow >= m_tLastHeartBeat + HEARTBEAT_TIMEOUT)
            {
                DisConnect(DISCONNECT_REASON_HEARTBEAT);
            }
            // 往业务线程同步网络数据srtt(10s一次)
            if (0 != m_ullBindData && tNow >= m_tNetWorkSyn + 10)
            {
                m_tNetWorkSyn = tNow;
                NetMgr::Inst().NotifyConnEvent(ENM_CONN_EVENT_TYPE_NETWORK, m_ulConnID, 0, m_oKcp.GetIKCPCB().rx_srtt);
            }
            break;
        }
        case CONN_STATE_DISCONNET:
        {
            // 断线超过重连时间,关闭连接
            if (tNow >= m_tDisconnect + RECONNECT_TIMEOUT)
            {
                CloseConnect(CLOSE_REASON_DISCONNET_TIMEOUT);
            }
            break;
        }
        case CONN_STATE_FIN_WAIT1:
        case CONN_STATE_FIN_WAIT2:
        {
            // 检查FIN超时
            if (tNow >= m_tFinBegin + FIN_WAIT_SECONDS)
            {
                CloseConnect(CLOSE_REASON_FIN_TIMEOUT);
            }
            break;
        }
        case CONN_STATE_TIME_WAIT:
        {
            // 检查TIME_WAIT超时
            if (tNow >= m_tTimeWait + TIME_WAIT_SECONDS)
            {
                CloseConnect(CLOSE_REASON_NONE);
            }
            break;
        }
        default:
        {
            break;
        }
    }

    // 检查绑定超时
    if (!IsClose() && 0 == m_ullBindData && tNow >= m_tCreateTime + BIND_TIMEOUT)
    {
        CloseConnect(CLOSE_REASON_BIND_TIMEOUT);
    }

    // 定时检查统计数据
    if (tNow >= m_sStatData.tRefreshTime + CONN_STATE_INTERVAL)
    {
        // 流量异常检测
        if (m_sStatData.ulRecvCnt > MAX_CONN_RECV_NUM || m_sStatData.ulRecvSize > MAX_CONN_RECV_SIZE)
        {
            CloseConnect(CLOSE_REASON_FLOW_CONTROL);
            LOG_ERR_FMT(ptrNetLogger, "suspicious traffic detected! connid[{}] client[{}] recvcnt[{}] recvsize[{}]",
                        m_ulConnID, sock_addr(&m_stClientAddr), m_sStatData.ulRecvCnt, m_sStatData.ulRecvSize);
        }
    
        // 重置
        ZeroStruct(m_sStatData.tRefreshTime);
        m_sStatData.tRefreshTime = tNow;
    }
}

int NetConnect::Proc(time_t tNow)
{
    // 检查连接状态
    if (m_eState != CONN_STATE_ESTABLISHED && 
        m_eState != CONN_STATE_FIN_WAIT1 && 
        m_eState != CONN_STATE_FIN_WAIT2)
    {
        return 0;
    }

    // 收取可靠包
    int iProcNum = RecvMsgFromKcp();

    // 驱动kcp update
    m_oKcp.Update(tNow);

    // 检查DL断线
    if (m_oKcp.IsDeadLink() && m_eState == CONN_STATE_ESTABLISHED)
    {
        DisConnect(DISCONNECT_REASON_KCP_DEAD_LINK);
        return 0;
    }

    // 检查安全关闭流程
    if (m_eState == CONN_STATE_FIN_WAIT1 && 0 == m_oKcp.GetWaitSnd())
    {
        // 200ms内不重复发送FIN
        if (m_tSendFin + 200 < tNow)
        {
            m_poNetWork->SendFinMsg(NET_PACKET_FIN, m_ulConnID, m_uiCloseReason, m_stClientAddr);
            m_tSendFin = tNow;
            LOG_DBG_FMT(ptrNetLogger, " conn[{}] send fin", m_ulConnID);
        }
    }
    return iProcNum;
}

int NetConnect::InitConnect(
    uint32_t ulConnID, NetWork *pNetWork, const sockaddr_in& stClientAddr, const char *szCookies, const struct STEncyptData& stEncyptData)
{
    CHECK_IF_PARAM_NULL(ptrNetLogger, pNetWork, -1);
    CHECK_IF_PARAM_NULL(ptrNetLogger, szCookies, -1);

    // 基础信息
    m_ulConnID = ulConnID;
    m_eState = CONN_STATE_ESTABLISHED;
    m_stClientAddr = stClientAddr;
    m_poNetWork = pNetWork;
    m_ullNetWorkID = m_poNetWork->GetID();

    m_tCreateTime = Now::TimeStamp();
    m_tLastHeartBeat = Now::TimeStamp();
    m_ullBindData = 0;

    // 统计信息
    ZeroStruct(m_sStatData);
    m_sStatData.tRefreshTime = Now::TimeStamp();

    // 加密信息
    m_stEncyptData = stEncyptData;
    snprintf(m_szCookies, sizeof(m_szCookies), "%s", szCookies);

    // 初始化加密器
    if (m_oSecurityMgr.SetEncKey(stEncyptData.szKey, stEncyptData.uiKeyLen))
    {
        LOG_ERR_FMT(ptrNetLogger, " conn[{}] set enckey fail, keylen[{}] client[{}]", m_ulConnID, stEncyptData.uiKeyLen, sock_addr(&stClientAddr));
        return -1;
    }

    // 初始化kcp
    if (0 != m_oKcp.Init())
    {
        LOG_ERR_FMT(ptrNetLogger, " conn[{}] init kcp fail, client[{}]", m_ulConnID, sock_addr(&stClientAddr));
        return -1;
    }
    return 0;
}

int NetConnect::Send2NetWork(const char* szData, int iLen, bool bReliable)
{
    CHECK_IF_PARAM_NULL(ptrNetLogger, szData, -1);

    // 加通用网络包头
    size_t ulNeedSize = iLen + sizeof(STNetMsgHead);
    STNetPacket *pstNetPacket = m_poNetWork->NewPacket(ulNeedSize);
    CHECK_IF_PARAM_NULL(ptrNetLogger, pstNetPacket, -1);

    NetWriter oWriter(pstNetPacket->szBuff, pstNetPacket->ulBuffLen);
    // 写头
    STNetMsgHead stHead = {0};
    stHead.bType = NET_PACKET_DATA;
    stHead.bIsReliable = bReliable;
    oWriter.Write(stHead);

    // 写数据
    oWriter.WriteRawData(szData, iLen);
    pstNetPacket->ulDataLen = oWriter.GetSize();
    pstNetPacket->ulConnectID = m_ulConnID;
    pstNetPacket->poConn = this;
    pstNetPacket->stClientAddr = m_stClientAddr;
    return 0;
}

int NetConnect::SendMsg(const char *pData, int iLen, const MsgOpt& stOpt)
{
    if (CONN_STATE_ESTABLISHED != m_eState)
    {
        LOG_ERR_FMT(ptrNetLogger, " conn[{}] send msg fail, cur state:{}", m_ulConnID, (int)m_eState);
        return -1;
    }

    // 网络数据编码
    static char szEncodeMsgBuff[MAX_ENCODE_BUF_SIZE];
    int iEncodeMsgLen = sizeof(szEncodeMsgBuff);
    if (0 != EncodeMsgData(pData, iLen, szEncodeMsgBuff, iEncodeMsgLen, stOpt.bIsEncrypt))
    {
        LOG_ERR_FMT(ptrNetLogger, " conn[{}] send msg fail, encode msg fail", m_ulConnID);
        return -1;
    }

    // 发送可靠包
    if (stOpt.bIsReliable)
    {
        return SendReliable(szEncodeMsgBuff, iEncodeMsgLen);
    }

    // 发送非可靠包
    return SendUnreliable(szEncodeMsgBuff, iEncodeMsgLen);
}

int NetConnect::EncodeMsgData(const char *pInData, int iInDataLen, char *pEncodeData, int& iEncodeDataLen, bool bEncrypt)
{
    DataHead stHead = {0};
    stHead.bIsEncrypt = bEncrypt;
    //char *pHead = pEncodeData;
    //char *pData = pEncodeData + sizeof(DataHead);

    // 数据加密
    if (bEncrypt)
    {
        ;
    }

    // 检查数据长度是否超过编码缓冲区
    if (sizeof(DataHead) + (size_t)iInDataLen > (size_t)iEncodeDataLen)
    {
        LOG_ERR_FMT(ptrNetLogger, " conn[{}] encode msg fail, data len: {}", m_ulConnID, iInDataLen);
        return -1;
    }

    // 写头
    NetWriter oWriter(pEncodeData, iEncodeDataLen);
    oWriter.WriteDataHead(stHead);
    // 写数据
    oWriter.WriteRawData(pInData, iInDataLen);

    LOG_DBG_FMT(ptrNetLogger, " conn[{}] encode succ without encrypt", m_ulConnID);
    return 0;
}

int NetConnect::SendUnreliable(const char *pData, int iDataLen)
{
    // 检查数据长度
    if (MAX_UNRELIABLE_MSG_SIZE < (size_t)iDataLen)
    {
        LOG_ERR_FMT(ptrNetLogger, " conn[{}] send unreliable msg fail, data len: {}", m_ulConnID, iDataLen);
        return -1;
    }

    // 检查并获取当前合并包
    STNetPacket *pstPacket = GetCurMergePacket(iDataLen);
    if (nullptr == pstPacket)
    {
        LOG_ERR_FMT(ptrNetLogger, " conn[{}] send unreliable msg fail, get merge packet fail.", m_ulConnID);
        return -1;
    }

    // 写消息
    NetWriter oWriter(m_poMergePacket->szBuff + m_poMergePacket->ulDataLen, m_poMergePacket->ulBuffLen - m_poMergePacket->ulDataLen);
    oWriter.WriteUnreliable(pData, iDataLen);
    m_poMergePacket->ulDataLen += oWriter.GetSize();
    return 0;
}

STNetPacket* NetConnect::GetCurMergePacket(size_t iLen)
{
    // 检查当前合并包空间是否足够
    if (nullptr != m_poMergePacket)
    {
        const int iRemain = m_poMergePacket->ulBuffLen - m_poMergePacket->ulDataLen;
        // buffer里面放合包数据，包之间用数据长度(uint16_t)分隔
        if ((size_t)iRemain > iLen + sizeof(uint16_t))
        {
            return m_poMergePacket;
        }
    }

    // 创建新的合并包
    m_poMergePacket = m_poNetWork->NewPacket(MAX_PACKET_SIZE);
    if (nullptr == m_poMergePacket)
    {
        LOG_ERR_FMT(ptrNetLogger, " conn[{}] get merge packet fail.", m_ulConnID);
        return nullptr;
    }

    // 初始化非可靠包
    m_poMergePacket->ulConnectID = m_ulConnID;
    m_poMergePacket->stClientAddr = m_stClientAddr;
    m_poMergePacket->poConn = this;

    // 写入包头
    STNetMsgHead stMsgHead = {0};
    stMsgHead.bType = NET_PACKET_DATA;
    stMsgHead.bIsReliable = 0;
    NetWriter oWriter(m_poMergePacket->szBuff, m_poMergePacket->ulBuffLen);
    oWriter.Write(stMsgHead);
    m_poMergePacket->ulDataLen = oWriter.GetSize();

    // 返回当前合并包
    return m_poMergePacket;
}

int NetConnect::SendReliable(const char *pData, int iDataLen)
{
    // 检查数据长度
    if (MAX_RELIABLE_MSG_SIZE < iDataLen)
    {
        LOG_ERR_FMT(ptrNetLogger, " conn[{}] send reliable msg fail, data len: {}", m_ulConnID, iDataLen);
        return -1;
    }

    // 发送到kcp
    int iRet = m_oKcp.Send(pData, iDataLen);
    if (0 != iRet)
    {
        LOG_ERR_FMT(ptrNetLogger, " conn[{}] send reliable msg fail, kcp send fail: {}", m_ulConnID, iRet);
        return -1;
    }

    // 统计下行拥塞
    if (m_sStatData.ulMaxWaitSend < (uint32_t)m_oKcp.GetWaitSnd())
    {
        m_sStatData.ulMaxWaitSend = m_oKcp.GetWaitSnd();
    }
    return 0;
}

int NetConnect::DisConnect(EnmDisconnetReason eReason)
{
    if (CONN_STATE_ESTABLISHED != m_eState)
    {
        LOG_ERR_FMT(ptrNetLogger, " conn[{}] is not in ESTABLISHED, state:{} reason:{}", m_ulConnID, (int)m_eState, (int)eReason);
        return -1;
    }

    // 设置连接状态
    m_eState = CONN_STATE_DISCONNET;
    m_tDisconnect = Now::TimeStamp();

    // 通知业务断线
    NetMgr::Inst().NotifyConnEvent(ENM_CONN_EVENT_TYPE_DISCONNECT, m_ulConnID, eReason, m_ullBindData);
    LOG_DBG_FMT(ptrNetLogger, " conn[{}] disconnect, reason:{}", m_ulConnID, (int)eReason);
    return 0;
}

int NetConnect::CloseConnect(EnmCloseReason eReason)
{
    if (IsClose())
    {
        return 0;
    }

    // 设置连接状态
    ClearConnData();
    m_eState = CONN_STATE_CLOSE;
    m_uiCloseReason = eReason;
    m_tClose = Now::TimeStamp();

    // 通知业务连接关闭
    if (CLOSE_REASON_NONE != eReason)
    {
        NetMgr::Inst().NotifyConnEvent(ENM_CONN_EVENT_TYPE_CLOSE, m_ulConnID, eReason, m_ullBindData);
    }

    // 移动到待回收map  
    NetConnectMgr::Inst().MoveToClosedMap(this);
    LOG_DBG_FMT(ptrNetLogger, " conn[{}] close, reason:{}", m_ulConnID, (int)eReason);
    return 0;
}

void NetConnect::ClearConnData()
{
    m_oKcp.Reset();
    m_poMergePacket = nullptr;
    m_ulCurMergeOffset = 0;
}

int NetConnect::RecvMsgFromKcp()
{
    if (!m_oKcp.NeedRecv())
    {
        return 0;
    }

    int iMsgNum = 0;
    uint64_t ullMsgSize = 0;

    while (true)
    {
        size_t iPeekLen = m_oKcp.PeekSize();
        if (0 > iPeekLen)
        {
            // 消息收完
            m_oKcp.ResetNeedRcv();
            break;
        }

        if (MAX_ENCODE_BUF_SIZE < iPeekLen || 0 >= iPeekLen)
        {
            // 消息长度异常
            m_oKcp.Recv(nullptr, iPeekLen);
            LOG_ERR_FMT(ptrNetLogger, " conn[{}] invalid msg len:{}", m_ulConnID, iPeekLen);
            continue;
        }

        // 收取可靠网络包
        static char szMsgBuff[MAX_ENCODE_BUF_SIZE];
        size_t iMsgLen = m_oKcp.Recv(szMsgBuff, sizeof(szMsgBuff));
        if (0 > iMsgLen || iMsgLen != iPeekLen)
        {
            LOG_ERR_FMT(ptrNetLogger, " conn[{}] kcp recv fail, msglen[{}] peeklen[{}]", m_ulConnID, iMsgLen, iPeekLen);
            break;
        }

        // 将数据解码后发给业务
        if (0 != RecvOneMsg(szMsgBuff, iMsgLen))
        {
            LOG_ERR_FMT(ptrNetLogger, " conn[{}] recv msg fail, msglen[{}]", m_ulConnID, iMsgLen);
            continue;
        }

        iMsgNum++;
        ullMsgSize += iMsgLen;
    }
    return 0;
}

int NetConnect::RecvOneMsg(const char* szMsg, int iMsgLen)
{
    if (sizeof(DataHead) > (size_t)iMsgLen)
    {
        LOG_ERR_FMT(ptrNetLogger, " conn[{}] invalid msg len:{}", m_ulConnID, iMsgLen);
        return -1;
    }

    // 先处理DataHead
    DataHead stHead = {0};
    NetReader oReader(szMsg, iMsgLen);
    oReader.ReadDataHead(stHead);

    // 消息头
    STRecvMsgHead stRecvHead = {0};
    stRecvHead.bMsgType = ENM_CONN_MSG_TYPE_PROTO;
    stRecvHead.bIsProc = false;
    stRecvHead.ulConnID = m_ulConnID;
    stRecvHead.iDataLen = oReader.GetRemain();

    // 数据解密
    if (stHead.bIsEncrypt)
    {
        ;
    }

    // 发送给主线程
    return NetMgr::Inst().SendN2MMsg(stRecvHead, oReader.GetBuff());
}

int NetConnect::BindConnect(const STBindConnMsg& stBindMsg)
{
    if (0 == stBindMsg.ullBindData)
    {
        LOG_ERR_FMT(ptrNetLogger, "conn[{}] bind fail, data:{}", m_ulConnID, stBindMsg.ullBindData);
        return -1;
    }

    if (0 != m_ullBindData)
    {
        LOG_ERR_FMT(ptrNetLogger, "conn[{}] bind fail, bind data:{} exists ", m_ulConnID, m_ullBindData);
        return -1;
    }

    // 设置绑定数据
    m_ullBindData = stBindMsg.ullBindData;

    // 通知绑定事件
    NetMgr::Inst().NotifyConnEvent(ENM_CONN_EVENT_TYPE_BIND, m_ulConnID, 0, stBindMsg.ullBindData, &m_stClientAddr, &stBindMsg);
    LOG_DBG_FMT(ptrNetLogger, "conn[{}] bind, bind data:{}", m_ulConnID, stBindMsg.ullBindData);
    return 0;
}

int NetConnect::FinConnect(EnmCloseReason eCloseReason)
{
    // 断线状态直接关闭
    if (CONN_STATE_DISCONNET == m_eState)
    {
        return CloseConnect(eCloseReason);
    }

    // 活跃状态才走Fin流程
    if (CONN_STATE_ESTABLISHED != m_eState)
    {
        LOG_ERR_FMT(ptrNetLogger, "conn[{}] fin fail, state[{}] invalid", m_ulConnID, (int)m_eState);
        return -1;
    }

    // 更新链接状态，等数据发送完成后再发送Fin包
    m_eState = CONN_STATE_FIN_WAIT1;
    m_uiCloseReason = eCloseReason;
    m_tFinBegin = Now::TimeStamp();

    LOG_DBG_FMT(ptrNetLogger, "conn[{}] fin, reason:{}", m_ulConnID, (int)eCloseReason);
    return 0;
}

int NetConnect::HandleNetMsg(const NetMsg& oMsg)
{
    // 检查断线
    if (IsDisConnect())
    {
        ClearConnData();
        m_poNetWork->SendRstMsg(CONN_RST_TYPE_DISCONENCT, m_ulConnID, m_stClientAddr);
        LOG_DBG_FMT(ptrNetLogger, "conn[{}] send rst msg for disconnect.", m_ulConnID);
        return 0;
    }

    // 检查关闭
    if (IsClose())
    {
        m_poNetWork->SendRstMsg(CONN_RST_TYPE_CLOSE, m_ulConnID, m_stClientAddr);
        LOG_DBG_FMT(ptrNetLogger, "conn[{}] send rst msg for close.", m_ulConnID);
        return 0;
    }

    // 更新统计数据
    m_sStatData.ulRecvCnt++;
    m_sStatData.ulRecvSize += oMsg.NetSize();

    // 消息处理
    const STNetMsgHead& stHead = oMsg.Head();
    LOG_DBG_FMT(ptrNetLogger, " conn[{}] handle msg, type[{}], size[{}], client[{}]", m_ulConnID, stHead.bType, oMsg.NetSize(), sock_addr(&m_stClientAddr));
    switch (stHead.bType)
    {
        case NET_PACKET_DATA:
        {
            const STDataPacket& stDataPacket = oMsg.Body().stData;
            if (stHead.bIsReliable)
            {
                return RecvReliable(stDataPacket.szBuff, stDataPacket.ullDataLen);
            }
            else
            {
                return RecvUnreliable(stDataPacket.szBuff, stDataPacket.ullDataLen);
            }
            break;
        }
        case NET_PACKET_HEARTBEAT:
        {
            return HandleHeartBeat(oMsg.Body().stHearBeat);
        }
        case NET_PACKET_CLOSE:
        {
            if (oMsg.Body().stFin.dwConnId != m_ulConnID)
            {
                LOG_ERR_FMT(ptrNetLogger, "client[{}, {}] invalid connID[{}]", sock_addr(&m_stClientAddr), oMsg.Body().stFin.dwConnId, m_ulConnID);
                return -1;
            }
            CloseConnect(CLOSE_REASON_CLIENT_FIN);
            break;
        }
        case NET_PACKET_FIN:
        {
            return HandleFin(oMsg.Body().stFin);
        }
        case NET_PACKET_FIN_ACK:
        {
            if (CONN_STATE_FIN_WAIT1 == m_eState)
            {
                m_eState = CONN_STATE_FIN_WAIT2;
            }
            break;
        }
        default:
        {
            LOG_ERR_FMT(ptrNetLogger, "client[{}] conn[{}] invalid packet type:{}", sock_addr(&m_stClientAddr), m_ulConnID, stHead.bType);
            break;
        }
    }
    return 0;
}

int NetConnect::RecvReliable(const char *pData, int iDataLen)
{
    CHECK_IF_PARAM_NULL(ptrNetLogger, pData, -1);
    // kcp input
    int iRet = m_oKcp.Input(pData, iDataLen);
    if (0 != iRet)
    {
        LOG_ERR_FMT(ptrNetLogger, "ret[{}] conn[{}] kcp info:{}", iRet, m_ulConnID, m_oKcp.GetLogInfo());
        return -1;
    }

    // 检查kcp状态
    if (m_oKcp.IsSnBroken())
    {
        // kcp状态异常，修正传输状态
        DisConnect(DISCONNECT_REASON_KCP_DEAD_LINK);
        ClearConnData();
        m_poNetWork->SendRstMsg(CONN_RST_TYPE_DISCONENCT, m_ulConnID, m_stClientAddr);
        LOG_ERR_FMT(ptrNetLogger, "conn[{}] kcp[{}] is broken, rst.", m_ulConnID, m_oKcp.GetLogInfo());
        return -1;
    }
    return 0;
}

int NetConnect::RecvUnreliable(const char *pData, int iDataLen)
{
    NetReader oReader(pData, iDataLen);
    while (0 < oReader.GetRemain())
    {
        // 逐个收取业务包，业务包之前用长度分隔
        const char *pBuf = nullptr;
        size_t iLen = oReader.PeekUnreliable(&pBuf);
        if (0 >= iLen || nullptr == pBuf)
        {
            LOG_ERR_FMT(ptrNetLogger, "conn[{}] recv unreliable msg fail, len:{}", m_ulConnID, iLen);
            break;
        }

        // 检查网络包大小
        if (MAX_UNRELIABLE_MSG_SIZE < iLen)
        {
            LOG_ERR_FMT(ptrNetLogger, "conn[{}] recv unreliable msg fail, len:{}", m_ulConnID, iLen);
            break;
        }

        // 数据解码后发到业务队列
        if (0 != RecvOneMsg(pBuf, iLen))
        {
            LOG_ERR_FMT(ptrNetLogger, "conn[{}] recv unreliable msg fail", m_ulConnID);
            break;
        }

        LOG_DBG_FMT(ptrNetLogger, "conn[{}] recv unreliable msg succ.", m_ulConnID);
    }
    return 0;
}

int NetConnect::HandleHeartBeat(const STHeartBeatPacket& stHearBeat)
{
    if (stHearBeat.dwConnId != m_ulConnID)
    {
        LOG_ERR_FMT(ptrNetLogger, "conn[{}] invalid, cur conn:{}", stHearBeat.dwConnId, m_ulConnID);
        return -1;
    }

    // 更新心跳
    m_tLastHeartBeat = Now::TimeStamp();

    // 回复心跳包    
    STNetPacket *pstPacket = m_poNetWork->NewPacket(HEARTBEAT_PACKET_SIZE);
    CHECK_IF_PARAM_NULL(ptrNetLogger, pstPacket, -1);
    pstPacket->ulConnectID = m_ulConnID;
    pstPacket->stClientAddr = m_stClientAddr;
    pstPacket->poConn = this;

    STNetMsgHead stHead = {NET_PACKET_HEARTBEAT_ACK, 1};
    STHeartBeatPacket stRsp = {0};
    stRsp.dwConnId = m_ulConnID;
    stRsp.ullClientTimeMs = stHearBeat.ullClientTimeMs;
    stRsp.ullServerTimeMs = Now::TimeStampMS();

    NetWriter oWriter(pstPacket->szBuff, pstPacket->ulBuffLen);
    oWriter.Write(stHead);
    oWriter.Write(stRsp);
    pstPacket->ulDataLen = oWriter.GetSize();
    LOG_DBG_FMT(ptrNetLogger, "conn[{}] rsp client[{}] heartbeat.", m_ulConnID, sock_addr(&m_stClientAddr));
    return 0;
}

int NetConnect::HandleFin(const STFinPacket& stFinPacket)
{
    // 客户端关闭走close，这里处理服务器主动关闭
    if (CONN_STATE_ESTABLISHED == m_eState)
    {
        LOG_ERR_FMT(ptrNetLogger, "conn[{}] fin fail, cur state:{}", m_ulConnID, (int)m_eState);
        return 0;
    }

    if (CONN_STATE_FIN_WAIT2 != m_eState && CONN_STATE_TIME_WAIT != m_eState)
    {
        LOG_ERR_FMT(ptrNetLogger, "conn[{}] fin fail, cur state:{}", m_ulConnID, (int)m_eState);
        return -1;
    }

    // 检查链接ID，避免旧包
    if (stFinPacket.dwConnId != m_ulConnID)
    {
        LOG_ERR_FMT(ptrNetLogger, "conn[{}] fin fail, conn[{}] invalid", m_ulConnID, stFinPacket.dwConnId);
        return -1;
    }

    if (CONN_STATE_FIN_WAIT2 == m_eState)
    {
        // 进入time wait状态
        m_eState = CONN_STATE_TIME_WAIT;
        m_tTimeWait = Now::TimeStamp();
        NetMgr::Inst().NotifyConnEvent(ENM_CONN_EVENT_TYPE_CLOSE, m_ulConnID, m_uiCloseReason, m_ullBindData);
        LOG_DBG_FMT(ptrNetLogger, "conn[{}] state FIN_WAIT2->TIME_WAIT", m_ulConnID);
    }
    
    m_poNetWork->SendFinMsg(NET_PACKET_FIN_ACK, m_ulConnID, m_uiCloseReason, m_stClientAddr);
    LOG_DBG_FMT(ptrNetLogger, "conn[{}] send fin ack", m_ulConnID);
    return 0;
}

void NetConnect::OnIOWrite(STNetPacket& stPacket)
{
    // 检查发送的是否为当前合并包
    if (&stPacket == m_poMergePacket)
    {
        m_poMergePacket = nullptr;
    }

    // 统计数据
    m_sStatData.ulSendCnt++;
    m_sStatData.ulSendSize += stPacket.ulDataLen;
}

int NetConnect::Reconnect(const sockaddr_in& stClientAddr)
{
    // 检查状态
    if (CONN_STATE_DISCONNET != m_eState && CONN_STATE_ESTABLISHED != m_eState)
    {
        LOG_ERR_FMT(ptrNetLogger, "conn[{}] state: {}", m_ulConnID, (int)m_eState);
        return -1;
    }

    // 更新kcp状态
    if (0 != m_oKcp.Reconnect())
    {
        LOG_ERR_FMT(ptrNetLogger, "conn[{}] kcp reconnect fail.", m_ulConnID);
        return -1;
    }

    // 更新链接状态
    const sockaddr_in& stOldAddr = m_stClientAddr;
    m_eState = CONN_STATE_ESTABLISHED;
    m_stClientAddr = stClientAddr;
    m_tLastHeartBeat = Now::TimeStamp();

    NetConnectMgr::Inst().SetConnAddr(this, &stOldAddr);
    NetMgr::Inst().NotifyConnEvent(ENM_CONN_EVENT_TYPE_RECONNECT, m_ulConnID, 0, m_ullBindData, &stClientAddr);
    LOG_DBG_FMT(ptrNetLogger, "conn[{}] reconnect, clinet:{}->{}", m_ulConnID, sock_addr(&stOldAddr), sock_addr(&stClientAddr));
    return 0;
}