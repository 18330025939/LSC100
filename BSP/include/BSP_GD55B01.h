/*!
    \file    BSP_GD55B01.h
    \brief   GD55B01GEYIGR driver

    \version 2025-8-25, V1, firmware for GD32F4xx
*/
#ifndef GD32F4XX_GD55B01_H
#define GD32F4XX_GD55B01_H

#include "gd32f4xx.h"
#include "systick.h"

// SPI 配置
#define FLASH_SPI_PORT               SPI0
#define FLASH_SPI_RCU_CLK            RCU_SPI0
#define FLASH_SPI_GPIO_RCU_CLK       RCU_GPIOG
#define FLASH_SPI_GPIO_AF            GPIO_AF_5

// CS (片选) 引脚
#define FLASH_CS_GPIO_PORT           GPIOG
#define FLASH_CS_PIN                 GPIO_PIN_15
#define FLASH_CS_GPIO_RCU_CLK        RCU_GPIOG



#define FLASH_SPI_GPIO_PORT GPIOB
// SCK (时钟) 引脚
#define FLASH_SCK_GPIO_PORT          GPIOB
#define FLASH_SCK_PIN                GPIO_PIN_3

// MISO 引脚
#define FLASH_MISO_GPIO_PORT         GPIOB
#define FLASH_MISO_PIN               GPIO_PIN_4

// MOSI 引脚
#define FLASH_MOSI_GPIO_PORT         GPIOB
#define FLASH_MOSI_PIN               GPIO_PIN_5

/* CS 引脚操作宏 */
#define FLASH_CS_LOW()      gpio_bit_reset(FLASH_CS_GPIO_PORT, FLASH_CS_PIN)
#define FLASH_CS_HIGH()     gpio_bit_set(FLASH_CS_GPIO_PORT, FLASH_CS_PIN)



/* GD55B01GE 指令集 (根据数据手册) */
#define WREN_CMD                     0x06  // 写使能 [cite: 442, 561]
#define WRDI_CMD                     0x04  // 写失能 [cite: 445]
#define RDSR1_CMD                    0x05  // 读状态寄存器-1 [cite: 482]
#define RDSR2_CMD                    0x35  // 读状态寄存器-2 [cite: 485]
#define WRSR_CMD                     0x01  // 写状态寄存器 [cite: 454]
#define READ_CMD                     0x03  // 读数据 [cite: 497]
#define FAST_READ_CMD                0x0B  // 快速读取 [cite: 504]
#define QUAD_FAST_READ_CMD           0xEB  // 四线快速读取 [cite: 515]
#define PP_CMD                       0x02  // 页编程 [cite: 543]
#define QUAD_PP_CMD                  0x32  // 四线页编程 [cite: 560]
#define SE_CMD                       0x20  // 扇区擦除 (4KB) [cite: 590]
#define BE32_CMD                     0x52  // 32KB 块擦除 [cite: 605]
#define BE64_CMD                     0xD8  // 64KB 块擦除 [cite: 620]
#define CE_CMD                       0xC7  // 全片擦除 [cite: 634]
#define RDID_CMD                     0x9F  // 读 JEDEC ID [cite: 678]
#define ENTER_4B_MODE_CMD            0xB7  // 进入4字节地址模式 [cite: 435]
#define EXIT_4B_MODE_CMD             0xE9  // 退出4字节地址模式 [cite: 439]

/* GD55B01GE 芯片信息 */
#define GD55B01GE_JEDEC_ID           0xC8471B // 制造商ID: C8, 设备ID: 471B
#define GD55B01GE_PAGE_SIZE          256      // 页大小: 256字节 [cite: 33]
#define GD55B01GE_SECTOR_SIZE        4096     // 扇区大小: 4KB [cite: 53]
#define GD55B01GE_BLOCK32_SIZE       32768    // 32KB 块大小
#define GD55B01GE_BLOCK64_SIZE       65536    // 64KB 块大小
#define GD55B01GE_TOTAL_SIZE         0x8000000 // 总容量 128MB [cite: 32]

/* 状态寄存器位定义 */
#define WIP_FLAG_MASK                0x01  // 正在写入 (Write In Progress) [cite: 315]
#define WEL_FLAG_MASK                0x02  // 写使能锁存 (Write Enable Latch) [cite: 317]

/* 函数原型 */
void gd55b01ge_init(void);
uint32_t gd55b01ge_read_jedec_id(void);
void gd55b01ge_write_enable(void);
void gd55b01ge_wait_for_write_end(void);

void gd55b01ge_erase_sector(uint32_t sector_addr);
void gd55b01ge_erase_block32(uint32_t block_addr);
void gd55b01ge_erase_block64(uint32_t block_addr);
void gd55b01ge_erase_chip(void);

void gd55b01ge_page_program(uint32_t addr, const uint8_t* p_data, uint16_t count);
void gd55b01ge_write_data(uint32_t addr, const uint8_t* p_data, uint32_t count);

void gd55b01ge_read_data(uint32_t addr, uint8_t* p_data, uint32_t count);

void gd55b01ge_enter_4byte_addr_mode(void);
void gd55b01ge_exit_4byte_addr_mode(void);

#endif /* GD32F4XX_GD55B01_H */
