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

private:
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


    int HandleNetMsg(const NetMsg& oMsg, const sockaddr_in& stClientAddr);

    int HandleHandShake1(const NetMsg& oMsg, const sockaddr_in& stClientAddr);

    int HandleHandShake2(const NetMsg& oMsg, const sockaddr_in& stClientAddr);

    int HandleReconnect(const NetMsg& oMsg, const sockaddr_in& stClientAddr);

    int HandleNetData(const NetMsg& oMsg, const sockaddr_in& stClientAddr);
    

private:
    uint64_t m_ullID;                       // 网络对象id
    int m_iSockFD;                          // udp fd
    struct sockaddr_in m_stSockAddr;        // 客户端地址
    SendQueue m_oSendQueue;                 // 发送队列
    NetMsg m_oMsg;                          // 网络包
    static char s_szRecvBuff[MAX_NET_RECV_BUF_LEN]; // 接收缓冲区
};