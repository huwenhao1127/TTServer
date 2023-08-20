#include "BaseClient.h"
#include "Now.h"
#include "NetCommDef.h"
#include "Socket.h"
#include "NetWorkMgr.h"
#include <arpa/inet.h>

int BaseClient::Init()
{
    // 初始化网络线程
    return NetMgr::Inst().Init(m_iPort);
}

int BaseClient::Connect(const char *szServerAddr, int iServerPort)
{
    NetMgr::Inst().Start();
    // 发起握手请求
    struct sockaddr_in stAddr;
    stAddr.sin_family = AF_INET;
    stAddr.sin_port = htons(iServerPort);
    inet_pton(AF_INET, szServerAddr, &(stAddr.sin_addr));
    NetWorkMgr::Inst().SendHandShake1Msg(stAddr);

    Now::Update();
    time_t tLastHandShake = Now::TimeStamp();
    while (!NetWorkMgr::Inst().ClientConnected())
    {
        Now::Update();
        if (tLastHandShake + 5 < Now::TimeStamp())
        {
            // 5s秒发起一次连接
            NetWorkMgr::Inst().SendHandShake1Msg(stAddr);
            tLastHandShake = Now::TimeStamp();
        }
        if (NetWorkMgr::Inst().ClientConnected())
        {
            break;
        }
    }
    LOG_DBG_FMT(ptrNetLogger, " connect server[{}] succ.", sock_addr(&stAddr));
    
    time_t tLastHeartBeat = Now::TimeStamp();
    //time_t tLastSendMsg = Now::TimeStamp();
    while (true)
    {
        usleep(1000);
        Now::Update();
        // 维护心跳
        if (tLastHeartBeat + 1 < Now::TimeStamp())
        {
            NetWorkMgr::Inst().SendHeartBeat(stAddr);
            tLastHeartBeat = Now::TimeStamp();
        }
    }
    return 0;
}