#pragma once
#include "Socket.h"
#include "NetCommDef.h"
#include <stdint.h>

// 网络包类
class NetMsg
{
public:
    NetMsg() {}
    ~NetMsg() {}

    /**
     * @brief 初始化
     * 
     * @return int 
     */
    int Init();

    /**
     * @brief 解码
     * 
     * @param szNetBuff  udp包数据
     * @param ullLen     udp包数据长度
     * @return int 
     */
    int DecodeMsg(const char *szNetBuff, uint64_t ullLen);

    /**
     * @brief 编码
     * 
     * @param szOutPutBuff [out]输出缓冲区
     * @param ullLen       [out]输出数据大小
     * @return int 
     */
    int EncodeMsg(char *szOutPutBuff, uint64_t& ullLen);

    /**
     * @brief 获取网络包头
     * 
     * @return const STNetMsgHead& 
     */
    inline const STNetMsgHead& Head() const {return m_stHead;}

    /**
     * @brief 获取网络包体
     * 
     * @return const UNetMsgBody& 
     */
    inline const UNetMsgBody& Body() const {return m_stBody;}

    /**
     * @brief 获取网络包大小
     * 
     * @return const uint64_t 
     */
    inline const uint64_t NetSize() const {return m_ullNetSize;}

private:
    STNetMsgHead m_stHead;  // 消息头
    UNetMsgBody m_stBody;   // 消息体
    uint64_t m_ullNetSize;  // 网络包大小
};

class NetReader
{
public:
    NetReader(const char *ptrBuff, uint64_t ullSize) : m_ptrBuff(ptrBuff), m_ullSize(ullSize) {}

    /**
     * @brief 获取未读缓冲区地址
     * 
     * @return const char* 
     */
    inline const char* GetBuff() const {return m_ptrBuff;}

    /**
     * @brief 获取未读缓冲区大小
     * 
     * @return uint64_t 
     */
    inline uint64_t GetRemain() const {return m_ullSize;}

    /**
     * @brief udp数据解码->网络包
     */
    int Read(STNetMsgHead& stHead);
    int Read(STHandShakePacket& stPacket);
    int Read(STHeartBeatPacket& stPacket);
    int Read(STReconnectPacket& stPacket);
    int Read(STRstPacket& stPacket);
    int Read(STFinPacket& stPacket);
    int ReadDataHead(DataHead& stHead);
private:
    const char *m_ptrBuff;  // 未读缓冲区地址
    uint64_t    m_ullSize;  // 未读缓冲区大小
};


class NetWritter
{
public:

private:
    
};