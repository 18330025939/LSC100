/*!
    \file  systick.c
    \brief the systick configuration file

    \version 2024-12-20, V3.3.1, demo for GD32F4xx
*/

/*
    Copyright (c) 2024, GigaDevice Semiconductor Inc.

    Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this 
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice, 
       this list of conditions and the following disclaimer in the documentation 
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors 
       may be used to endorse or promote products derived from this software without 
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
OF SUCH DAMAGE.
*/

#include "gd32f4xx.h"
#include "systick.h"

volatile static uint32_t delay = 0;
volatile static uint32_t system_tick = 0; /* 系统时间计数器，单位：毫秒 */

/*!
    \brief      configure systick
    \param[in]  none
    \param[out] none
    \retval     none
*/
void systick_config(void)
{
    /* setup systick timer for 1000Hz interrupts */
    if (SysTick_Config(SystemCoreClock /1000U)){
        /* capture error */
        while (1){
        }
    }
    /* configure the systick handler priority */
    NVIC_SetPriority(SysTick_IRQn, 0x00U);
}

/*!
    \brief      delay a time in milliseconds
    \param[in]  count: count in milliseconds
    \param[out] none
    \retval     none
*/
void delay_ms(uint32_t count)
{
    delay = count;

    while(0U != delay){
    }
}
/*!
    \brief      微秒延时函数
    \param[in]  nus: 要延时的微秒数
    \param[out] none
    \retval     none
    \note       纯寄存器空循环
*/
void delay_us(uint32_t nus)
{
    uint32_t ticks;
    uint32_t told, tnow, tcnt = 0;
    uint32_t reload = SysTick->LOAD;
    ticks = nus * (SystemCoreClock / 1000000U);
    told = SysTick->VAL;
    while(1)
    {
        tnow = SysTick->VAL;
        if(tnow != told)
        {
            if(tnow < told)
                tcnt += told - tnow;
            else 
                tcnt += reload - tnow + told;
            told = tnow;
            if(tcnt >= ticks) break;
        }
    }

}


/*!
    \brief      get system tick count
    \param[in]  none
    \param[out] none
    \retval     system tick count in milliseconds
*/
uint32_t get_system_tick(void)
{
    return system_tick;
}

/*!
    \brief      delay decrement
    \param[in]  none
    \param[out] none
    \retval     none
*/
void delay_decrement(void)
{
    if (0U != delay){
        delay--;
    }
    system_tick++; /* 每毫秒递增系统时间计数器 */
}



/*!
    \brief      delay us
    \param[in]  none
    \param[out] none
    \retval     none
*/
void delay_us_nop(uint32_t ms)
{
    const uint32_t per_us = 200;

    while (ms--) {
        volatile uint32_t i = per_us;
        while (i--) {
            __NOP();
        }
    }
}

/*!
    \brief      delay ms
    \param[in]  none
    \param[out] none
    \retval     none
*/
void delay_ms_nop(uint32_t ms)
{
    const uint32_t per_ms = 200000;

    while (ms--) {
        volatile uint32_t i = per_ms;
        while (i--) {
            __NOP();
        }
    }
}



uint32_t gd55_millis(void) { return system_tick; }
