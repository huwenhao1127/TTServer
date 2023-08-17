#include "SendQueue.h"

SendQueue::SendQueue()
{
    m_ulSize = 0;
    m_stHead.pre = &m_stHead;
    m_stHead.next = &m_stHead;
}

SendQueue::~SendQueue()
{
}

int SendQueue::Push(SendQueueNode *pNode)
{
    SendQueue::AddTail(&m_stHead, pNode);
    ++m_ulSize;
    return 0;
}

int SendQueue::Pop()
{
    if (IsEmpty())
    {
        return 0;
    }

    SendQueueNode *pstNode = Front();
    return DelNode(pstNode);
}

int SendQueue::DelNode(SendQueueNode *pNode)
{
    Del(pNode);
    --m_ulSize;
    return 0;
}

SendQueueNode* SendQueue::Front()
{
    return m_stHead.next;
}

SendQueueNode* SendQueue::Back()
{
    return m_stHead.pre;
}

void SendQueue::AddTail(SendQueueNode *pHead, SendQueueNode *pNode)
{
    pNode->pre = pHead->pre;
    pNode->next = pHead;
    pHead->pre->next = pNode;
    pHead->pre = pNode;
}

void SendQueue::Del(SendQueueNode *pNode)
{
    pNode->pre->next = pNode->next;
    pNode->next->pre = pNode->pre;
}
