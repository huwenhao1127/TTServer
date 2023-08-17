#pragma once
#include <stddef.h>

// 队列节点
struct SendQueueNode
{
    SendQueueNode* pre;
    SendQueueNode* next;
};

// 根据node地址获取type地址
#define send_queue_entry(ptr, type, member) ((type*)((char*)(ptr) - (size_t)(&((type*)0)->member)))

class SendQueue
{
public:
    SendQueue();
    ~SendQueue();

    /**
     * @brief 队尾添加节点
     * 
     * @param pNode 
     * @return int 
     */
    int Push(SendQueueNode *pNode);

    /**
     * @brief 队首弹出节点
     * 
     * @return int 
     */
    int Pop();
public:
    /**
     * @brief 获取队列头部
     * 
     * @return const SendQueueNode* 
     */
    inline const SendQueueNode* Head() const {return &m_stHead;}

    /**
     * @brief 队内节点数
     * 
     * @return size_t 
     */
    inline size_t Size() const {return m_ulSize;}

    /**
     * @brief 队列是否空
     * 
     * @return true 
     * @return false 
     */
    inline bool IsEmpty() const {return (0 == m_ulSize);}

     /**
     * @brief 获取队首节点
     * 
     * @return SendQueueNode* 
     */
    SendQueueNode* Front();

    /**
     * @brief 获取队尾节点
     * 
     * @return SendQueueNode* 
     */
    SendQueueNode* Back();

    /**
     * @brief 删除节点
     * 
     * @param pNode 
     * @return int 
     */
    int DelNode(SendQueueNode *pNode);

public:
    /** 链表操作 **/

    /**
     * @brief 访问节点next
     * 
     * @param pNode 
     * @return SendQueueNode* 
     */
    static inline SendQueueNode* Next(SendQueueNode *pNode) {return pNode->next;}

    /**
     * @brief 访问节点pre
     * 
     * @param pNode 
     * @return SendQueueNode* 
     */
    static inline SendQueueNode* Pre(SendQueueNode *pNode) {return pNode->pre;}

    /**
     * @brief 在head尾部添加节点
     * 
     * @param pHead 
     * @param pNode 
     */
    static inline void AddTail(SendQueueNode *pHead, SendQueueNode *pNode);

    /**
     * @brief 删除节点
     * 
     * @param pNode 
     */
    static inline void Del(SendQueueNode *pNode);

private:
    SendQueueNode m_stHead;      // 队首
    size_t m_ulSize;            // 队列大小
};


