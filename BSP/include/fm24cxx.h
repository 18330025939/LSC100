#ifndef __FM24CXX_H
#define __FM24CXX_H


/* FM24C02配置 */
#define FM24CXX_DEV_ADDR     0xA0    // 设备地址（可根据A0/A1/A2调整）
#define FM24CXX_I2C_SPEED    400000U

/* FM24C02函数声明 */
void fm24cxx_init(void);
uint8_t fm24cxx_write_data(uint8_t addr, uint8_t *data, uint16_t len);
uint8_t fm24cxx_read_data(uint8_t addr, uint8_t *data, uint16_t len);

#endif /* __FM24CXX_H */
