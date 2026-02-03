#include "BSP_ONEWIRE.h"
#include <stdio.h>
#include "systick.h"

/**
 * @brief  单总线初始化
 * @param  self: 单总线指针
 * @retval true-成功,false-失败
 */
bool onewire_init(Platform_OneWire_t* self)
{
    if (self == NULL)
        return false;
    
    /* 使能时钟 */
    rcu_periph_clock_enable(self->rcu);
    /* 配置DQ为输出模式 */
    gpio_mode_set(self->port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, self->pin);
    /* 配置为推挽输出 50MHZ */
    gpio_output_options_set(self->port, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, self->pin);

    return true;
}


/**
 * @brief  单总线复位
 * @param  self: 单总线指针
 * @retval true-成功,false-失败
 */
bool onewire_reset(Platform_OneWire_t* self)
{
    if (self == NULL)
        return false;
    
    gpio_bit_reset(self->port, self->pin);
    Drv_DelayUs(750);
    
    gpio_bit_set(self->port, self->pin);
    Drv_DelayUs(15);
    
    return true;
}

/**
 * @brief  单总线检测从机是否存在
 * @param  self: 单总线指针
 * @retval 0-存在， 1-不存在
 */
uint8_t onewire_check(Platform_OneWire_t* self)
{
    uint8_t retry = 0;
    uint8_t rval = 0;

    while (gpio_input_bit_get(self->port, self->pin) == SET && retry < 200) {
        retry++;
        Drv_DelayUs(1);
    }

    if (retry >= 200) {
        rval = 1;
    } else {
        retry = 0;
        while (gpio_input_bit_get(self->port, self->pin) == RESET && retry < 240) {
            retry++;
            Drv_DelayUs(1);
        }

        if (retry > 240) {
            rval = 1;
        }
    }

    return rval;
}

/**
 * @brief  单总线写位数据
 * @param  self: 单总线指针
 * @param  bit: 待写位数据
 * @retval 无
 */
static void onewire_write_bit(Platform_OneWire_t* self, uint8_t bit)
{
    if (self == NULL)
        return;
    
    gpio_bit_reset(self->port, self->pin);
    Drv_DelayUs(bit ? ONEWIRE_WRITE_1_DELAY : ONEWIRE_WRITE_0_DELAY);
    
    if (bit) {
        gpio_bit_set(self->port, self->pin);
    } else {
        gpio_bit_reset(self->port, self->pin);
    }

    Drv_DelayUs(ONEWIRE_WRITE_0_DELAY);
    gpio_bit_set(self->port, self->pin);
}

/**
 * @brief  单总线读位数据
 * @param  self: 单总线指针
 * @retval 读取的位数据
 */
static uint8_t onewire_read_bit(Platform_OneWire_t* self)
{
    if (self == NULL)
        return 0;
    
    uint8_t bit = 0;
    
    gpio_bit_reset(self->port, self->pin);
    Drv_DelayUs(1);
    
    gpio_bit_set(self->port, self->pin);
    Drv_DelayUs(2);

    if (gpio_input_bit_get(self->port, self->pin) == SET) {
        bit = 1;
    }
    
    Drv_DelayUs(60);
    
    return bit;
}

/**
 * @brief  单总线写字节数据
 * @param  self: 单总线指针
 * @param  byte: 待写字节数据
 * @retval 无
 */
void onewire_write_byte(Platform_OneWire_t* self, uint8_t byte)
{
    uint8_t data = byte;

    for (uint8_t i = 0; i < 8; i++) {
        onewire_write_bit(self, data & 0x01);
        data >>= 1;
    }
}

/**
 * @brief  单总线读字节数据
 * @param  self: 单总线指针
 * @retval 读取的字节数据
 */
uint8_t onewire_read_byte(Platform_OneWire_t* self)
{
    uint8_t byte = 0;
    uint8_t b = 0;

    for (uint8_t i = 0; i < 8; i++) {
        b = onewire_read_bit(self);
        byte |= b << i;
    }

    return byte;
}
