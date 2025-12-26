/*!
    \file    Q468V1.h
    \brief    Q468V1 driver header file
 

    \version 2025-8-25, V1, firmware for GD32F4xx
*/
#ifndef GD32F4XX_Q468V1_H
#define GD32F4XX_Q468V1_H

#include "gd32f4xx.h"

#include "BSP_CM2248.h"
#include "BSP_FM24C02.h"
#include "BSP_GD55B01.h"
#include "BSP_DS18B20.h"

#define LED_D5_RCU RCU_GPIOE
#define LED_D5_PORT GPIOE
#define LED_D5_PIN GPIO_PIN_12

#define LED_D6_RCU RCU_GPIOE
#define LED_D6_PORT GPIOE
#define LED_D6_PIN GPIO_PIN_13

#define LED_D9_RCU RCU_GPIOF
#define LED_D9_PORT GPIOF
#define LED_D9_PIN GPIO_PIN_5

#define LED_D10_RCU RCU_GPIOF
#define LED_D10_PORT GPIOF
#define LED_D10_PIN GPIO_PIN_6

#define LED_D11_RCU RCU_GPIOF
#define LED_D11_PORT GPIOF
#define LED_D11_PIN GPIO_PIN_7

#define KEY_K1_RCU RCU_GPIOD
#define KEY_K1_PORT GPIOD
#define KEY_K1_PIN GPIO_PIN_14

#define KEY_K2_RCU RCU_GPIOD
#define KEY_K2_PORT GPIOD
#define KEY_K2_PIN GPIO_PIN_15

#define DI_RCU RCU_GPIOE
#define DI_PORT GPIOE
#define DI_PIN GPIO_PIN_14

#define RELAY_RAY1_RCU RCU_GPIOE
#define RELAY_RAY1_PORT GPIOE
#define RELAY_RAY1_PIN GPIO_PIN_0

#define RELAY_RAY2_RCU RCU_GPIOE
#define RELAY_RAY2_PORT GPIOE
#define RELAY_RAY2_PIN GPIO_PIN_1

#define DEBUG_UART_RCU RCU_GPIOD
#define DEBUG_UART_RCU_USART RCU_USART1
#define DEBUG_UART USART1
#define DEBUG_UART_PORT GPIOD
#define DEBUG_AF GPIO_AF_7
#define DEBUG_UART_TX_PIN GPIO_PIN_5
#define DEBUG_UART_RX_PIN GPIO_PIN_6

void led_init(void);
void key_init(void);
void DI_init(void);
void relay_init(void);
void debug_uart_init(void);


// #define D5(x)    gpio_bit_write(LED_D5_PORT,LED_D5_PIN, (x?RESET:SET) )
// #define D6(x)    gpio_bit_write(LED_D6_PORT,LED_D6_PIN, (x?RESET:SET) )
// #define D9(x)    gpio_bit_write(LED_D9_PORT,LED_D9_PIN, (x?RESET:SET) )
// #define D10(x)    gpio_bit_write(LED_D10_PORT,LED_D10_PIN,(x?RESET:SET) )
// #define D11(x)    gpio_bit_write(LED_D11_PORT,LED_D11_PIN, (x?RESET:SET) )

#define D5_LED_TOGLE()     do { \
                                gpio_bit_toggle(LED_D5_PORT, LED_D5_PIN); \
                            } while (0)

#define D6_LED_TOGLE()     do { \
                                gpio_bit_toggle(LED_D6_PORT, LED_D6_PIN); \
                            } while (0)

#define D5_LED_ON()     do { \
                                gpio_bit_reset(LED_D5_PORT, LED_D5_PIN); \
                            } while (0)

#define D5_LED_OFF()     do { \
                                gpio_bit_set(LED_D5_PORT, LED_D5_PIN); \
                            } while (0)

#define D6_LED_ON()     do { \
                                gpio_bit_reset(LED_D6_PORT, LED_D6_PIN); \
                            } while (0)

#define D6_LED_OFF()     do { \
                                gpio_bit_set(LED_D6_PORT, LED_D6_PIN); \
                            } while (0)

#define RUN_LED_TOGLE()     do { \
                                gpio_bit_toggle(LED_D9_PORT, LED_D9_PIN); \
                            } while (0)

#define RUN_LED_ON()        do { \
                                gpio_bit_reset(LED_D9_PORT,LED_D9_PIN); \
                            } while (0)

#define RUN_LED_OFF()       do { \
                                gpio_bit_set(LED_D9_PORT,LED_D9_PIN); \
                            } while (0)

#define ALARM_LED_TOGLE()   do { \
                                gpio_bit_toggle(LED_D10_PORT, LED_D10_PIN); \
                            } while (0)

#define ALARM_LED_ON()      do { \
                                gpio_bit_reset(LED_D10_PORT,LED_D10_PIN); \
                            } while (0)

#define ALARM_LED_OFF()     do { \
                                gpio_bit_set(LED_D10_PORT,LED_D10_PIN); \
                            } while (0)


#define ALARM1_LED_TOGLE()   do { \
                                gpio_bit_toggle(LED_D11_PORT, LED_D11_PIN); \
                            } while (0)

#define ALARM1_LED_ON()      do { \
                                gpio_bit_reset(LED_D11_PORT,LED_D11_PIN); \
                            } while (0)

#define ALARM1_LED_OFF()     do { \
                                gpio_bit_set(LED_D11_PORT,LED_D11_PIN); \
                            } while (0)


// #define RAY1(x)    gpio_bit_write(RELAY_RAY1_PORT,RELAY_RAY1_PIN, (x?SET:RESET) )
// #define RAY2(x)    gpio_bit_write(RELAY_RAY2_PORT,RELAY_RAY2_PIN, (x?SET:RESET) )

#define ALARM1_SHORT_CIRCUIT()  do { \
                                    gpio_bit_reset(RELAY_RAY1_PORT,RELAY_RAY1_PIN); \
                                } while (0)

#define ALARM1_RECOVERY()       do { \
                                    gpio_bit_set(RELAY_RAY1_PORT,RELAY_RAY1_PIN); \
                                } while (0)
                                
#define ALARM2_SHORT_CIRCUIT()  do { \
                                    gpio_bit_reset(RELAY_RAY2_PORT,RELAY_RAY2_PIN); \
                                } while (0)

#define ALARM2_RECOVERY()       do { \
                                    gpio_bit_set(RELAY_RAY2_PORT,RELAY_RAY2_PIN); \
                                } while (0)
                    
                                
#define K1_GET()        gpio_input_bit_get(KEY_K1_PORT,KEY_K1_PIN)
#define K2_GET()        gpio_input_bit_get(KEY_K2_PORT,KEY_K2_PIN)

// #define DI_GET()        gpio_input_bit_get(DI_PORT,DI_PIN)

#define ALARM_CLEAR_STATUS()     gpio_input_bit_get(DI_PORT,DI_PIN)

#endif /* GD32F4XX_Q468V1_H */
