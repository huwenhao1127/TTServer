#pragma once
#include "NetMgr.h"

class BaseClient
{
public:
    BaseClient(int iPort) : m_ulConnID(0), m_iPort(iPort) {}
    virtual ~BaseClient() {}

private:
    // 禁止拷贝
    BaseClient(const BaseThread&);
    BaseClient& operator=(const BaseThread&);

public:
    /**
     * @brief 初始化
     * @return int 
     */
    int Init();

    /**
     * @brief 建立连接
     * @return int 
     */
    int Connect(const char *szServerAddr, int iServerPort);

private:


protected:
    uint32_t m_ulConnID;
    int m_iPort;
};

