#ifndef __UTILS_H
#define __UTILS_H


#include <stdint.h>
#include "semphr.h"

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#define MAX_QUEUE_SIZE  10
#define MAX_QUEUE_DATA_SIZE  128
typedef struct
{
    uint8_t ucFront;
    uint8_t ucRear;
    uint8_t ucBuf[MAX_QUEUE_SIZE][MAX_QUEUE_DATA_SIZE];
    SemaphoreHandle_t mutex;
} ThreadSafeCirQue;

typedef struct
{
    uint8_t ucYear;
    uint8_t ucMonth;
    uint8_t ucDay;
    uint8_t ucHour;
    uint8_t ucMin;
    uint8_t ucScd;
} __attribute__((packed)) SystemTime_t;

typedef union
{
    double d;
    uint64_t u;
} DoubleUInt64_t;

typedef union
{
    float f;
    uint32_t u;
} FloatUInt32_t;

uint8_t initCirQueue(ThreadSafeCirQue *que);
uint8_t enQueue(ThreadSafeCirQue *q, uint8_t *buf, uint16_t len);
uint8_t deQueue(ThreadSafeCirQue *q, uint8_t *buf);

uint32_t datetime_to_timestamp(uint16_t year, uint8_t month, uint8_t day, 
                               uint8_t hour, uint8_t min, uint8_t sec);
void timestamp_to_datetime(uint32_t timestamp, SystemTime_t *time);

uint8_t bcd_to_dec(uint8_t bcd_val);
uint8_t dec_to_bcd(uint8_t dec_val);

#endif /* __UTILS_H */

