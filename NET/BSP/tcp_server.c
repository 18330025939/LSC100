#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "lwip/ip_addr.h"
#include "lwip/sys.h"
#include <string.h>
#include "socket_adapter.h"
#include "tcp_server.h"
#if 1
#include "Q468V1.h"
#include "sd2505.h"
#include "BSP_EMMC.h"
#include "BSP_FM24C02.h"
#include "msg_parse.h"
#include "fm24cxx.h"
#include "rtt_log.h"
#endif
// TCP Server任务（核心逻辑：监听→接收连接→数据收发）
void tcp_server_task(void *pvParameters)
{
    int8_t ret = 0;
    uint8_t recv_buf[TCP_RECV_BUF_LEN] = {0};
    // socket_handle_t conn_handle;  // 单个Client连接句柄（如需多Client，可改用数组/链表）
    TcpServerHandle_t *pHandle = (TcpServerHandle_t*)pvParameters;
    
    D5_LED_ON();
    ret = socket_init(&pHandle->listen_handle, PROTO_TCP, TCP_SERVER_PORT);
    if (ret != 0) { 
        vTaskDelete(NULL); 
        return;
    }
    
    ret = socket_tcp_listen(&pHandle->listen_handle, TCP_SERVER_BACKLOG);
    if (ret != 0) {
        socket_close(&pHandle->listen_handle);
        vTaskDelete(NULL);
        return;
    }
    printf("TCP Server start, listen port: %d\n", TCP_SERVER_PORT);

    while (1) {
        ret = socket_tcp_accept(&pHandle->listen_handle, &pHandle->conn_handle);
        if (ret != 0) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        // APP_PRINTF("TCP Client connected, peer IP: %s, port: %d\n", 
        //        inet_ntoa(pHandle->conn_handle.peer_addr.sin_addr), 
        //        ntohs(pHandle->conn_handle.peer_addr.sin_port));

        while (pHandle->conn_handle.stat == SOCK_STAT_CONNECTED) {
            memset(recv_buf, 0, TCP_RECV_BUF_LEN);
            ret = socket_recv(&pHandle->conn_handle, recv_buf, TCP_RECV_BUF_LEN, NULL);
            if (ret <= 0) {
                APP_PRINTF("TCP Client disconnect, peer IP: %s\n", 
                       inet_ntoa(pHandle->conn_handle.peer_addr.sin_addr));
                break;
            }
            
           if(xQueueSend(pHandle->queue, recv_buf, pdMS_TO_TICKS(100)) != pdPASS)
           {
               APP_PRINTF("TCP Server queue send error\n");
           }
        }

        socket_close(&pHandle->conn_handle);
        memset(&pHandle->conn_handle, 0, sizeof(socket_handle_t));
    }
}

// TCP Server启动（创建任务）
void tcp_server_start(TcpServerHandle_t *handle)
{
    if (handle == NULL) {
        return;
    }

    xTaskCreate((TaskFunction_t)tcp_server_task,
                 (const char*   )"TCP_Server_Task",
                 (uint16_t      )TCP_SERVER_TASK_STACK, 
                 (void*         )handle,
                 (UBaseType_t   )TCP_SERVER_TASK_PRIO,
                 (TaskHandle_t* )&handle->task_handle);

}

// TCP Server发送数据（对外接口）
int8_t tcp_server_send_data(TcpServerHandle_t *handle, const uint8_t *data, uint32_t data_len)
{
    if (handle == NULL || data == NULL || data_len == 0 || handle->conn_handle.stat != SOCK_STAT_CONNECTED) {
        return -1;
    }
    return socket_send(&handle->conn_handle, data, data_len, NULL);  // TCP发送无需指定对端地址
}
