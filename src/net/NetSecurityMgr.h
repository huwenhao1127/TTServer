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

    /**
     * @brief 
     * 
     * @param key 
     * @param keyLength 
     * @param szInfo 
     */
    static void PrintKey(const unsigned char* key, size_t keyLength, const char *szInfo);

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
    
    /**
     * @brief 客户端生成公钥私钥
     * 
     * @param szA       [out] 公钥
     * @param ulALen 
     * @param szB       [out] 私钥
     * @param ulBLen 
     * @return int 
     */
    int HandleHandShake1ACK(uint8_t *szA, uint16_t& ulALen, uint8_t *szB, uint16_t& ulBLen);

    /**
     * @brief 客户端生成密钥
     * 
     * @param szServer  [in] 服务器公钥
     * @param ulALen 
     * @param szA       [in] 客户端公钥
     * @param ulALen 
     * @param szB       [in] 客户端私钥
     * @param ulBLen    
     * @param szKey     [out] key
     * @param ulKeyLen 
     * @return int 
     */
    int HandleHandShake2ACK(
        const uint8_t *szServer, uint16_t ulServer, const uint8_t *szA, uint16_t ulALen, const uint8_t *szB, uint16_t ulBLen, uint8_t *szKey, uint16_t& ulKeyLen);

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