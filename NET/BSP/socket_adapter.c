#include "socket_adapter.h"
#include <stdint.h>
#include <string.h>

// 通用Socket初始化（创建Socket+绑定本地端口）
int8_t socket_init(socket_handle_t *handle, proto_type_e proto, uint16_t local_port)
{
    if (handle == NULL || (proto != PROTO_TCP && proto != PROTO_UDP)) {
        return -1;  // 参数无效
    }

    // 1. 初始化句柄默认值
    memset(handle, 0, sizeof(socket_handle_t));
    handle->proto = proto;
    handle->stat = SOCK_STAT_CLOSED;
    handle->local_addr.sin_family = AF_INET;
    handle->local_addr.sin_addr.s_addr = htonl(INADDR_ANY);  // 绑定所有本地网卡
    handle->local_addr.sin_port = htons(local_port);  // 本地端口（网络字节序）
    handle->recv_timeout = 0;  // 默认接收超时1s
    handle->send_timeout = 1000;  // 默认发送超时1s

    // 2. 创建Socket（SOCK_STREAM=TCP，SOCK_DGRAM=UDP，IPPROTO_IP=通用IP协议）
    handle->sock_fd = socket(AF_INET, 
                             (proto == PROTO_TCP) ? SOCK_STREAM : SOCK_DGRAM, 
                             IPPROTO_IP);
    if (handle->sock_fd < 0) {
        return -2;  // Socket创建失败
    }

    // 3. 绑定本地端口（TCP/UDP均需绑定，避免端口随机分配）
    if (bind(handle->sock_fd, (struct sockaddr *)&handle->local_addr, sizeof(struct sockaddr_in)) < 0) {
        socket_close(handle);
        handle->sock_fd = -1;
        return -3;  // 端口绑定失败
    }

    handle->stat = (proto == PROTO_TCP) ? SOCK_STAT_CLOSED : SOCK_STAT_CONNECTED;  // UDP创建即就绪
    return 0;  // 初始化成功
}

// 设置Socket超时（接收/发送，基于FreeRTOS lwIP的SO_RCVTIMEO/SO_SNDTIMEO）
void socket_set_timeout(socket_handle_t *handle, uint16_t recv_timeout, uint16_t send_timeout)
{
    if (handle == NULL || handle->sock_fd < 0) {
        return;
    }
    handle->recv_timeout = recv_timeout;
    handle->send_timeout = send_timeout;
    int optval = 1;
    // 设置接收超时
    setsockopt(handle->sock_fd, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout, sizeof(recv_timeout));
    // 设置发送超时
    setsockopt(handle->sock_fd, SOL_SOCKET, SO_SNDTIMEO, &send_timeout, sizeof(send_timeout));

    setsockopt(handle->sock_fd, IPPROTO_TCP, TCP_NODELAY, (void *)&optval, sizeof(optval));
}

// Socket接收数据（TCP：阻塞/超时接收；UDP：接收指定端口的广播/单播数据）
int8_t socket_recv(socket_handle_t *handle, uint8_t *buf, uint32_t buf_len, struct sockaddr_in *peer_addr)
{
    if (handle == NULL || buf == NULL || buf_len == 0 || handle->sock_fd < 0) {
        return -1;  // 参数无效
    }

    socklen_t peer_addr_len = sizeof(struct sockaddr_in);
    int32_t recv_len = 0;

    if (handle->proto == PROTO_TCP) {
        // TCP接收（无需关注对端地址，已连接时固定）
        recv_len = recv(handle->sock_fd, buf, buf_len, 0);
        if (recv_len <= 0) {
            handle->stat = SOCK_STAT_CLOSED;  // TCP接收失败视为连接断开
            return -2;  // 接收失败（超时/连接断开）
        }
    } else {
        // UDP接收（需获取对端地址，用于后续回复）
        recv_len = recvfrom(handle->sock_fd, buf, buf_len, 0, 
                           (struct sockaddr *)peer_addr, &peer_addr_len);
        if (recv_len <= 0) {
            return -3;  // UDP接收失败（超时）
        }
    }
    return recv_len;  // 返回实际接收字节数
}

// Socket发送数据（TCP：已连接直接发；UDP：需指定对端IP+端口）
int8_t socket_send(socket_handle_t *handle, const uint8_t *buf, uint32_t buf_len, struct sockaddr_in *peer_addr)
{
    if (handle == NULL || buf == NULL || buf_len == 0 || handle->sock_fd < 0) {
        return -1;
    }

    int32_t send_len = 0;
    socklen_t peer_addr_len = sizeof(struct sockaddr_in);

    if (handle->proto == PROTO_TCP) {
        // TCP发送（已连接，peer_addr可选，传NULL则用已绑定的对端）
        if (handle->stat != SOCK_STAT_CONNECTED) {
            return -2;  // TCP未连接，发送失败
        }
        send_len = send(handle->sock_fd, buf, buf_len, 0);
    } else {
        // UDP发送（必须指定对端地址，否则无法定位目标）
        if (peer_addr == NULL) {
            return -3;  // UDP未指定对端地址，发送失败
        }
        send_len = sendto(handle->sock_fd, buf, buf_len, 0, 
                         (struct sockaddr *)peer_addr, peer_addr_len);
    }

    return (send_len > 0) ? send_len : -4;  // 返回实际发送字节数，失败返回-4
}

// TCP Server监听（启动端口监听，等待Client连接）
int8_t socket_tcp_listen(socket_handle_t *handle, uint8_t backlog)
{
    if (handle == NULL || handle->proto != PROTO_TCP || handle->sock_fd < 0) {
        return -1;
    }

    // 启动监听（backlog：最大等待连接队列长度，建议1-5）
    if (listen(handle->sock_fd, backlog) < 0) {
        return -2;  // 监听启动失败
    }
    handle->stat = SOCK_STAT_LISTENING;
    return 0;  // 监听成功
}

// TCP Server接收连接（阻塞/超时等待Client连接，返回新连接句柄）
int8_t socket_tcp_accept(socket_handle_t *listen_handle, socket_handle_t *conn_handle)
{
    if (listen_handle == NULL || conn_handle == NULL || listen_handle->stat != SOCK_STAT_LISTENING) {
        return -1;
    }

    socklen_t peer_addr_len = sizeof(struct sockaddr_in);
    memset(conn_handle, 0, sizeof(socket_handle_t));

    // 等待Client连接（返回新的Socket FD，与原监听FD分离）
    conn_handle->sock_fd = accept(listen_handle->sock_fd, 
                                 (struct sockaddr *)&conn_handle->peer_addr, 
                                 &peer_addr_len);
    if (conn_handle->sock_fd < 0) {
        return -2;  // 接收连接失败（超时/异常）
    }

    // 初始化新连接句柄参数
    conn_handle->proto = PROTO_TCP;
    conn_handle->stat = SOCK_STAT_CONNECTED;
    conn_handle->local_addr = listen_handle->local_addr;  // 继承监听端本地地址
    conn_handle->recv_timeout = listen_handle->recv_timeout;
    conn_handle->send_timeout = listen_handle->send_timeout;
    // 同步超时设置到新Socket
    socket_set_timeout(conn_handle, conn_handle->recv_timeout, conn_handle->send_timeout);

    return 0;  // 连接接收成功
}

// TCP Client连接（主动连接指定TCP Server）
int8_t socket_tcp_connect(socket_handle_t *handle, const char *peer_ip, uint16_t peer_port)
{
    if (handle == NULL || handle->proto != PROTO_TCP || handle->sock_fd < 0 || peer_ip == NULL) {
        return -1;
    }

    // 初始化对端地址（IP+端口）
    handle->peer_addr.sin_family = AF_INET;
    handle->peer_addr.sin_addr.s_addr = inet_addr(peer_ip);  // 转换IP为网络字节序
    handle->peer_addr.sin_port = htons(peer_port);          // 转换端口为网络字节序

    handle->stat = SOCK_STAT_CONNECTING;
    // 主动发起连接
    if (connect(handle->sock_fd, (struct sockaddr *)&handle->peer_addr, sizeof(struct sockaddr_in)) < 0) {
        handle->stat = SOCK_STAT_CLOSED;
        return -2;  // 连接失败（超时/Server未监听）
    }

    handle->stat = SOCK_STAT_CONNECTED;
    return 0;  // 连接成功
}

// 关闭Socket并释放资源
void socket_close(socket_handle_t *handle)
{
    if (handle == NULL) {
        return;
    }

    if (handle->sock_fd >= 0) {
        closesocket(handle->sock_fd);
        handle->sock_fd = -1;
    }
    
    handle->stat = SOCK_STAT_CLOSED;
    memset(&handle->local_addr, 0, sizeof(struct sockaddr_in));
    memset(&handle->peer_addr, 0, sizeof(struct sockaddr_in));
}
