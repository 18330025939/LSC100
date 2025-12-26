/*!
    \file    CM2248_driver.h
    \brief   CM2248 driver

    \version 2025-8-25, V1, firmware for GD32F4xx
*/
#ifndef GD32F4XX_CM2248_H
#define GD32F4XX_CM2248_H

#include "gd32f4xx.h"

// #define RCU_CLK RCU_GPIOE
// #define PORT_CLK GPIOE
// #define GPIO_CLK GPIO_PIN_2

//#define CLK_OUT()   gpio_mode_set(PORT_CLK,GPIO_MODE_OUTPUT,GPIO_PUPD_PULLUP,GPIO_CLK)
//#define CLK(x)    gpio_bit_write(PORT_CLK,GPIO_CLK, (x?SET:RESET) )

#define CLK_RCU RCU_GPIOE
#define CLK_PORT GPIOE
#define CLK_PIN GPIO_PIN_2

#define RESET_RCU RCU_GPIOE
#define RESET_PORT GPIOE
#define RESET_PIN GPIO_PIN_3

#define CS_RCU RCU_GPIOE
#define CS_PORT GPIOE
#define CS_PIN GPIO_PIN_4

#define CONVST_A_RCU RCU_GPIOE
#define CONVST_A_PORT GPIOE
#define CONVST_A_PIN GPIO_PIN_6

#define CONVST_B_RCU RCU_GPIOE
#define CONVST_B_PORT GPIOE
#define CONVST_B_PIN GPIO_PIN_7

#define DOUTA_RCU RCU_GPIOE
#define DOUTA_PORT GPIOE
#define DOUTA_PIN GPIO_PIN_5

#define DOUTB_RCU RCU_GPIOF
#define DOUTB_PORT GPIOF
#define DOUTB_PIN GPIO_PIN_8

#define FRSTDATA_RCU RCU_GPIOE
#define FRSTDATA_PORT GPIOE
#define FRSTDATA_PIN GPIO_PIN_11

#define ADC_BUSY_RCU RCU_GPIOE
#define ADC_BUSY_PORT GPIOE
#define ADC_BUSY_PIN GPIO_PIN_10


#define CM2248_CHANNEL_NUM 4


#define CM2248_CLK_H gpio_bit_set(GPIOE,GPIO_PIN_2)
#define CM2248_CLK_L gpio_bit_reset(GPIOE,GPIO_PIN_2)
#define CM2248_RESET_H gpio_bit_set(GPIOE,GPIO_PIN_3)
#define CM2248_RESET_L gpio_bit_reset(GPIOE,GPIO_PIN_3)
#define CM2248_CS_H gpio_bit_set(GPIOE,GPIO_PIN_4)
#define CM2248_CS_L gpio_bit_reset(GPIOE,GPIO_PIN_4)

#define CM2248_ADC_DOUTA gpio_input_bit_get(GPIOE,GPIO_PIN_5)
#define CM2248_ADC_DOUTB gpio_input_bit_get(GPIOF,GPIO_PIN_8)

#define CM2248_CONVST_A_H gpio_bit_set(GPIOE,GPIO_PIN_6)
#define CM2248_CONVST_A_L gpio_bit_reset(GPIOE,GPIO_PIN_6)
#define CM2248_CONVST_B_H gpio_bit_set(GPIOE,GPIO_PIN_7)
#define CM2248_CONVST_B_L gpio_bit_reset(GPIOE,GPIO_PIN_7)

#define CM2248_ADC_BUSY gpio_input_bit_get(GPIOE,GPIO_PIN_10)

#define CM2248_ADC_FRSTDATA_H gpio_bit_set(GPIOE,GPIO_PIN_11)
#define CM2248_ADC_FRSTDATA_L gpio_bit_reset(GPIOE,GPIO_PIN_11)
#define CM2248_ADC_FRSTDATA gpio_input_bit_get(GPIOE,GPIO_PIN_11)

void cm2248_init(void);
void cm2248_start_conv(void);
uint16_t cm2248_start_read_data(void);
void cm2248_reset(void);
void cm2248_read_both_channels(uint16_t *data_a, uint16_t *data_b);



#endif /* GD32F4XX_CM2248_H */
