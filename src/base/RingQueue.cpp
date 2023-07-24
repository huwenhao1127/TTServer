#include "RingQueue.h"
#include<string.h>

RingQueue::RingQueue(size_t ulDataBufSize)
{
    m_pDataBuf = malloc(ulDataBufSize);
    m_ulDataBufSize = ulDataBufSize;
    m_ulHead = 0;
    m_ulTail = 0;
    m_bSameCircle = true;
}

RingQueue::~RingQueue()
{
    free(m_pDataBuf);
}

int RingQueue::Pop(void* pDataBuf, RingQueueDataHead& stDataHead)
{

    return 0;
}

int RingQueue::Push(void* pData, size_t ulDataLen)
{
    if (ulDataLen + sizeof(RingQueueDataHead) > RemainingSpace())
    {
        LOG_ERR(ptrLogger, "push fail.");
        return -1;
    }
    RingQueueDataHead stDataHead = {ulDataLen, m_ulTail + sizeof(RingQueueDataHead)};
    if (m_ulTail >= m_ulHead)
    {
        size_t ulLenTail = m_ulDataBufSize - m_ulTail;
        size_t ulLenHead = ulDataLen - ulLenTail > 0 ? ulDataLen - ulLenTail : 0;
        memcpy(m_pDataBuf + m_ulTail, pData, ulLenTail);
        memcpy(m_pDataBuf, pData, ulLenHead);
    }
    else 
    {
        memcpy(m_pDataBuf + m_ulTail, pData, ulDataLen);
    }
    return 0;
}

size_t RingQueue::RemainingSpace()
{
    if (m_ulTail >= m_ulHead && m_bSameCircle)
    {
        return (m_ulDataBufSize - m_ulTail + m_ulHead);
    }
    else if (m_ulTail < m_ulHead && !m_bSameCircle)
    {
        return (m_ulHead - m_ulTail);
    }
    LOG_ERR(ptrLogger, "get remaining space fail.");
    return 0;
}