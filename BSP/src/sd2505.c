/**
 ****************************************************************************************************
 * @file        sd2505.c
 * @author      xxxxxx
 * @version     V1.0
 * @date        2024-12-12
 * @brief       sd2505 驱动代码  
 ****************************************************************************************************
 * @attention
 *
 * 项目:L2407092
 *
 * 修改说明
 * V1.0 20241212
 * 第一次发布
 * 
 ****************************************************************************************************
 */

#include "systick.h"
#include "BSP_I2C.h"
#include "sd2505.h"


#define delay_ms(ms) delay_ms_nop(ms);

uint8_t sd2505_write_on(void);
uint8_t sd2505_write_off(void);

static I2C_HandleTypeDef g_sd2505_Handle = {
                            .i2cx = I2C_I2C1,
                            .i2c_periph = I2C_I2C1_PERIPH, 
                            .gpio_clk = I2C_I2C1_GPIO_CLOCK,
                            .gpio_port = I2C_I2C1_GPIO_PORT,
                            .scl_pin = I2C_I2C1_SCL_PIN,
                            .sda_pin = I2C_I2C1_SDA_PIN,
                            .gpio_af = I2C_I2C1_GPIO_AF,
                            .dev_addr = SD2505_DEV_ADDR,
                            .clk_speed = SD2505_CLOCK_SPEED
                            };

/**
 * @brief       RTC芯片SD2505初始化
 * @param       无
 * @retval      无
 */
void sd2505_init(void)
{
    uint8_t bCnt = 3;
    uint8_t bValue = 0;
    uint8_t bRValue = 0;
    // rtc_time time = {0x00, 0x42, 0x16, 0x05, 0x19, 0x12, 0x25};

    I2C_Driver_Init(&g_sd2505_Handle);
    delay_ms(20);
    while(bCnt--)
    {   
        bValue = 0x82;
        sd2505_write_on();
        I2C_Driver_Write(&g_sd2505_Handle, SD2505_DEV_ADDR, SD2505_ADDR_CHARGE, &bValue, sizeof(uint8_t));
        sd2505_write_off();
        delay_ms(10);
        I2C_Driver_Read(&g_sd2505_Handle, SD2505_DEV_ADDR, SD2505_ADDR_CHARGE, &bRValue, sizeof(uint8_t));
        if (bRValue == bValue)
        {
            break;
        }
       delay_ms(10);
    }
    // sd2505_set_time(&time);
//    delay_ms(10);
//    sd2505_get_time(&time1);
//    printf("timer year %x moth %x week %x day %x hour %x min %x sec %x\r\n", time1.ucYear, time1.ucMonth, time1.ucWeek, time1.ucDay,
//                                                                         time1.ucHour, time1.ucMinute, time1.ucSecond);
}

/**
 * @brief       RTC芯片SD2505写打开
 * @param       无
 * @retval      False 失败,True 成功
 */
uint8_t sd2505_write_on(void)
{
    uint8_t bValue = 0;
    
    bValue = 0x80;
    if (I2C_Driver_Write(&g_sd2505_Handle, SD2505_DEV_ADDR, SD2505_ADDR_CTR2, &bValue, sizeof(uint8_t)))
    {
        return False;
    }
    bValue = 0xff;
    if (I2C_Driver_Write(&g_sd2505_Handle, SD2505_DEV_ADDR, SD2505_ADDR_CTR1, &bValue, sizeof(uint8_t)))
    {
        return False;
    }

    return True;
}

/**
 * @brief       RTC芯片SD2505写关闭
 * @param       无
 * @retval      False 失败,True 成功
 */
uint8_t sd2505_write_off(void)
{
    uint8_t bValue = 0;
    
    bValue = 0x7b;
    if (I2C_Driver_Write(&g_sd2505_Handle, SD2505_DEV_ADDR, SD2505_ADDR_CTR1, &bValue, sizeof(uint8_t)))
    {
        return False;
    }
    bValue = 0x0;
    if (I2C_Driver_Write(&g_sd2505_Handle, SD2505_DEV_ADDR, SD2505_ADDR_CTR2, &bValue, sizeof(uint8_t)))
    {
        return False;
    }

    return True;
}

/**
 * @brief       RTC芯片SD2505设置系统时间
 * @param       time: 要设置的时间结构指针
 * @retval      False 失败,True 成功
 */
uint8_t sd2505_set_time(rtc_time *time)
{   
    if (NULL == time)
    {
        return False;
    }
    
    sd2505_write_on();
    if (I2C_Driver_Write(&g_sd2505_Handle, SD2505_DEV_ADDR, SD2505_ADDR_SECOND, (uint8_t*)time, sizeof(rtc_time)))
    {
        sd2505_write_off();
        return False;
    }
    delay_ms(10);
    sd2505_write_off();

    return True;
}

/**
 * @brief       RTC芯片SD2505获取系统时间
 * @param       time: 要获取的时间结构指针
 * @retval      False 失败,True 成功
 */
uint8_t sd2505_get_time(rtc_time *time)
{
    if (NULL == time)
    {
        return False;
    }

    if (I2C_Driver_Read(&g_sd2505_Handle, SD2505_DEV_ADDR, SD2505_ADDR_SECOND, (uint8_t*)time, sizeof(rtc_time)))
    {
        return False;
    }

    return True;
}   
