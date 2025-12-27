#ifndef __TCP_SERVER_H
#define __TCP_SERVER_H

#include "socket_adapter.h"
#include "semphr.h"

// TCP Server配置参数（可根据实际需求修改）
#define TCP_SERVER_PORT       8080    // 监听端口
#define TCP_SERVER_BACKLOG    1       // 最大等待连接数
#define TCP_RECV_BUF_LEN      128     // 接收缓冲区长度
#define TCP_SEND_BUF_LEN      128     // 发送缓冲区长度
#define TCP_SERVER_TASK_STACK 1024    // 任务栈大小
#define TCP_SERVER_TASK_PRIO  3       // 任务优先级（FreeRTOS优先级，数值越大越高）

typedef struct 
{
    socket_handle_t listen_handle;
    socket_handle_t conn_handle;
    TaskHandle_t task_handle;
    QueueHandle_t queue;
}TcpServerHandle_t;

void tcp_server_task(void *pvParameters);

// TCP Server启动（创建独立任务，后台运行）
void tcp_server_start(TcpServerHandle_t *handle);

// TCP Server发送数据（向已连接的Client发送）
int8_t tcp_server_send_data(TcpServerHandle_t *handle, const uint8_t *data, uint32_t data_len);

#endif /* TCP_SERVER_H */
