#ifndef UDP_COMM_H
#define UDP_COMM_H

#include "lwip/netconn.h"
#include "os_adapter.h"
#include <stdint.h>

// UDP服务器配置
#define UDP_SERVER_PORT    8081  // 绑定端口
#define UDP_SERVER_RECV_BUF_SIZE 1024  // 接收缓冲区大小

// UDP客户端配置
#define UDP_CLIENT_PORT    8082  // 客户端绑定端口（可选）
#define UDP_CLIENT_RECV_BUF_SIZE 1024  // 客户端接收缓冲区大小

/**
 * @brief  UDP服务器初始化（创建服务器任务）
 * @param  prio: 任务优先级
 * @return err_ok: 成功，err_mem: 内存不足，err_inval: 参数无效
 */
err_t udp_server_init(uint8_t prio);

/**
 * @brief  UDP客户端初始化（创建客户端任务）
 * @param  prio: 任务优先级
 * @return err_ok: 成功，err_mem: 内存不足
 */
err_t udp_client_init(uint8_t prio);

/**
 * @brief  UDP发送数据
 * @param  conn: UDP连接句柄
 * @param  dest_ip: 目标IP地址（字符串格式）
 * @param  dest_port: 目标端口
 * @param  data: 待发送数据
 * @param  len: 数据长度
 * @return 实际发送长度（<0表示失败）
 */
int32_t udp_send_data(struct netconn* conn, const char* dest_ip, uint16_t dest_port, 
                     const uint8_t* data, uint32_t len);

/**
 * @brief  UDP关闭连接
 * @param  conn: UDP连接句柄
 */
void udp_close(struct netconn* conn);

#endif // UDP_COMM_Hss