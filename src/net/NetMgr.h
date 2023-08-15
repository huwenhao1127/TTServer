#pragma once
#include "Singleton.h"
#include "RingQueue.h"
#include "NetCommDef.h"
#include "NetThread.h"

#define MAX_NET_QUEUE_DATA_SIZE (MAX_BIZ_MSG_SIZE + sizeof(STSendMsgHead))   // 业务包最大长度+业务包头

class NetMgr : public Singleton<NetMgr>
{
public:
    NetMgr(); 
    ~NetMgr();

    /**
     * @brief 初始化
     * 
     * @return int 
     */
    int Init();

public:
    /**
     * @brief 业务发包接口
     * 
     * @param szData 
     * @param iLen   
     * @param stOpt  发包选项
     * @return int 
     */
    int SendMsg(const char* szData, int iLen, MsgOpt& stOpt);

    /**
     * @brief 业务收包接口
     * 
     * @param szData [out]
     * @param iLen   [out]
     * @return int 
     */
    int RecvMsg(char* szData, int& iLen);

    /**
     * @brief 给业务分发连接事件
     * 
     * @param eType         连接ID
     * @param ulConnID      事件类型
     * @param bReason       事件原因
     * @param ullData       额外数据
     * @param pstClientAddr 客户端地址
     * @param pstBindMsg    绑定信息
     * @return int 
     */
    int NotifyConnEvent(EnmConnEventType eType, 
        uint32_t ulConnID, uint8_t bReason, uint64_t ullData, const sockaddr_in *pstClientAddr = nullptr, const STBindConnMsg *pstBindMsg = nullptr);

    /**
     * @brief 开启网络线程
     * 
     * @return int 
     */
    int Start();

    /**
     * @brief 停止网络线程
     * 
     * @return int 
     */
    int Stop();

    /**
     * @brief 主循环
     * 
     * @return int 单帧处理消息数
     */
    int Proc();

    /**
     * @brief tick 1s
     * 
     * @return int 
     */
    int Tick1S();

    /**
     * @brief tick 20s
     * 
     * @return int 
     */
    int Tick20S();

    /**
     * @brief 处理业务消息
     * 
     * @return int 消息数
     */
    int ProcM2NMsg();

    /**
     * @brief 往业务主线程发收到的业务包
     * 
     * @param stRecvHead    业务收包包头
     * @param pMsg          业务包数据      
     * @return int 
     */
    int SendN2MMsg(const STRecvMsgHead& stRecvHead, const void *pMsg);

public:
    /**
     * @brief 获取发包队列
     * 
     * @return RingQueue& 
     */
    RingQueue& GetM2NQueue() {return m_oM2NQueue;}

    /**
     * @brief 获取收包队列
     * 
     * @return RingQueue& 
     */
    RingQueue& GetN2MQueue() {return m_oN2MQueue;}

private:
    NetThread* m_poNetThread;   // 网络线程
    RingQueue m_oM2NQueue;      // 发包队列 主线程->网络线程
    RingQueue m_oN2MQueue;      // 收包队列 网络线程->主线程
    char m_szMsgBuffer[MAX_NET_QUEUE_DATA_SIZE];    // 业务包合并缓冲区
};