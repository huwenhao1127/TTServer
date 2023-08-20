#include "NetSecurityMgr.h"
#include <sstream>
#include <iomanip>

NetSecurityMgr::NetSecurityMgr()
{
    m_pBigNumP = nullptr;
    m_pBigNumG = nullptr;
    m_ulBigNumPBitNum = 0;
}

NetSecurityMgr::~NetSecurityMgr()
{
    // 单例的指针不用手动释放内存
}

int NetSecurityMgr::Init()
{
    // 初始化参数G
    uint32_t ulG = htonl(5);
    m_pBigNumG = BN_new();
    m_pBigNumG = BN_bin2bn((const unsigned char*)&ulG, sizeof(uint32_t), nullptr);
    if (nullptr == m_pBigNumG)
    {
        LOG_ERR_FMT(ptrNetLogger, "BN_bin2bn fail");
        return -1;
    }

    // 初始化参数P
    m_ulBigNumPBitNum = 128;
    m_pBigNumP = BN_new();
    CHECK_IF_PARAM_NULL(ptrNetLogger, m_pBigNumP, -1);
    char szP[] = "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFF61";    // 128bit最大素数
    int iRet = BN_hex2bn(&m_pBigNumP, szP);
    if (0 >= iRet || nullptr == m_pBigNumP)
    {
        LOG_ERR_FMT(ptrNetLogger, "BN_hex2bn fail, ret:{}", iRet);
        return -1;
    }
    uint32_t ulPBitNum = BN_num_bits(m_pBigNumP);
    if (m_ulBigNumPBitNum != ulPBitNum)
    {
        LOG_ERR_FMT(ptrNetLogger, "BN_num_bits[{}] invalid.", ulPBitNum);
        return -1;
    }

    // 验证DH参数是否合法
    DH *pDH = DH_new();
    DH_set0_pqg(pDH, BN_dup(m_pBigNumP), nullptr, BN_dup(m_pBigNumG));
    int iStatus = 0;
    iRet = DH_check(pDH, &iStatus);
    if ((iRet != 1) || (iStatus != 0 && iStatus != 2))
    {
        LOG_ERR_FMT(ptrNetLogger, "DH check fail, ret:{} status:{}", iRet, iStatus);
        return -1;
    }
    DH_free(pDH);
    LOG_DBG_FMT(ptrNetLogger, "NetSecurityMgr init succ.");
    return 0;
}

int NetSecurityMgr::HandleHandShake2(const uint8_t *szA, uint16_t ulALen, uint8_t *szB, uint16_t& ulBLen, uint8_t *szRawKey, uint16_t& ulRawKeyLen)
{
    CHECK_IF_PARAM_NULL(ptrNetLogger, szA, -1);
    CHECK_IF_PARAM_NULL(ptrNetLogger, szB, -1);
    CHECK_IF_PARAM_NULL(ptrNetLogger, szRawKey, -1);
    DH *pDH = DH_new();
    DH_set0_pqg(pDH, BN_dup(m_pBigNumP), nullptr, BN_dup(m_pBigNumG));

    // 生成公钥私钥
    int iRet = DH_generate_key(pDH);
    if (1 != iRet) 
    {
        LOG_ERR_FMT(ptrNetLogger, "DH_generate_key fail, ret:{}", iRet);
        DH_free(pDH);
        return -1;
    }

    // 公钥为大数B
    const BIGNUM *pPubKey = DH_get0_pub_key(pDH);
    int iPubKeyLen = BN_num_bytes(pPubKey);
    if ((int)ulBLen < iPubKeyLen)
    {
        LOG_ERR_FMT(ptrNetLogger, "PubKey len[{}] invalid, A len:{}", iPubKeyLen, ulBLen);
        DH_free(pDH);
        return -1;
    }
    ulBLen = BN_bn2bin(pPubKey, szB);
    if (ulBLen == 0)
    {
        LOG_ERR_FMT(ptrNetLogger, "PubKey len invalid, A len:{}", ulBLen);
        DH_free(pDH);
        return -1;
    }

    NetSecurityMgr::PrintKey(szA, ulALen, "client pub key: ");
    NetSecurityMgr::PrintKey(szB, ulBLen, "server pub key: ");
        
    // 生成共享密钥, 客户端公钥大数A+本地私钥
    BIGNUM *pBigNumA = BN_bin2bn((const unsigned char*)szA, ulALen, nullptr);
    if (nullptr == pBigNumA)
    {
        LOG_ERR_FMT(ptrNetLogger, "big num A bin2bn fail, len:{}", ulALen);
        DH_free(pDH);
        return -1;
    }

    int iLen = DH_compute_key((unsigned char*)szRawKey, pBigNumA, pDH);
    iRet = 0;
    if (0 >= iLen || ulRawKeyLen < iLen)
    {
        LOG_ERR_FMT(ptrNetLogger, "dh compute key fail, len: {}, {}", iLen, ulRawKeyLen);
        iRet = -1;
    }

    ulRawKeyLen = (uint16_t)iLen;
    BN_free(pBigNumA);
    DH_free(pDH);
    return iRet;
}

int NetSecurityMgr::HandleHandShake1ACK(uint8_t *szA, uint16_t& ulALen, uint8_t *szB, uint16_t& ulBLen)
{
    CHECK_IF_PARAM_NULL(ptrNetLogger, szA, -1);
    DH *pDH = DH_new();
    CHECK_IF_PARAM_NULL(ptrNetLogger, pDH, -1);
    DH_set0_pqg(pDH, BN_dup(m_pBigNumP), nullptr, BN_dup(m_pBigNumG));

    // 生成公钥私钥
    int iRet = DH_generate_key(pDH);
    if (1 != iRet) 
    {
        LOG_ERR_FMT(ptrNetLogger, "DH_generate_key fail, ret:{}", iRet);
        DH_free(pDH);
        return -1;
    }

    // 公钥为大数A
    const BIGNUM *pPubKey = DH_get0_pub_key(pDH);
    int iPubKeyLen = BN_num_bytes(pPubKey);
    if ((int)ulALen < iPubKeyLen)
    {
        LOG_ERR_FMT(ptrNetLogger, "PubKey len[{}] invalid, A len:{}", iPubKeyLen, ulALen);
        DH_free(pDH);
        return -1;
    }
    ulALen = BN_bn2bin(pPubKey, szA);
    if (ulALen == 0)
    {
        LOG_ERR_FMT(ptrNetLogger, "PubKey len invalid, A len:{}", ulALen);
        DH_free(pDH);
        return -1;
    }

    // 私钥
    const BIGNUM *pPrivKey = DH_get0_priv_key(pDH);
    int iPrivKeyLen = BN_num_bytes(pPrivKey);
    if ((int)ulBLen < iPrivKeyLen)
    {
        LOG_ERR_FMT(ptrNetLogger, "PubKey len[{}] invalid, B len:{}", iPrivKeyLen, ulBLen);
        DH_free(pDH);
        return -1;
    }
    ulBLen = BN_bn2bin(pPrivKey, szB);
    if (ulBLen == 0)
    {
        LOG_ERR_FMT(ptrNetLogger, "PubKey len invalid, B len:{}", ulBLen);
        DH_free(pDH);
        return -1;
    }

    NetSecurityMgr::PrintKey(szA, ulALen, "client pub key: ");
    NetSecurityMgr::PrintKey(szB, ulBLen, "client priv key: ");

    DH_free(pDH);
    return 0;
}

int NetSecurityMgr::HandleHandShake2ACK(
    const uint8_t *szServer, uint16_t ulServer, const uint8_t *szA, uint16_t ulALen, const uint8_t *szB, uint16_t ulBLen, uint8_t *szKey, uint16_t& ulKeyLen)
{
    DH *pDH = DH_new();
    DH_set0_pqg(pDH, BN_dup(m_pBigNumP), nullptr, BN_dup(m_pBigNumG));
    BIGNUM *pBigNumA = BN_bin2bn((const unsigned char*)szA, ulALen, nullptr);
    if (nullptr == pBigNumA)
    {
        LOG_ERR_FMT(ptrNetLogger, "big num A bin2bn fail, len:{}", ulALen);
        DH_free(pDH);
        return -1;
    }
    BIGNUM *pBigNumB = BN_bin2bn((const unsigned char*)szB, ulBLen, nullptr);
    if (nullptr == pBigNumB)
    {
        LOG_ERR_FMT(ptrNetLogger, "big num A bin2bn fail, len:{}", ulBLen);
        BN_free(pBigNumA);
        DH_free(pDH);
        return -1;
    }
    DH_set0_key(pDH, pBigNumA, pBigNumB);

    BIGNUM *pBigNumServer = BN_bin2bn((const unsigned char*)szServer, ulServer, nullptr);
    if (nullptr == pBigNumServer)
    {
        LOG_ERR_FMT(ptrNetLogger, "big num server bin2bn fail, len:{}", ulServer);
        DH_free(pDH);
        return -1;
    }

    NetSecurityMgr::PrintKey(szServer, ulServer, "server pub key: ");
    NetSecurityMgr::PrintKey(szA, ulALen, "client pub key: ");
    NetSecurityMgr::PrintKey(szB, ulBLen, "client priv key: ");

    // 生成共享密钥
    int iLen = DH_compute_key((unsigned char*)szKey, pBigNumServer, pDH);
    int iRet = 0;
    if (0 >= iLen || ulKeyLen < iLen)
    {
        LOG_ERR_FMT(ptrNetLogger, "dh compute key fail, len: {}, {}", iLen, ulKeyLen);
        iRet = -1;
    }

    ulKeyLen = (uint16_t)iLen;
    BN_free(pBigNumServer);
    DH_free(pDH);
    return iRet;
}

int NetSecurityMgr::SetEncKey(const uint8_t *pszEncKey, int iKeyLen)
{
    CHECK_IF_PARAM_NULL(ptrNetLogger, pszEncKey, -1);
    // 初始化key
    if (MAX_ENCRYPT_DATA_LEN != iKeyLen)
    {
        LOG_ERR_FMT(ptrNetLogger, "AES SetEncKey fail, len: {}", iKeyLen);
        return -1;
    }
    memcpy(m_szEncyptKey, pszEncKey, iKeyLen);
    return 0;
}

int NetSecurityMgr::EncryptData(const char *szInput, size_t ulInLen, char *szOutput, size_t& ulOutLen)
{    
    AES_set_encrypt_key((const unsigned char*)m_szEncyptKey, 128, &m_stAESKey);              
    ZeroStruct(m_szIVec);

    // 输出字节长度为16字节整数倍,不够填充
    ulOutLen = ((ulInLen / 16) + 1) * 16;
    // 先加密能被16字节整除的块
    size_t ulFullBlockSize = (ulInLen / 16) * 16;
    AES_cbc_encrypt((const unsigned char*)szInput, (unsigned char*)szOutput, ulFullBlockSize, &m_stAESKey, m_szIVec, AES_ENCRYPT);
    szOutput += ulFullBlockSize;

    // 再加密余下不够16字节的块,没有余下的块,也额外添加一个全0块描述原始数据长度
    unsigned char szLeftBlock[16];
    size_t ulLeftDataLen = ulInLen - ulFullBlockSize;
    size_t ulPaddingLen = ulOutLen - ulInLen;
    memcpy(szLeftBlock, (szInput + ulFullBlockSize), ulLeftDataLen);
    for (int i = ulLeftDataLen; i < 16; ++i)
    {
        szLeftBlock[i] = (unsigned char)ulPaddingLen;
        //LOG_DBG_FMT(ptrNetLogger, "cur: {}, padding: {}", i, (int)szLeftBlock[i]);
    }
    AES_cbc_encrypt(szLeftBlock, (unsigned char*)szOutput, 16, &m_stAESKey, m_szIVec, AES_ENCRYPT);
    LOG_DBG_FMT(ptrNetLogger, "encrypt data succ, in:{} out:{}", ulInLen, ulOutLen);
    return 0;
}

int NetSecurityMgr::DecryptData(const char *szInput, size_t ulInLen, char *szOutput, size_t& ulOutLen)
{
    AES_set_decrypt_key((const unsigned char*)m_szEncyptKey, 128, &m_stAESKey);
    ZeroStruct(m_szIVec);
    AES_cbc_encrypt((const unsigned char*)szInput, (unsigned char*)szOutput, ulInLen, &m_stAESKey, m_szIVec, AES_DECRYPT);
    // 获取填充的数据长度
    size_t ulPaddingLen = (size_t)szOutput[ulInLen - 1];
    // 尾部16字节全部是填充的内容
    ulPaddingLen = (0 == ulPaddingLen) ? 16 : ulPaddingLen;
    LOG_DBG_FMT(ptrNetLogger, "decrypt data succ, pad size:{}", ulPaddingLen);
    ulOutLen = ulInLen - ulPaddingLen;
    LOG_DBG_FMT(ptrNetLogger, "decrypt data succ, in:{} out:{}", ulInLen, ulOutLen);
    return 0;
}

void NetSecurityMgr::PrintKey(const unsigned char* key, size_t keyLength, const char *szInfo)
{
    std::stringstream ss;
    
    // 将key转换为十六进制字符串
    ss << std::hex << std::setfill('0');
    for (size_t i = 0; i < keyLength; ++i) {
        ss << std::setw(2) << static_cast<unsigned>(key[i]);
    }
    
    LOG_DBG_FMT(ptrNetLogger, "{} key len: {}, key:{}", szInfo, keyLength, ss.str().c_str());
}