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

    m_poMergePoint = nullptr;
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

int NetConnect::InitConnect()
{
    
    return 0;
}

int NetConnect::Send2NetWork(const char* szData, int iLen, bool bReliable)
{
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
    m_poMergePoint = nullptr;
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