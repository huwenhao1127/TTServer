
#include "NetMgr.h"
#include "SendQueue.h"

struct TestPacket
{
    uint32_t ulConnectID;                   // 连接ID
    size_t ulDataLen;                       // 数据长度
    SendQueueNode stQueNode;                // 发送队列节点
    size_t ulBuffLen;                       // Buffer长度
    char szBuff[0];                         // Buffer首地址
};

void SendQueueTest()
{
    SendQueue oQueue;
    for (int i = 0; i < 10; i++)
    {
        std::string sMsg = "test queue data: " + std::to_string(i);
        TestPacket* pData = (TestPacket*)malloc(sizeof(TestPacket) + 20);
        pData->ulConnectID = i;
        pData->ulDataLen = sizeof(TestPacket) + 20;
        pData->ulBuffLen = 20;
        memcpy(pData->szBuff, sMsg.c_str(), pData->ulBuffLen);
        oQueue.Push(&pData->stQueNode);
    }

    std::cout << "queue size: " << oQueue.Size() << std::endl;
    while (!oQueue.IsEmpty())
    {
        SendQueueNode *pstNode = oQueue.Front();
        TestPacket *pstData = send_queue_entry(pstNode, TestPacket, stQueNode);
        std::cout << "id: " << pstData->ulConnectID << std::endl;
        std::cout << "data len: " << pstData->ulDataLen << std::endl;
        std::cout << "buff len: " << pstData->ulBuffLen << std::endl;
        char szMsg[20] = {0};
        memcpy(szMsg, pstData->szBuff, pstData->ulBuffLen);
        std::cout << "msg: " << szMsg << std::endl;
        oQueue.Pop();
        free(pstData);
    }
}

int main()
{
    int iRet = NetMgr::Inst().Init();
    if (0 == iRet)
    {
        NetMgr::Inst().Start();
    }
    while (1)
    {
        usleep(1000);
    }
    return 0;
}