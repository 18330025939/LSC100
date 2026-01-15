/*!
    \file    Q468V1.c
    \brief    Q468V1 driver source file

    \version 2025-8-25, V1, firmware for GD32F4xx
*/
#include "Q468V1.h"
#include "systick.h"

void led_init(void)
{
    rcu_periph_clock_enable(LED_D5_RCU);//使能时钟
    rcu_periph_clock_enable(LED_D9_RCU);

    gpio_mode_set(LED_D5_PORT,GPIO_MODE_OUTPUT,GPIO_PUPD_NONE,LED_D5_PIN);//设置为推挽输出
    gpio_mode_set(LED_D6_PORT,GPIO_MODE_OUTPUT,GPIO_PUPD_NONE,LED_D6_PIN);

    gpio_mode_set(LED_D9_PORT,GPIO_MODE_OUTPUT,GPIO_PUPD_NONE,LED_D9_PIN);
    gpio_mode_set(LED_D10_PORT,GPIO_MODE_OUTPUT,GPIO_PUPD_NONE,LED_D10_PIN);
    gpio_mode_set(LED_D11_PORT,GPIO_MODE_OUTPUT,GPIO_PUPD_NONE,LED_D11_PIN);

    gpio_output_options_set(LED_D5_PORT,GPIO_OTYPE_PP,GPIO_OSPEED_50MHZ,LED_D5_PIN);//设置输出速度为50MHz
    gpio_output_options_set(LED_D6_PORT,GPIO_OTYPE_PP,GPIO_OSPEED_50MHZ,LED_D6_PIN);

    gpio_output_options_set(LED_D9_PORT,GPIO_OTYPE_PP,GPIO_OSPEED_50MHZ,LED_D9_PIN);
    gpio_output_options_set(LED_D10_PORT,GPIO_OTYPE_PP,GPIO_OSPEED_50MHZ,LED_D10_PIN);
    gpio_output_options_set(LED_D11_PORT,GPIO_OTYPE_PP,GPIO_OSPEED_50MHZ,LED_D11_PIN);


    gpio_bit_set(LED_D5_PORT,LED_D5_PIN);//输出高电平
    gpio_bit_set(LED_D6_PORT,LED_D6_PIN);
    gpio_bit_set(LED_D9_PORT,LED_D9_PIN);
    gpio_bit_set(LED_D10_PORT,LED_D10_PIN);
    gpio_bit_set(LED_D11_PORT,LED_D11_PIN);

}

__weak void cal_Key1_irq_handler_cb(void)
{

}

__weak void cal_Key2_irq_handler_cb(void)
{

}

/*!
    \brief      this function handles external lines 10 to 15 interrupt request
    \param[in]  none
    \param[out] none
    \retval     none
*/
void EXTI10_15_IRQHandler(void)
{
    if (RESET != exti_interrupt_flag_get(CAL_KEY1_EXTI_LINE)) {
        exti_interrupt_flag_clear(CAL_KEY1_EXTI_LINE);
        cal_Key1_irq_handler_cb();
    }

    if (RESET != exti_interrupt_flag_get(CAL_KEY2_EXTI_LINE)) {
        exti_interrupt_flag_clear(CAL_KEY2_EXTI_LINE);
        cal_Key2_irq_handler_cb();
    }
}


void key_init(void)
{
 
    // rcu_periph_clock_enable(KEY_K1_RCU);//使能时钟
    rcu_periph_clock_enable(CAL_KEY1_GPIO_CLK);
    // rcu_periph_clock_enable(CAL_KEY2_GPIO_CLK);
    rcu_periph_clock_enable(RCU_SYSCFG);

    gpio_mode_set(CAL_KEY1_GPIO_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, CAL_KEY1_GPIO_PIN);
    gpio_mode_set(CAL_KEY2_GPIO_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, CAL_KEY2_GPIO_PIN);
    // gpio_mode_set(KEY_K1_PORT, GPIO_MODE_INPUT, GPIO_PUPD_NONE, KEY_K1_PIN);//设置为输入模式
    // gpio_mode_set(KEY_K2_PORT, GPIO_MODE_INPUT, GPIO_PUPD_NONE, KEY_K2_PIN);

    nvic_irq_enable(CAL_KEY1_EXTI_IRQn, 3U, 1U);
    // nvic_irq_enable(CAL_KEY2_EXTI_IRQn, 1U, 1U);

    /* connect key EXTI line to key GPIO pin */
    syscfg_exti_line_config(CAL_KEY1_EXTI_PORT_SOURCE, CAL_KEY1_EXTI_PIN_SOURCE);
    syscfg_exti_line_config(CAL_KEY2_EXTI_PORT_SOURCE, CAL_KEY2_EXTI_PIN_SOURCE);

    /* configure key EXTI line */
    exti_init(CAL_KEY1_EXTI_LINE, EXTI_INTERRUPT, EXTI_TRIG_FALLING);
    exti_interrupt_flag_clear(CAL_KEY1_EXTI_LINE);

    exti_init(CAL_KEY2_EXTI_LINE, EXTI_INTERRUPT, EXTI_TRIG_FALLING);
    exti_interrupt_flag_clear(CAL_KEY2_EXTI_LINE);
}

void DI_init(void)
{
 
    rcu_periph_clock_enable(DI_RCU);//使能时钟
    gpio_mode_set(DI_PORT, GPIO_MODE_INPUT, GPIO_PUPD_NONE, DI_PIN);//设置为输入模式
}



void relay_init(void)
{
    rcu_periph_clock_enable(RELAY_RAY1_RCU);//使能时钟

    gpio_mode_set(RELAY_RAY1_PORT,GPIO_MODE_OUTPUT,GPIO_PUPD_NONE,RELAY_RAY1_PIN);//设置为推挽输出
    gpio_mode_set(RELAY_RAY2_PORT,GPIO_MODE_OUTPUT,GPIO_PUPD_NONE,RELAY_RAY2_PIN);

    gpio_output_options_set(RELAY_RAY1_PORT,GPIO_OTYPE_PP,GPIO_OSPEED_50MHZ,RELAY_RAY1_PIN);//设置输出速度为50MHz
    gpio_output_options_set(RELAY_RAY2_PORT,GPIO_OTYPE_PP,GPIO_OSPEED_50MHZ,RELAY_RAY2_PIN);

    gpio_bit_set(RELAY_RAY1_PORT,RELAY_RAY1_PIN);//输出高电平
    gpio_bit_set(RELAY_RAY2_PORT,RELAY_RAY2_PIN);
}

void debug_uart_init(void)
{
    rcu_periph_clock_enable(DEBUG_UART_RCU);//使能GPIOD时钟

    rcu_periph_clock_enable(DEBUG_UART_RCU_USART);

    gpio_af_set(DEBUG_UART_PORT, DEBUG_AF, DEBUG_UART_TX_PIN);//设置复用为USART1
    gpio_af_set(DEBUG_UART_PORT, DEBUG_AF, DEBUG_UART_RX_PIN);


    gpio_mode_set(DEBUG_UART_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP,DEBUG_UART_TX_PIN);//设置为复用模式
    gpio_output_options_set(DEBUG_UART_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,DEBUG_UART_TX_PIN);//设置输出速度为50MHz

    gpio_mode_set(DEBUG_UART_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP,DEBUG_UART_RX_PIN);
    gpio_output_options_set(DEBUG_UART_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,DEBUG_UART_RX_PIN);

    usart_deinit(DEBUG_UART);//复位串口
    usart_baudrate_set(DEBUG_UART,115200U);//设置波特率为115200
    usart_receive_config(DEBUG_UART, USART_RECEIVE_ENABLE);//使能接收
    usart_transmit_config(DEBUG_UART, USART_TRANSMIT_ENABLE);//使能发送
    usart_enable(DEBUG_UART);//使能串口
}
