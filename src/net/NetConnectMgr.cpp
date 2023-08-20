#include "NetConnectMgr.h"
#include "NetCommDef.h"
#include "Now.h"

int NetConnectMgr::Init()
{
    m_mapConn.clear();
    m_mapClosedConn.clear();
    m_setExistID.clear();
    m_ulCurID = 1;
    m_tLastUpdate = 0;
    LOG_DBG_FMT(ptrNetLogger, "NetConnectMgr init succ.");
    return 0;
}

void NetConnectMgr::Tick1S()
{
    for (auto iter = m_mapConn.begin(); iter != m_mapConn.end(); ++iter)
    {
        NetConnect *poConn = iter->second;
        CHECK_IF_PARAM_NULL_CONTINUE(ptrNetLogger, poConn);
        poConn->Tick1S();
    }
}

int NetConnectMgr::Proc()
{
    int iProcNum = 0;
    int64_t llNow = Now::TimeStampMS();

    // 连接update2ms一次
    if (llNow >= m_tLastUpdate + 2)
    {
        m_tLastUpdate = llNow;
        for (auto& kv : m_mapConn)
        {
            iProcNum += kv.second->Proc(m_tLastUpdate);
        }

        // 处理待删除的连接
        ProcClosedConn();
    }
    return iProcNum;
}

void NetConnectMgr::MoveToClosedMap(NetConnect *poConn)
{
    CHECK_IF_PARAM_NULL_VOID(ptrNetLogger, poConn);
    m_mapClosedConn[poConn->GetConnectID()] = poConn;
}

void NetConnectMgr::ProcClosedConn()
{
    for (auto& kv : m_mapClosedConn)
    {
        FreeConn(kv.second);
    }
    m_mapClosedConn.clear();
}

int NetConnectMgr::FreeConn(NetConnect* poConn)
{
    CHECK_IF_PARAM_NULL(ptrNetLogger, poConn, -1);
    uint32_t ulConnID = poConn->GetConnectID();
    const sockaddr_in stClientAddr = poConn->GetClientAddr();

    // 释放连接对象
    delete poConn;

    // 删除索引
    uint64_t ullAddrID = sockaddr_to_id(stClientAddr);
    m_mapConn.erase(ullAddrID);
    m_setExistID.erase(ulConnID);
    LOG_DBG_FMT(ptrNetLogger, " free conn[{}], client", ulConnID, sock_addr(&stClientAddr));
    return 0;
}

NetConnect* NetConnectMgr::GetConnByID(uint32_t ullConnID)
{
    for (auto& kv : m_mapConn)
    {
        if (kv.second->GetConnectID() == ullConnID)
        {
            return kv.second;
        }
    }
    return nullptr;
}

NetConnect* NetConnectMgr::GetConnByAddr(const sockaddr_in& stAddr)
{
    uint64_t ullAddrID = sockaddr_to_id(stAddr);
    if (m_mapConn.end() != m_mapConn.find(ullAddrID))
    {
        return m_mapConn[ullAddrID];
    }
    return nullptr;
}

int NetConnectMgr::SetConnAddr(NetConnect *poConn, const sockaddr_in *pstOldAddr /* = nullptr */)
{
    CHECK_IF_PARAM_NULL(ptrNetLogger, poConn, -1);

    uint64_t ullAddrID = 0;
    if (nullptr != pstOldAddr)
    {
        // 删除旧索引
        ullAddrID = sockaddr_to_id(*pstOldAddr);
        m_mapConn.erase(ullAddrID);
    }

    // 添加新索引
    ullAddrID = sockaddr_to_id(poConn->GetClientAddr());
    m_mapConn[ullAddrID] = poConn;
    return 0;
}

int NetConnectMgr::AllocID(uint32_t& ulID)
{
    uint32_t ulMax = UINT32_MAX;
    uint32_t ulCnt = 0;
    while (m_setExistID.end() != m_setExistID.find(m_ulCurID))
    {
        ulCnt++;
        if (ulCnt == ulMax)
        {
            LOG_ERR_FMT(ptrNetLogger, "alloc connID fail, existID: {}, conn size:{}", m_setExistID.size(), m_mapConn.size());
            return -1;
        }
        m_ulCurID++;
    }

    LOG_DBG_FMT(ptrNetLogger, "alloc connID:{}, existID: {}, conn size:{}", m_ulCurID, m_setExistID.size(), m_mapConn.size());
    ulID = m_ulCurID;
    m_setExistID.insert(ulID);
    return 0;
}

NetConnect* NetConnectMgr::CreateNewConn(
    NetWork *poNetWork, const sockaddr_in& stClientAddr, const char *szCookie, const STEncyptData& stEncyptData, uint32_t ulConnID /* = 0 */)
{
    if (0 == ulConnID)
    {
        if (0 != AllocID(ulConnID))
        {
            LOG_ERR_FMT(ptrNetLogger, "alloc connID fail, existID: {}", m_setExistID.size());
            return nullptr;
        }
    }

    LOG_DBG_FMT(ptrNetLogger, "begin create conn:{}", ulConnID);

    NetConnect *poConn = new NetConnect;
    if (nullptr == poConn)
    {
        LOG_ERR_FMT(ptrNetLogger, "new conn fail, conn size:{}", m_mapConn.size());
        return nullptr;
    }

    // 初始化
    if (0 != poConn->InitConnect(ulConnID, poNetWork, stClientAddr, szCookie, stEncyptData))
    {
        LOG_ERR_FMT(ptrNetLogger, "conn[{}] init fail", ulConnID);
        delete poConn;
        return nullptr;
    }

    SetConnAddr(poConn);
    return poConn;
}