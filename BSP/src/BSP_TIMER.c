#include "gd32f4xx.h"
#include "BSP_TIMER.h"


void timer0_init(void)
{
    timer_parameter_struct timer_initpara;

    rcu_periph_clock_enable(RCU_TIMER0);
    rcu_timer_clock_prescaler_config(RCU_TIMER_PSC_MUL2); // 定时器时钟

    timer_deinit(TIMER0);

    timer_initpara.prescaler         = 240 - 1;          // 预分频值
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE; // 边沿对齐
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;   // 向上计数
    timer_initpara.period            = 1000 - 1;         // 自动重装值
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;   // 时钟不分频
    timer_initpara.repetitioncounter = 0U;                 // 无重复计数
    timer_init(TIMER0, &timer_initpara);

    timer_flag_clear(TIMER0, TIMER_FLAG_UP);
    timer_interrupt_enable(TIMER0, TIMER_INT_UP); // 使能更新（溢出）中断

    // timer_enable(TIMER0);
}

void timer0_start(void)
{
    timer_enable(TIMER0);
}

__weak void timer0_irq_handler_cb(void)
{


}
/* -------------------------- TIMER0中断服务函数 -------------------------- */
void TIMER0_UP_TIMER9_IRQHandler(void)
{
    if(timer_interrupt_flag_get(TIMER0, TIMER_INT_FLAG_UP) != RESET)
    {
        timer_interrupt_flag_clear(TIMER0, TIMER_INT_FLAG_UP);
        timer0_irq_handler_cb();
    }
}


