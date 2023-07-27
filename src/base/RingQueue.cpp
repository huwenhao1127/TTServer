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
}

RingQueue::~RingQueue()
{
    free(m_pDataBuf);
    m_pDataBuf = nullptr;
}

int RingQueue::Push(char* pData, uint32_t ulDataLen)
{
    RingQueueDataHead stHead = {ulDataLen};
    uint32_t dwCurSize = RemainingSpace();
    
    if (ulDataLen + sizeof(stHead) > dwCurSize)
    {
        LOG_ERR_FMT(ptrLogger, " write data fail. cur size[{}] data size[{}]", dwCurSize, ulDataLen);
        return -1;
    }

    // 写数据头
    WriteData((char*)&stHead, sizeof(stHead));
    // 写数据
    WriteData(pData, ulDataLen);
    return 0;
}

void RingQueue::WriteData(char* pData, uint32_t ulDataLen)
{
    // (m_ulHead & (m_ulDataBufSize - 1))高效取模，计算头指针真实位置
    uint32_t dwCurHead = m_ulHead & (m_ulDataBufSize - 1);

    // 拷贝到数组尾部 
    uint32_t dwLenTail = min(ulDataLen, m_ulDataBufSize - dwCurHead);
    memcpy(m_pDataBuf + dwCurHead, pData, dwLenTail);

    // 拷贝到数组头部
    uint32_t dwLenHead = ulDataLen - dwLenTail;
    memcpy(m_pDataBuf, pData + dwLenTail, dwLenHead);
    
    m_ulHead += ulDataLen;
}

uint32_t RingQueue::Pop(char* pData)
{
    // 读数据头
    RingQueueDataHead stHead = {0};
    ReadData((char*)&stHead, sizeof(stHead));

    // 读数据
    ReadData(pData, stHead.ulDataLen);
    return stHead.ulDataLen;
}

void RingQueue::ReadData(char* pData, uint32_t dwDataLen)
{
    uint32_t dwCurTail= m_ulTail & (m_ulDataBufSize - 1);

    // 拷贝数组尾部数据
    uint32_t dwLenTail = min(dwDataLen, m_ulDataBufSize - dwCurTail);
    memcpy(pData, m_pDataBuf + dwCurTail, dwLenTail);

    // 拷贝数组头部数据
    uint32_t dwLenHead = dwDataLen - dwLenTail;
    memcpy(pData + dwLenTail, m_pDataBuf, dwLenHead);

    m_ulTail += dwDataLen;
}

void RingQueue::Debug()
{
    std::cout << "cur size:" << RemainingSpace() << 
        " cur head:" << (m_ulHead & (m_ulDataBufSize - 1)) << 
        " cur tail:" << (m_ulTail & (m_ulDataBufSize - 1)) << std::endl;
}