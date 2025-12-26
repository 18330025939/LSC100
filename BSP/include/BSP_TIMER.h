#ifndef __BSP_TIMER_H
#define __BSP_TIMER_H


extern void timer0_irq_handler_cb(void);
void timer0_init(void);
void timer0_start(void);

#endif /* GD32F4XX_TIMER_H */
