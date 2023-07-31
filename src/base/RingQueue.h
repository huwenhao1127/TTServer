/**
 * @file RingQueue.h
 * @author huwenhao ()
 * @brief 无锁环形队列（单消费者单生产者无锁）
 * @date 2023-07-27
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#pragma once
#include "Logger.h"
#include <atomic>

#define MAX_RING_QUEUE_DATA_SIZE 10*1024*1024

static tts::Logger::ptr ptrLogger(new tts::Logger("RingQueue", 
                                tts::EnmLoggerLevel::ERROR, 
                                LOGGER_APPENDER_ROTATE + LOGGER_APPENDER_STDOUT,
                                nullptr,
                                "rqlog",
                                "/home/wenhowhu/TTServer/log/rqlog"));

struct RingQueueDataHead
{
    uint32_t ulDataLen;   // 数据长度
};

class RingQueue
{
public:
    RingQueue(uint32_t ulDataBufSize);
    ~RingQueue();
    void Debug();

    /**
     * @brief 消费数据
     * 
     * @param pDataBuf  [out] 数据地址
     * @return uint32_t 数据大小
     */
    uint32_t Pop(char* pData);

    /**
     * @brief 添加数据
     * 
     * @param pData     数据
     * @param ulDataLen 数据大小
     * @return int      0succ
     */
    int Push(char* pData, uint32_t ulDataLen);

private:
    /**
     * @brief 当前剩余空间
     * 
     * @return size_t 
     */
    inline uint32_t RemainingSpace() {return m_ulDataBufSize - m_ulHead + m_ulTail;}

    inline bool Empty() {return m_ulHead == m_ulTail;}

    /**
     * @brief 写数据（该接口只负责写数据，外部调用需要判断buf容量是否足够）
     * 
     * @param pData 待写入数据地址
     * @param ulDataLen 数据大小
     */
    void WriteData(char* pData, uint32_t ulDataLen);

    /**
     * @brief 读数据
     * 
     * @param pData [out] 数据读入地址
     * @param dwDataLen 需要读入的数据大小
     */
    void ReadData(char* pData, uint32_t dwDataLen);

private:
    char* m_pDataBuf;                       // 数据区
    volatile uint32_t m_ulDataBufSize;      // 数据区总大小
    volatile uint32_t m_ulHead;             // 队列头指针（完整写完头和数据后才会更新）
    volatile uint32_t m_ulTail;             // 队列尾指针（完整读完头和数据后才会更新）
    volatile uint32_t m_ulWriteHead;        // 队列写数据时的指针
    volatile uint32_t m_ulReadTail;         // 队列读数据时的指针
    std::atomic<bool> m_bOnWrite;           // 写锁
    std::atomic<bool> m_bOnRead;            // 读锁
};