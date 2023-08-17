#pragma once
#include <memory>
#include "ikcp.h"
#include "Noncopyable.h"

class NetConnect;
class KcpWrapper : public Noncopyable
{
public:
    typedef std::shared_ptr<KcpWrapper> ptr;
    KcpWrapper(NetConnect& oConnect) : m_oConn(oConnect) {}
    ~KcpWrapper();

    /**
     * @brief 内存初始化
     * 
     */
    void MemInit();

    /**
     * @brief 业务初始化
     * 
     * @return int 
     */
    int Init();

    /**
     * @brief 对象回收
     * 
     */
    void Reclaim();

    /**
     * @brief 重置kcp
     * 
     * @return int 
     */
    int Reset();

public:
    /**
     * @brief update，时间戳ms
     * 
     * @param ullNow 
     */
    void Update(uint64_t ullNow);

    /**
     * @brief 发数据包，将数据包拆成报文段放入队列
     * 
     * @param szData 
     * @param iLen 
     * @return int 
     */
    int Send(const char* szData, int iLen);

    /**
     * @brief 收数据，epoll驱动将数据放入缓冲区，顺序正确的包移入接收队列
     * 
     * @param szData 
     * @param iLen 
     * @return int 
     */
    int Input(const char* szData, int iLen);

    /**
     * @brief 接收队列中第一个整包大小（包含多个报文段）
     * 
     * @return int 
     */
    int PeekSize();

    /**
     * @brief 从接收队列内接收数据包
     * 
     * @param szBuffer 
     * @param iLen 
     * @return int 
     */
    int Recv(char* szBuffer, int iLen);

    /**
     * @brief 重连
     * 
     * @return int 
     */
    int Reconnect();

    inline void ResetNeedRcv() {m_bNeedRecv = false;}

public:
    /**
     * @brief 获取kcp实例所属连接
     * 
     * @return NetConnect& 
     */
    inline NetConnect& GetConnect() {return m_oConn;}

    /**
     * @brief 获取待发送报文数量（队列和buffer中的总数）
     * 
     * @return int 
     */
    inline int GetWaitSnd() const {return m_ikcp.conv > 0 ? ikcp_waitsnd(&m_ikcp) : 0;}

    /**
     * @brief 获取kcp实例
     * 
     * @return const ikcpcb& 
     */
    inline const ikcpcb& GetIKCPCB() {return m_ikcp;}

    /**
     * @brief 是否断连
     * 
     * @return true 
     * @return false 
     */
    inline bool IsDeadLink() const {return (-1 == (int)m_ikcp.state);}

    /**
     * @brief 异常检测
     * 
     * @return true 
     * @return false 
     */
    inline const bool IsSnBroken() {return (-2 == (int)m_ikcp.state);}

    /**
     * @brief 业务是否需要收包
     * 
     * @return true 
     * @return false 
     */
    inline bool NeedRecv() const {return m_bNeedRecv;}

    /**
     * @brief 获取日志信息
     * 
     * @return const char* 
     */
    const char* GetLogInfo();


private:
    NetConnect& m_oConn;
    ikcpcb m_ikcp;
    bool m_bNeedRecv;
    static char s_szLogInfo[128];
};