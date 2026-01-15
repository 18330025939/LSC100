#ifndef __DS18B20_H
#define __DS18B20_H


#include "BSP_ONEWIRE.h"
#include "FreeRTOS.h"
#include "semphr.h"


#define DS18B20_LOCK_TIMEOUT         pdMS_TO_TICKS(10) // 互斥锁超时（50ms）

// DS18B20命令定义（设备特有）
#define DS18B20_CMD_RESET     0x00 // 复位命令（实际为时序，无字节命令）
#define DS18B20_CMD_SKIP_ROM  0xCC // 跳过ROM命令
#define DS18B20_CMD_CONVERT   0x44 // 温度转换命令
#define DS18B20_CMD_READ_SCR  0xBE // 读暂存器命令

#define DS18B20_GPIO_RCU      RCU_GPIOB
#define DS18B20_GPIO_PORT     GPIOB
#define DS18B20_GPIO_PIN      GPIO_PIN_0

// DS18B20设备对象结构体
typedef struct {
    // 属性：单总线GPIO+温度缓存
    Platform_OneWire_t *ow_dev; // 单总线绑定的GPIO（需支持输入/输出切换）
    float temp_c;                // 缓存当前温度（℃）
    SemaphoreHandle_t mutexSem;       // RTOS 互斥信号量（避免多任务冲突）
    bool is_ready;
    
    // // 方法指针：统一接口
    // Dev_StatusTypeDef (*Init)(struct DS18B20_ObjectTypeDef* self);       // 初始化（复位+检查设备）
    // Dev_StatusTypeDef (*StartConvert)(struct DS18B20_ObjectTypeDef* self); // 启动温度转换
    // Dev_StatusTypeDef (*ReadTemp)(struct DS18B20_ObjectTypeDef* self);   // 读取温度
} DS18B20_Object_t;

bool ds18b20_start_convert(void);
bool ds18b20_read_temp(void);
float ds18b20_get_temp(void);
void ds18b20_init(void);


#endif /* __DS18B20_H */
