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

/**
 * @brief  DS18B20启动温度转换
 * @param  void
 * @retval true-成功,false-失败
 */
bool ds18b20_start_convert(void)
{
    DS18B20_Object_t *self = (DS18B20_Object_t *)&g_ds18b20; 

    if (xSemaphoreTake(self->mutexSem, DS18B20_LOCK_TIMEOUT) != pdPASS) {
        return false;
    }

    onewire_reset(self->ow_dev);
    onewire_check(self->ow_dev);

    onewire_write_byte(self->ow_dev, DS18B20_CMD_SKIP_ROM);
    onewire_write_byte(self->ow_dev, DS18B20_CMD_CONVERT);

    xSemaphoreGive(self->mutexSem);

    return true;
}

/**
 * @brief  DS18B20读取温度
 * @param  void
 * @retval true-成功,false-失败
 */
bool ds18b20_read_temp(void)
{
    DS18B20_Object_t *self = (DS18B20_Object_t *)&g_ds18b20; 

    uint8_t temp_raw[2] = {0};
    uint16_t temp_val = 0;

    if (xSemaphoreTake(self->mutexSem, DS18B20_LOCK_TIMEOUT) != pdPASS) {
        return false;
    }
  
    onewire_reset(self->ow_dev);
    onewire_check(self->ow_dev);

    onewire_write_byte(self->ow_dev, DS18B20_CMD_SKIP_ROM);
    onewire_write_byte(self->ow_dev, DS18B20_CMD_READ_SCR);
    
    temp_raw[0] = onewire_read_byte(self->ow_dev); // 温度低位
    temp_raw[1] = onewire_read_byte(self->ow_dev); // 温度高位

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

/**
 * @brief  DS18B20读取温度
 * @param  void
 * @retval 实际温度数据
 */
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

/**
 * @brief  DS18B20初始化
 * @param  void
 * @retval void
 */
void ds18b20_init(void)
{
    DS18B20_Object_t *self = (DS18B20_Object_t *)&g_ds18b20; 

    self->ow_dev = &g_oneWire;
    self->temp_c = 0.0f;
    self->is_ready = true;
    self->mutexSem = xSemaphoreCreateMutex();
    onewire_init(self->ow_dev);
    onewire_reset(self->ow_dev);
    onewire_check(self->ow_dev);
    ds18b20_start_convert();
}

