#ifndef __BSP_I2C_H
#define __BSP_I2C_H

#include <stdint.h>
#include "gd32f4xx.h"
/* 宏定义 */
#define I2C_TIMEOUT_MS    100     // I2C超时时间

/* I2C外设枚举 */
typedef enum {
    I2C_PERIPH_0 = 0,
    I2C_PERIPH_1,
    I2C_PERIPH_MAX
} I2C_Periph_TypeDef;

#define I2C_I2C0             I2C0
#define I2C_I2C1             I2C1

#define I2C_I2C0_PERIPH      RCU_I2C0
#define I2C_I2C1_PERIPH      RCU_I2C1

#define I2C_I2C0_GPIO_CLOCK  RCU_GPIOB
#define I2C_I2C1_GPIO_CLOCK  RCU_GPIOF

#define I2C_I2C0_GPIO_PORT  GPIOB
#define I2C_I2C0_SCL_PIN    GPIO_PIN_6
#define I2C_I2C0_SDA_PIN    GPIO_PIN_7
#define I2C_I2C0_GPIO_AF    GPIO_AF_4


#define I2C_I2C1_GPIO_PORT  GPIOF
#define I2C_I2C1_SCL_PIN    GPIO_PIN_1
#define I2C_I2C1_SDA_PIN    GPIO_PIN_0
#define I2C_I2C1_GPIO_AF    GPIO_AF_4


/* I2C硬件配置结构体 */
typedef struct {
    uint32_t i2cx;
    rcu_periph_enum i2c_periph;    // GD32标准I2C外设（I2C0/I2C1）
    rcu_periph_enum gpio_clk;      // GPIO时钟
    uint32_t gpio_port;            // GPIO端口
    uint32_t scl_pin;              // SCL引脚
    uint32_t sda_pin;              // SDA引脚
    uint8_t gpio_af;               // GPIO复用功能
    uint8_t dev_addr;              // 挂载设备地址
    uint32_t clk_speed;
} I2C_HandleTypeDef;

/* 通用I2C驱动函数声明 */
void I2C_Driver_Init(I2C_HandleTypeDef *hI2c);
uint8_t I2C_Driver_Write(I2C_HandleTypeDef *hI2c, uint8_t dev_addr, uint8_t reg, uint8_t *data, uint16_t len);
uint8_t I2C_Driver_Read(I2C_HandleTypeDef *hI2c, uint8_t dev_addr, uint8_t reg, uint8_t *data, uint16_t len);

#endif /* __BSP_I2C_H */
