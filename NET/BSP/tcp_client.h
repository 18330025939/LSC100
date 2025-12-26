
#ifndef __TCP_CLIENT_H
#define __TCP_CLIENT_H


// TCP Client配置参数（可根据实际需求修改）
#define TCP_CLIENT_RECV_BUF_LEN 1024    // 接收缓冲区长度
#define TCP_CLIENT_SEND_BUF_LEN 1024    // 发送缓冲区长度
#define TCP_CLIENT_TASK_STACK 2048      // 任务栈大小（字节）
#define TCP_CLIENT_TASK_PRIO 4          // 任务优先级
#define TCP_RECONNECT_INTERVAL 3000     // 断线重连间隔（ms）

// TCP Client启动（指定Server IP和端口，创建独立任务）
void tcp_client_start(const char *server_ip, uint16_t server_port);

// TCP Client发送数据（主动向Server发送）
int32_t tcp_client_send_data(const uint8_t *data, uint32_t data_len);

#endif /* __TCP_CLIENT_H */