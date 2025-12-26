#ifndef __SOCKET_ADAPTER_H
#define __SOCKET_ADAPTER_H

#include "FreeRTOS.h"
#include "task.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"

// 通信协议类型定义
typedef enum {
    PROTO_TCP = 0,  // TCP协议
    PROTO_UDP       // UDP协议
} proto_type_e;

// Socket通信状态定义
typedef enum {
    SOCK_STAT_CLOSED = 0,  // 未连接/已关闭
    SOCK_STAT_CONNECTING,  // 连接中（仅TCP）
    SOCK_STAT_CONNECTED,   // 已连接（仅TCP）
    SOCK_STAT_LISTENING    // 监听中（仅TCP Server）
} sock_stat_e;

// Socket通用句柄结构体（适配TCP/UDP、Client/Server）
typedef struct {
    int32_t sock_fd;        // Socket文件描述符
    proto_type_e proto;     // 协议类型（TCP/UDP）
    sock_stat_e stat;       // 当前通信状态
    struct sockaddr_in local_addr;  // 本地IP+端口
    struct sockaddr_in peer_addr;   // 对端IP+端口
    uint16_t recv_timeout;  // 接收超时（ms，0表示阻塞）
    uint16_t send_timeout;  // 发送超时（ms，0表示阻塞）
} socket_handle_t;

// 通用Socket初始化（创建Socket并绑定本地端口）
int8_t socket_init(socket_handle_t *handle, proto_type_e proto, uint16_t local_port);

// Socket接收数据（适配TCP/UDP，自动填充对端地址）
int8_t socket_recv(socket_handle_t *handle, uint8_t *buf, uint32_t buf_len, struct sockaddr_in *peer_addr);

// Socket发送数据（适配TCP/UDP，需指定对端地址（UDP必传，TCP可选））
int8_t socket_send(socket_handle_t *handle, const uint8_t *buf, uint32_t buf_len, struct sockaddr_in *peer_addr);

// TCP Server监听（仅TCP Server调用）
int8_t socket_tcp_listen(socket_handle_t *handle, uint8_t backlog);

// TCP Server接收连接（仅TCP Server调用，返回新连接句柄）
int8_t socket_tcp_accept(socket_handle_t *listen_handle, socket_handle_t *conn_handle);

// TCP Client连接（仅TCP Client调用）
int8_t socket_tcp_connect(socket_handle_t *handle, const char *peer_ip, uint16_t peer_port);

// 设置Socket超时（接收/发送）
void socket_set_timeout(socket_handle_t *handle, uint16_t recv_timeout, uint16_t send_timeout);

// 关闭Socket并释放资源
void socket_close(socket_handle_t *handle);

#endif // SOCKET_OS_ADAPT_H


