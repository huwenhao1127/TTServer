#include <arpa/inet.h>
#include <net/if.h>
#include <limits.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/types.h>
#include <fcntl.h>
#include "NetCommdef.h"
#include "Socket.h"

char** localIP(const char* dev)
{
    int fd;
    int k = 0,n = 0;
    int total = 0;

    char **ipa = nullptr;
    struct ifreq buf[MAXINTERFACES];
    struct ifconf ifc;
    char tmp_dev_name[PATH_MAX];
    char* dev_delimiter_pos;

    struct sockaddr_in* sin;
    if ((fd = socket(AF_INET,SOCK_DGRAM,0)) < 0)
    {
        return NULL;
    }
    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = (caddr_t) buf;
    if (!ioctl(fd, SIOCGIFCONF, (char*)&ifc))
    {
        n = total = ifc.ifc_len/sizeof(struct ifreq);
        if (total == 0)
        {
            close(fd);
            return NULL;
        }
        ipa = (char **)malloc(sizeof(char*)*(n+1));
        memset(ipa,0,sizeof(char*)*(n+1));

        while(n-- > 0)
        {
            if (!ioctl(fd,SIOCGIFADDR,(char*)&buf[n]))
            {

                strncpy(tmp_dev_name, buf[n].ifr_name, sizeof(tmp_dev_name));
                dev_delimiter_pos = strchr(tmp_dev_name, '.');
                if (dev_delimiter_pos)
                {
                    *dev_delimiter_pos = '\0';
                }

                if (strcmp(tmp_dev_name, dev) == 0)
                {
                    sin = (struct sockaddr_in*)&buf[n].ifr_addr;
                    ipa[k++] = strdup(inet_ntoa(sin->sin_addr));
                }
            }
        }
    }
    close(fd);
    return ipa;
}

short sock_port(const struct sockaddr_in *addr)
{
    return ntohs(addr->sin_port);
}

char* sock_ip(const struct sockaddr_in *addr)
{
    static char s_ip[64];
    snprintf(s_ip, 64, "%s", inet_ntoa(addr->sin_addr));
    return s_ip;
}

char* sock_iip(uint32_t iip)
{
    static char s_ip[64];
    struct sockaddr_in addr;
    addr.sin_addr.s_addr = iip;
    snprintf(s_ip, 64, "%s", inet_ntoa(addr.sin_addr));
    return s_ip;
}

char* sock_iip1(uint32_t iip)
{
    static char s_ip_1[64];
    struct sockaddr_in addr;
    addr.sin_addr.s_addr = iip;
    snprintf(s_ip_1, 64, "%s", inet_ntoa(addr.sin_addr));
    return s_ip_1;
}

char* sock_iip2(uint32_t iip)
{
    static char s_ip_2[64];
    struct sockaddr_in addr;
    addr.sin_addr.s_addr = iip;
    snprintf(s_ip_2, 64, "%s", inet_ntoa(addr.sin_addr));
    return s_ip_2;
}

char* sock_addr(const struct sockaddr_in *addr)
{
    static char s_ip[64];
    snprintf(s_ip, 64, "%s:%d", inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
    return s_ip;
}

int sock_buf(int fd, int size, int flag)
{
    if (setsockopt(fd, SOL_SOCKET, flag, (char*)&size, sizeof(int)) == -1)
    {
        LOG_ERR_FMT(ptrNetLogger, "fd:{} Set Buf size:{} Fail!", fd, size);
        return -1;
    }

    size_t nsize = 0;
    socklen_t n = sizeof(nsize);
    // 这里设置buf大小，如果设置成功，则为size*2
    // 如果不是，则可能是size小于或者大于系统默认的最小最大值, 则任务设置失败
    // cat /proc/sys/net/core/wmem_max  cat /proc/sys/net/core/rmem_max 查看最大最小值
    if (getsockopt(fd, SOL_SOCKET, flag, (char*)&nsize, &n) == 0)
    {
        if (2 * size != nsize)
        {
            LOG_ERR_FMT(ptrNetLogger, "fd:{} Set Buf size:{} nsize:{} Fail! Please use server/run/tool/fix_socket_buf.sh to Change Max Socket Buf", fd, size, (int)nsize);
            return -1;
        }
    }

    LOG_ERR_FMT(ptrNetLogger, "fd:{} Set Buf size:{} nsize:{} Succ!", fd, size, (int)nsize);
    return 0;
}

int sock_host2i(const char* host, uint32_t* uip)
{
    //获得主机信息入口
    if ((*uip = inet_addr(host)) != INADDR_NONE)
    {
        return 0;
    }
    struct hostent *host_ent;
    if ((host_ent = gethostbyname(host)) != NULL)
    {
        memcpy(uip, host_ent->h_addr, host_ent->h_length);
        return 0;
    }
    //若地址格式为xxx.xxx.xxx.xxx
    else
    {
        LOG_ERR_FMT(ptrNetLogger, "[Socket] net address=%{} fail", host);
        return -1;
    }
    return -1;
}

int sock_toaddr(const char* host, unsigned short port, struct sockaddr_in *addr)
{
    if (nullptr == host)
    {
        LOG_ERR_FMT(ptrNetLogger, "[Socket] host is null");
        return -1;
    }

    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);

    struct hostent *host_ent;
    memset(host_ent, 0, sizeof(hostent));
    if ((host_ent = gethostbyname(host)) != NULL)
    {
        memcpy(&addr->sin_addr, host_ent->h_addr, host_ent->h_length);
        return 0;
    }
    else
    {
        // host是点分十进制
        if ((addr->sin_addr.s_addr = inet_addr(host)) == INADDR_NONE)
        {
            LOG_ERR_FMT(ptrNetLogger, "[Socket] net address=%{} fail", host);
            return -1;
        }
    }
    return 0;
}

int sock_block(int fd, char isBlock)
{
    int ret = 0;
    if (isBlock)
    {
        ret = fcntl(fd, F_SETFL, O_SYNC);
    }
    else
    {
        ret = fcntl(fd, F_SETFL, O_NONBLOCK);
    }
    if (0 != ret)
    {
        LOG_ERR_FMT(ptrNetLogger, "[Socket] fcntl fd[{}] fail: {}", fd, GetSocketError());
    }
    return ret;
}

int sock_reuse(int fd, int opt)
{
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) < 0)
    {
        LOG_ERR_FMT(ptrNetLogger, "[Socket] set sock opt fail: {}", fd, GetSocketError());
        return -1;
    }
    return 0;
}

int sock_reuse_port(int fd, int opt)
{
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (const char*)&opt, sizeof(opt)) < 0)
    {
        LOG_ERR_FMT(ptrNetLogger, "[Socket] set sock opt fail: {}", fd, GetSocketError());
        return -1;
    }
    return 0;
}

int sock_error(int fd)
{
    int error;
    socklen_t len = sizeof(int);
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
    {
        LOG_ERR_FMT(ptrNetLogger, "[Socket] get sock opt fail: {}", fd, GetSocketError());
        return -1;
    }
    return error;
}

int sock_rbuf(int fd, int size)
{
    if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char*)&size, sizeof(int)) == -1)
    {
        LOG_ERR_FMT(ptrNetLogger, "fd:{} Set read Buf size:{} Fail, {}", fd, size, GetSocketError());
        return -1;
    }
    size_t nsize = 0;
    socklen_t n = sizeof(nsize);
    if (getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char*)&nsize, &n) == 0)
    {
        return 0;
    }
    else
    {
        LOG_ERR_FMT(ptrNetLogger, "fd:{} Set read Buf size:{} Fail, cur size:{}, {}", fd, size, nsize, GetSocketError());
        return -1;
    }
    return 0;
}

int sock_wbuf(int fd, int size)
{
    if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char*)&size, sizeof(int)) == -1)
    {
        LOG_ERR_FMT(ptrNetLogger, "fd:{} Set write Buf size:{} Fail, {}", fd, size, GetSocketError());
        return -1;
    }
    size_t nsize = 0;
    socklen_t n = sizeof(nsize);
    if (getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char*)&nsize, &n) == 0)
    {
        return 0;
    }
    else
    {
        LOG_ERR_FMT(ptrNetLogger, "fd:{} Set wirte Buf size:{} Fail, cur size:{}, {}", fd, size, nsize, GetSocketError());
        return -1;
    }
    return 0;
}

int sock_get_rbuf(int fd)
{
    size_t nsize = 0;
    socklen_t n = sizeof(nsize);
    if (getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char*)&nsize, &n) == 0)
    {
        return nsize;
    }
    LOG_ERR_FMT(ptrNetLogger, "fd:{} get read Buf size Fail, {}", fd, nsize, GetSocketError());
    return -1;
}

int sock_get_wbuf(int fd)
{
    size_t nsize = 0;
    socklen_t n = sizeof(nsize);
    if (getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char*)&nsize, &n) == 0)
    {
        return nsize;
    }
    LOG_ERR_FMT(ptrNetLogger, "fd:{} get wirte Buf size Fail, {}", fd, nsize, GetSocketError());
    return -1;
}

uint64_t sockaddr_to_id(const struct sockaddr_in& addr)
{
    uint64_t ullId = addr.sin_port + (((uint64_t)addr.sin_addr.s_addr) << 16);
    return ullId;
}