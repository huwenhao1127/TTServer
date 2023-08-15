#include "NetMgr.h"
#include "NetCommDef.h"
#include "NetWorkMgr.h"
#include "NetConnectMgr.h"
#include "EpollMgr.h"

NetMgr::NetMgr() : m_oM2NQueue(MAX_M2N_QUEUE_SIZE), m_oN2MQueue(MAX_N2M_QUEUE_SIZE) 
{
}

NetMgr::~NetMgr() 
{
}

int NetMgr::Init()
{
    CHECK_PARAM_NOT_ZERO(ptrNetLogger, EpollMgr::Inst().Init(), -1);
    CHECK_PARAM_NOT_ZERO(ptrNetLogger, NetWorkMgr::Inst().Init(), -1);
    CHECK_PARAM_NOT_ZERO(ptrNetLogger, NetConnectMgr::Inst().Init(), -1);
    LOG_DBG_FMT(ptrNetLogger, "net mgr init succ.");
    return 0;
}

int NetMgr::Proc()
{
    int iProcNum = 0;
    // 处理网络IO事件
    iProcNum = NetWorkMgr::Inst().Proc();

    // 处理M2N中业务包
    iProcNum += ProcM2NMsg();

    // 处理连接任务
    iProcNum += NetConnectMgr::Inst().Proc();
    return iProcNum;
}

int NetMgr::ProcM2NMsg()
{
    // 发到UDP发送缓冲区
    return 0;
}

int NetMgr::SendMsg(const char* szData, int iLen, MsgOpt& stOpt)
{
    if (MAX_BIZ_MSG_SIZE < iLen)
    {
        LOG_ERR_FMT(ptrNetLogger, "send msg fail, msg size[{}] large than {}", iLen, MAX_BIZ_MSG_SIZE);
        return -1;
    }

    // 构造Head
    STSendMsgHead stHead;
    stHead.bMsgType = ENM_CONN_MSG_TYPE_PROTO;
    stHead.stOpt = stOpt;
    stHead.iDataLen = iLen;

    // 这里多一次内存拷贝，后续再优化
    char* pStart = m_szMsgBuffer;
    memcpy(pStart, &stHead, sizeof(STSendMsgHead));
    pStart += sizeof(STSendMsgHead);
    memcpy(pStart, szData, iLen);

    int iRet = m_oM2NQueue.Push(m_szMsgBuffer, iLen + sizeof(stHead));
    if (0 > iRet)
    {
        LOG_ERR_FMT(ptrNetLogger, "M2NQueue Push fail.");
    }
    return iRet;
}

int NetMgr::RecvMsg(char* szData, int& iLen)
{
    m_oN2MQueue.Pop(m_szMsgBuffer);

    char* pStart = m_szMsgBuffer;
    STSendMsgHead stHead = *(STSendMsgHead*)pStart;
    if (0 >= stHead.iDataLen || MAX_BIZ_MSG_SIZE < stHead.iDataLen)
    {
        LOG_ERR_FMT(ptrNetLogger, "recv msg fail, msg size[{}]", iLen);
        return -1;
    }
    pStart += sizeof(STSendMsgHead);

    memcpy(szData, pStart, stHead.iDataLen);
    iLen = stHead.iDataLen;
    return 0;
}

int NetMgr::NotifyConnEvent(EnmConnEventType eType, 
    uint32_t ulConnID, uint8_t bReason, uint64_t ullData, const sockaddr_in *pstClientAddr /* = nullptr */, const STBindConnMsg *pstBindMsg /* = nullptr */)
{
    STConnEventMsg stEvent = {0};
    stEvent.bEventType = eType;
    stEvent.bReason = bReason;
    if (nullptr != pstClientAddr)
    {
        stEvent.stClientAddr = *pstClientAddr;
    }

    if (eType == ENM_CONN_EVENT_TYPE_BIND && nullptr != pstBindMsg)
    {
        stEvent.stData.stBindMsg = *pstBindMsg;
    }
    else
    {   
        stEvent.stData.ullData = ullData;
    }

    STRecvMsgHead stRecvMsgHead = {0};
    stRecvMsgHead.bMsgType = ENM_CONN_MSG_TYPE_EVENT;
    stRecvMsgHead.ulConnID = ulConnID;
    stRecvMsgHead.iDataLen = sizeof(stEvent);

    return SendN2MMsg(stRecvMsgHead, &stEvent);
}

int NetMgr::SendN2MMsg(const STRecvMsgHead& stRecvHead, const void *pMsg)
{
    CHECK_IF_PARAM_NULL(ptrNetLogger, pMsg, -1);
    char* pStart = m_szMsgBuffer;

    memcpy(pStart, &stRecvHead, sizeof(STRecvMsgHead));
    pStart += sizeof(STRecvMsgHead);
    memcpy(pStart, pMsg, stRecvHead.iDataLen);
    pStart += stRecvHead.iDataLen;

    int iRet = m_oN2MQueue.Push(m_szMsgBuffer, stRecvHead.iDataLen + sizeof(STRecvMsgHead));
    if (0 > iRet)
    {
        LOG_ERR_FMT(ptrNetLogger, "N2MQueue Push fail.");
    }
    return iRet;
}

int NetMgr::Tick1S()
{
    NetConnectMgr::Inst().Tick1S();
    return 0;
}

int NetMgr::Tick20S()
{
    return 0;
}

int NetMgr::Start()
{
    m_poNetThread = new NetThread();
    if (nullptr == m_poNetThread)
    {
        LOG_ERR_FMT(ptrNetLogger, " create net thread fail.");
        return -1;
    }

    if (0 != m_poNetThread->Start())
    {
        LOG_ERR_FMT(ptrNetLogger, " start net thread fail.");
        return -1;
    }

    LOG_DBG_FMT(ptrNetLogger, " net thread start succ.");
    return 0;
}


int NetMgr::Stop()
{
    if (nullptr != m_poNetThread)
    {
        delete m_poNetThread;
        m_poNetThread = nullptr;
        LOG_DBG_FMT(ptrNetLogger, " net thread stop succ.");
    }
    return 0;
}