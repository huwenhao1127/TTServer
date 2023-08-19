#pragma once
#include "Singleton.h"
#include "NetCommDef.h"
#include <openssl/aes.h>
#include <openssl/bn.h>
#include <openssl/dh.h>

class NetSecurityMgr : public Singleton<NetSecurityMgr>
{
public:
    NetSecurityMgr();
    ~NetSecurityMgr();

    /**
     * @brief 初始化
     * 
     * @return int 
     */
    int Init();

public:
    /** DH算法相关 **/
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

public:
    /** AES算法相关 **/
    /**
     * @brief 设置密钥
     * 
     * @param pszEncKey 
     * @param iKeyLen 
     * @return int 
     */
    int SetEncKey(const uint8_t *pszEncKey, int iKeyLen);

    /**
     * @brief 加密
     * 
     * @param szInput 
     * @param ulInLen 
     * @param szOutput 
     * @param ulOutLen 
     * @return int 0:succ 
     */
    int EncryptData(const char *szInput, size_t ulInLen, char *szOutput, size_t& ulOutLen);

    /**
     * @brief 解密
     * 
     * @param szInput 
     * @param ulInLen 
     * @param szOutput 
     * @param ulOutLen 
     * @return int 0:succ
     */
    int DecryptData(const char *szInput, size_t ulInLen, char *szOutput, size_t& ulOutLen);

private:
    /** DH算法相关 BEGIN **/
    BIGNUM *m_pBigNumP;            // 参数P,大素数
    uint32_t m_ulBigNumPBitNum;    // P位数
    BIGNUM *m_pBigNumG;            // 参数G
    /** DH算法相关 END **/

    /** AES算法相关 BEGIN **/
    char m_szEncyptKey[MAX_ENCRYPT_DATA_LEN];   // key
    unsigned char m_szIVec[16];                 // iv
    AES_KEY m_stAESKey;                         // AES KEY
    /** AES算法相关 END **/
};