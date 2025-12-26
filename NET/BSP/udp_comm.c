#include "udp_comm.h"
#include "lwip/ip_addr.h"
#include "lwip/sys.h"
#include <string.h>

// UDP服务器句柄（全局，供发送函数使用）
static struct netconn* udp_server_conn = NULL;
// UDP客户端句柄（全局，供发送函数使用）
static struct netconn* udp_client_conn = NULL;

// UDP服务器任务
static void udp_server_task(void* arg);
// UDP客户端任务
static void udp_client_task(void* arg);

err_t udp_server_init(uint8_t prio)
{
    os_task_handle_t task_handle = os_task_create(udp_server_task, "udp_server", 
                                                 OS_TASK_STACK_MID, NULL, prio);
    return (task_handle != NULL) ? err_ok : err_mem;
}

static void udp_server_task(void* arg)
{
    struct netbuf* recv_buf = NULL;
    ip_addr_t client_ip;
    uint16_t client_port;
    err_t err;
    uint8_t recv_data[UDP_SERVER_RECV_BUF_SIZE] = {0};

    // 1. 创建UDP连接
    udp_server_conn = netconn_new(NETCONN_UDP);
    if (udp_server_conn == NULL) {
        goto task_exit;
    }

    // 2. 绑定端口
    err = netconn_bind(udp_server_conn, IP_ADDR_ANY, UDP_SERVER_PORT);
    if (err != ERR_OK) {
        netconn_delete(udp_server_conn);
        udp_server_conn = NULL;
        goto task_exit;
    }

    printf("UDP server started, bound to port %d...\n", UDP_SERVER_PORT);

    while (1) {
        // 3. 接收数据（阻塞）
        err = netconn_recvfrom(udp_server_conn, &recv_buf, &client_ip, &client_port);
        if (err != ERR_OK || recv_buf == NULL) {
            continue;
        }

        // 4. 处理接收数据
        uint32_t recv_len = netbuf_len(recv_buf);
        if (recv_len > UDP_SERVER_RECV_BUF_SIZE) {
            recv_len = UDP_SERVER_RECV_BUF_SIZE;
        }
        memcpy(recv_data, recv_buf->p->payload, recv_len);
        recv_data[recv_len] = '\0';
        printf("UDP server recv from %s:%d: %s (len: %d)\n", 
               ipaddr_ntoa(&client_ip), client_port, recv_data, recv_len);

        // 示例：回声响应（将数据原样返回给客户端）
        udp_send_data(udp_server_conn, ipaddr_ntoa(&client_ip), client_port, recv_data, recv_len);

        // 5. 释放缓冲区
        netbuf_delete(recv_buf);
        recv_buf = NULL;
    }

task_exit:
    printf("UDP server task exit\n");
    vTaskDelete(NULL);
}

err_t udp_client_init(uint8_t prio)
{
    os_task_handle_t task_handle = os_task_create(udp_client_task, "udp_client", 
                                                 OS_TASK_STACK_MID, NULL, prio);
    return (task_handle != NULL) ? err_ok : err_mem;
}

static void udp_client_task(void* arg)
{
    struct netbuf* recv_buf = NULL;
    ip_addr_t server_ip;
    uint16_t server_port;
    err_t err;
    uint8_t recv_data[UDP_CLIENT_RECV_BUF_SIZE] = {0};

    // 1. 创建UDP连接
    udp_client_conn = netconn_new(NETCONN_UDP);
    if (udp_client_conn == NULL) {
        goto task_exit;
    }

    // 2. 绑定客户端端口（可选，不绑定则系统自动分配）
    err = netconn_bind(udp_client_conn, IP_ADDR_ANY, UDP_CLIENT_PORT);
    if (err != ERR_OK) {
        netconn_delete(udp_client_conn);
        udp_client_conn = NULL;
        goto task_exit;
    }

    printf("UDP client started, bound to port %d...\n", UDP_CLIENT_PORT);

    while (1) {
        // 3. 接收数据（阻塞）
        err = netconn_recvfrom(udp_client_conn, &recv_buf, &server_ip, &server_port);
        if (err != ERR_OK || recv_buf == NULL) {
            continue;
        }

        // 4. 处理接收数据
        uint32_t recv_len = netbuf_len(recv_buf);
        if (recv_len > UDP_CLIENT_RECV_BUF_SIZE) {
            recv_len = UDP_CLIENT_RECV_BUF_SIZE;
        }
        memcpy(recv_data, recv_buf->p->payload, recv_len);
        recv_data[recv_len] = '\0';
        printf("UDP client recv from %s:%d: %s (len: %d)\n", 
               ipaddr_ntoa(&server_ip), server_port, recv_data, recv_len);

        // 5. 释放缓冲区
        netbuf_delete(recv_buf);
        recv_buf = NULL;
    }

task_exit:
    printf("UDP client task exit\n");
    vTaskDelete(NULL);
}

int32_t udp_send_data(struct netconn* conn, const char* dest_ip, uint16_t dest_port, 
                     const uint8_t* data, uint32_t len)
{
    if (conn == NULL || dest_ip == NULL || dest_port == 0 || data == NULL || len == 0) {
        return -1;
    }

    ip_addr_t dest_ip_addr;
    if (ipaddr_aton(dest_ip, &dest_ip_addr) == 0) {
        return -2;  // IP解析失败
    }

    err_t err = netconn_sendto(conn, data, len, &dest_ip_addr, dest_port);
    if (err != ERR_OK) {
        printf("UDP send failed (err: %d)\n", err);
        return -3;
    }

    return len;
}

void udp_close(struct netconn* conn)
{
    if (conn != NULL) {
        netconn_close(conn);
        netconn_delete(conn);
        if (conn == udp_server_conn) {
            udp_server_conn = NULL;
        }
        if (conn == udp_client_conn) {
            udp_client_conn = NULL;
        }
    }
}