#pragma once
#include <stdint.h>
#include "Logger.h"
#include "SendQueue.h"
#include "Socket.h"

static tts::Logger::ptr ptrNetLogger(new tts::Logger("NetLog", 
                                    tts::EnmLoggerLevel::ERROR, 
                                    LOGGER_APPENDER_ROTATE + LOGGER_APPENDER_STDOUT,
                                    nullptr,
                                    "netlog",
                                    "/home/wenhowhu/TTServer/log/netlog"));

#define MAX_HANDSHAKE_COOKIES_SIZE 20     // 握手Cookies大小
#define MAX_ENCRYPT_DATA_LEN       16     // 握手包中加密相关数据(密钥和大数)长度, 使用128bit的密钥，长度就是16byte

/** 公用枚举定义 BEGIN **/
// 网络包类型
enum EnmNetPacketType 
{
    NET_PACKET_HANDSHAKE1           = 1,    // 第一次握手包 
    NET_PACKET_HANDSHAKE1_ACK       = 2,    // 第一次握手包ACK 
    NET_PACKET_HANDSHAKE2           = 3,    // 第二次握手包 
    NET_PACKET_HANDSHAKE2_ACK       = 4,    // 第二次握手包ACK
    NET_PACKET_DATA                 = 5,    // 数据包 
    NET_PACKET_HEARTBEAT            = 6,    // 心跳包 客户端->服务器
    NET_PACKET_HEARTBEAT_ACK        = 7,    // 心跳包ACK
    NET_PACKET_RECONNECT            = 8,    // 重连包
    NET_PACKET_RECONNECT_ACK        = 9,    // 重连包
    NET_PACKET_ENCRYPT              = 10,   // 加密包
    NET_PACKET_CLOSE                = 11,   // 客户端主动关闭连接
    NET_PACKET_CLOSE_ACK            = 12,   // 客户端主动关闭连接ACK
    NET_PACKET_FIN                  = 13,   // 服务器主动关闭连接, 保证可靠包发送成功
    NET_PACKET_FIN_ACK              = 14,   // 服务器主动关闭连接ACK, 保证可靠包发送成功
    NET_PACKET_RST                  = 15,   // RST包, 通知断线或者关闭
};

// 连接断线原因
enum EnmDisconnetReason 
{
    DISCONNECT_REASON_NONE                  = 0,
    DISCONNECT_REASON_HEARTBEAT             = 1,    // 心跳超时
    DISCONNECT_REASON_KCP_DEAD_LINK         = 5,    // kcp触发dead link
};

// 连接关闭原因
enum EnmCloseReason 
{
    CLOSE_REASON_NONE                   = 0,    // 普通关闭(主线程关闭)
    CLOSE_REASON_CLIENT_FIN             = 1,    // 客户端主动关闭
    CLOSE_REASON_SERVER_FIN             = 2,    // 服务端主动关闭
    CLOSE_REASON_DISCONNET_TIMEOUT      = 3,    // 断线状态超时回收
    CLOSE_REASON_FIN_TIMEOUT            = 4,    // FIN_WAIT1，FIN_WAIT2状态下超时回收
    CLOSE_REASON_BIND_TIMEOUT           = 5,    // 连接超时未绑定
    CLOSE_REASON_FLOW_CONTROL           = 6,    // 流量控制

    // @NOTE 100之后属于业务原因(不能超过255)
    CLOSE_REASON_CHANGE_SVR_CLOSE_OLD_UDP                   = 101,  // 切ZONE关闭旧的UDP连接
    CLOSE_REASON_RECONECT_BUT_PLAYER_NOT_EXIST              = 102,  // UDP重连但是玩家已经不存在
    CLOSE_REASON_RECONECT_BUT_BINDID_NOT_EQUAL_ROLEID       = 103,  // 重连bindid与角色roleid不一致
    CLOSE_REASON_BIND_ROLE_INFO_VALID                       = 104,  // 请求绑定的角色信息非法
    CLOSE_REASON_KICK_BY_LOGIN_SAME_ZONE                    = 105,  // 重复登录到同一个ZONE
    CLOSE_REASON_KICK_BY_HALL_LOG_OUT                       = 106,  // hall tcp请求下线
    CLOSE_REASON_FREE_TIMEOUT_PLAYER_KCP_DISTCONNECT        = 107,  // UDP断连超时清除
    CLOSE_REASON_KCP_DISCONNECT_NOT_ROLE_LOGIN_OK           = 108,  // UDP断连并且此时尚未登录成功
    CLOSE_REASON_KICK_BY_SERVER_QUIT                        = 109,  // 停服下线

    // 以下仅客户端使用
    // 服务器除了通过Fin/FinAck下发断网原因, 还会通过Rst包干涉UDP网络状态,
    // 因此客户端也用几个枚举表示服务器的Fin/FinAck未涵盖的原因
    CLOSE_REASON_RECONNECT_TIMEOUT                          = 210,  // 客户端请求重连超时,尝试次数超过最大限制
    CLOSE_REASON_SERVER_FORCE_RST                           = 211,  // 收到服务器的Rst包被告知连接已关闭 
    CLOSE_REASON_CLIENT_FORCE_CLOSE                         = 212,  // 客户端主动关闭
    CLOSE_REASON_ERROR_OCCUR                                = 213,  // 发生错误,如收到非自己ConnId的包
    CLOSE_REASON_KCP_DEADLINK                               = 214,  // KCP DeadLink (据说此错误码客户端目前也没使用)
    CLOSE_REASON_SERVER_CONNID_ZERO                         = 215,  // 服务器下发ReconnectAck的dwConnId为0,表示服务器侧连接不存在,接下来需要关闭连接
};

// 连接事件类型
enum EnmConnEventType
{
    ENM_CONN_EVENT_TYPE_NONE            = 0,    // 无效
    ENM_CONN_EVENT_TYPE_CONNECT         = 1,    // 连接事件
    ENM_CONN_EVENT_TYPE_DISCONNECT      = 2,    // 断连事件
    ENM_CONN_EVENT_TYPE_RECONNECT       = 3,    // 重连事件
    ENM_CONN_EVENT_TYPE_CLOSE           = 4,    // 关闭连接
    ENM_CONN_EVENT_TYPE_BIND            = 5,    // 绑定连接
    ENM_CONN_EVENT_TYPE_NETWORK         = 6,    // 网络信息
};

// 业务消息类型
enum EnmMsgType {
    ENM_CONN_MSG_TYPE_NONE          = 0,    // 无效值
    ENM_CONN_MSG_TYPE_PROTO         = 1,    // 普通协议
    ENM_CONN_MSG_TYPE_CLOSE         = 2,    // 关闭连接
    ENM_CONN_MSG_TYPE_EVENT         = 3,    // 连接事件
    ENM_CONN_MSG_TYPE_BIND          = 4,    // 连接绑定
};

/** 公用枚举定义 END **/;


/** 网络包结构定义 BEGIN **/
#pragma pack(1)
// 网络报消息头
struct STNetMsgHead
{
    uint8_t bType:7;                          // 网络包类型(EnmNetPacketType)
    uint8_t bIsReliable:1;                    // 是否可靠包
};

// 握手包
struct STHandShakePacket {
    uint32_t     dwTimeStamp;        // 生成cookies的时间戳 
    uint8_t      bScreteKey;         // 生成cookise时的bKeySeq，会动态变化
    uint32_t     dwConnId;           // 连接ID, 握手成功后，为当前连接分配的唯一ID
    uint8_t      bIsKey;             // 是否秘钥 bIsKey=1 表示是直接给秘钥 bIsKey=0 表示返回的是大数
    uint8_t      bEncryptDataLen;    // 加密数据长度
    uint64_t     ullExtData;         // 额外数据
    char         szEncryptData[MAX_ENCRYPT_DATA_LEN];    // 加密数据
    char         szCookies[MAX_HANDSHAKE_COOKIES_SIZE];  // 根据[客户端地址 + 当前时间戳 + bKeySeq]生成
};

// 心跳包
struct STHeartBeatPacket {
    uint32_t     dwConnId;           // 连接ID
    uint64_t     ullClientTimeMs;    // 客户端时间戳
    uint64_t     ullServerTimeMs;    // 服务器时间戳
};

// 重连包
struct STReconnectPacket {
    uint32_t     dwConnId;           // 连接ID
    char         szCookies[MAX_HANDSHAKE_COOKIES_SIZE];  // 握手cookies
};

// RST包
struct STRstPacket {
    uint32_t     dwConnId;           // 连接ID
    uint8_t      bRstType;           // RST类型
};

// FIN包
struct STFinPacket {
    uint32_t     dwConnId;           // 连接ID
    uint8_t      bReason;            // 关闭原因
};

// 数据包
struct STDataPacket
{
    const char *szBuff;             // 数据地址
    uint64_t    ullDataLen;         // 数据长度
};

// 网络报消息体
union UNetMsgBody
{
    STHandShakePacket stHandShake;      // 握手包
    STHeartBeatPacket stHearBeat;       // 心跳包
    STReconnectPacket stReconnect;      // 重连包
    STRstPacket  stRst;                 // Rst包
    STFinPacket  stFin;                 // Fin包
    STDataPacket stData;                // 数据包
};

// 连接RST类型
enum EnmConnRstType {
    CONN_RST_TYPE_NONE              = 0, 
    CONN_RST_TYPE_DISCONENCT        = 1,    // 连接已断线 
    CONN_RST_TYPE_CLOSE             = 2,    // 连接已关闭
};

// 网络包
struct NetPacket
{
    SendQueueNode stQueNode;                // 发送队列节点
    struct sockaddr_in stClientAddr;        // 客户端地址
    uint32_t ulConnectID;                   // 连接ID
    size_t ulDataLen;                       // 数据长度
    size_t ulBuffLen;                       // Buffer长度
    char szBuff[0];                         // Buffer首地址
};

// 网络数据头部结构
struct DataHead {
    uint8_t     bIsEncrypt;     //是否加密
};
/** 网络包结构定义 END **/


/** 业务包结构定义 BEGIN **/
// 业务包选项
struct MsgOpt
{
    uint8_t bIsReliable:1;      // 是否可靠包
    uint8_t bIsEncrypt:2;       // 是否加密
    uint8_t bReserve:5;         // 保留
};

// 业务发包包头
struct STSendMsgHead
{
    uint8_t bMsgType;       // 业务包类型（EnmMsgType）
    int iConnCnt;           // 连接数
    int iDataLen;           // 业务包大小
    MsgOpt  stOpt;          // 业务发包选项
};

// 业务收包包头
struct STRecvMsgHead
{
    uint8_t bMsgType;       // 业务包类型（EnmMsgType）
    uint8_t bIsProc;        // 是否已经被处理
    uint32_t ulConnID;      // 连接ID
    int iDataLen;           // 业务包大小 
};

// 绑定连接业务包结构
struct STBindConnMsg
{
    int64_t llBindSeq;      // 绑定序号
    uint64_t ullBindData;   // 绑定数据
};

struct STConnEventMsg
{
    uint8_t bEventType;             // 事件类型
    uint8_t bReason;                // 事件原因
    sockaddr_in stClientAddr;       // 客户端地址
    union {
        uint64_t ullData;
        STBindConnMsg stBindMsg;
    } stData;                       // 数据
};


/** 业务包结构定义 END **/
#pragma pack()

/*************** 2. 标准辅助定义 ***************/

// 2.1 消息大小相关规范
// IP报文头部长度
#define IP_HEAD_SIZE (20)

// UDP报文头部长度
#define UDP_HEAD_SIZE (8)

// 网络报文MTU 
#define NET_MTU (576)

// 最大网络包大小
#define MAX_PACKET_SIZE (NET_MTU - IP_HEAD_SIZE - UDP_HEAD_SIZE)

// 最大网络包数据大小
#define MAX_PACKET_DATA_SIZE (MAX_PACKET_SIZE - sizeof(struct STNetMsgHead))

// KCP算法MTU
#define KCP_MTU (MAX_PACKET_DATA_SIZE)

// 最大网络包接收缓冲大小(收包限制设宽松点)
#define MAX_NET_RECV_BUF_LEN    (1500)

// 最大业务包长度(未编码原文数据)
#define MAX_BIZ_MSG_SIZE    (1<<20)

// 最大业务包编码长度(在原文长度上加256字节冗余)
#define MAX_ENCODE_BUF_SIZE (MAX_BIZ_MSG_SIZE + sizeof(DataHead) + 256)

// 最大可靠业务包编码大小(DataHead+加密/明文数据)
#define MAX_RELIABLE_MSG_SIZE   (MAX_BIZ_MSG_SIZE + 256)

// 最大非可靠业务包编码大小(DataHead+加密/明文数据)
#define MAX_UNRELIABLE_MSG_SIZE (MAX_PACKET_DATA_SIZE - sizeof(uint16_t))

// 可靠包包头最大长度
#define MAX_RELIABLE_MERGE_HEAD_SIZE 22

// 2.2 时间相关规范


// COOKIES 有效时间
#define MAX_HANDSHAKE_COOKIES_TIME (10)

/** 网络基础配置 **/
// 最大连接数
#define MAX_CONN_NUM            500

// 发包队列大小
#define MAX_M2N_QUEUE_SIZE      20971520

// 收包队列大小
#define MAX_N2M_QUEUE_SIZE      20971520


/** KCP配置 **/
// update间隔(ms)
#define KCP_CONF_UPDATE_INTERVAL 10

// 是否开启nodelay模式
#define KCP_CONF_NODELAY 1

// 是否关闭拥塞控制
#define KCP_CONF_NOCWND 1

// 快速重传阈值
#define KCP_CONF_FAST_RESEND 2

// 发送窗口大小
#define KCP_CONF_SND_WND 128

// 接收窗口大小
#define KCP_CONF_RCV_WND 128

// 最大待发送消息数量
#define KCP_CONF_MAX_WAITSND 3000

// 弱网RTT阈值
#define KCP_CONF_WEAKNET_RTT 200

// 是否使用0.0.0.0作为监听地址（1：单FD监听所有网卡地址 0：每个网卡地址创建一个FD）
#define UDP_ADDR_USE_ANY_ADDRESS    1

/** UDP配置 **/
// udp端口
#define UDP_ADDR_PORT               10000

// 套接字发送缓冲大小(SO_SNDBUF:30M)
#define SOCKET_SND_BUF_SIZE 31457280

// 套接字接收缓冲大小(SO_RCVBUF:30M)
#define SOCKET_RCV_BUF_SIZE 31457280

// 最大网卡数量
#define MAX_NIC_NUM     3

// epoll调用间隔(ms, 待优化掉)
#define EPOLL_WAIT_INTERVAL     2

#define HEARTBEAT_TIMEOUT       30                    // 心跳超时时间(s)
#define RECONNECT_TIMEOUT       240                   // 重连超时时间(4min)
#define MSL_SECONDS             3                     // 报文最大生存时间 
#define TIME_WAIT_SECONDS       (2 * MSL_SECONDS)     // Time wait等待超时时间
#define FIN_WAIT_SECONDS        (MSL_SECONDS)         // 关闭流程等待的超时时间
#define BIND_TIMEOUT            60                    // 绑定超时时间(s)
#define CONN_STATE_INTERVAL     10                    // 连接的统计周期
#define MAX_CONN_RECV_NUM       5000                  // 防攻击-单连接每10秒收包数量上限
#define MAX_CONN_RECV_SIZE      1024000               // 防攻击-单连接每10秒收包流量上限
#define MAX_LOOP_MSG_NUM        1000                  // 单次循环处理的最大消息数

