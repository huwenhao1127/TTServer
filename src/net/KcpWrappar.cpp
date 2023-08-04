#include "KcpWrapper.h"
#include "NetCommdef.h"
#include "NetConnect.h"
#include "CommDef.h"
#include <string.h>

// kcp发送数据回调
static int KcpOutPut(const char *buf, int len, struct IKCPCB *kcp, void *user)
{
    KcpWrapper* poKcp = reinterpret_cast<KcpWrapper*>(user);
    CHECK_IF_PARAM_NULL(ptrNetLogger, poKcp, -1);
    return poKcp->GetConnect().Send2NetWork(buf, len, true);
}

// kcp日志接口
static void WriteLog(const char *log, struct IKCPCB *kcp, void *user)
{
    LOG_DBG_FMT(ptrNetLogger, "conn[{}], {}", kcp->conv, log);
    return;
}

// kcp编码缓冲区
static char s_kcp_encode_buf[3 * KCP_MTU] = {0};

KcpWrapper::~KcpWrapper()
{
    Reclaim();
}

void KcpWrapper::MemInit()
{
    memset(&m_ikcp, 0, sizeof(m_ikcp));
    m_bNeedRecv = false;
}

int KcpWrapper::Init()
{
    if (0 == m_oConn.GetConnectID())
    {
        LOG_ERR_FMT(ptrNetLogger, "invalid connect: {}", m_oConn.GetConnectID());
        return -1;
    }

    if (0 < m_ikcp.conv)
    {
        LOG_ERR_FMT(ptrNetLogger, "connect[{}] kcp[{}] repeated init.", m_oConn.GetConnectID(), m_ikcp.conv);
        return -1;
    }

    m_bNeedRecv = false;

    const ikcpcb* kcp_res = ikcp_create(m_oConn.GetConnectID(), this, &m_ikcp, KCP_MTU, s_kcp_encode_buf);
    if (kcp_res != (&m_ikcp))
    {
        LOG_ERR_FMT(ptrNetLogger, "connect[{}] kcp create fail.", m_oConn.GetConnectID());
        return -1;
    }

    // 设置kcp参数
    m_ikcp.output = KcpOutPut;
    m_ikcp.writelog = WriteLog;
    ikcp_wndsize(&m_ikcp, KCP_CONF_SND_WND, KCP_CONF_RCV_WND);
    ikcp_nodelay(&m_ikcp, KCP_CONF_NODELAY, KCP_CONF_UPDATE_INTERVAL, KCP_CONF_FAST_RESEND, KCP_CONF_NOCWND);

    LOG_DBG_FMT(ptrNetLogger, "conn[{}] init succ, kcp[{}]", m_oConn.GetConnectID(), GetLogInfo());
}

void KcpWrapper::Reclaim()
{
    if (0 < m_ikcp.conv)
    {
        LOG_DBG_FMT(ptrNetLogger, "connID[{}] kcp[{}].", m_oConn.GetConnectID(), GetLogInfo());
        ikcp_release(&m_ikcp);
        ZeroStruct(m_ikcp);
    }
}

int KcpWrapper::Reset()
{
    LOG_DBG_FMT(ptrNetLogger, "connID[{}] kcp[{}].", m_oConn.GetConnectID(), GetLogInfo());
    Reclaim();
    if (0 != Init())
    {
        LOG_ERR_FMT(ptrNetLogger, "connID[{}] reset fail.", m_oConn.GetConnectID());
        return -1;
    }
    return 0;
}

void KcpWrapper::Update(uint64_t ullNow)
{
    IUINT32 current = (ullNow & 0xfffffffful);
    ikcp_update(&m_ikcp, current);
}

int KcpWrapper::Send(const char* szData, int iLen)
{
    // 待发送业务包超过阈值时业务包直接丢弃
    if (KCP_CONF_MAX_WAITSND <= GetWaitSnd())
    {
        return -1001;
    }
    return ikcp_send(&m_ikcp, szData, iLen);
}

int KcpWrapper::Input(const char* szData, int iLen)
{
    // 缓冲区可读时做个标记
    m_bNeedRecv = true;
    return ikcp_input(&m_ikcp, szData, iLen);
}

int KcpWrapper::PeekSize()
{
    return ikcp_peeksize(&m_ikcp);
}

int KcpWrapper::Recv(char* szBuffer, int iLen)
{
    return ikcp_recv(&m_ikcp, szBuffer, iLen);
}

int KcpWrapper::Reconnect()
{
    LOG_DBG_FMT(ptrNetLogger, "connID[{}] kcp[{}].", m_oConn.GetConnectID(), GetLogInfo());

    // kcp实例被清除，重新初始化
    if (0 == m_ikcp.conv)
    {
        if (0 != Init())
        {
            LOG_ERR_FMT(ptrNetLogger, "connID[{}] init fail.", m_oConn.GetConnectID());
            return -1;
        }
    }

    // 重置kcp状态
    m_ikcp.state = 0;

    // 重置发送缓冲区报文段的发送计数
    IQUEUEHEAD *head, *p;
    head = &(m_ikcp.snd_buf);
    for (p = head->next; p != head; p = p->next)
    {
        IKCPSEG *seg = iqueue_entry(p, IKCPSEG, node);
        seg->xmit = 0;
    }

    return 0;
}

const char* KcpWrapper::GetLogInfo()
{
    snprintf(s_szLogInfo, sizeof(s_szLogInfo), "conn[%u] ackcount[%d] snd[%d, %d] rcv[%d, %d]", 
            m_ikcp.conv, m_ikcp.ackcount,
            m_ikcp.nsnd_buf, m_ikcp.nsnd_que,
            m_ikcp.nrcv_buf, m_ikcp.nrcv_que);
    return s_szLogInfo;
}