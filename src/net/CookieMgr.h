#pragma once
#include <stdint.h>
#include <openssl/hmac.h>
#include <stdlib.h>
#include "Singleton.h"
#include "Socket.h"

#define SECRET_KEY_LEN      64  // 密钥长度
#define SECRET_KEY_NUM      2   // 密钥数量
#define COOKIE_HMAC_USER_DATA_LEN   (sizeof(sockaddr_in) + sizeof(time_t))    // 用户数据长度

struct STHMACKey
{
    uint8_t szKey[SECRET_KEY_LEN];
};

struct STCookieCtxData
{
    uint8_t bKeySeq;                        // 密钥序列号
    time_t  tUpdateTime;                    // 更新时间
    STHMACKey astKeys[SECRET_KEY_NUM];      // 密钥列表
};

class CookieMgr : public Singleton<CookieMgr>
{
public:

    int Init();

    void Tick20S();

    /**
     * @brief 创建cookie
     * 
     * @param stClientAddr 
     * @param tTimeStamp 
     * @param bKeySeq 
     * @param szCookie 
     * @param ulCookieLen 
     * @return int 
     */
    int CreateCookie(const sockaddr_in& stClientAddr, time_t tTimeStamp, uint8_t bKeySeq, char *szCookie, const size_t ulCookieLen);

    /**
     * @brief 获取当前密钥序列号
     * 
     * @return uint8_t 
     */
    inline uint8_t GetKeySeq() const {return m_stCtxData.bKeySeq;}

    /**
     * @brief 获取更新时间
     * 
     * @return time_t 
     */
    inline time_t GetUpdateTime() const {return m_stCtxData.tUpdateTime;}

    /**
     * @brief 生成[_min, _max]范围内随机数
     * 
     * @param _min 
     * @param _max 
     * @return int 
     */
    static int RandomInt(int _min, int _max);
    static int RandomInt2(int _min, int _max);
private:
    STCookieCtxData m_stCtxData;
    HMAC_CTX        *m_pHmacCtx;
    const EVP_MD    *m_pHmacAlg;
};
