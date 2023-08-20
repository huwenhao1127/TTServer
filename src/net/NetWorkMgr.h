/**
 * @file NetWorkMgr.h
 * @author huwenhao ()
 * @brief 网络资源管理
 * @date 2023-08-04
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#pragma once
#include <unordered_map>
#include "Singleton.h"
#include "NetCommDef.h"
#include "NetWork.h"


class NetWorkMgr : public Singleton<NetWorkMgr>
{
public:
    // key: fd, val: network对象
    typedef std::unordered_map<int, NetWork::ptr> NetWorkMap;

    NetWorkMgr();
    ~NetWorkMgr();
    
    /**
     * @brief 初始化
     * 
     * @return int 
     */
    int Init(int iPort = -1);

    /**
     * @brief 监听io事件
     * 
     * @return int 
     */
    int Proc();

    /**
     * @brief 客户端发第一次握手包
     * 
     * @param stServerAddr 
     * @return int 
     */
    int SendHandShake1Msg(const sockaddr_in& stServerAddr);

    /**
     * @brief 发送心跳包
     * 
     * @return int 
     */
    int SendHeartBeat(const sockaddr_in& stServerAddr);

    /**
     * @brief 客户端是否成功连接
     * 
     * @return true 
     * @return false 
     */
    bool ClientConnected();

private:
    /**
     * @brief 创建网络对象
     * 
     * @param sNIC 网卡名
     * @return NetWork::ptr 
     */
    NetWork::ptr CreateNetWork(std::string& sNIC);

    /**
     * @brief 创建网络对象
     * 
     * @param stAddr ip地址
     * @return NetWork::ptr 
     */
    NetWork::ptr CreateNetWork(in_addr_t stAddr);

    /**
     * @brief 创建socket
     * 
     * @param stAddr 
     * @return int 
     */
    int CreateSocket(const struct sockaddr_in& stAddr);

private:
    int m_iPort;
    NetWorkMap m_mapNetWork;  
};