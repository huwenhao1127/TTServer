#pragma once
#include "KcpWrapper.h"

class NetConnect 
{
public:
    NetConnect();
    ~NetConnect();
    void Init();
    void Reclaim();
public:

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

private:
    uint32_t m_ulConnID;

    // kcp
    KcpWrapper m_oKcp;

    // 网络对象

    

};