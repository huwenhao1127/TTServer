#pragma once
#include <stdint.h>
#include "Logger.h"

static tts::Logger::ptr ptrNetLogger(new tts::Logger("NetLog", 
                                    tts::EnmLoggerLevel::ERROR, 
                                    LOGGER_APPENDER_ROTATE + LOGGER_APPENDER_STDOUT,
                                    nullptr,
                                    "netlog",
                                    "/home/wenhowhu/TTServer/log/netlog"));

/*************** 1. 协议相关定义 ***************/

// 连接状态定义
enum EnmNetConnState {
    CONN_STATE_CLOSE                = 1,    // 关闭状态
    CONN_STATE_ESTABLISHED          = 2,    // 活跃状态
    CONN_STATE_DISCONNET            = 3,    // 断线状态
    CONN_STATE_RECONNETING          = 4,    // 重连状态
    CONN_STATE_FIN_WAIT1            = 5,    // 主动关闭状态1
    CONN_STATE_FIN_WAIT2            = 6,    // 主动关闭状态2
    CONN_STATE_TIME_WAIT            = 7,    // 主动关闭TIME_WAIT 
    CONN_STATE_CLOSE_WAIT           = 8,    // 被动关闭状态1
    CONN_STATE_LAST_ACK             = 9,    // 被动关闭等待ACK状态
    CONN_STATE_HANDSHAKE1           = 101,  // 第一次握手(客户端使用)
    CONN_STATE_HANDSHAKE2           = 102,  // 第二次握手(客户端使用)
};

// 网络协议类型定义
enum EnmNetPacketType {
    NET_PACKET_HANDSHAKE1           = 1,    // 第一次握手包 
    NET_PACKET_HANDSHAKE1_ACK       = 2,    // 第一次握手包ACK 
    NET_PACKET_HANDSHAKE2           = 3,    // 第二次握手包 
    NET_PACKET_HANDSHAKE2_ACK       = 4,    // 第二次握手包ACK
    NET_PACKET_DATA                 = 5,    // 数据包 
    NET_PACKET_HEARTBEAT            = 6,    // 心跳包   客户端不断发给服务器
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

// 连接RST类型
enum EnmConnRstType {
    CONN_RST_TYPE_NONE              = 0, 
    CONN_RST_TYPE_DISCONENCT        = 1,    // 连接已断线 
    CONN_RST_TYPE_CLOSE             = 2,    // 连接已关闭
};

// 握手Cookies大小
#define MAX_HANDSHAKE_COOKIES_SIZE (20)

// 握手包中加密相关数据(密钥和大数)长度, 使用128bit的密钥，长度就是16byte
#define MAX_ENCRYPT_DATA_LEN (16)

#pragma pack(1)

// 网络包头部结构
struct CommPacketHead {
    uint8_t     bType:7;        // 消息类型(EnmNetPacketType)
    uint8_t     bIsReliable:1;  // 是否可靠消息 
};

// 网络数据头部结构
struct DataHead {
    uint8_t     bIsEncrypt;     //是否加密
};

// 握手包结构
struct HandShakePacket {
   uint32_t     dwTimeStamp;        // 生成cookies的时间戳 
   uint8_t      bScreteKey;         // 生成cookise时的bKeySeq，会动态变化
   char         szCookies[MAX_HANDSHAKE_COOKIES_SIZE];  // 根据[客户端地址 + 当前时间戳 + bKeySeq]生成
   uint32_t     dwConnId;           // 连接ID, 握手成功后，为当前连接分配的唯一ID
   uint8_t      bIsKey;             // 是否秘钥 bIsKey=1 表示是直接给秘钥 bIsKey=0 表示返回的是大数
   uint8_t      bEncryptDataLen;    // 加密数据长度
   char         szEncryptData[MAX_ENCRYPT_DATA_LEN];    //加密数据
   uint64_t     ullExtData;         //额外数据
};

// 心跳包结构
struct HeartBeatPacket {
   uint32_t     dwConnId;           // 连接ID
   uint64_t     ullClientTimeMs;    // 客户端时间戳
   uint64_t     ullServerTimeMs;    // 服务器时间戳
};

// 重连包结构
struct ReconnectPacket {
   uint32_t     dwConnId;           // 连接ID
   char         szCookies[MAX_HANDSHAKE_COOKIES_SIZE];  // 握手cookies
};

// RST包结构
struct RstPacket {
   uint32_t     dwConnId;           // 连接ID
   uint8_t      bRstType;           // RST类型
};

// FIN包结构
struct FinPacket {
   uint32_t     dwConnId;           // 连接ID
   uint8_t      bReason;            // 关闭原因
};

// 网络包水印数据结构
struct WaterMarkData {
    uint32_t    dwFootPrint;    // 水印指纹
    uint32_t    dwSeqID;        // 序列号
};

// 网络包水印标记结构
struct WaterMarkFlag {
    uint8_t     bIsOpenWaterMark;     // 是否开启水印
};


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
#define MAX_PACKET_DATA_SIZE (MAX_PACKET_SIZE - sizeof(struct CommPacketHead))

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

// 2.2 时间相关规范

// 报文最大生存时间 
#define MSL_SECONDS (3) 

#define TIME_WAIT_SECONDS (2 * MSL_SECONDS)

// 关闭流程等待的超时时间
#define FIN_WAIT_SECONDS  (MSL_SECONDS)

// COOKIES 有效时间
#define MAX_HANDSHAKE_COOKIES_TIME (10)

// 2.3 配置相关

// #是否使用0.0.0.0作为监听地址
// use_any_address=1

// #套接字发送缓冲大小(SO_SNDBUF:30M)
// sock_sndbuf_size=31457280

// #套接字接收缓冲大小(SO_RCVBUF:30M)
// sock_rcvbuf_size=31457280

// #epoll调用间隔(ms, 待优化掉)
// epoll_interval=2

// #绑定超时时间(s)
// bind_timeout=60

// #心跳超时时间(s)
// heartbeat_timeout=30

// #重连超时时间(s)
// reconnect_timeout=240

// #单次循环处理的最大消息数
// max_loop_msg_num=1000

// #防攻击-单连接每10秒收包数量上限
// max_conn_recv_num=5000

// #防攻击-单连接每10秒收包流量上限
// max_conn_recv_size=1024000


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


/*************** 3. 公共错误码定义 ***************/

// 连接断线原因
enum EnmDisconnetReason {
    DISCONNECT_REASON_NONE                  = 0,
    DISCONNECT_REASON_HEARTBEAT             = 1,    // 心跳超时
    DISCONNECT_REASON_KCP_DEAD_LINK         = 5,    // kcp触发dead link
    DISCONNECT_REASON_GM                    = 6,    // GM断线
};

// 连接关闭原因
enum EnmCloseReason {
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