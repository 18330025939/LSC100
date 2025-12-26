/*!
    \file    BSP_FM24C02.h
    \brief   FM24C02 driver

    \version 2025-8-25, V1, firmware for GD32F4xx
*/
#ifndef GD32F4XX_FM24C02_H
#define GD32F4XX_FM24C02_H

#include "gd32f4xx.h"
#include <stdbool.h>
#include "systick.h"

// 选择使用的 I2C 外设与引脚
#ifndef FM24C_I2C
#define FM24C_I2C          I2C0
#endif

#ifndef FM24C_GPIO_PORT
#define FM24C_GPIO_PORT    GPIOB
#endif

#ifndef FM24C_SCL_PIN
#define FM24C_SCL_PIN      GPIO_PIN_6
#endif

#ifndef FM24C_SDA_PIN
#define FM24C_SDA_PIN      GPIO_PIN_7
#endif

#ifndef FM24C_GPIO_AF
#define FM24C_GPIO_AF      GPIO_AF_4  // I2C0/1 常用 AF4，按原理图校对
#endif

// 外部硬件地址脚（A2/A1）的接法
#ifndef FM24C_A2
#define FM24C_A2           0  // A2 接 VSS=0, VCC=1
#endif

#ifndef FM24C_A1
#define FM24C_A1           0  // A1 接 VSS=0, VCC=1
#endif

// I2C 速率
#ifndef FM24C_I2C_SPEED
#define FM24C_I2C_SPEED    400000u  // 400kHz
#endif

// 轮询写完成的超时（毫秒）
#ifndef FM24C_WRITE_TIMEOUT_MS
#define FM24C_WRITE_TIMEOUT_MS  20u  // 大于 15ms 上限留裕量
#endif

// API
void     fm24c_init(void);
bool     fm24c_read(uint16_t addr, uint8_t *buf, size_t len);
bool     fm24c_write(uint16_t addr, const uint8_t *data, size_t len);
bool     fm24c_write_byte(uint16_t addr, uint8_t data);
bool     fm24c_read_byte(uint16_t addr, uint8_t *data);

static bool i2c_start_addr_write(uint8_t dev7);
static bool i2c_start_addr_read(uint8_t dev7);
static void i2c_stop(void);
static bool i2c_send_byte_wait(uint8_t data);
static bool i2c_recv_byte(uint8_t *data, bool ack);
static bool eeprom_ack_poll(uint8_t dev7, uint32_t timeout_ms);

#endif /* FM24C02_H */
