#include "BSP_I2C.h"
#include "fm24cxx.h"
#include "systick.h"

static I2C_HandleTypeDef g_sd2505_Handle = {
                            .i2cx = I2C_I2C0,
                            .i2c_periph = I2C_I2C0_PERIPH, 
                            .gpio_clk = I2C_I2C0_GPIO_CLOCK,
                            .gpio_port = I2C_I2C0_GPIO_PORT,
                            .scl_pin = I2C_I2C0_SCL_PIN,
                            .sda_pin = I2C_I2C0_SDA_PIN,
                            .gpio_af = I2C_I2C0_GPIO_AF,
                            .dev_addr = FM24CXX_DEV_ADDR,
                            .clk_speed = FM24CXX_I2C_SPEED
                            };

/**
 * @brief  FM24C02初始化（绑定I2C0硬件）
 */
void fm24cxx_init(void) {
    I2C_Driver_Init(&g_sd2505_Handle);
}

/**
 * @brief  FM24C02写数据
 * @param  addr: FM24C02内部存储地址（0~255）
 * @param  data: 待写数据
 * @param  len: 数据长度
 * @retval 0-成功，1-失败
 */
uint8_t fm24cxx_write_data(uint8_t addr, uint8_t *data, uint16_t len)
{
    uint8_t ret = I2C_Driver_Write(&g_sd2505_Handle, FM24CXX_DEV_ADDR, addr, data, len);
    return ret;
}

/**
 * @brief  FM24C02读数据
 * @param  addr: FM24C02内部存储地址
 * @param  data: 读取数据缓冲区
 * @param  len: 读取长度
 * @retval 0-成功，1-失败
 */
uint8_t fm24cxx_read_data(uint8_t addr, uint8_t *data, uint16_t len)
{
    return I2C_Driver_Read(&g_sd2505_Handle, FM24CXX_DEV_ADDR, addr, data, len);
}
