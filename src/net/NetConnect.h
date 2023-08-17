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
    uint8_t     szKey[MAX_ENCRYPT_DATA_LEN];    // 密钥
    uint16_t    uiNumALen;                      // 大数A长度
    uint8_t     szNumA[MAX_ENCRYPT_DATA_LEN];   // 大数A(客户端公钥)
    uint16_t    uiNumBLen;                      // 大数B长度
    uint8_t     szNumB[MAX_ENCRYPT_DATA_LEN];   // 大数B(服务器公钥)
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

    /**
     * @brief 链接初始化
     * 
     * @param ulConnID          链接ID
     * @param pNetWork          网络对象
     * @param stClientAddr      客户端地址
     * @param szCookies         cookie信息
     * @param stEncyptData      加密数据
     * @return int 
     */
    int InitConnect(
        uint32_t ulConnID, NetWork *pNetWork, const sockaddr_in& stClientAddr, const char *szCookies, const struct STEncyptData& stEncyptData);

    /**
     * @brief 可靠包（kcp）发送网络数据
     * 
     * @param szData    
     * @param iLen      
     * @param bReliable 是否可靠包
     * @return int 
     */
    int Send2NetWork(const char* szData, int iLen, bool bReliable);

    /**
     * @brief 发送业务数据
     * 
     * @param pData     业务数据
     * @param iLen      业务数据大小
     * @param stOpt     消息选项
     * @return int 
     */
    int SendMsg(const char *pData, int iLen, const MsgOpt& stOpt);

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
     * @brief 清除连接数据
     * 
     */
    void ClearConnData();

    /**
     * @brief 绑定链接
     * 
     * @param stBindMsg 
     * @return int 
     */
    int BindConnect(const STBindConnMsg& stBindMsg);

    /**
     * @brief 结束链接
     * 
     * @param eCloseReason 
     * @return int 
     */
    int FinConnect(EnmCloseReason eCloseReason);

    /**
     * @brief 处理网络消息
     * 
     * @param oMsg 
     * @return int 
     */
    int HandleNetMsg(const NetMsg& oMsg);

    /**
     * @brief 写IO时回调
     * 
     * @param stPacket 
     */
    void OnIOWrite(STNetPacket& stPacket);

    /**
     * @brief 执行重连
     * 
     * @param stClientAddr 
     * @return int 
     */
    int Reconnect(const sockaddr_in& stClientAddr);

public:
    /**
     * @brief 获取连接id
     * 
     * @return uint32_t 
     */
    inline const uint32_t GetConnectID() {return m_ulConnID;}

    /**
     * @brief 获取客户端地址
     * 
     * @return const sockaddr_in 
     */
    inline const sockaddr_in& GetClientAddr() {return m_stClientAddr;}

    /**
     * @brief 获取加密数据
     * 
     * @return const STEncyptData& 
     */
    inline const STEncyptData& GetEncyptData() {return m_stEncyptData;}

    /**
     * @brief 获取cookie
     * 
     * @return const char* 
     */
    inline const char* GetCookie() {return m_szCookies;}

    /**
     * @brief 是否断线
     * 
     * @return true 
     * @return false 
     */
    inline const bool IsDisConnect() {return (m_eState == CONN_STATE_DISCONNET);}

    /**
     * @brief 连接是否关闭
     * 
     * @return true 
     * @return false 
     */
    inline const bool IsClose() {return (m_eState == CONN_STATE_CLOSE);}

private:
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

    /**
     * @brief 数据编码
     * 
     * @param pInData             原数据
     * @param iInDataLen          原数据长度
     * @param pEncodeData       [out]编码后的数据
     * @param iEncodeDataLen    [out]编码后的数据长度
     * @param bEncrypt          是否加密
     * @return int 
     */
    int EncodeMsgData(const char *pInData, int iInDataLen, char *pEncodeData, int& iEncodeDataLen, bool bEncrypt);

    /**
     * @brief 发送可靠包
     * @return int 
     */
    int SendUnreliable(const char *pData, int iDataLen);

    /**
     * @brief 发送非可靠包
     * @return int 
     */
    int SendReliable(const char *pData, int iDataLen);

    /**
     * @brief 检查并获取当前合包
     * 
     * @param iLen 待发送新数据长度
     * @return STNetPacket* 
     */
    STNetPacket* GetCurMergePacket(size_t iLen);

    /**
     * @brief 接收可靠网络包
     * @return int 
     */
    int RecvReliable(const char *pData, int iDataLen);

    /**
     * @brief 接收不可靠网络包
     * @return int 
     */
    int RecvUnreliable(const char *pData, int iDataLen);

    /**
     * @brief 处理心跳包
     * @return int 
     */
    int HandleHeartBeat(const STHeartBeatPacket& stHearBeat);

    /**
     * @brief 处理Fin包
     * @return int 
     */
    int HandleFin(const STFinPacket& stFinPacket);

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

    STNetPacket     *m_poMergePacket;   // 当前合包指针
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