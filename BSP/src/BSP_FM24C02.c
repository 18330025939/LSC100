/*!
    \file    BSP_FM24C02.c
    \brief   FM24C02 driver

    \version 2025-8-25, V1, firmware for GD32F4xx
*/
#include "BSP_FM24C02.h"
#include "systick.h"

#include "Q468V1.h"
#include <stdio.h>

/*********************************************
 * 函数名：FM24C_delay_us
 * 描  述：基于循环的微秒延时函数（用于FM24C02 I2C通信，不依赖SysTick）
 * 输  入：count - 延时微秒数
 * 输  出：无
 * 说  明：此函数用于RTOS环境中的I2C通信，不依赖SysTick中断
 *         使用循环延时实现，延时精度取决于CPU频率
 *         每个循环大约消耗3-4个CPU周期（包含__NOP()）
 ********************************************/
// static void FM24C_delay_us(uint32_t count)
// {
//     volatile uint32_t i, j;
//     for (i = 0; i < count; i++) {
//         // 每个循环约1微秒（需要根据实际CPU频率调整）
//         for (j = 0; j < 40; j++) {  // 40是经验值，可根据实际测试调整
//             __NOP();
//         }
//     }
// }
#define FM24C_delay_us(us) delay_us_nop(us)

static inline uint8_t dev_addr_with_block(uint16_t mem_addr) {
    uint8_t block = (mem_addr >> 8) & 0x1; // 24C04: 2 blocks of 256B
    return (uint8_t)(0x50 | (FM24C_A2 << 2) | (FM24C_A1 << 1) | block);
}

void fm24c_init(void) {
    // 1) 使能时钟
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_I2C0);

    // 2) 配置引脚为复用开漏，上拉
    gpio_af_set(FM24C_GPIO_PORT, FM24C_GPIO_AF, FM24C_SCL_PIN | FM24C_SDA_PIN);
    gpio_mode_set(FM24C_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, FM24C_SCL_PIN | FM24C_SDA_PIN);
    gpio_output_options_set(FM24C_GPIO_PORT, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, FM24C_SCL_PIN | FM24C_SDA_PIN);

    // 3) 复位 I2C 外设
    i2c_deinit(FM24C_I2C);

    // 4) 配置时钟和工作模式
    i2c_clock_config(FM24C_I2C, FM24C_I2C_SPEED, I2C_DTCY_2);
    i2c_mode_addr_config(FM24C_I2C, I2C_I2CMODE_ENABLE, I2C_ADDFORMAT_7BITS, 0);
    i2c_enable(FM24C_I2C);
    i2c_ack_config(FM24C_I2C, I2C_ACK_ENABLE);
}

bool fm24c_read(uint16_t addr, uint8_t *buf, size_t len) {
    if (!buf || len == 0) return false;

    while (len > 0) {
        uint8_t dev7 = dev_addr_with_block(addr);

        // dummy write: 发送字地址
        if (!i2c_start_addr_write(dev7)) return false;
        if (!i2c_send_byte_wait((uint8_t)(addr & 0xFF))) {
            i2c_stop(); return false;
        }

        // 重启进入读
        if (!i2c_start_addr_read(dev7)) { i2c_stop(); return false; }

        // 连续读：最后一字节 NACK
        size_t chunk = len; // 跨块读也可行，下一轮会切换 block
        while (chunk--) {
            bool ack = (chunk != 0);
            if (!i2c_recv_byte(buf, ack)) { i2c_stop(); return false; }
            buf++;
            addr++;
        }

        i2c_stop();
        // 这里直接一次性完成所需长度；如需按块拆分，可自定义 chunk
        len = 0;
    }
    return true;
}

bool fm24c_read_byte(uint16_t addr, uint8_t *data) {
    return fm24c_read(addr, data, 1);
}

bool fm24c_write(uint16_t addr, const uint8_t *data, size_t len) {
    if (!data || len == 0) return false;

    while (len > 0) {
        // 计算当前页剩余空间，避免跨页
        uint8_t page_off = addr & 0x0F;               // 16-byte page
        uint8_t space = 16 - page_off;
        size_t chunk = (len < space) ? len : space;

        uint8_t dev7 = dev_addr_with_block(addr);

        // 起始 + 器件写地址
        if (!i2c_start_addr_write(dev7)) return false;

        // 发送字地址（页内偏移）
        if (!i2c_send_byte_wait((uint8_t)(addr & 0xFF))) { i2c_stop(); return false; }

        // 发送页内数据
        for (size_t i = 0; i < chunk; ++i) {
            if (!i2c_send_byte_wait(data[i])) { i2c_stop(); return false; }
        }
        i2c_stop();

        // ACK 轮询等待内部写完成
        if (!eeprom_ack_poll(dev7, FM24C_WRITE_TIMEOUT_MS)) return false;

        // 前进
        addr += (uint16_t)chunk;
        data += chunk;
        len  -= chunk;
    }
    return true;
}

bool fm24c_write_byte(uint16_t addr, uint8_t data) {
    return fm24c_write(addr, &data, 1);
}

/* ---------------- I2C 基础传输 ---------------- */

static bool i2c_wait_flag(i2c_flag_enum flag, FlagStatus status, uint32_t tout_us) {
    while (i2c_flag_get(FM24C_I2C, flag) != (FlagStatus)status) {
        if (tout_us-- == 0) return false;
        FM24C_delay_us(1);
    }
    return true;
}

static bool i2c_bus_idle(uint32_t tout_us) {
    while (i2c_flag_get(FM24C_I2C, I2C_FLAG_I2CBSY)) {
        if (tout_us-- == 0) return false;
        FM24C_delay_us(1);
    }
    return true;
}

static bool i2c_start_addr_write(uint8_t dev7) {
    if (!i2c_bus_idle(2000)) return false;
    i2c_start_on_bus(FM24C_I2C);
    if (!i2c_wait_flag(I2C_FLAG_SBSEND, SET, 2000)) return false;
    i2c_master_addressing(FM24C_I2C, (uint32_t)(dev7 << 1), I2C_TRANSMITTER);
    if (!i2c_wait_flag(I2C_FLAG_ADDSEND, SET, 4000)) return false;
    i2c_flag_clear(FM24C_I2C, I2C_FLAG_ADDSEND);
    return true;
}

static bool i2c_start_addr_read(uint8_t dev7) {
    i2c_start_on_bus(FM24C_I2C);
    if (!i2c_wait_flag(I2C_FLAG_SBSEND, SET, 2000)) return false;
    i2c_master_addressing(FM24C_I2C, (uint32_t)(dev7 << 1), I2C_RECEIVER);
    if (!i2c_wait_flag(I2C_FLAG_ADDSEND, SET, 4000)) return false;
    i2c_flag_clear(FM24C_I2C, I2C_FLAG_ADDSEND);
    return true;
}

static void i2c_stop(void) {
    i2c_stop_on_bus(FM24C_I2C);
    // 等待 STOP 发送完毕
    while (I2C_CTL0(FM24C_I2C) & I2C_CTL0_STOP) { }
}

static bool i2c_send_byte_wait(uint8_t data) {
    if (!i2c_wait_flag(I2C_FLAG_TBE, SET, 4000)) return false;
    i2c_data_transmit(FM24C_I2C, data);
    if (!i2c_wait_flag(I2C_FLAG_BTC, SET, 8000)) return false;
    return true;
}

static bool i2c_recv_byte(uint8_t *data, bool ack) {
    if (ack) {
        i2c_ack_config(FM24C_I2C, I2C_ACK_ENABLE);
    } else {
        i2c_ack_config(FM24C_I2C, I2C_ACK_DISABLE);
    }
    if (!i2c_wait_flag(I2C_FLAG_RBNE, SET, 8000)) return false;
    *data = (uint8_t)i2c_data_receive(FM24C_I2C);
    return true;
}

static bool eeprom_ack_poll(uint8_t dev7, uint32_t timeout_ms) {
    // 反复发送 START + 写地址，直到 ADDSEND 成功或超时
    uint32_t us = timeout_ms * 1000u;
    while (us > 0) {
        if (i2c_start_addr_write(dev7)) {
            i2c_stop();
            return true;
        }
        // 清错误，尝试下一次
        i2c_stop();
        FM24C_delay_us(50);
        if (us > 50) us -= 50; else us = 0;
    }
    return false;
}
