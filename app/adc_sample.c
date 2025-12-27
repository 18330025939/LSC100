#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "BSP_CM2248.h"
#include "task.h"
#include "semphr.h"
#include "BSP_TIMER.h"
#include "BSP_EMMC.h"
#include "Q468V1.h"
#include "BSP_GD55B01.h"
#include "msg_parse.h"
#include "sd2505.h"
#include "storage_manage.h"
#include "adc_sample.h"

#include "rtt_log.h"


volatile uint64_t g_sample_tick = 0ULL;

TimeSync_t g_TimeSync = {0};
AlarmNotify_t g_AlarmNotify = {0};
AdcItemCache_t g_AdcItemCache = {0};

TaskHandle_t SampleTask_Handle;
TaskHandle_t TimeSyncTask_Handle;

static void Adc_Save_Alarm(void);
static void Adc_Clear_Alarm(uint16_t raw_data);

/**
 * @brief  初始化ADC环形缓存
 * @param  cache: 指向环形缓存结构体的指针
 * @retval 0-成功，1-失败（互斥锁创建失败）
 */
uint8_t Adc_Cache_Init(AdcItemCache_t *cache)
{
    if (cache == NULL) {
        return 2; 
    }

    cache->wr_idx = 0;
    cache->rd_idx = 0;
    cache->count = 0;

    cache->mutex = xSemaphoreCreateMutex();
    if (cache->mutex == NULL) {
        return 1; 
    }

    return 0;
}

/**
 * @brief  写入单条数据到环形缓存
 * @param  cache: 指向环形缓存结构体的指针
 * @param  data: 待写入的单条采样数据
 * @retval 0-成功，1-缓存满，2-参数无效
 */
uint8_t Adc_Cache_Write_One(AdcItemCache_t *cache, const AdcItem_t *data)
{
    if (cache == NULL || data == NULL) {
        return 2; 
    }

    if (xSemaphoreTake(cache->mutex, pdMS_TO_TICKS(10)) != pdPASS) {
        return 3; 
    }

    uint8_t ret = 0;

    if (cache->count >= sizeof(cache->item) / sizeof(AdcItem_t)) {
        ret = 1; 
    } else {
        memcpy(&cache->item[cache->wr_idx], data, sizeof(AdcItem_t));
        cache->wr_idx = (cache->wr_idx + 1) % (sizeof(cache->item) / sizeof(AdcItem_t));
        cache->count++;
        ret = 0;
    }

    xSemaphoreGive(cache->mutex);
    return ret;
}

/**
 * @brief  批量写入数据到环形缓存
 * @param  cache: 指向环形缓存结构体的指针
 * @param  data: 待写入的采样数据数组
 * @param  len: 待写入的数据条数
 * @retval 实际写入的条数（0表示全部失败，≤len）
 */
uint16_t Adc_Cache_Write_Batch(AdcItemCache_t *cache, const AdcItem_t *data, uint16_t len) 
{
    if (cache == NULL || data == NULL || len == 0) {
        return 0;
    }

    if (xSemaphoreTake(cache->mutex, pdMS_TO_TICKS(10)) != pdPASS) {
        return 0; 
    }

    uint16_t cache_max = sizeof(cache->item) / sizeof(AdcItem_t);
    uint16_t free_space = cache_max - cache->count;
    uint16_t write_len = (len < free_space) ? len : free_space; 
    uint16_t written = 0;

    if (write_len > 0) {
        uint16_t len1 = cache_max - cache->wr_idx;
        len1 = (len1 < write_len) ? len1 : write_len;

        memcpy(&cache->item[cache->wr_idx], data, len1 * sizeof(AdcItem_t));
        written += len1;

        if (written < write_len) {
            memcpy(&cache->item[0], &data[len1], (write_len - len1) * sizeof(AdcItem_t));
            written = write_len;
        }

        cache->wr_idx = (cache->wr_idx + write_len) % cache_max;
        cache->count += write_len;
    }

    xSemaphoreGive(cache->mutex);
    return written;
}

/**
 * @brief  读取单条数据从环形缓存（读取后从缓存移除）
 * @param  cache: 指向环形缓存结构体的指针
 * @param  data: 存储读取数据的缓冲区
 * @retval 0-成功，1-缓存空，2-参数无效
 */
uint8_t Adc_Cache_Read_One(AdcItemCache_t *cache, AdcItem_t *data)
{
    if (cache == NULL || data == NULL) {
        return 2;
    }

    if (xSemaphoreTake(cache->mutex, pdMS_TO_TICKS(10)) != pdPASS) {
        return 3;
    }

    uint8_t ret = 0;
    if (cache->count == 0) {
        ret = 1;
    } else {
        memcpy(data, &cache->item[cache->rd_idx], sizeof(AdcItem_t));
        cache->rd_idx = (cache->rd_idx + 1) % (sizeof(cache->item) / sizeof(AdcItem_t));
        cache->count--;
        ret = 0;
    }

    xSemaphoreGive(cache->mutex);
    return ret;
}

/**
 * @brief  批量读取数据从环形缓存（读取后从缓存移除）
 * @param  cache: 指向环形缓存结构体的指针
 * @param  data: 存储读取数据的缓冲区
 * @param  max_len: 缓冲区最大可接收的条数
 * @retval 实际读取的条数（0表示缓存空）
 */
uint16_t Adc_Cache_Read_Batch(AdcItemCache_t *cache, AdcItem_t *data, uint16_t max_len)
{
    if (cache == NULL || data == NULL || max_len == 0) {
        return 0;
    }

    if (xSemaphoreTake(cache->mutex, pdMS_TO_TICKS(10)) != pdPASS) {
        return 0; 
    }

    uint16_t read_len = (cache->count < max_len) ? cache->count : max_len;
    uint16_t read = 0;
    uint16_t cache_max = sizeof(cache->item) / sizeof(AdcItem_t);

    if (read_len > 0) {
        uint16_t len1 = cache_max - cache->rd_idx;
        len1 = (len1 < read_len) ? len1 : read_len;

        memcpy(data, &cache->item[cache->rd_idx], len1 * sizeof(AdcItem_t));
        read += len1;

        if (read < read_len) {
            memcpy(&data[len1], &cache->item[0], (read_len - len1) * sizeof(AdcItem_t));
            read = read_len;
        }

        cache->rd_idx = (cache->rd_idx + read_len) % cache_max;
        cache->count -= read_len;
    }

    xSemaphoreGive(cache->mutex);
    return read;
}

/**
 * @brief  查询环形缓存当前数据量
 * @param  cache: 指向环形缓存结构体的指针
 * @retval 当前缓存数据条数（0~2048）
 */
uint16_t Adc_Cache_Get_Count(AdcItemCache_t *cache) 
{
    if (cache == NULL) {
        return 0;
    }

    uint16_t count = 0;
    if (xSemaphoreTake(cache->mutex, pdMS_TO_TICKS(10)) == pdPASS) {
        count = cache->count;
        xSemaphoreGive(cache->mutex);
    }
    return count;
}

/**
 * @brief  清空环形缓存
 * @param  cache: 指向环形缓存结构体的指针
 * @retval 0-成功，1-参数无效
 */
uint8_t Adc_Cache_Clear(AdcItemCache_t *cache) 
{
    if (cache == NULL) {
        return 1; 
    }

    if (xSemaphoreTake(cache->mutex, pdMS_TO_TICKS(10)) != pdPASS) {
        return 2; 
    }

    cache->wr_idx = 0;
    cache->rd_idx = 0;
    cache->count = 0;

    xSemaphoreGive(cache->mutex);
    return 0;
}

/**
 * @brief  适配秒级RTC的绝对时间戳计算函数
 * @param  curr_tick: 当前采样计数值（g_sample_tick）
 * @retval 64位毫秒级时间戳
 */
uint64_t TimeSync_Get_Absolute_Time(uint64_t curr_tick, SystemTime_t *time)
{
    uint64_t abs_time = 0;
    // uint16_t total_ms_offset = 0;
    // uint8_t add_sec = 0;
    // uint16_t final_ms = 0;
    // uint64_t tick_diff = 0;

    // if(g_TimeSync.sync_valid == 0) {
    //     return (uint64_t)curr_tick; // 未同步时返回相对时间
    // }

    if(xSemaphoreTake(g_TimeSync.mutex, pdMS_TO_TICKS(10)) == pdPASS) {
        memcpy(time, &g_TimeSync.sys_time, sizeof(SystemTime_t));
        #if 0
        // tick_diff = curr_tick - g_TimeSync.tick_base; // 1. 计算从同步后到当前的毫秒差
        total_ms_offset = g_TimeSync.ms_offset + (curr_tick - g_TimeSync.tick_base);  // 2. 总毫秒偏移 = 同步时的毫秒偏移 + 采样差
        add_sec = total_ms_offset / 1000;         // 3. 计算需要追加的秒数（跨秒处理）
        final_ms = total_ms_offset % 1000;         // 4. 最终毫秒部分（0~999）
        abs_time = (uint64_t)(g_TimeSync.rtc_base_sec  + add_sec);
        abs_time = abs_time * 1000 + final_ms; // 5. 拼接最终毫秒级时间戳
        #else
        abs_time = (uint64_t)(g_TimeSync.rtc_base_sec) * 1000;
        abs_time += curr_tick % 1000;
        #endif
        xSemaphoreGive(g_TimeSync.mutex);
    }

    return abs_time;
}

/**
 * @brief  时间同步任务（适配秒级RTC）
 */
void time_sync_task(void *pvParameters)
{
    rtc_time time;
    //uint64_t tick = 0;
    TickType_t xLastWakeTime = xTaskGetTickCount(); 
    
    memset(&g_TimeSync, 0 , sizeof(TimeSync_t));
    g_TimeSync.mutex = xSemaphoreCreateMutex();
    if (g_TimeSync.mutex == NULL) {
        return;
    }

    // sd2505_get_time(&time);
    // g_TimeSync.flag = 0;
    // g_TimeSync.sys_time.ucYear = time.ucYear;
    // g_TimeSync.sys_time.ucMonth = time.ucMonth;
    // g_TimeSync.sys_time.ucDay = time.ucDay;
    // g_TimeSync.sys_time.ucHour = time.ucHour;
    // g_TimeSync.sys_time.ucMin = time.ucMinute;
    // g_TimeSync.sys_time.ucScd = time.ucSecond;
    // g_TimeSync.rtc_base_sec = datetime_to_timestamp(bcd_to_dec(g_TimeSync.sys_time.ucYear) + 2000,
    //                                                 bcd_to_dec(g_TimeSync.sys_time.ucMonth),
    //                                                 bcd_to_dec(g_TimeSync.sys_time.ucDay),
    //                                                 bcd_to_dec(g_TimeSync.sys_time.ucHour),
    //                                                 bcd_to_dec(g_TimeSync.sys_time.ucMin),
    //                                                 bcd_to_dec(g_TimeSync.sys_time.ucScd));

    while(1) {
        #if 1
        if(xSemaphoreTake(g_TimeSync.mutex, pdMS_TO_TICKS(10)) == pdPASS) {
            #if 1
            sd2505_get_time(&time);
            g_TimeSync.sys_time.ucYear = time.ucYear;
            g_TimeSync.sys_time.ucMonth = time.ucMonth;
            g_TimeSync.sys_time.ucDay = time.ucDay;
            g_TimeSync.sys_time.ucHour = time.ucHour;
            g_TimeSync.sys_time.ucMin = time.ucMinute;
            g_TimeSync.sys_time.ucScd = time.ucSecond;
            #endif
            // if (g_TimeSync.flag) {
            g_TimeSync.rtc_base_sec = datetime_to_timestamp(bcd_to_dec(g_TimeSync.sys_time.ucYear) + 2000,
                                                        bcd_to_dec(g_TimeSync.sys_time.ucMonth),
                                                        bcd_to_dec(g_TimeSync.sys_time.ucDay),
                                                        bcd_to_dec(g_TimeSync.sys_time.ucHour),
                                                        bcd_to_dec(g_TimeSync.sys_time.ucMin),
                                                        bcd_to_dec(g_TimeSync.sys_time.ucScd));
            //     g_TimeSync.flag = 0;                                       
            // } else {
            //     g_TimeSync.rtc_base_sec += 1;
            // }
            
            #if 0
            g_TimeSync.ms_offset = g_sample_tick % 1000; // 2. 计算同步时刻的毫秒偏移（tick取模1000）
            g_TimeSync.tick_base = g_sample_tick; // 3. 记录同步时的采样计数值
            // g_TimeSync.sync_valid = 1; // 4. 标记同步有效
            #endif
            xSemaphoreGive(g_TimeSync.mutex);
        }
        #endif
        // APP_PRINTF("time_sync_task, time:%x-%x-%x %x:%x:%x, g_TimeSync.rtc_base_sec:%lu, g_TimeSync.tick_base:%lu\r\n", 
        //                             g_TimeSync.sys_time.ucYear, g_TimeSync.sys_time.ucMonth, g_TimeSync.sys_time.ucDay,
        //                             g_TimeSync.sys_time.ucHour, g_TimeSync.sys_time.ucMin, g_TimeSync.sys_time.ucScd,
        //                             g_TimeSync.rtc_base_sec, g_TimeSync.tick_base);
        RUN_LED_TOGLE();

        if (g_AlarmNotify.save_sta == ALARM_STA_STR_SAVE) {
            Adc_Save_Alarm();
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000));
    }
}

void timer0_irq_handler_cb(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    g_sample_tick += 1;

    if(SampleTask_Handle != NULL)
    {
        vTaskNotifyGiveFromISR(SampleTask_Handle, &xHigherPriorityTaskWoken);
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/**
 * @brief       报警通知结构体初始化
 * @param       无
 * @retval      1 失败,0 成功
 */
uint8_t Alarm_Notify_Init(AlarmNotify_t *notify)
{
    if (notify == NULL) {
        return 1;
    }

    notify->mutex = xSemaphoreCreateMutex();
    if (notify->mutex == NULL) {
        return 1; 
    }

    notify->alarm_sem = xSemaphoreCreateBinary();
    if(notify->alarm_sem == NULL) {
        return 1;
    }

    notify->alarm_queue = xQueueCreate(5, sizeof(AlarmMsg_t));
    if(notify->alarm_queue == NULL) {
        vSemaphoreDelete(notify->alarm_sem);
        return 1;
    }

    return 0;
}

/**
 * @brief       ADC数据报警检测
 * @param       item: 指向ADC对象结构体的指针
 * @param       ts: 当前ms级时间戳
 * @retval      无
 */
static void Adc_Check_Alarm(AdcItem_t *item)
{
    uint8_t flag = 0;
    AlarmMsg_t alarm_msg = {0};
    float r_v = 0.0f;
    float f_v = 0.0f;

    if (item->data[SENSOR_FRONT_CURRENT].f > BUS_CURRENT_IDLE_THRESHOLD) {
        f_v = CALC_BUS_CURRENT_LOW_THRESHOLD(item->data[SENSOR_FRONT_VOLTAGE].f, g_ConfigInfo.f_Vmin.f);
        f_v = CALC_BUS_CURRENT_UP_THRESHOLD(item->data[SENSOR_FRONT_VOLTAGE].f, g_ConfigInfo.f_Vmax.f);
        if (item->data[SENSOR_FRONT_CURRENT].f > f_v) { 
            flag |= 1 << 0;
        }
    }
    if (item->data[SNESOR_REAR_CURRENT].f > BUS_CURRENT_IDLE_THRESHOLD) {
        r_v = CALC_BUS_CURRENT_LOW_THRESHOLD(item->data[SNESOR_REAR_VOLTAGE].f, g_ConfigInfo.r_Vmin.f);
        r_v = CALC_BUS_CURRENT_UP_THRESHOLD(item->data[SNESOR_REAR_VOLTAGE].f, g_ConfigInfo.r_Vmax.f);
        if (item->data[SNESOR_REAR_CURRENT].f > r_v) {
            flag |= 1 << 1;
        }
    }
    // APP_PRINTF("Adc_Check_Alarm, r_c:%f, r_Vmin;%f, r_Vmax;%f, ts:%llu\r\n", item->data[SNESOR_REAR_CURRENT].f, r_Vmin, r_Vmax, item->ts);
    if (flag) {
        if (xSemaphoreTake(g_AlarmNotify.mutex, pdMS_TO_TICKS(10)) != pdPASS) {
            return ; 
        }   
        alarm_msg.type = flag == 1 ? BUS_ALARM_FRONT_VOL : (flag == 2 ? BUS_ALARM_REAR_VOL : BUS_ALARM_FRONT_REAR_VOL);
        if (alarm_msg.type == g_AlarmNotify.type) {
            xSemaphoreGive(g_AlarmNotify.mutex);
            return;
        }
        memcpy(&alarm_msg.time, &item->time, sizeof(SystemTime_t));
        alarm_msg.idx = item->ts % 1000;
        alarm_msg.ts = item->ts / 1000;

        g_AlarmNotify.alarm_sta = ALARM_STA_RAISED;
        g_AlarmNotify.save_sta = ALARM_STA_STR_SAVE;
        g_AlarmNotify.type = alarm_msg.type;
        g_AlarmNotify.alarm_ts = alarm_msg.ts - 1;
        g_AlarmNotify.index = 0;

        BaseType_t ret = xQueueSendToBack(g_AlarmNotify.alarm_queue, &alarm_msg, 0);
        if (ret != pdPASS) {
            xSemaphoreGive(g_AlarmNotify.mutex);
            return;
        }
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(g_AlarmNotify.alarm_sem, &xHigherPriorityTaskWoken);

        xSemaphoreGive(g_AlarmNotify.mutex);
    }
}

/**
 * @brief       消除报警
 * @param       无
 * @retval      无
 */
static void Adc_Clear_Alarm(uint16_t raw_data)
{
    float adc = 0.0f;

    adc = ((float)raw_data / 65536.0f) * 10.0f;
    if (0 == IS_CLEANR_ALARM(adc)) {
        return ;
    }

    if (xSemaphoreTake(g_AlarmNotify.mutex, pdMS_TO_TICKS(10)) != pdPASS) {
        return ; 
    }   

    if (g_AlarmNotify.alarm_sta == ALARM_STA_RAISED) {
        g_AlarmNotify.alarm_sta = ALARM_STA_CLEARED;
        ALARM1_RECOVERY();
        ALARM_LED_OFF();
    }

    xSemaphoreGive(g_AlarmNotify.mutex);
}

/**
 * @brief       消除报警
 * @param       无
 * @retval      无
 */
static void Adc_Save_Alarm(void)
{
    AlarmMsg_t alarm_msg = {0};

    if (xSemaphoreTake(g_AlarmNotify.mutex, pdMS_TO_TICKS(10)) != pdPASS) {
        return ; 
    }  
    
    alarm_msg.type = BUS_ALARM_SAVE;
    alarm_msg.ts = g_AlarmNotify.alarm_ts + g_AlarmNotify.index;

    BaseType_t ret = xQueueSendToBack(g_AlarmNotify.alarm_queue, &alarm_msg, 0);
    if (ret != pdPASS) {
        xSemaphoreGive(g_AlarmNotify.mutex);
        return;
    }

    g_AlarmNotify.index += 1;
    if (g_AlarmNotify.index >= 3 && g_AlarmNotify.save_sta == ALARM_STA_STR_SAVE) {
        g_AlarmNotify.type = BUS_ALARM_NONE;
        g_AlarmNotify.save_sta = ALARM_STA_END_SAVE;
    }
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(g_AlarmNotify.alarm_sem, &xHigherPriorityTaskWoken);

    xSemaphoreGive(g_AlarmNotify.mutex);
}

/**
 * @brief  ADC采样任务（1kHz触发）
 */
void sample_task(void *pvParameters)
{
    uint32_t notify_value = 0;
    AdcItem_t item;
    uint8_t i = 0;
    float sensor_data = 0.0f;
    uint16_t raw_adc[4] = {0};

    timer0_start();
    
    while (1)
    {
        notify_value = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        if(notify_value > 0)
        {
            // taskENTER_CRITICAL();// 关调度器，保证时序不被打断
            for (i = 0; i < ADC_SAMPLE_CHANNEL; i++) {
                item.raw_data[i] = cm2248_start_read_data();
                // cm2248_read_both_channels(&item.raw_data[i], &raw_adc[i]);
            }
            cm2248_start_conv();
            // taskEXIT_CRITICAL(); // 开调度器
            item.ts = TimeSync_Get_Absolute_Time(g_sample_tick, &item.time);
            for (i = 0; i < ADC_SAMPLE_CHANNEL; i ++) {
                sensor_data = ((float)item.raw_data[i] / 65536.0f) * 10.0f;//65535.0f) * (2.0f * 10.0f) - 10.0f;// 32768.0f) * 10.0f;
                if (i % 2 == 0) {
                    item.data[i].f = CONVERT_SENSOR_TO_BUS_CURRENT(sensor_data);
                } else {
                    item.data[i].f = CONVERT_SENSOR_TO_BUS_VOLTAGE(sensor_data);
                }
            }
            if (item.data[SENSOR_FRONT_CURRENT].f < BUS_CURRENT_IDLE_THRESHOLD && item.data[SENSOR_FRONT_VOLTAGE].f < BUS_VOLTAGE_IDLE_THRESHOLD &&
                item.data[SNESOR_REAR_CURRENT].f < BUS_CURRENT_IDLE_THRESHOLD && item.data[SNESOR_REAR_VOLTAGE].f < BUS_VOLTAGE_IDLE_THRESHOLD) {
                continue;
            }
            // APP_PRINTF("sample_task, f_c:%lf, f_v:%lf, r_c:%lf, r_v:%lf\r\n", item.data[SENSOR_FRONT_CURRENT].f, item.data[SENSOR_FRONT_VOLTAGE].f, item.data[SNESOR_REAR_CURRENT].f, item.data[SNESOR_REAR_VOLTAGE].f);
            // if (item.data[SENSOR_FRONT_CURRENT].f < BUS_CURRENT_IDLE_THRESHOLD && item.data[SENSOR_FRONT_VOLTAGE].f < BUS_VOLTAGE_IDLE_THRESHOLD &&
            //     item.data[SNESOR_REAR_CURRENT].f < BUS_CURRENT_IDLE_THRESHOLD && item.data[SNESOR_REAR_VOLTAGE].f < BUS_VOLTAGE_IDLE_THRESHOLD) {
            //     continue;
            // }

            Adc_Cache_Write_One((AdcItemCache_t *)&g_AdcItemCache, (AdcItem_t *)&item);
            Adc_Clear_Alarm(raw_adc[0]);
            Adc_Check_Alarm(&item);
        }
    }
}

void adc_sample_start(void)
{
    Alarm_Notify_Init(&g_AlarmNotify);

    Adc_Cache_Init(&g_AdcItemCache);

    APP_PRINTF("AdcItem_t size:%d, SystemTime_t size:%d\r\n", sizeof(AdcItem_t), sizeof(SystemTime_t));
    
    xTaskCreate((TaskFunction_t)time_sync_task,             // 任务函数
                (const char*   )"time_sync_task",           // 任务名称
                (uint16_t      )TIME_SYNC_STK_SIZE,         // 任务栈大小
                (void*         )NULL,                       // 任务参数
                (UBaseType_t   )TIME_SYNC_TASK_PRIO,        // 任务优先级（低于定时器中断优先级）
                (TaskHandle_t* )&TimeSyncTask_Handle);      // 任务句柄

    // 创建接收定时器通知的任务
    xTaskCreate((TaskFunction_t)sample_task,                // 任务函数
                (const char*   )"sample_task",              // 任务名称
                (uint16_t      )SAMPLE_STK_SIZE,            // 任务栈大小
                (void*         )NULL,                       // 任务参数
                (UBaseType_t   )SAMPLE_TASK_PRIO,           // 任务优先级（低于定时器中断优先级）
                (TaskHandle_t* )&SampleTask_Handle);        // 任务句柄
}
