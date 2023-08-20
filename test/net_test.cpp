
#include <iostream>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <arpa/inet.h>
#include <openssl/bn.h>
#include <openssl/dh.h>
#include <openssl/aes.h>
#include "SendQueue.h"
#include "NetSecurityMgr.h"

struct TestPacket
{
    uint32_t ulConnectID;                   // 连接ID
    size_t ulDataLen;                       // 数据长度
    SendQueueNode stQueNode;                // 发送队列节点
    size_t ulBuffLen;                       // Buffer长度
    char szBuff[0];                         // Buffer首地址
};

// 队列测试
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

// DH协商密钥测试
void DHTest(uint8_t *szKey, int& iKeyLen)
{
    int key_length = 128;
    uint32_t generator = 5;
    char szP[] = "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFF61";    // 128bit最大素数

    // 客户端
    BIGNUM *pP = BN_new();
    int iRet = BN_hex2bn(&pP, szP);
    if (0 >= iRet)
    {
        std::cout << "BN_hex2bn error: " << iRet << std::endl;
        return;
    }
    uint32_t ulPBitNum = BN_num_bits(pP);
    if ((uint32_t)key_length != ulPBitNum)
    {
        std::cout << "error P len: " << ulPBitNum << std::endl;
        return;
    }
    uint32_t ulG = htonl(generator);
    BIGNUM *pG = BN_bin2bn((const unsigned char*)&ulG, sizeof(ulG), nullptr);

    DH *pClientDH = DH_new();
    // 设置DH参数
    DH_set0_pqg(pClientDH, pP, nullptr, pG);
    int iStatus = 0;
    iRet = DH_check(pClientDH, &iStatus);
    if ((iRet != 1) || (iStatus != 0 && iStatus != 2))
    {   
        std::cout << "client dh check fail: " << iRet << " " << iStatus << std::endl;
        return;
    }

    // 生成公钥私钥
    if (DH_generate_key(pClientDH) != 1) 
    {
        std::cout << "DH_generate_key error" << std::endl;
        return;
    }
    const BIGNUM *pClientPubKey = DH_get0_pub_key(pClientDH);
    const BIGNUM *pClientPrivKey = DH_get0_priv_key(pClientDH);
    const char *szClientPubKey = BN_bn2hex(pClientPubKey);
    std::cout << "client pub key: " << szClientPubKey << std::endl;
    const char *szClientPrivKey = BN_bn2hex(pClientPrivKey);
    std::cout << "client priv key: " << szClientPrivKey << std::endl;

    // 服务器
    DH *pServerDH = DH_new();
    // 设置DH参数
    DH_set0_pqg(pServerDH, pP, nullptr, pG);
    iStatus = 0;
    iRet = DH_check(pServerDH, &iStatus);
    if ((iRet != 1) || (iStatus != 0 && iStatus != 2))
    {   
        std::cout << "server dh check fail: " << iRet << " " << iStatus << std::endl;
        return;
    }

    // 生成公钥私钥
    if (DH_generate_key(pServerDH) != 1) 
    {
        std::cout << "DH_generate_key error" << std::endl;
        return;
    }
    const BIGNUM *pServerPubKey = DH_get0_pub_key(pServerDH);
    const BIGNUM *pserverPrivKey = DH_get0_priv_key(pServerDH);
    const char *szServerPubKey = BN_bn2hex(pServerPubKey);
    std::cout << "server pub key: " << szServerPubKey << std::endl;
    const char *szServerPrivKey = BN_bn2hex(pserverPrivKey);
    std::cout << "server priv key: " << szServerPrivKey << std::endl;
    // 生成密钥128bit
    uint8_t szServerKey[16];    
    int iServerKeyLen = DH_compute_key(szServerKey, pClientPubKey, pServerDH);
    if (0 > iServerKeyLen)
    {
        std::cout << "DH_compute_key error: " << iServerKeyLen << std::endl;
        return;
    }
    std::cout << "server key len: " << iServerKeyLen << std::endl;
    std::cout << "server key: " << std::endl;
    PrintKey(szServerKey, iServerKeyLen);

    // 客户端生成密钥
    uint8_t szClientKey[16]; 
    int iClientKeyLen = DH_compute_key(szClientKey, pServerPubKey, pClientDH);
    if (0 > iClientKeyLen)
    {
        std::cout << "DH_compute_key error: " << iClientKeyLen << std::endl;
        return;
    }
    std::cout << "client key len: " << iClientKeyLen << std::endl;
    std::cout << "client key: " << std::endl;
    PrintKey(szClientKey, iClientKeyLen);

    memcpy(szKey, szClientKey, iClientKeyLen);
    iKeyLen = iClientKeyLen;
    return;
}

// AES加密测试
void AESTest()
{
    uint8_t szKey[16];
    int iKeyLen = 0;
    DHTest(szKey, iKeyLen);
    if (0 == iKeyLen)
    {
        std::cout << "key error: " << iKeyLen << std::endl;
        return;
    }
    std::cout << "-------------------" << std::endl;

    std::string sMsg("hello AES test!");
    std::cout << "old msg: " << sMsg << std::endl;
    std::cout << "old msg size: " << sMsg.size() << std::endl;

    int iRet = NetSecurityMgr::Inst().Init();
    if (0 != iRet)
    {
        LOG_ERR_FMT(ptrNetLogger, "init error.");
    }
    iRet = NetSecurityMgr::Inst().SetEncKey(szKey, iKeyLen);
    if (0 != iRet)
    {
        LOG_ERR_FMT(ptrNetLogger, "set EncKey error.");
    }

    // 加密
    char szCipher[50] = {0};
    size_t ulLen = 0; 
    NetSecurityMgr::Inst().EncryptData(sMsg.c_str(), sMsg.size(), szCipher, ulLen);
    std::cout << "Cipher len: " << ulLen << std::endl;

    // 解密
    char szDecrypted[50] = {0};
    size_t ulOutLen = 0; 
    NetSecurityMgr::Inst().DecryptData(szCipher, ulLen, szDecrypted, ulOutLen);
    std::cout << "Decrypted msg: " << std::string(szDecrypted) << std::endl;
    std::cout << "Decrypted msg size: " << ulOutLen << std::endl;
}

bool IsBigEnd()
{
    uint16_t usNum = 0x1234;
    const unsigned char* pNum = (unsigned char*)&usNum;
    if (*pNum == 0x12)
    {
        std::cout << "BigEnd" << std::endl;
        return true;
    }
    std::cout << "LittleEnd" << std::endl;
    return false;
}

int main()
{
    AESTest();
    return 0;
}