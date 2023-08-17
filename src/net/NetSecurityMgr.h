#pragma once
#include "NetCommDef.h"
#include "Singleton.h"

class NetSecurityMgr : public Singleton<NetSecurityMgr>
{
public:
    int Init();

    /**
     * @brief 设置密钥
     * 
     * @param pszEncKey 
     * @param iKeyLen 
     * @return int 
     */
    int SetEncKey(const uint8_t *pszEncKey, int iKeyLen);

    /**
     * @brief 处理客户端握手请求(生成大数B和密钥)
     * 
     * @param szA           大数A
     * @param ulALen        大数A长度
     * @param szB           [out]大数B
     * @param ulBLen        [out]大数B长度
     * @param szRawKey      [out]密钥
     * @param ulRawKeyLen   [out]密钥长度
     * @return int 
     */
    int HandleHandShake2(const uint8_t *szA, uint16_t ulALen, uint8_t *szB, uint16_t& ulBLen, uint8_t *szRawKey, uint16_t& ulRawKeyLen);
    
private:
    char m_szEncyptKey[MAX_ENCRYPT_DATA_LEN];
};