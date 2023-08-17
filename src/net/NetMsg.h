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
    inline size_t GetRemain() const {return m_ullSize;}

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

    /**
     * @brief 读取非可靠包消息
     * 
     * @param szOutBuff 消息地址
     * @return int 消息长度
     */
    int PeekUnreliable(const char **szOutBuff);
private:
    const char *m_ptrBuff;  // 未读缓冲区地址
    size_t      m_ullSize;  // 未读缓冲区大小
};


class NetWriter
{
public:
    NetWriter(char *ptrBuff, size_t ulSize) : m_ptrBuff(ptrBuff), m_ulMaxSize(ulSize), m_ulSize(0) {}

    /**
     * @brief 检查写入空间是否足够
     * 
     * @param ulSize 
     * @return true 
     * @return false 
     */
    inline bool CheckBuff(size_t ulSize) const {return ulSize <= (m_ulMaxSize - m_ulSize);}

    /**
     * @brief 获取写缓冲区已写数据长度
     * 
     * @return size_t 
     */
    inline size_t GetSize() const {return m_ulSize;}

    /**
     * @brief 业务数据编码->网络包
     */
    int Write(STNetMsgHead& stHead);
    int Write(STHandShakePacket& stPacket);
    int Write(STHeartBeatPacket& stPacket);
    int Write(STReconnectPacket& stPacket);
    int Write(STRstPacket& stPacket);
    int Write(STFinPacket& stPacket);
    int WriteDataHead(DataHead& stHead);
    int WriteUnreliable(const char *pData, size_t ulLen);
    int WriteRawData(const char *pData, size_t ulLen);

private:
    char        *m_ptrBuff;     // 写缓冲区
    size_t      m_ulMaxSize;    // 写缓冲区大小
    size_t      m_ulSize;       // 已用大小
};