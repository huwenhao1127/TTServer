#pragma once
#include "Logger.h"

#define MAX_RING_QUEUE_DATA_SIZE 10*1024*1024

static tts::Logger::ptr ptrLogger(new tts::Logger("RingQueue", 
                                tts::EnmLoggerLevel::ERROR, 
                                LOGGER_APPENDER_ROTATE + LOGGER_APPENDER_STDOUT,
                                nullptr,
                                "rqlog",
                                "/home/wenhowhu/TTServer/rqlog"));

struct RingQueueDataHead
{
    size_t ulDataLen;
    size_t ulDataStart;
};

class RingQueue
{
public:
    RingQueue(size_t ulDataBufSize);
    ~RingQueue();

    int Pop(void* pDataBuf, RingQueueDataHead& stDataHead);

    int Push(void* pData, size_t ulDataLen);

private:
    /**
     * @brief 当前剩余空间
     * 
     * @return size_t 
     */
    size_t RemainingSpace();

private:
    void* m_pDataBuf;             // 数据区
    size_t m_ulDataBufSize;       // 数据区总大小
    size_t m_ulHead;              // 队列头指针
    size_t m_ulTail;              // 队列尾指针
    bool m_bSameCircle;           // 头尾是否在同一圈
};