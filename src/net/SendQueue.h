#pragma once


struct SendQueueNode
{
    SendQueueNode* pre;
    SendQueueNode* next;
};


class SendQueue
{
public:
    SendQueue() {}
    ~SendQueue() {}

private:
    SendQueueNode m_oHead;      // 队首
    size_t m_ulSize;            // 队列大小
};


