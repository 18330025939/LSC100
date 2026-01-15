#include "BSP_DS18B20.h"
#include <string.h>
#include "BSP_ONEWIRE.h"
#include "FreeRTOS.h"
#include "semphr.h"

#include "rtt_log.h"

static Platform_OneWire_t g_oneWire = {
    .rcu = DS18B20_GPIO_RCU,
    .port = DS18B20_GPIO_PORT,
    .pin = DS18B20_GPIO_PIN
};

static DS18B20_Object_t g_ds18b20;

// 私有方法：启动DS18B20温度转换
bool ds18b20_start_convert(void)
{
    DS18B20_Object_t *self = (DS18B20_Object_t *)&g_ds18b20; 

    if (xSemaphoreTake(self->mutexSem, DS18B20_LOCK_TIMEOUT) != pdPASS) {
        return false;
    }

    onewire_reset(self->ow_dev);
    onewire_check(self->ow_dev);
    
    // 2. 发送命令：跳过ROM→启动转换
    onewire_write_byte(self->ow_dev, DS18B20_CMD_SKIP_ROM);
    onewire_write_byte(self->ow_dev, DS18B20_CMD_CONVERT);

    xSemaphoreGive(self->mutexSem);
    // 3. 温度转换需要时间（最大750ms，此处不阻塞，需外部延时后读数据）
    return true;
}

// 私有方法：读取DS18B20温度
bool ds18b20_read_temp(void)
{
    DS18B20_Object_t *self = (DS18B20_Object_t *)&g_ds18b20; 

    uint8_t temp_raw[2] = {0};
    uint16_t temp_val = 0;

    if (xSemaphoreTake(self->mutexSem, DS18B20_LOCK_TIMEOUT) != pdPASS) {
        return false;
    }
  
    // 1. 单总线复位
    onewire_reset(self->ow_dev);
    onewire_check(self->ow_dev);

        // 2. 发送命令：跳过ROM→读暂存器
    onewire_write_byte(self->ow_dev, DS18B20_CMD_SKIP_ROM);
    onewire_write_byte(self->ow_dev, DS18B20_CMD_READ_SCR);
    
    // 3. 读暂存器前2字节（温度低位+高位）
    temp_raw[0] = onewire_read_byte(self->ow_dev); // 温度低位
    temp_raw[1] = onewire_read_byte(self->ow_dev); // 温度高位

    // 4. 数据转换（DS18B20分辨率0.0625℃，16位有符号数）
    if (temp_raw[1] > 7) {
        temp_val = temp_raw[1] << 8 | temp_raw[0];
        temp_val = ~temp_val;
        self->temp_c = -(float)(temp_val + 1) * 0.0625f; 
    } else {
        temp_val = temp_raw[1] << 8 | temp_raw[0];
        self->temp_c = (float)temp_val * 0.0625f;
    }

    xSemaphoreGive(self->mutexSem);
    return true;
}

// 私有方法：获取缓存温度
float ds18b20_get_temp(void)
{
    DS18B20_Object_t *self = (DS18B20_Object_t *)&g_ds18b20; 
    float tmp = 0.0f;
    
    if (xSemaphoreTake(self->mutexSem, DS18B20_LOCK_TIMEOUT) != pdPASS) {
        return false;
    }

    tmp = self->temp_c;

    xSemaphoreGive(self->mutexSem);
    
    return tmp;
}

// 对外接口：创建DS18B20对象
void ds18b20_init(void)
{
    DS18B20_Object_t *self = (DS18B20_Object_t *)&g_ds18b20; 

    // 初始化DS18B20属性
    self->ow_dev = &g_oneWire;
    self->temp_c = 0.0f;
    self->is_ready = true;
    self->mutexSem = xSemaphoreCreateMutex();
    onewire_init(self->ow_dev);
    onewire_reset(self->ow_dev);
    onewire_check(self->ow_dev);
    ds18b20_start_convert();
}

