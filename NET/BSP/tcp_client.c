#include "tcp_client.h"
#include "FreeRTOS.h"
#include "task.h"
#include "socket_adapter.h"

// 全局变量（TCP Client连接句柄）
static socket_handle_t g_tcp_client_handle;
static char g_server_ip[16] = {0};  // 保存Server IP（用于重连）
static uint16_t g_server_port = 0;  // 保存Server端口（用于重连）

// TCP Client任务（核心逻辑：连接Server→数据收发→断线重连）
static void tcp_client_task(void *pvParameters) {
    int32_t ret = 0;
    uint8_t recv_buf[TCP_CLIENT_RECV_BUF_LEN] = {0};

    while (1) {
        // 1. 未连接状态，尝试连接Server
        if (g_tcp_client_handle.stat != SOCK_STAT_CONNECTED) {
            printf("TCP Client connecting to %s:%d...\n", g_server_ip, g_server_port);
            // 初始化Client句柄（本地端口随机，也可指定固定端口）
            ret = socket_init(&g_tcp_client_handle, PROTO_TCP, 0);
            if (ret != 0) {
                vTaskDelay(pdMS_TO_TICKS(TCP_RECONNECT_INTERVAL));
                continue;
            }
            // 主动连接Server
            ret = socket_tcp_connect(&g_tcp_client_handle, g_server_ip, g_server_port);
            if (ret != 0) {
                socket_close(&g_tcp_client_handle);
                printf("TCP Client connect failed, retry after %dms\n", TCP_RECONNECT_INTERVAL);
                vTaskDelay(pdMS_TO_TICKS(TCP_RECONNECT_INTERVAL));
                continue;
            }
            printf("TCP Client connect success\n");
        }

        // 2. 已连接，循环收发数据
        memset(recv_buf, 0, TCP_CLIENT_RECV_BUF_LEN);
        ret = socket_os_recv(&g_tcp_client_handle, recv_buf, TCP_CLIENT_RECV_BUF_LEN, NULL);
        if (ret <= 0) {
            printf("TCP Client disconnect, start reconnect...\n");
            socket_os_close(&g_tcp_client_handle);
            vTaskDelay(pdMS_TO_TICKS(TCP_RECONNECT_INTERVAL));
            continue;
        }
        printf("TCP Client recv data (len: %d): %s\n", ret, recv_buf);

        // 此处可添加业务逻辑（如解析接收数据、触发动作等）
    }
}

// TCP Client启动（传入Server IP和端口，创建任务）
void tcp_client_start(const char *server_ip, uint16_t server_port)
{
    if (server_ip == NULL || server_port == 0) {
        return;
    }
    strncpy(g_server_ip, server_ip, sizeof(g_server_ip)-1);
    g_server_port = server_port;
    xTaskCreate(tcp_client_task, "TCP_Client_Task", TCP_CLIENT_TASK_STACK, 
                NULL, TCP_CLIENT_TASK_PRIO, NULL);
}

// TCP Client发送数据（对外接口）
int32_t tcp_client_send_data(const uint8_t *data, uint32_t data_len)
{
    if (data == NULL || data_len == 0 || g_tcp_client_handle.stat != SOCK_STAT_CONNECTED) {
        return -1;
    }
    return socket_send(&g_tcp_client_handle, data, data_len, NULL);
}