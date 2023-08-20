/**
 * @file NetWork.h
 * @author huwenhao ()
 * @brief 网络对象，一个对象实例绑定一个fd
 * @date 2023-08-04
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#pragma once
#include <memory.h>
#include "Socket.h"
#include "SendQueue.h"
#include "NetMsg.h"

class NetWork
{
public:
    typedef std::shared_ptr<NetWork> ptr;
    NetWork(uint64_t ullID, int iSockFD, const sockaddr_in& stSockAddr);
    ~NetWork();

    /**
     * @brief 获取ID
     * 
     * @return uint64_t 
     */
    inline uint64_t GetID() const {return m_ullID;}

public:
    /**
     * @brief 处理udp fd IO事件
     * 
     * @param iEvent 
     * @return int 
     */
    int ProcIOEvent(int iEvent);

    /**
     * @brief 发送FIN或FIN确认包
     * 
     * @param eType         包类型（FIN或FIN ACK）
     * @param ulConnID      连接ID
     * @param bReason       关闭原因
     * @param stClientAddr  客户端地址
     * @return int 
     */
    int SendFinMsg(EnmNetPacketType eType, uint32_t ulConnID, uint8_t bReason, const sockaddr_in& stClientAddr);

    /**
     * @brief 创建新的网络包，并加入发送队列
     * 
     * @param ulBufferSize 
     * @return STNetPacket* 
     */
    STNetPacket* NewPacket(size_t ulBufferSize);

    /**
     * @brief 发送网络数据
     * 
     * @return int 
     */
    int DoIOWrite();

    /**
     * @brief 接受网络数据
     * 
     * @return int 
     */
    int DoIORead();

    /**
     * @brief 处理网络消息
     * 
     * @param oMsg 
     * @param stClientAddr 
     * @return int 
     */
    int HandleNetMsg(const NetMsg& oMsg, const sockaddr_in& stClientAddr);

    /**
     * @brief 处理第一次握手请求
     * 
     * @param oMsg 
     * @param stClientAddr 
     * @return int 
     */
    int HandleHandShake1(const NetMsg& oMsg, const sockaddr_in& stClientAddr);

    /**
     * @brief 处理第二次握手请求
     * 
     * @param oMsg 
     * @param stClientAddr 
     * @return int 
     */
    int HandleHandShake2(const NetMsg& oMsg, const sockaddr_in& stClientAddr);

    /**
     * @brief 处理重连请求
     * 
     * @param oMsg 
     * @param stClientAddr 
     * @return int 
     */
    int HandleReconnect(const NetMsg& oMsg, const sockaddr_in& stClientAddr);

    /**
     * @brief 处理网络数据
     * 
     * @param oMsg 
     * @param stClientAddr 
     * @return int 
     */
    int HandleNetData(const NetMsg& oMsg, const sockaddr_in& stClientAddr);

    /**
     * @brief 将握手包加入发送队列
     * 
     * @param stHead 
     * @param stPacket 
     * @param stClient 
     * @return int 
     */
    int SendHandShakeMsg(STNetMsgHead& stHead, STHandShakePacket& stPacket, const sockaddr_in& stClient);
    int SendHandShakeSucc(const STHandShakePacket& stReq, NetConnect& oConn);
    int SendHandShakeFail(const STHandShakePacket& stReq, const sockaddr_in& stClient, int iReason = 0);
    
    /**
     * @brief 发送重连确认包
     * 
     * @param stClientAddr 
     * @param poConn 
     * @return int 
     */
    int SendReconnctAck(const sockaddr_in& stClientAddr, NetConnect* poConn);

    /**
     * @brief 发送rst包
     * 
     * @param bType 
     * @param ulConnID 
     * @param bReason 
     * @param stClientAddr 
     * @return int 
     */
    int SendRstMsg(uint8_t bType, uint32_t ulConnID, const sockaddr_in& stClientAddr);

public:
    // 客户端处理
    /**
     * @brief 发第一次握手包
     * 
     * @param stServerAddr 
     * @return int 
     */
    int SendHandShake1Msg(const sockaddr_in& stServerAddr);

    /**
     * @brief 处理第一次握手包ack
     * 
     * @param oMsg 
     * @param stClientAddr 
     * @return int 
     */
    int HandleHandShake1Ack(const NetMsg& oMsg, const sockaddr_in& stServerAddr);

    /**
     * @brief 处理第二次握手包ack,连接建立
     * 
     * @param oMsg 
     * @param stClientAddr 
     * @return int 
     */
    int HandleHandShake2Ack(const NetMsg& oMsg, const sockaddr_in& stServerAddr);

    /**
     * @brief 客户端成功连接
     * 
     * @return true 
     * @return false 
     */
    inline bool Connected() const {return m_ulConnID > 0;}

    /**
     * @brief 发送心跳包
     * 
     * @return int 
     */
    int SendHeartBeat(const sockaddr_in& stServerAddr);
private:
    uint64_t m_ullID;                       // 网络对象id
    int m_iSockFD;                          // udp fd
    struct sockaddr_in m_stSockAddr;        // 客户端地址
    SendQueue m_oSendQueue;                 // 发送队列
    NetMsg m_oMsg;                          // 网络包
    static char s_szRecvBuff[MAX_NET_RECV_BUF_LEN]; // 接收缓冲区

    /** 客户端握手缓存数据 **/
    uint32_t m_ulConnID;                    // 客户端使用,与服务端建立的连接id
    STEncyptData  m_stEncyptData;           // 加密数据    
};