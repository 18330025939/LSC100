#ifndef __BSP_ONEWIRE_H
#define __BSP_ONEWIRE_H

#include "gd32f4xx.h"
#include <stdbool.h>
#include "systick.h"

// 单总线时序参数（us）
#define ONEWIRE_RESET_PULSE     750
#define ONEWIRE_PRESENCE_PULSE  70
#define ONEWIRE_BIT_DELAY       70
#define ONEWIRE_WRITE_1_DELAY   2
#define ONEWIRE_WRITE_0_DELAY   60
#define ONEWIRE_READ_LOW        2
#define ONEWIRE_READ_SAMPLE     12

#define Drv_DelayUs(x)   delay_us(x) 

typedef struct {
    rcu_periph_enum rcu;
    uint32_t port;
    uint32_t pin;
} Platform_OneWire_t;

bool onewire_reset(Platform_OneWire_t* self);
uint8_t onewire_check(Platform_OneWire_t* self);
void onewire_write_byte(Platform_OneWire_t* self, uint8_t byte);
uint8_t onewire_read_byte(Platform_OneWire_t* self);
bool onewire_init(Platform_OneWire_t* self);

#endif /* __BSP_ONEWIRE_H */
