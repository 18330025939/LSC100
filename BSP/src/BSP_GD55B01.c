/*!
    \file    BSP_GD55B01.c
    \brief   GD55B01GEYIGR driver

    \version 2025-8-25, V1, firmware for GD32F4xx
*/
#include "BSP_GD55B01.h"
#include "systick.h"

/* 内部低层函数 */
static void spi_config(void);
static uint8_t spi_send_recv_byte(uint8_t byte);
static void gd55b01ge_send_address(uint32_t addr);

// 假设芯片上电后默认是3字节地址模式
static uint8_t is_4byte_mode = 0;


/**
 * @brief  初始化 GD55B01GE 的 SPI 接口和 GPIO
 * @param  无
 * @retval 无
 */
void gd55b01ge_init(void) {
    spi_config();
    FLASH_CS_HIGH(); // 取消片选

    // 由于 GD55B01GE 容量大于 128Mbit，必须使用4字节地址模式
    gd55b01ge_enter_4byte_addr_mode();
}

/**
 * @brief  配置 SPI 和 GPIO
 * @param  无
 * @retval 无
 */
static void spi_config(void) {
    spi_parameter_struct spi_init_struct;

    /* 使能时钟 */
    rcu_periph_clock_enable(FLASH_SPI_RCU_CLK);
    rcu_periph_clock_enable(FLASH_SPI_GPIO_RCU_CLK);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(FLASH_CS_GPIO_RCU_CLK);

    /* 配置 SCK, MISO, MOSI 引脚为复用功能 */
    gpio_af_set(FLASH_SPI_GPIO_PORT, FLASH_SPI_GPIO_AF, FLASH_SCK_PIN | FLASH_MISO_PIN | FLASH_MOSI_PIN);
    gpio_mode_set(FLASH_SPI_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, FLASH_SCK_PIN | FLASH_MISO_PIN | FLASH_MOSI_PIN);
    gpio_output_options_set(FLASH_SPI_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, FLASH_SCK_PIN | FLASH_MOSI_PIN);

    /* 配置 CS 引脚为推挽输出 */
    gpio_mode_set(FLASH_CS_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, FLASH_CS_PIN);
    gpio_output_options_set(FLASH_CS_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, FLASH_CS_PIN);

    /* SPI 参数配置 */
    spi_init_struct.trans_mode           = SPI_TRANSMODE_FULLDUPLEX;
    spi_init_struct.device_mode          = SPI_MASTER;
    spi_init_struct.frame_size           = SPI_FRAMESIZE_8BIT;
    spi_init_struct.clock_polarity_phase = SPI_CK_PL_HIGH_PH_2EDGE; // SPI Mode 3
    spi_init_struct.nss                  = SPI_NSS_SOFT;
    spi_init_struct.prescale             = SPI_PSC_4; // 根据系统时钟调整分频
    spi_init_struct.endian               = SPI_ENDIAN_MSB;
    spi_init(FLASH_SPI_PORT, &spi_init_struct);

    /* 使能 SPI */
    spi_enable(FLASH_SPI_PORT);
}

/**
 * @brief  通过 SPI 发送并接收一个字节
 * @param  byte: 要发送的字节
 * @retval 接收到的字节
 */
static uint8_t spi_send_recv_byte(uint8_t byte) {
    while (RESET == spi_i2s_flag_get(FLASH_SPI_PORT, SPI_FLAG_TBE));
    spi_i2s_data_transmit(FLASH_SPI_PORT, byte);

    while (RESET == spi_i2s_flag_get(FLASH_SPI_PORT, SPI_FLAG_RBNE));
    return spi_i2s_data_receive(FLASH_SPI_PORT);
}

/**
 * @brief  发送地址 (自动判断3字节或4字节模式)
 * @param  addr: 24位或32位地址
 * @retval 无
 */
static void gd55b01ge_send_address(uint32_t addr) {
    if (is_4byte_mode) {
        spi_send_recv_byte((addr & 0xFF000000) >> 24);
    }
    spi_send_recv_byte((addr & 0xFF0000) >> 16);
    spi_send_recv_byte((addr & 0xFF00) >> 8);
    spi_send_recv_byte(addr & 0xFF);
}


/**
 * @brief  读取 JEDEC ID
 * @param  无
 * @retval 32位的 JEDEC ID (实际有效24位)
 */
uint32_t gd55b01ge_read_jedec_id(void) {
    uint32_t id = 0;
    uint8_t manufacturer_id, device_id_1, device_id_2;

    FLASH_CS_LOW();
    spi_send_recv_byte(RDID_CMD); // 发送读ID指令 [cite: 678]
    manufacturer_id = spi_send_recv_byte(0xFF);
    device_id_1 = spi_send_recv_byte(0xFF);
    device_id_2 = spi_send_recv_byte(0xFF);
    FLASH_CS_HIGH();

    id = (manufacturer_id << 16) | (device_id_1 << 8) | device_id_2;
    return id;
}

/**
 * @brief  发送写使能指令
 * @param  无
 * @retval 无
 */
void gd55b01ge_write_enable(void) {
    FLASH_CS_LOW();
    spi_send_recv_byte(WREN_CMD); // 发送写使能指令 [cite: 442]
    FLASH_CS_HIGH();
}

/**
 * @brief  等待WIP位清零，表示内部操作完成
 * @param  无
 * @retval 无
 */
void gd55b01ge_wait_for_write_end(void) {
    uint8_t status = 0;
    FLASH_CS_LOW();
    spi_send_recv_byte(RDSR1_CMD); // 读状态寄存器1 [cite: 482]
    do {
        status = spi_send_recv_byte(0xFF);
    } while ((status & WIP_FLAG_MASK) == SET); // 等待WIP位变为0 [cite: 315]
    FLASH_CS_HIGH();
}

/**
 * @brief  进入4字节地址模式
 * @param  无
 * @retval 无
 */
void gd55b01ge_enter_4byte_addr_mode(void) {
    FLASH_CS_LOW();
    spi_send_recv_byte(ENTER_4B_MODE_CMD); // 发送进入4字节地址模式指令 [cite: 435]
    FLASH_CS_HIGH();
    is_4byte_mode = 1;
}

/**
 * @brief  退出4字节地址模式
 * @param  无
 * @retval 无
 */
void gd55b01ge_exit_4byte_addr_mode(void) {
    FLASH_CS_LOW();
    spi_send_recv_byte(EXIT_4B_MODE_CMD); // 发送退出4字节地址模式指令 [cite: 439]
    FLASH_CS_HIGH();
    is_4byte_mode = 0;
}

/**
 * @brief  擦除一个扇区 (4KB)
 * @param  sector_addr: 要擦除的扇区内的任意地址
 * @retval 无
 */
void gd55b01ge_erase_sector(uint32_t sector_addr) {
    gd55b01ge_write_enable();
    FLASH_CS_LOW();
    spi_send_recv_byte(SE_CMD); // 发送扇区擦除指令 [cite: 590]
    gd55b01ge_send_address(sector_addr);
    FLASH_CS_HIGH();
    gd55b01ge_wait_for_write_end();
}

/**
 * @brief  擦除一个32KB块
 * @param  block_addr: 要擦除的块内的任意地址
 * @retval 无
 */
void gd55b01ge_erase_block32(uint32_t block_addr) {
    gd55b01ge_write_enable();
    FLASH_CS_LOW();
    spi_send_recv_byte(BE32_CMD); // 发送32KB块擦除指令 [cite: 605]
    gd55b01ge_send_address(block_addr);
    FLASH_CS_HIGH();
    gd55b01ge_wait_for_write_end();
}

/**
 * @brief  擦除一个64KB块
 * @param  block_addr: 要擦除的块内的任意地址
 * @retval 无
 */
void gd55b01ge_erase_block64(uint32_t block_addr) {
    gd55b01ge_write_enable();
    FLASH_CS_LOW();
    spi_send_recv_byte(BE64_CMD); // 发送64KB块擦除指令 [cite: 620]
    gd55b01ge_send_address(block_addr);
    FLASH_CS_HIGH();
    gd55b01ge_wait_for_write_end();
}

/**
 * @brief  全片擦除
 * @param  无
 * @retval 无
 */
void gd55b01ge_erase_chip(void) {
    gd55b01ge_write_enable();
    FLASH_CS_LOW();
    spi_send_recv_byte(CE_CMD); // 发送全片擦除指令 [cite: 634]
    FLASH_CS_HIGH();
    gd55b01ge_wait_for_write_end();
}

/**
 * @brief  页编程 (在指定地址写入最多256字节的数据)
 * @param  addr: 目标地址
 * @param  p_data: 指向数据缓冲区的指针
 * @param  count: 要写入的字节数 (不能超过256)
 * @retval 无
 */
void gd55b01ge_page_program(uint32_t addr, const uint8_t* p_data, uint16_t count) {
    if (count > GD55B01GE_PAGE_SIZE) {
        // 错误处理: 写入大小不能超过一页
        return;
    }

    gd55b01ge_write_enable();
    FLASH_CS_LOW();
    spi_send_recv_byte(PP_CMD); // 发送页编程指令 [cite: 543]
    gd55b01ge_send_address(addr);

    for (uint16_t i = 0; i < count; i++) {
        spi_send_recv_byte(p_data[i]);
    }

    FLASH_CS_HIGH();
    gd55b01ge_wait_for_write_end();
}

/**
 * @brief  在指定地址写入任意长度的数据 (自动处理跨页)
 * @param  addr: 目标地址
 * @param  p_data: 指向数据缓冲区的指针
 * @param  count: 要写入的总字节数
 * @retval 无
 */
void gd55b01ge_write_data(uint32_t addr, const uint8_t* p_data, uint32_t count) {
    uint32_t current_addr = addr;
    const uint8_t* p_buf = p_data;
    uint32_t bytes_remaining = count;

    while (bytes_remaining > 0) {
        uint16_t bytes_on_page = GD55B01GE_PAGE_SIZE - (current_addr % GD55B01GE_PAGE_SIZE);
        uint16_t bytes_to_write = (bytes_remaining < bytes_on_page) ? bytes_remaining : bytes_on_page;

        gd55b01ge_page_program(current_addr, p_buf, bytes_to_write);

        current_addr += bytes_to_write;
        p_buf += bytes_to_write;
        bytes_remaining -= bytes_to_write;
    }
}


/**
 * @brief  在指定地址读取任意长度的数据
 * @param  addr: 目标地址
 * @param  p_data: 指向接收数据缓冲区的指针
 * @param  count: 要读取的总字节数
 * @retval 无
 */
void gd55b01ge_read_data(uint32_t addr, uint8_t* p_data, uint32_t count) {
    FLASH_CS_LOW();
    spi_send_recv_byte(READ_CMD); // 发送读数据指令 [cite: 497]
    gd55b01ge_send_address(addr);

    for (uint32_t i = 0; i < count; i++) {
        p_data[i] = spi_send_recv_byte(0xFF);
    }

    FLASH_CS_HIGH();
}

