#include "gd32f4xx.h"
#include "BSP_I2C.h"
#include <stdio.h>
#include "systick.h"


// /* 静态全局变量：存储各I2C外设的硬件配置 */
// static I2C_Hw_Config i2c_hw_config[I2C_PERIPH_MAX] = {0};

/**
 * @brief  I2C底层初始化（通用）
 * @param  i2c_periph: 自定义I2C外设枚举（I2C_PERIPH_0/I2C_PERIPH_1）
 * @param  config: 硬件配置结构体指针
 */
void I2C_Driver_Init(I2C_HandleTypeDef *hI2c) 
{
    if (hI2c == NULL) {
        return;
    }

    /* 1. 使能时钟 */
    rcu_periph_clock_enable(hI2c->gpio_clk);
    rcu_periph_clock_enable(hI2c->i2c_periph);


    /* 2. 配置GPIO为开漏复用 */
    gpio_af_set(hI2c->gpio_port, hI2c->gpio_af, hI2c->scl_pin);
    gpio_af_set(hI2c->gpio_port, hI2c->gpio_af, hI2c->sda_pin);
    
    gpio_mode_set(hI2c->gpio_port, GPIO_MODE_AF, GPIO_PUPD_PULLUP, hI2c->scl_pin);
    gpio_output_options_set(hI2c->gpio_port, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, hI2c->scl_pin);
    
    gpio_mode_set(hI2c->gpio_port, GPIO_MODE_AF, GPIO_PUPD_PULLUP, hI2c->sda_pin);
    gpio_output_options_set(hI2c->gpio_port, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, hI2c->sda_pin);


    /* 3. 配置I2C参数 */
    i2c_deinit(hI2c->i2cx);
    i2c_clock_config(hI2c->i2cx, hI2c->clk_speed, I2C_DTCY_2);  // 400KHz快速模式
    i2c_mode_addr_config(hI2c->i2cx, I2C_I2CMODE_ENABLE, I2C_ADDFORMAT_7BITS, 0);
    i2c_enable(hI2c->i2cx);
    i2c_ack_config(hI2c->i2cx, I2C_ACK_ENABLE);
}

/* 等待事件，超时返回 1 */
static uint8_t i2c_wait_flag(I2C_HandleTypeDef *hI2c, i2c_flag_enum flag)
{
    uint32_t tout = I2C_TIMEOUT_MS * 1000;

    while(i2c_flag_get(hI2c->i2cx, flag) == RESET && --tout);
    return tout == 0;
}

/**
 * @brief  通用I2C写函数
 * @param  i2c_periph: 自定义I2C外设枚举
 * @param  dev_addr: 设备地址（覆盖配置中的地址，兼容多设备挂载）
 * @param  reg: 寄存器地址
 * @param  data: 待写数据缓冲区
 * @param  len: 数据长度
 * @retval 0-成功，1-失败
 */
uint8_t I2C_Driver_Write(I2C_HandleTypeDef *hI2c, uint8_t dev_addr, uint8_t reg, uint8_t *data, uint16_t len) 
{
    if (data == NULL || len == 0) {
        return 1;
    }

    i2c_start_on_bus(hI2c->i2cx);
    if (i2c_wait_flag(hI2c, I2C_FLAG_SBSEND)) return 1;

    i2c_master_addressing(hI2c->i2cx, dev_addr, I2C_TRANSMITTER);
    if (i2c_wait_flag(hI2c, I2C_FLAG_ADDSEND)) return 1;
    i2c_flag_clear(hI2c->i2cx, I2C_FLAG_ADDSEND);

    i2c_data_transmit(hI2c->i2cx, reg);
    if (i2c_wait_flag(hI2c, I2C_FLAG_TBE)) return 1;

    for (uint16_t i = 0; i < len; i++) {
        i2c_data_transmit(hI2c->i2cx, data[i]);
        if (i2c_wait_flag(hI2c, I2C_FLAG_TBE)) return 1;
    }

    i2c_stop_on_bus(hI2c->i2cx);
    while (I2C_CTL0(hI2c->i2cx) & I2C_CTL0_STOP);

    return 0;
}

/**
 * @brief  通用I2C读函数
 * @param  i2c_periph: 自定义I2C外设枚举
 * @param  dev_addr: 设备地址
 * @param  reg: 寄存器地址
 * @param  data: 待读数据缓冲区
 * @param  len: 数据长度
 * @retval 0-成功，1-失败
 */
uint8_t I2C_Driver_Read(I2C_HandleTypeDef *hI2c, uint8_t dev_addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0) {
        return 1;
    }

    i2c_start_on_bus(hI2c->i2cx);
    if (i2c_wait_flag(hI2c, I2C_FLAG_SBSEND)) return 1;

    i2c_master_addressing(hI2c->i2cx, dev_addr, I2C_TRANSMITTER);
    if (i2c_wait_flag(hI2c, I2C_FLAG_ADDSEND)) return 1;
    i2c_flag_clear(hI2c->i2cx, I2C_FLAG_ADDSEND);

    i2c_data_transmit(hI2c->i2cx, reg);
    if (i2c_wait_flag(hI2c, I2C_FLAG_TBE)) return 1;

    i2c_start_on_bus(hI2c->i2cx);
    if (i2c_wait_flag(hI2c, I2C_FLAG_SBSEND)) return 1;

    i2c_master_addressing(hI2c->i2cx, dev_addr, I2C_RECEIVER);
    if (i2c_wait_flag(hI2c, I2C_FLAG_ADDSEND)) return 1;
    i2c_flag_clear(hI2c->i2cx, I2C_FLAG_ADDSEND);

    for (uint16_t i = 0; i < len; i++) {
        if (i == len - 1) {
            i2c_ack_config(hI2c->i2cx, I2C_ACK_DISABLE); 
        }
        if (i2c_wait_flag(hI2c, I2C_FLAG_RBNE)) return 1;
        data[i] = i2c_data_receive(hI2c->i2cx);
    }

    i2c_stop_on_bus(hI2c->i2cx);
    while (I2C_CTL0(hI2c->i2cx) & I2C_CTL0_STOP);
    i2c_ack_config(hI2c->i2cx, I2C_ACK_ENABLE); 
    return 0;
}
