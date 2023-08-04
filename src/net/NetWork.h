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
#include "Socket.h"

class NetWork
{
public:
    NetWork();
    ~NetWork();
    int Init();

public:
    int Recv();

    int Send();

private:
    uint64_t m_ullID;
    int m_iSockFD;
    struct sockaddr_in m_stSockAddr;


};