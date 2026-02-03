#include "gd32f4xx.h"
#include "systick.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "Q468V1.h"
#include "sd2505.h"
#include "fm24cxx.h"
#include "BSP_EMMC.h"
#include "BSP_TIMER.h"
#include "BSP_GD55B01.h"
#include "BSP_FM24C02.h"
#include "FreeRTOS.h"
#include "app_task.h"
#include "sys_arch.h"

#include "SEGGER_RTT.h"
#include "rtt_log.h"


/*!
    \brief      configures the nested vectored interrupt controller
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void nvic_configuration(void)
{
    nvic_priority_group_set(NVIC_PRIGROUP_PRE2_SUB2);
    nvic_vector_table_set(NVIC_VECTTAB_FLASH, 0);
    rcu_periph_clock_enable(RCU_SYSCFG);
    nvic_irq_enable(ENET_IRQn, 3, 0);  /* 以太网中断优先级：在PRE2_SUB2模式下，抢占优先级范围是0-3，使用3确保较高优先级 */
                                       /* 注意：YT8522H使用PRE4_SUB0模式，优先级5是有效的，但这里使用PRE2_SUB2，所以用3 */
    nvic_irq_enable(TIMER0_UP_TIMER9_IRQn, 1, 0);  // 配置NVIC：优先级2，子优先级0
    nvic_irq_enable(SDIO_IRQn, 2, 0);
    nvic_irq_enable(DMA1_Channel6_IRQn, 2, 1);
}


int main(void)
{								
    systick_config();    // system clocks configuration	
    
    led_init();          // 状态指示灯初始化
    key_init();          // 按键初始化
    #if 0
    debug_uart_init();
    #else
    SEGGER_RTT_ConfigUpBuffer(0, NULL, NULL, 0, SEGGER_RTT_MODE_NO_BLOCK_SKIP);
    APP_PRINTF("RTT Config OK!\r\n");
    #endif
    relay_init();        // 继电器初始化    
    nvic_configuration(); 
    emmcdriveinit();     // EMMC初始化
    sd2505_init();       // 时钟芯片初始化
	gd55b01ge_init();    // FLASH芯片初始化
    ds18b20_init();      // 温度传感器初始化
	fm24c_init();        // EEPROM初始化
	cm2248_init();       // AD芯片初始化
    lwip_stack_init();   // initilaize the LwIP stack
    timer0_init();       // TIME0初始化
    app_task();
}

/* retarget the C library printf function to the USART */
int fputc(int ch, FILE *f)
{
    usart_data_transmit(USART1, (uint8_t) ch);
    while (RESET == usart_flag_get(USART1,USART_FLAG_TBE));
    return ch;
}

