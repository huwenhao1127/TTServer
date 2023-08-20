#include "NetMgr.h"
#include "NetCommDef.h"
#include "NetWorkMgr.h"
#include "NetConnectMgr.h"
#include "EpollMgr.h"
#include "CookieMgr.h"
#include "NetSecurityMgr.h"

NetMgr::NetMgr() : m_oM2NQueue(MAX_M2N_QUEUE_SIZE), m_oN2MQueue(MAX_N2M_QUEUE_SIZE) 
{
}

NetMgr::~NetMgr() 
{
}

int NetMgr::Init(int iPort /* = UDP_ADDR_PORT */)
{
    m_bIsClient = (iPort == UDP_ADDR_PORT) ? false : true;
    
    CHECK_PARAM_NOT_ZERO(ptrNetLogger, NetSecurityMgr::Inst().Init(), -1);
    CHECK_PARAM_NOT_ZERO(ptrNetLogger, CookieMgr::Inst().Init(), -1);
    CHECK_PARAM_NOT_ZERO(ptrNetLogger, EpollMgr::Inst().Init(), -1);
    CHECK_PARAM_NOT_ZERO(ptrNetLogger, NetWorkMgr::Inst().Init(iPort), -1);
    CHECK_PARAM_NOT_ZERO(ptrNetLogger, NetConnectMgr::Inst().Init(), -1);
    LOG_DBG_FMT(ptrNetLogger, "NetMgr init succ, is client:{}", m_bIsClient);
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
    int iMsgNum = 0;
    size_t ulMsgSize = 0;

    while (iMsgNum < 1000)
    {
        if (m_oM2NQueue.Empty())
        {
            break;
        }

        size_t ulMsgLen = m_oM2NQueue.Pop(m_szMsgBuffer);
        if (ulMsgLen < sizeof(STSendMsgHead))
        {
            LOG_ERR_FMT(ptrNetLogger, "proc m2n msg fail, msg size[{}]", ulMsgLen);
            continue;
        }

        // 消息头部解码
        const STSendMsgHead *pstHead =  (STSendMsgHead*)m_szMsgBuffer;
        if (0 >= pstHead->iDataLen || 0 >= pstHead->iConnCnt)
        {
            LOG_ERR_FMT(ptrNetLogger, "proc m2n msg, data len[{}] conn cnt[{}]", pstHead->iDataLen, pstHead->iConnCnt);
            continue;
        }

        // 消息数据校验
        size_t ulDataLen = ulMsgLen - sizeof(STSendMsgHead) - (sizeof(uint32_t) * pstHead->iConnCnt);
        if (ulDataLen != (size_t)pstHead->iDataLen)
        {   
            LOG_ERR_FMT(ptrNetLogger, "proc m2n msg, data len:{}, {}", pstHead->iDataLen, ulDataLen);
            continue;
        }

        if (ENM_CONN_MSG_TYPE_BIND == pstHead->bMsgType && ulDataLen != sizeof(STBindConnMsg))
        {
            LOG_ERR_FMT(ptrNetLogger, "proc m2n msg, bind conn msg len[{}] invalid.", ulDataLen);
            continue;
        }

        if (ENM_CONN_MSG_TYPE_CLOSE == pstHead->bMsgType && ulDataLen != sizeof(STCloseConnMsg))
        {
            LOG_ERR_FMT(ptrNetLogger, "proc m2n msg, close conn msg len[{}] invalid.", ulDataLen);
            continue;
        }

        if (ENM_CONN_MSG_TYPE_PROTO != pstHead->bMsgType && 
            ENM_CONN_MSG_TYPE_BIND != pstHead->bMsgType && 
            ENM_CONN_MSG_TYPE_CLOSE != pstHead->bMsgType)
        {
            LOG_ERR_FMT(ptrNetLogger, "proc m2n msg, msg type[{}] invalid.", (int)pstHead->bMsgType);
            continue;
        }

        // 消息处理
        const uint32_t *ptrConnList =  (uint32_t*)(m_szMsgBuffer + sizeof(STSendMsgHead));
        const char* ptrData = m_szMsgBuffer + sizeof(STSendMsgHead) + (sizeof(uint32_t) * pstHead->iConnCnt);
        for (int i = 0; i < pstHead->iConnCnt; i++)
        {
            uint32_t ulConnID = ptrConnList[i];
            NetConnect *poConn = NetConnectMgr::Inst().GetConnByID(ulConnID);
            if (nullptr == poConn)
            {
                LOG_ERR_FMT(ptrNetLogger, "proc m2n msg[{}], get conn[{}] fail.", (int)pstHead->bMsgType, ulConnID);
                continue;
            }

            if (ENM_CONN_MSG_TYPE_PROTO == pstHead->bMsgType)
            {
                poConn->SendMsg(ptrData, pstHead->iDataLen, pstHead->stOpt);
            }
            else if (ENM_CONN_MSG_TYPE_BIND == pstHead->bMsgType)
            {
                const STBindConnMsg *pBindMsg = (STBindConnMsg*)ptrData;
                poConn->BindConnect(*pBindMsg);
            }
            else if (ENM_CONN_MSG_TYPE_CLOSE == pstHead->bMsgType)
            {
                const STCloseConnMsg *pCloseMsg = (STCloseConnMsg*)ptrData;
                poConn->FinConnect((EnmCloseReason)pCloseMsg->bReason);
            }
            else
            {
                LOG_ERR_FMT(ptrNetLogger, "proc m2n msg, msg type[{}] invalid.", (int)pstHead->bMsgType);
                break;
            }
            iMsgNum++;
            ulMsgSize += ulDataLen;
        }
    }
    return iMsgNum;
}

int NetMgr::SendMsg(const char* szData, int iLen, MsgOpt& stOpt, uint32_t ulConnID)
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
    stHead.iConnCnt = 1;
    
    // 这里多一次内存拷贝，后续再优化
    char* pStart = m_szMsgBuffer;
    // 添加头
    memcpy(pStart, &stHead, sizeof(STSendMsgHead));
    pStart += sizeof(STSendMsgHead);
    // 添加链接列表
    memcpy(pStart, &ulConnID, sizeof(ulConnID));
    pStart += sizeof(ulConnID);
    // 添加数据
    memcpy(pStart, szData, iLen);

    int iRet = m_oM2NQueue.Push(m_szMsgBuffer, iLen + sizeof(stHead));
    if (0 > iRet)
    {
        LOG_ERR_FMT(ptrNetLogger, "M2NQueue Push fail.");
    }
    return iRet;
}

int NetMgr::BroadcastMsg(const char* szData, int iLen, MsgOpt& stOpt, int iConnListLen, const uint32_t *pConnList)
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
    stHead.iConnCnt = iConnListLen;
    
    // 这里多一次内存拷贝，后续再优化
    char* pStart = m_szMsgBuffer;
    // 添加头
    memcpy(pStart, &stHead, sizeof(STSendMsgHead));
    pStart += sizeof(STSendMsgHead);
    // 添加链接列表
    size_t ulConnListLen = sizeof(uint32_t) * iConnListLen;
    memcpy(pStart, pConnList, ulConnListLen);
    pStart += ulConnListLen;
    // 添加数据
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
    size_t ulDataLen = m_oN2MQueue.Pop(m_szMsgBuffer);
    if (ulDataLen < sizeof(STRecvMsgHead))
    {
        LOG_ERR_FMT(ptrNetLogger, "recv msg fail, data len[{}]", ulDataLen);
        return -1;
    }

    char* pStart = m_szMsgBuffer;
    STRecvMsgHead stHead = *(STRecvMsgHead*)pStart;
    if (0 >= stHead.iDataLen || MAX_BIZ_MSG_SIZE < stHead.iDataLen)
    {
        LOG_ERR_FMT(ptrNetLogger, "recv msg fail, msg size[{}]", iLen);
        return -1;
    }
    pStart += sizeof(STRecvMsgHead);

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
    CookieMgr::Inst().Tick20S();
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