/**
 ****************************************************************************************************
 * @file        utils.c
 * @author      xxxxxx
 * @version     V1.0
 * @date        2025-12-31
 * @brief       公用功能模块
 ****************************************************************************************************
 * @attention
 *
 * 项目:LSC100
 *
 * 修改说明
 * V1.0 20251231
 * 第一次发布
 *
 ****************************************************************************************************
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "utils.h"

/**
 * @brief       初始化循环队列
 * @note        None
 * @param       CanMsgCirQue *q 循环队列结构体指针
 * @retval      TRUE  : 成功
                FALSE : 失败;
 */
uint8_t initCirQueue(ThreadSafeCirQue *que)
{
    if (que == NULL)
    {
        return FALSE;
    }

    que->ucRear = 0;
    que->ucFront = 0;
    memset(que->ucBuf, 0x00, sizeof(MAX_QUEUE_SIZE * MAX_QUEUE_DATA_SIZE));
    que->mutex = xSemaphoreCreateMutex();
    return TRUE;
}

/**
 * @brief       判断循环队列是否为满
 * @note        None
 * @param       CanMsgCirQue *q 循环队列结构体指针
 * @retval      TRUE  : 队列已满
                FALSE : 队列未满
 */
static uint8_t isQueueFull(ThreadSafeCirQue *q)
{
    if ((q->ucRear + 1) % MAX_QUEUE_SIZE == q->ucFront)
    {
        return TRUE;
    }

    return FALSE;
}

/**
 * @brief       判断循环队列是否为空
 * @note        None
 * @param       CanMsgCirQue *q 循环队列结构体指针
 * @retval      TRUE  : 队列已空
                FALSE : 队列未空
 */
static uint8_t isQueueEmpty(ThreadSafeCirQue *q)
{
    if (q->ucFront == q->ucRear)
    {
        return TRUE;
    }

    return FALSE;
}

/**
 * @brief       循环队列入队
 * @note        None
 * @param       CanMsgCirQue *q  循环队列结构体指针
 * @param       uint8_t *buf 需要入队的数组指针
 * @param       uint8_t len 需要入队的数组长度
 * @retval      TRUE  : 入队成功
                FALSE : 入队失败
 */
uint8_t enQueue(ThreadSafeCirQue *q, uint8_t *buf, uint16_t len)
{
    xSemaphoreTake(q->mutex, portMAX_DELAY);
    if (TRUE == isQueueFull(q))
    {
        xSemaphoreGive(q->mutex);
        return FALSE;
    }

    memcpy(q->ucBuf[q->ucRear], buf, len);
    q->ucRear = (q->ucRear + 1) % MAX_QUEUE_SIZE;
    xSemaphoreGive(q->mutex);
    
    return TRUE;
}

/**
 * @brief       循环队列出队
 * @note        None
 * @param       CanMsgCirQue *q  循环队列结构体指针
 * @param       uint8_t *buf 需要出队的数组指针
 * @retval      TRUE  : 出队成功
                FALSE : 出队失败
 */
uint8_t deQueue(ThreadSafeCirQue *q, uint8_t *buf)
{
    xSemaphoreTake(q->mutex, portMAX_DELAY);
    if (TRUE == isQueueEmpty(q))
    {
        xSemaphoreGive(q->mutex);
        return FALSE;
    }
    
    memcpy((void*)buf, q->ucBuf[q->ucFront], MAX_QUEUE_DATA_SIZE);
    memset(q->ucBuf[q->ucFront], 0x00, MAX_QUEUE_DATA_SIZE);
    q->ucFront = (q->ucFront + 1) % MAX_QUEUE_SIZE;
    xSemaphoreGive(q->mutex);
    
    return TRUE;
}

/**
 * @brief 判断是否为闰年
 * @param year 年份（如2025）
 * @return true=闰年，false=平年
 */
static bool is_leap_year(uint16_t year)
{
    // 闰年规则：能被4整除但不能被100整除，或能被400整除
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
        return true;
    }
    return false;
}

/**
 * @brief 获取指定年份的总天数
 * @param year 年份
 * @return 全年天数（365/366）
 */
static uint16_t get_days_of_year(uint16_t year)
{
    return is_leap_year(year) ? 366 : 365;
}

/**
 * @brief 获取指定年月的天数
 * @param year 年份
 * @param month 月份（1-12）
 * @return 当月天数
 */
static uint8_t get_days_of_month(uint16_t year, uint8_t month)
{
    const uint8_t month_days[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    
    if (month == 2 && is_leap_year(year)) {
        return 29; 
    }
    return month_days[month - 1]; // 月份转索引（1→0）
}

/**
 * @brief 年月日时分秒转Unix时间戳（32位秒级）
 * @param year 年（范围：1970~2106，超出32位上限会溢出）
 * @param month 月（1-12）
 * @param day 日（1-31）
 * @param hour 时（0-23）
 * @param min 分（0-59）
 * @param sec 秒（0-59）
 * @return 32位Unix时间戳（秒），溢出返回0
 */
uint32_t datetime_to_timestamp(uint16_t year, uint8_t month, uint8_t day, 
                               uint8_t hour, uint8_t min, uint8_t sec)
{
    if (year < 1970 || month < 1 || month > 12 || day < 1 || day > 31 ||
        hour > 23 || min > 59 || sec > 59) {
        return 0;
    }
    
    uint32_t total_days = 0;
    for (uint16_t y = 1970; y < year; y++) {
        total_days += get_days_of_year(y);
    }
    
    for (uint8_t m = 1; m < month; m++) {
        total_days += get_days_of_month(year, m);
    }
    
    total_days += (day - 1);
    
    uint32_t timestamp = total_days * 86400UL; 
    timestamp += (uint32_t)hour * 3600UL;     
    timestamp += (uint32_t)min * 60UL;        
    timestamp += sec;                         
    
    if (timestamp < (uint32_t)hour * 3600UL) {
        return 0;
    }
    
    return timestamp;
}

// 月份天数表（平年）
static const uint8_t days_in_month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// 判断是否为闰年
#define IS_LEAP_YEAR(y) (((y) % 4 == 0 && (y) % 100 != 0) || ((y) % 400 == 0))

/**
 * @brief 将秒级时间戳转换为年月日时分秒
 * @param timestamp 秒级时间戳（从1970-01-01 00:00:00 UTC开始）
 * @param time  输出参数
 */
void timestamp_to_datetime(uint32_t timestamp, SystemTime_t *time)
{
    uint32_t days, secs_of_day;
    uint16_t y = 1970;
    uint8_t m;

    days = timestamp / 86400;          // 总天数
    secs_of_day = timestamp % 86400;   // 当天的秒数

    time->ucHour = secs_of_day / 3600;
    time->ucMin = (secs_of_day % 3600) / 60;
    time->ucScd = secs_of_day % 60;

    while (days >= (IS_LEAP_YEAR(y) ? 366 : 365)) {
        days -= IS_LEAP_YEAR(y) ? 366 : 365;
        y++;
    }
    time->ucYear = y - 2000;

    for (m = 1; m <= 12; m++) {
        uint8_t days_in_this_month = days_in_month[m - 1];

        if (m == 2 && IS_LEAP_YEAR(y)) {
            days_in_this_month = 29;
        }

        if (days < days_in_this_month) {
            break;
        }
        days -= days_in_this_month;
    }
    time->ucMonth = m;
    time->ucDay = days + 1;  // days从0开始，需要+1
}

/**
 * @brief BCD码数据转十进制数据
 * @param bcd_val BCD码数据
 * @param 十进制数据
 */
uint8_t bcd_to_dec(uint8_t bcd_val)
{
    return ((bcd_val >> 4) * 10) + (bcd_val & 0x0F);
}

/**
 * @brief 十进制数据转BCD码数据
 * @param dec_val 十进制数据
 * @param BCD码数据
 */
uint8_t dec_to_bcd(uint8_t dec_val)
{
    return (((dec_val / 10) & 0x0F) << 4) | (dec_val % 10);
}


