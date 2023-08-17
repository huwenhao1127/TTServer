#include "NetMsg.h"
#include "ikcp.h"

/** 编解码函数定义（搬运kcp） BEGIN **/
#ifndef IWORDS_BIG_ENDIAN
    #ifdef _BIG_ENDIAN_
        #if _BIG_ENDIAN_
            #define IWORDS_BIG_ENDIAN 1
        #endif
    #endif
    #ifndef IWORDS_BIG_ENDIAN
        #if defined(__hppa__) || \
            defined(__m68k__) || defined(mc68000) || defined(_M_M68K) || \
            (defined(__MIPS__) && defined(__MIPSEB__)) || \
            defined(__ppc__) || defined(__POWERPC__) || defined(_M_PPC) || \
            defined(__sparc__) || defined(__powerpc__) || \
            defined(__mc68000__) || defined(__s390x__) || defined(__s390__)
            #define IWORDS_BIG_ENDIAN 1
        #endif
    #endif
    #ifndef IWORDS_BIG_ENDIAN
        #define IWORDS_BIG_ENDIAN  0
    #endif
#endif

#ifndef IWORDS_MUST_ALIGN
	#if defined(__i386__) || defined(__i386) || defined(_i386_)
		#define IWORDS_MUST_ALIGN 0
	#elif defined(_M_IX86) || defined(_X86_) || defined(__x86_64__)
		#define IWORDS_MUST_ALIGN 0
	#elif defined(__amd64) || defined(__amd64__)
		#define IWORDS_MUST_ALIGN 0
	#else
		#define IWORDS_MUST_ALIGN 1
	#endif
#endif

/* encode 8 bits unsigned int */
static inline char *Encode8u(char *p, unsigned char c)
{
	*(unsigned char*)p++ = c;
	return p;
}

/* decode 8 bits unsigned int */
static inline const char *Decode8u(const char *p, unsigned char *c)
{
	*c = *(unsigned char*)p++;
	return p;
}

/* encode 16 bits unsigned int (lsb) */
static inline char *Encode16u(char *p, unsigned short w)
{
#if IWORDS_BIG_ENDIAN || IWORDS_MUST_ALIGN
	*(unsigned char*)(p + 0) = (w & 255);
	*(unsigned char*)(p + 1) = (w >> 8);
#else
	memcpy(p, &w, 2);
#endif
	p += 2;
	return p;
}

/* decode 16 bits unsigned int (lsb) */
static inline const char *Decode16u(const char *p, unsigned short *w)
{
#if IWORDS_BIG_ENDIAN || IWORDS_MUST_ALIGN
	*w = *(const unsigned char*)(p + 1);
	*w = *(const unsigned char*)(p + 0) + (*w << 8);
#else
	memcpy(w, p, 2);
#endif
	p += 2;
	return p;
}

/* encode 32 bits unsigned int (lsb) */
static inline char *Encode32u(char *p, uint32_t l)
{
#if IWORDS_BIG_ENDIAN || IWORDS_MUST_ALIGN
	*(unsigned char*)(p + 0) = (unsigned char)((l >>  0) & 0xff);
	*(unsigned char*)(p + 1) = (unsigned char)((l >>  8) & 0xff);
	*(unsigned char*)(p + 2) = (unsigned char)((l >> 16) & 0xff);
	*(unsigned char*)(p + 3) = (unsigned char)((l >> 24) & 0xff);
#else
	memcpy(p, &l, 4);
#endif
	p += 4;
	return p;
}

/* decode 32 bits unsigned int (lsb) */
static inline const char *Decode32u(const char *p, uint32_t *l)
{
#if IWORDS_BIG_ENDIAN || IWORDS_MUST_ALIGN
	*l = *(const unsigned char*)(p + 3);
	*l = *(const unsigned char*)(p + 2) + (*l << 8);
	*l = *(const unsigned char*)(p + 1) + (*l << 8);
	*l = *(const unsigned char*)(p + 0) + (*l << 8);
#else 
	memcpy(l, p, 4);
#endif
	p += 4;
	return p;
}

/* encode 64 bits unsigned int (lsb) */
static inline char *Encode64u(char *p, uint64_t l)
{
#if IWORDS_BIG_ENDIAN || IWORDS_MUST_ALIGN
	*(unsigned char*)(p + 0) = (unsigned char)((l >>  0) & 0xff);
	*(unsigned char*)(p + 1) = (unsigned char)((l >>  8) & 0xff);
	*(unsigned char*)(p + 2) = (unsigned char)((l >> 16) & 0xff);
	*(unsigned char*)(p + 3) = (unsigned char)((l >> 24) & 0xff);
    *(unsigned char*)(p + 4) = (unsigned char)((l >> 32) & 0xff);
	*(unsigned char*)(p + 5) = (unsigned char)((l >> 40) & 0xff);
	*(unsigned char*)(p + 6) = (unsigned char)((l >> 48) & 0xff);
	*(unsigned char*)(p + 7) = (unsigned char)((l >> 56) & 0xff);
#else
	memcpy(p, &l, 8);
#endif
	p += 8;
	return p;
}

/* decode 64 bits unsigned int (lsb) */
static inline const char *Decode64u(const char *p, uint64_t *l)
{
#if IWORDS_BIG_ENDIAN || IWORDS_MUST_ALIGN
	*l = *(const unsigned char*)(p + 3);
	*l = *(const unsigned char*)(p + 2) + (*l << 8);
	*l = *(const unsigned char*)(p + 1) + (*l << 8);
	*l = *(const unsigned char*)(p + 0) + (*l << 8);
#else 
	memcpy(l, p, 8);
#endif
	p += 8;
	return p;
}
/** 编解码函数定义 END **/

int NetMsg::Init()
{
    ZeroStruct(m_stHead);
    ZeroStruct(m_stBody);
    m_ullNetSize = 0;
    return 0;
}

int NetMsg::DecodeMsg(const char *szNetBuff, uint64_t ullLen)
{
    CHECK_IF_PARAM_NULL(ptrNetLogger, szNetBuff, -1);
    m_ullNetSize = ullLen;

    NetReader oReader(szNetBuff, ullLen);
    // 解消息头
    if (0 != oReader.Read(m_stHead))
    {
        LOG_ERR_FMT(ptrNetLogger, " msg head read error, len:{}", ullLen);
        return -1;
    }

    // 解消息体
    switch (m_stHead.bType)
    {
        case NET_PACKET_HANDSHAKE1:
        case NET_PACKET_HANDSHAKE2:
        {
            // 握手包
            if (0 != oReader.Read(m_stBody.stHandShake))
            {
                LOG_ERR_FMT(ptrNetLogger, " handshake read error, len:{}", ullLen);
                return -1;
            }
            break;
        }
        case NET_PACKET_DATA:
        {
            // 数据包
            m_stBody.stData.szBuff = oReader.GetBuff();
            m_stBody.stData.ullDataLen = oReader.GetRemain();
            break;
        }
        case NET_PACKET_HEARTBEAT:
        {
            // 心跳包
            if (0 != oReader.Read(m_stBody.stHearBeat))
            {
                LOG_ERR_FMT(ptrNetLogger, " hearbeat read error, len:{}", ullLen);
                return -1;
            }
            break;
        }
        case NET_PACKET_RECONNECT:
        {
            // 重连包
            if (0 != oReader.Read(m_stBody.stReconnect))
            {
                LOG_ERR_FMT(ptrNetLogger, " reconnect read error, len:{}", ullLen);
                return -1;
            }
            break;
        }
        case NET_PACKET_CLOSE:
        case NET_PACKET_FIN:
        case NET_PACKET_FIN_ACK:
        {
            // Fin包
            if (0 != oReader.Read(m_stBody.stFin))
            {
                LOG_ERR_FMT(ptrNetLogger, " fin read error, len:{}", ullLen);
                return -1;
            }
            break;
        }
        case NET_PACKET_RST:
        {
            // Rst包
            if (0 != oReader.Read(m_stBody.stRst))
            {
                LOG_ERR_FMT(ptrNetLogger, " rst read error, len:{}", ullLen);
                return -1;
            }
            break;
        }
        default:
        {
            LOG_ERR_FMT(ptrNetLogger, " net msg type[{}] error", (int)m_stHead.bType);
            return -1;
        }
    }
    return 0;
}

int NetMsg::EncodeMsg(char *szOutPutBuff, uint64_t& ullLen)
{
    
    return 0;
}

int NetReader::PeekUnreliable(const char **szOutBuff)
{
    if (m_ullSize < (uint64_t)sizeof(uint16_t))
    {
        return -1;
    }

    uint16_t usDataLen = 0;
    Decode16u(m_ptrBuff, &usDataLen);

    if (m_ullSize < (uint64_t)(usDataLen + sizeof(uint16_t)))
    {
        return -1;
    }

    *szOutBuff = m_ptrBuff + sizeof(uint16_t);
    m_ptrBuff = m_ptrBuff + (sizeof(uint16_t) + usDataLen);
    m_ullSize -= (sizeof(uint16_t) + usDataLen);
    return usDataLen;
}

int NetReader::Read(STNetMsgHead& stHead)
{
    uint64_t ullHeadSize = sizeof(STNetMsgHead);
    if (m_ullSize < ullHeadSize)
    {
        return -1;
    }
    memcpy(&stHead, m_ptrBuff, ullHeadSize);
    m_ptrBuff += ullHeadSize;
    m_ullSize -= ullHeadSize;
    return 0;
}

int NetReader::ReadDataHead(DataHead& stHead)
{
    stHead.bIsEncrypt = (*(uint8_t*)(m_ptrBuff)) & 0x01;
    m_ptrBuff += sizeof(stHead);
    m_ullSize -= sizeof(stHead);
    return 0;
}

int NetReader::Read(STHandShakePacket& stPacket)
{
    uint64_t ullPacketSize = sizeof(stPacket);
    if (m_ullSize < ullPacketSize)
    {
        return -1;
    }

    m_ptrBuff = Decode32u(m_ptrBuff, &(stPacket.dwTimeStamp));
    m_ptrBuff = Decode8u(m_ptrBuff, &(stPacket.bScreteKey));
    m_ptrBuff = Decode32u(m_ptrBuff, &(stPacket.dwConnId));
    m_ptrBuff = Decode8u(m_ptrBuff, &(stPacket.bIsKey));
    m_ptrBuff = Decode8u(m_ptrBuff, &(stPacket.bEncryptDataLen));
    m_ptrBuff = Decode64u(m_ptrBuff, &(stPacket.ullExtData));

    memcpy(&stPacket.szEncryptData, m_ptrBuff, MAX_ENCRYPT_DATA_LEN);
    m_ptrBuff += MAX_ENCRYPT_DATA_LEN;
    memcpy(&stPacket.szCookies, m_ptrBuff, MAX_HANDSHAKE_COOKIES_SIZE);
    m_ptrBuff += MAX_HANDSHAKE_COOKIES_SIZE;

    m_ullSize -= ullPacketSize;
    return 0;
}

int NetReader::Read(STHeartBeatPacket& stPacket)
{
    uint64_t ullPacketSize = sizeof(stPacket);
    if (m_ullSize < ullPacketSize)
    {
        return -1;
    }

    m_ptrBuff = Decode32u(m_ptrBuff, &(stPacket.dwConnId));
    m_ptrBuff = Decode64u(m_ptrBuff, &(stPacket.ullClientTimeMs));
    m_ptrBuff = Decode64u(m_ptrBuff, &(stPacket.ullServerTimeMs));
    m_ullSize -= ullPacketSize;
    return 0;
}

int NetReader::Read(STReconnectPacket& stPacket)
{
    uint64_t ullPacketSize = sizeof(stPacket);
    if (m_ullSize < ullPacketSize)
    {
        return -1;
    }

    m_ptrBuff = Decode32u(m_ptrBuff, &(stPacket.dwConnId));
    memcpy(&stPacket.szCookies, m_ptrBuff, MAX_HANDSHAKE_COOKIES_SIZE);
    m_ptrBuff += MAX_HANDSHAKE_COOKIES_SIZE;
    m_ullSize -= ullPacketSize;
    return 0;
}

int NetReader::Read(STRstPacket& stPacket)
{
    uint64_t ullPacketSize = sizeof(stPacket);
    if (m_ullSize < ullPacketSize)
    {
        return -1;
    }

    m_ptrBuff = Decode32u(m_ptrBuff, &(stPacket.dwConnId));
    m_ptrBuff = Decode8u(m_ptrBuff, &(stPacket.bRstType));
    m_ullSize -= ullPacketSize;
    return 0;
}

int NetReader::Read(STFinPacket& stPacket)
{
    uint64_t ullPacketSize = sizeof(stPacket);
    if (m_ullSize < ullPacketSize)
    {
        return -1;
    }

    m_ptrBuff = Decode32u(m_ptrBuff, &(stPacket.dwConnId));
    m_ptrBuff = Decode8u(m_ptrBuff, &(stPacket.bReason));
    m_ullSize -= ullPacketSize;
    return 0;
}

int NetWriter::Write(STNetMsgHead& stHead)
{
    size_t ulSize = sizeof(STNetMsgHead);
    if (!CheckBuff(ulSize))
    {
        return -1;
    }

    memcpy(m_ptrBuff + m_ulSize, &stHead, ulSize);
    m_ulSize += ulSize;
    return 0;
}

int NetWriter::Write(STHandShakePacket& stPacket)
{
    size_t ulSize = sizeof(STHandShakePacket);
    if (!CheckBuff(ulSize))
    {
        return -1;
    }

    char *ptr = m_ptrBuff + m_ulSize;
    ptr = Encode32u(ptr, stPacket.dwTimeStamp);
    ptr = Encode8u(ptr, stPacket.bScreteKey);
    ptr = Encode32u(ptr, stPacket.dwConnId);
    ptr = Encode8u(ptr, stPacket.bIsKey);
    ptr = Encode8u(ptr, stPacket.bEncryptDataLen);
    ptr = Encode64u(ptr, stPacket.ullExtData);

    memcpy(ptr, stPacket.szEncryptData, MAX_ENCRYPT_DATA_LEN);
    ptr += MAX_ENCRYPT_DATA_LEN;
    memcpy(ptr, stPacket.szCookies, MAX_HANDSHAKE_COOKIES_SIZE);
    ptr += MAX_HANDSHAKE_COOKIES_SIZE;

    m_ulSize += ulSize;
    return 0;
}

int NetWriter::Write(STHeartBeatPacket& stPacket)
{
    size_t ulSize = sizeof(STHeartBeatPacket);
    if (!CheckBuff(ulSize))
    {
        return -1;
    }

    char *ptr = m_ptrBuff + m_ulSize;
    ptr = Encode32u(ptr, stPacket.dwConnId);
    ptr = Encode64u(ptr, stPacket.ullClientTimeMs);
    ptr = Encode64u(ptr, stPacket.ullServerTimeMs);
    m_ulSize += ulSize;
    return 0;
}

int NetWriter::Write(STReconnectPacket& stPacket)
{
    size_t ulSize = sizeof(STReconnectPacket);
    if (!CheckBuff(ulSize))
    {
        return -1;
    }

    char *ptr = m_ptrBuff + m_ulSize;
    ptr = Encode32u(ptr, stPacket.dwConnId);
    memcpy(ptr, &stPacket.szCookies, MAX_HANDSHAKE_COOKIES_SIZE);
    m_ulSize += ulSize;
    return 0;
}

int NetWriter::Write(STRstPacket& stPacket)
{
    size_t ulSize = sizeof(STRstPacket);
    if (!CheckBuff(ulSize))
    {
        return -1;
    }

    char *ptr = m_ptrBuff + m_ulSize;
    ptr = Encode32u(ptr, stPacket.dwConnId);
    ptr = Encode8u(ptr, stPacket.bRstType);
    m_ulSize += ulSize;
    return 0;
}

int NetWriter::Write(STFinPacket& stPacket)
{
    size_t ulSize = sizeof(STFinPacket);
    if (!CheckBuff(ulSize))
    {
        return -1;
    }

    char *ptr = m_ptrBuff + m_ulSize;
    ptr = Encode32u(ptr, stPacket.dwConnId);
    ptr = Encode8u(ptr, stPacket.bReason);
    m_ulSize += ulSize;
    return 0;
}

int NetWriter::WriteDataHead(DataHead& stHead)
{
    size_t ulSize = sizeof(uint8_t);
    if (!CheckBuff(ulSize))
    {
        return -1;
    }

    char *ptr = m_ptrBuff + m_ulSize;
    (*(uint8_t*)ptr) = 0;
    (*(uint8_t*)ptr) = (*(uint8_t*)ptr) | stHead.bIsEncrypt;
    m_ulSize += ulSize;
    return 0;
}

int NetWriter::WriteUnreliable(const char *pData, size_t ulLen)
{
    size_t ulSize = sizeof(uint16_t) + ulLen;
    if (!CheckBuff(ulSize))
    {
        return -1;
    }

    // 将数据长度加在数据前
    char *ptr = m_ptrBuff + m_ulSize;
    ptr = Encode16u(ptr, (unsigned short)ulLen);
    memcpy(ptr, pData, ulLen);
    m_ulSize += ulSize;
    return 0;
}

int NetWriter::WriteRawData(const char *pData, size_t ulLen)
{
    if (!CheckBuff(ulLen))
    {
        return -1;
    }
    
    char *ptr = m_ptrBuff + m_ulSize;
    memcpy(ptr, pData, ulLen);
    m_ulSize += ulLen;
    return 0;
}