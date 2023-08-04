/**
 * @file Socket.h
 * @author huwenhao ()
 * @brief socket操作封装
 * @date 2023-08-04
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#pragma once

#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>

#define MAXINTERFACES 32

/**
 * @brief 获取指定网卡ip
 * 
 * @param dev 
 * @return char** 
 */
char** localIP(const char* dev);

/**
 * @brief 获取端口
 * 
 * @param addr 
 * @return short 
 */
short sock_port(const struct sockaddr_in *addr);

/**
 * @brief 获取点分十进制ip
 * 
 * @param addr 
 * @return char* 
 */
char* sock_ip(const struct sockaddr_in *addr);
char* sock_iip(uint32_t iip);
char* sock_iip1(uint32_t iip);
char* sock_iip2(uint32_t iip);

/**
 * @brief 获取ip+端口
 * 
 * @param addr 
 * @return char* 
 */
char* sock_addr(const struct sockaddr_in *addr);

/**
 * @brief 设置socket缓冲区大小
 * 
 * @param fd 
 * @param size 
 * @param flag SO_RCVBUF SO_SNDBUF
 * @return int 
 */
int sock_buf(int fd, int size, int flag);

/**
 * @brief 获取host十进制ip
 * 
 * @param host 点分十进制ip 或 主机名
 * @param uip [out]
 * @return int 
 */
int sock_host2i(const char* host, uint32_t* uip);

/**
 * @brief 用host和port构造sockaddr
 * 
 * @param host 
 * @param port 
 * @param addr [out]
 * @return int 
 */
int sock_toaddr(const char* host, unsigned short port, struct sockaddr_in *addr);

/**
 * @brief 设置fd为（非）阻塞模式
 * 
 * @param fd 
 * @param isBlock true 阻塞
 * @return int 
 */
int sock_block(int fd, char isBlock);

/**
 * @brief 设置SO_REUSEADDR. 对于处于time_wait状态的tcp socket, 端口可以重复绑定使用
 * 
 * @param fd 
 * @param opt 
 * @return int 
 */
int sock_reuse(int fd, int opt);

/**
 * @brief 设置SO_REUSEPORT. 允许多线程或进程监听同一个ip:port
 * 
 * @param fd 
 * @param opt 
 * @return int 
 */
int sock_reuse_port(int fd, int opt);

/**
 * @brief 获取fd错误码
 * 
 * @param fd 
 * @return int 
 */
int sock_error(int fd);

/**
 * @brief 设置socket读缓冲区大小
 * 
 * @param fd 
 * @param size 
 * @return int 
 */
int sock_rbuf(int fd, int size);

/**
 * @brief 设置socket写缓冲区大小
 * 
 * @param fd 
 * @param size 
 * @return int 
 */
int sock_wbuf(int fd, int size);

/**
 * @brief 获取socket读缓冲区大小
 * 
 * @param fd 
 * @return int 
 */
int sock_get_rbuf(int fd);

/**
 * @brief 获取socket写缓冲区大小
 * 
 * @param fd 
 * @return int 
 */
int sock_get_wbuf(int fd);

/**
 * @brief sockaddr转id（ip地址左移）
 * 
 * @param addr 
 * @return uint64_t 
 */
uint64_t sockaddr_to_id(const struct sockaddr_in& addr);

/**
 * @brief 获取错误码对应的错误信息
 * 
 * @return char* 
 */
static inline char* GetSocketError()
{
  return strerror(errno);
}



