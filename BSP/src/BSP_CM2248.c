/*!
    \file    CM2248.c
    \brief   CM2248 driver

    \version 2025-8-25, V1, firmware for GD32F4xx
*/
#include "BSP_CM2248.h"
#include "systick.h"

/*********************************************
 * 函数名：CM2248_delay_us
 * 描  述：基于循环的微秒延时函数（用于CM2248通信，不依赖SysTick）
 * 输  入：count - 延时微秒数
 * 输  出：无
 * 说  明：此函数用于RTOS环境中的CM2248通信，不依赖SysTick中断
 *         使用循环延时实现，延时精度取决于CPU频率
 *         每个循环大约消耗3-4个CPU周期（包含__NOP()）
 ********************************************/
// static void CM2248_delay_us(uint32_t count)
// {
//     volatile uint32_t i, j;
//     for (i = 0; i < count; i++) {
//         // 每个循环约1微秒（需要根据实际CPU频率调整）
//         for (j = 0; j < 40; j++) {  // 40是经验值，可根据实际测试调整
//             __NOP();
//         }
//     }
// }
#define CM2248_delay_us(us)  delay_us_nop(us)

void cm2248_init(void){

    rcu_periph_clock_enable(CLK_RCU);//使能时钟
    rcu_periph_clock_enable(DOUTB_RCU);//使能时钟


    gpio_mode_set(CLK_PORT,GPIO_MODE_OUTPUT,GPIO_PUPD_NONE,CLK_PIN);//设置为推挽输出
    gpio_mode_set(RESET_PORT,GPIO_MODE_OUTPUT,GPIO_PUPD_NONE,RESET_PIN);//设置为推挽输出
    gpio_mode_set(CS_PORT,GPIO_MODE_OUTPUT,GPIO_PUPD_NONE,CS_PIN);//设置为推挽输出
    gpio_mode_set(CONVST_A_PORT,GPIO_MODE_OUTPUT,GPIO_PUPD_NONE,CONVST_A_PIN);//设置为推挽输出
    gpio_mode_set(CONVST_B_PORT,GPIO_MODE_OUTPUT,GPIO_PUPD_NONE,CONVST_B_PIN);//设置为推挽输出
 

    gpio_mode_set(DOUTA_PORT, GPIO_MODE_INPUT, GPIO_PUPD_NONE, DOUTA_PIN);//设置为输入模式
    gpio_mode_set(DOUTB_PORT, GPIO_MODE_INPUT, GPIO_PUPD_NONE, DOUTB_PIN);//设置为输入模式
    gpio_mode_set(FRSTDATA_PORT, GPIO_MODE_INPUT, GPIO_PUPD_NONE, FRSTDATA_PIN);//设置为输入模式
    gpio_mode_set(ADC_BUSY_PORT, GPIO_MODE_INPUT, GPIO_PUPD_NONE, ADC_BUSY_PIN);//设置为输入模式

    CM2248_RESET_L;
    CM2248_CLK_L;
    CM2248_CS_H;
    CM2248_CONVST_A_L;
    CM2248_CONVST_B_L;
    cm2248_reset();

    // cm2248_start_conv();
}

void cm2248_start_conv(void){
    CM2248_CONVST_A_L;
    CM2248_CONVST_B_L;
    CM2248_delay_us(10);  // 1ms = 1000us
    CM2248_CONVST_A_H;
    CM2248_CONVST_B_H;
}

uint16_t cm2248_start_read_data(void)
{
    uint16_t usdata = 0;
    int i = 0;

    CM2248_CS_L;    
    for (i = 15; i >= 0; i--)
    {
        CM2248_CLK_H;
        if(CM2248_ADC_DOUTA)
        {
            usdata |= (1U << i);
        }
        CM2248_CLK_L;
    }
    CM2248_CS_H;
    return usdata;
}

void cm2248_reset(void)
{
    CM2248_RESET_H;
    CM2248_delay_us(10);  
    CM2248_RESET_L;
}

void cm2248_read_both_channels(uint16_t *data_a, uint16_t *data_b)
{
    uint16_t usdata_a = 0;
    uint16_t usdata_b = 0;
    int i = 0;
    
    CM2248_CS_L;

    for (i = 15; i >= 0; i--)
    {
        CM2248_CLK_H;
        if(CM2248_ADC_DOUTA)
        {
            usdata_a |= (1U << i);
        }
        if(CM2248_ADC_DOUTB)
        {
            usdata_b |= (1U << i);
        }
        CM2248_CLK_L;
    }

    CM2248_CS_H;
    
    *data_a = usdata_a;
    *data_b = usdata_b;
}
