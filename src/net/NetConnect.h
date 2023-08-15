#pragma once
#include "KcpWrapper.h"
#include "Socket.h"
#include "NetWork.h"
#include "NetCommDef.h"
#include "NetSecurityMgr.h"

// 连接状态
enum EnmNetConnState
{
    CONN_STATE_CLOSE                = 1,    // 关闭状态
    CONN_STATE_ESTABLISHED          = 2,    // 活跃状态
    CONN_STATE_DISCONNET            = 3,    // 断线状态
    CONN_STATE_RECONNETING          = 4,    // 重连状态
    CONN_STATE_FIN_WAIT1            = 5,    // 主动关闭状态1
    CONN_STATE_FIN_WAIT2            = 6,    // 主动关闭状态2
    CONN_STATE_TIME_WAIT            = 7,    // 主动关闭TIME_WAIT 
    CONN_STATE_CLOSE_WAIT           = 8,    // 被动关闭状态1
    CONN_STATE_LAST_ACK             = 9,    // 被动关闭等待ACK状态
    CONN_STATE_HANDSHAKE1           = 10,   // 第一次握手
    CONN_STATE_HANDSHAKE2           = 11,   // 第二次握手
};

// 连接统计数据
struct STConnStatData
{
    time_t tRefreshTime;        // 刷新时间
    uint32_t ulMaxWaitSend;     // kcp最大发送队列长度
    uint32_t ulRecvCnt;         // 收到的包量(UDP)
    uint32_t ulRecvSize;        // 收到的流量
    uint32_t ulSendCnt;         // 发送的包量
    uint32_t ulSendSize;        // 发送的流量
};

// 可靠包合包状态信息
struct STMergePacketData
{
    uint32_t    ulHeadLen;                                      // 包头序列化长度
    uint32_t    ulOffset;                                       // 当前可靠包合包偏移
    uint32_t    ulMergeBodyBegin;                               // 预留包头空间后合包起始位置
    char        szSerializeHead[MAX_RELIABLE_MERGE_HEAD_SIZE];  // 包头序列化信息
    uint32_t    ulMergeNum;                                     // 已合包数
    bool        bNeedEncrypt;                                   // 合包是否需要加密
};

// DH算法加密相关数据
struct STEncyptData
{
    bool        bIsDHKey;                       // 密钥是否由DH生成
    uint16_t    uiKeyLen;                       // 密钥长度
    char        szKey[MAX_ENCRYPT_DATA_LEN];    // 密钥
    uint16_t    uiNumALen;                      // 大数A长度
    char        szNumA[MAX_ENCRYPT_DATA_LEN];   // 大数A(客户端公钥)
    uint16_t    uiNumBLen;                      // 大数B长度
    char        szNumB[MAX_ENCRYPT_DATA_LEN];   // 大数B(服务器公钥)
};

class NetConnect 
{
public:
    NetConnect();
    ~NetConnect();
public:
    /**
     * @brief 连接状态切换
     * 
     */
    void Tick1S();

    /**
     * @brief 连接proc
     * 
     * @return int 
     */
    int Proc(time_t tNow);

    int InitConnect();


    /**
     * @brief 发送网络数据
     * 
     * @param szData    
     * @param iLen      
     * @param bReliable 是否可靠包
     * @return int 
     */
    int Send2NetWork(const char* szData, int iLen, bool bReliable);

public:
    /**
     * @brief 获取连接id
     * 
     * @return uint32_t 
     */
    inline uint32_t GetConnectID() const {return m_ulConnID;}

    /**
     * @brief 获取客户端地址
     * 
     * @return const sockaddr_in 
     */
    inline const sockaddr_in& GetClientAddr() const {return m_stClientAddr;}

    /**
     * @brief 连接是否关闭
     * 
     * @return true 
     * @return false 
     */
    inline bool IsClose() const {return (m_eState == CONN_STATE_CLOSE);}

private:
    /**
     * @brief 断开连接
     * 
     * @param eReason 
     * @return int 
     */
    int DisConnect(EnmDisconnetReason eReason);

    /**
     * @brief 关闭连接
     * 
     * @param eReason 
     * @return int 
     */
    int CloseConnect(EnmCloseReason eReason);

    /**
     * @brief 清楚连接数据
     * 
     */
    void ClearConnData();

    /**
     * @brief 从kcp收取消息
     * 
     * @return int 
     */
    int RecvMsgFromKcp();

    /**
     * @brief 收取消息
     * 
     * @param szMsg 
     * @param iMsgLen 
     * @return int 
     */
    int RecvOneMsg(const char* szMsg, int iMsgLen);

private:
    uint32_t        m_ulConnID;         // 连接id
    EnmNetConnState m_eState;           // 连接状态
    sockaddr_in     m_stClientAddr;     // 客户端地址
    time_t          m_tCreateTime;      // 连接建立时间
    uint64_t        m_ullBindData;      // 绑定数据
    STConnStatData  m_sStatData;        // 统计数据

    NetWork         *m_poNetWork;       // 网络对象
    uint64_t        m_ullNetWorkID;     // 网络对象ID
    KcpWrapper      m_oKcp;             // kcp对象

    NetPacket       *m_poMergePoint;    // 当前合包指针
    uint32_t        m_ulCurMergeOffset; // 合包指针偏移
    char            m_szMergePacketBuffer[MAX_PACKET_DATA_SIZE];    // 可靠包合包缓冲区
    STMergePacketData m_stMergeState;   // 合包状态信息
    bool            m_bEnableMerge;     // 开启合包标记

    STEncyptData    m_stEncyptData;     // 加密数据
    NetSecurityMgr  m_oSecurityMgr;     // 消息加解密
    char            m_szCookies[MAX_HANDSHAKE_COOKIES_SIZE];        // cookies

    time_t          m_tLastHeartBeat;   // 上次心跳时间
    time_t          m_tDisconnect;      // 触发断线时间
    time_t          m_tFinBegin;        // FIN流程开始时间
    time_t          m_tSendFin;         // 发送FIN包时间
    time_t          m_tTimeWait;        // TIME_WAIT开始时间
    time_t          m_tClose;           // 缓存关闭时间
    time_t          m_tNetWorkSyn;      // 上次同步时间
    uint8_t         m_uiCloseReason;    // 缓存关闭原因 
};