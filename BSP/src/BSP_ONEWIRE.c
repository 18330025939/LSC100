#include "BSP_ONEWIRE.h"
#include <stdio.h>
#include "systick.h"

// 初始化单总线引脚（开漏输出，无上下拉）
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

    // // 初始化为高电平
    // gpio_bit_write(self->port, self->pin, SET);
    return true;
}


// 单总线复位（发送复位脉冲，检测从机存在）
bool onewire_reset(Platform_OneWire_t* self)
{
    if (self == NULL)
        return false;
    
    // 发送复位脉冲（拉低750us）
    gpio_bit_reset(self->port, self->pin);
    Drv_DelayUs(750);
    
    // 释放总线（拉高）
    gpio_bit_set(self->port, self->pin);
    Drv_DelayUs(15);
    
    return true;
}

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

// 写1位数据
static void onewire_write_bit(Platform_OneWire_t* self, uint8_t bit)
{
    if (self == NULL)
        return;
    
    // platform_gpio_set_output(self->port, self->pin);
    // 拉低总线
    gpio_bit_reset(self->port, self->pin);
    Drv_DelayUs(bit ? ONEWIRE_WRITE_1_DELAY : ONEWIRE_WRITE_0_DELAY);
    
    // 释放总线
    if (bit) {
        gpio_bit_set(self->port, self->pin);
    } else {
        gpio_bit_reset(self->port, self->pin);
    }
    // gpio_bit_set(self->port, self->pin);
    // Drv_DelayUs(bit ? ONEWIRE_WRITE_0_DELAY : ONEWIRE_WRITE_1_DELAY);
    Drv_DelayUs(ONEWIRE_WRITE_0_DELAY);
    gpio_bit_set(self->port, self->pin);
}

// 读1位数据
static uint8_t onewire_read_bit(Platform_OneWire_t* self)
{
    if (self == NULL)
        return 0;
    
    uint8_t bit = 0;
    
    // gpio_mode_set(self->port, GPIO_MODE_OUTPUT,GPIO_PUPD_PULLUP, self->pin);
    // 拉低总线
    gpio_bit_reset(self->port, self->pin);
    Drv_DelayUs(1);
    
    // // 释放总线
    gpio_bit_set(self->port, self->pin);
    Drv_DelayUs(2);
    // 读取总线电平
    if (gpio_input_bit_get(self->port, self->pin) == SET) {
        bit = 1;
    }
    
    Drv_DelayUs(60);
    
    return bit;
}

// 写1字节数据（LSB先行）
void onewire_write_byte(Platform_OneWire_t* self, uint8_t byte)
{
    uint8_t data = byte;

    for (uint8_t i = 0; i < 8; i++) {
        onewire_write_bit(self, data & 0x01);
        data >>= 1;
    }
}

// 读1字节数据（LSB先行）
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
