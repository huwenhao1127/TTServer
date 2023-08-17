#include "CookieMgr.h"
#include "NetCommDef.h"
#include "Now.h" 
#include <random>

int CookieMgr::Init()
{
    // 初始化密钥
    ZeroStruct(m_stCtxData);
    for (int i = 0; i < SECRET_KEY_NUM; ++i)
    {
        STHMACKey& stKey = m_stCtxData.astKeys[i];
        std::string sKey = "";
        for (int j = 0; j < SECRET_KEY_LEN; ++j)
        {
            stKey.szKey[j] = (uint8_t)CookieMgr::RandomInt2(0, 255);
            sKey = sKey + std::to_string(stKey.szKey[j]);
        }
        
        LOG_DBG_FMT(ptrNetLogger, "random key: {}", sKey);
    }

    // 初始化HMAC
    m_pHmacCtx = HMAC_CTX_new();
    CHECK_IF_PARAM_NULL(ptrNetLogger, m_pHmacCtx, -1);
    HMAC_CTX_reset(m_pHmacCtx);
    m_pHmacAlg = EVP_sha1();

    LOG_DBG_FMT(ptrNetLogger, "CookieMgr init succ.");
    return 0;
}

void CookieMgr::Tick20S()
{
    // 更新时间戳和序列号
    m_stCtxData.bKeySeq++;
    m_stCtxData.tUpdateTime = Now::TimeStamp();
}

int CookieMgr::CreateCookie(const sockaddr_in& stClientAddr, time_t tTimeStamp, uint8_t bKeySeq, char *szCookie, const size_t ulCookieLen)
{
    // 需要认证的数据: 客户端地址 + 时间戳
    uint8_t szUserData[COOKIE_HMAC_USER_DATA_LEN];
    memcpy(szUserData, &stClientAddr, sizeof(sockaddr_in));
    memcpy(szUserData + sizeof(sockaddr_in), &tTimeStamp, sizeof(time_t));

    // 根据序列号获取key
    STHMACKey& stKey = m_stCtxData.astKeys[bKeySeq % SECRET_KEY_NUM];

    // 用HMAC计算cookie
    int iRet = HMAC_Init_ex(m_pHmacCtx, stKey.szKey, SECRET_KEY_LEN, m_pHmacAlg, nullptr);
    if (1 != iRet)
    {
        LOG_ERR_FMT(ptrNetLogger, "HMAC_Init_ex fail, ret:{} client:{}", iRet, sock_addr(&stClientAddr));
        memcpy(szCookie, szUserData, COOKIE_HMAC_USER_DATA_LEN);
        return -1;
    }

    iRet = HMAC_Update(m_pHmacCtx, szUserData, COOKIE_HMAC_USER_DATA_LEN);
    if (1 != iRet)
    {
        LOG_ERR_FMT(ptrNetLogger, "HMAC_Update fail, ret:{} client:{}", iRet, sock_addr(&stClientAddr));
        memcpy(szCookie, szUserData, COOKIE_HMAC_USER_DATA_LEN);
        return -1;
    }

    uint32_t ulLen = ulCookieLen;
    iRet = HMAC_Final(m_pHmacCtx, (unsigned char*)szCookie, &ulLen);
    if (1 != iRet || ulLen != ulCookieLen)
    {
        LOG_ERR_FMT(ptrNetLogger, "HMAC_Update fail, ret:{} client:{}, len:{}", iRet, sock_addr(&stClientAddr), ulLen);
        memcpy(szCookie, szUserData, COOKIE_HMAC_USER_DATA_LEN);
        return -1;
    }

    return 0;
}

int CookieMgr::RandomInt(int _min, int _max)
{
    if (_min == _max)
    {
        return _min;
    }
    else if (_min > _max)
    {
        return _max;
    }
    
    if (_min < 0)
    {
        return _min + RandomInt(0, _max - _min);
    }

    return _min + (int)((_max - _min + 1) * (rand() / (RAND_MAX + 1.0))); 
}

int CookieMgr::RandomInt2(int _min, int _max)
{
    std::random_device rd;
    std::mt19937 generator(rd());  // 使用 Mersenne Twister 作为随机数引擎
    std::uniform_int_distribution<int> distribution(_min, _max);
    return distribution(generator);
}