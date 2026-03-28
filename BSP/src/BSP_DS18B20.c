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
    float temp_c = 0.0f;

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
        temp_c = -(float)(temp_val + 1) * 0.0625f; //0.125f;
    } else {
        temp_val = temp_raw[1] << 8 | temp_raw[0];
        temp_c = (float)temp_val * 0.0625f; //0.125f
    }
    
    if (temp_c > 0) {
        self->temp_c = temp_c;
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

void ds18b20_set_resolution(uint8_t res)
{
    uint8_t cfg_val = 0; 
    DS18B20_Object_t *self = (DS18B20_Object_t *)&g_ds18b20;
    uint8_t th_val = 0;
    uint8_t tl_val = 0;

    if (xSemaphoreTake(self->mutexSem, DS18B20_LOCK_TIMEOUT) != pdPASS) {
        return ;
    }

    onewire_reset(self->ow_dev);
    onewire_check(self->ow_dev);

    onewire_write_byte(self->ow_dev, DS18B20_CMD_SKIP_ROM);
    onewire_write_byte(self->ow_dev, DS18B20_CMD_READ_SCR);
    
    onewire_read_byte(self->ow_dev); 
    onewire_read_byte(self->ow_dev); 
    th_val = onewire_read_byte(self->ow_dev);
    tl_val = onewire_read_byte(self->ow_dev);
    cfg_val = onewire_read_byte(self->ow_dev);

    // APP_PRINTF("th_val:0x%x, tl_val:0x%x, cfg_val:0x%x\r\n", th_val, tl_val, cfg_val);
    switch (res) {
        case DS18B20_RESOLUTION_9:
            cfg_val = 0x1F;
        break;
        case DS18B20_RESOLUTION_10:
            cfg_val = 0x3F;
        break;
        case DS18B20_RESOLUTION_11:
            cfg_val = 0x5F;
        break;
        case DS18B20_RESOLUTION_12:
            cfg_val = 0x7F;
        break;
        default :
        xSemaphoreGive(self->mutexSem);
        return ;
    }

    onewire_reset(self->ow_dev);
    onewire_check(self->ow_dev);

    onewire_write_byte(self->ow_dev, DS18B20_CMD_SKIP_ROM);
    onewire_write_byte(self->ow_dev, DS18B20_CMD_WRITE_SCR);

    onewire_write_byte(self->ow_dev, th_val);
    onewire_write_byte(self->ow_dev, tl_val);
    onewire_write_byte(self->ow_dev, cfg_val);
    
    xSemaphoreGive(self->mutexSem);
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
    //ds18b20_set_resolution(DS18B20_RESOLUTION_11);
}

