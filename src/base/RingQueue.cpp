#include "RingQueue.h"
#include <string.h>
#include <stdlib.h>
#include <iostream>

// 将num向上扩展为2的幂
static inline uint32_t round_pow_of_two(uint32_t num)
{
    if (num < 2)
        return 1;
    return 1 << (int)(sizeof(uint32_t) * 8 - __builtin_clz(num - 1));
}

static inline uint32_t min(uint32_t a, uint32_t b)
{
    return (a < b) ? a : b;
}

RingQueue::RingQueue(uint32_t ulDataBufSize)
{
    if (ulDataBufSize & (ulDataBufSize - 1))
    {
        ulDataBufSize = round_pow_of_two(ulDataBufSize);
    }
    m_ulDataBufSize = ulDataBufSize;
    m_pDataBuf = (char*)malloc(ulDataBufSize);
    m_ulHead = 0;
    m_ulTail = 0;
    m_ulWriteHead = 0;
    m_ulReadTail = 0;
    m_bOnWrite.store(false);
    m_bOnRead.store(false);
}

RingQueue::~RingQueue()
{
    free(m_pDataBuf);
    m_pDataBuf = nullptr;
}

int RingQueue::Push(char* pData, uint32_t ulDataLen)
{
    while (true)
    {
        if (!m_bOnWrite.load())
        {
            m_bOnWrite.store(true);
            break;
        }
    }

    RingQueueDataHead stHead = {ulDataLen};
    uint32_t ulCurSize = RemainingSpace();
    
    if (ulDataLen + sizeof(stHead) > ulCurSize)
    {
        LOG_ERR_FMT(ptrLogger, " write data fail. cur size[{}] data size[{}]", ulCurSize, ulDataLen);
        m_bOnWrite.store(false);
        return -1;
    }

    // 写数据头
    WriteData((char*)&stHead, sizeof(stHead));
    // 写数据
    WriteData(pData, ulDataLen);

    // 完成数据写入后更新头指针
    m_ulHead = m_ulWriteHead;
    m_bOnWrite.store(false);
    return 0;
}

int RingQueue::AllocNewData(char* pData, uint32_t ulDataLen)
{
    return 0;
}

void RingQueue::WriteData(char* pData, uint32_t ulDataLen)
{
    // (m_ulHead & (m_ulDataBufSize - 1))位运算取模，计算头指针真实位置
    uint32_t ulCurHead = m_ulWriteHead & (m_ulDataBufSize - 1);

    // 拷贝到数组尾部 
    uint32_t ulLenTail = min(ulDataLen, m_ulDataBufSize - ulCurHead);
    memcpy(m_pDataBuf + ulCurHead, pData, ulLenTail);

    // 拷贝到数组头部
    uint32_t ulLenHead = ulDataLen - ulLenTail;
    memcpy(m_pDataBuf, pData + ulLenTail, ulLenHead);
    
    m_ulWriteHead += ulDataLen;
}

uint32_t RingQueue::Pop(char* pData)
{
    while (true)
    {
        if (!m_bOnRead.load())
        {
            m_bOnRead.store(true);
            break;
        }
    }

    if (Empty())
    {
        m_bOnRead.store(false);
        return 0;
    }

    // 读数据头
    RingQueueDataHead stHead = {0};
    ReadData((char*)&stHead, sizeof(stHead));
    // 读数据
    ReadData(pData, stHead.ulDataLen);

    // 完成数据读出后设置尾指针
    m_ulTail = m_ulReadTail;
    m_bOnRead.store(false);
    return stHead.ulDataLen;
}

void RingQueue::ReadData(char* pData, uint32_t dwDataLen)
{
    uint32_t dwCurTail= m_ulReadTail & (m_ulDataBufSize - 1);

    // 拷贝数组尾部数据
    uint32_t dwLenTail = min(dwDataLen, m_ulDataBufSize - dwCurTail);
    memcpy(pData, m_pDataBuf + dwCurTail, dwLenTail);

    // 拷贝数组头部数据
    uint32_t dwLenHead = dwDataLen - dwLenTail;
    memcpy(pData + dwLenTail, m_pDataBuf, dwLenHead);
    
    m_ulReadTail += dwDataLen;
}

void RingQueue::Debug()
{
    std::cout << "cur size:" << RemainingSpace() << 
        " cur head:" << (m_ulHead & (m_ulDataBufSize - 1)) << 
        " cur tail:" << (m_ulTail & (m_ulDataBufSize - 1)) << std::endl;
}