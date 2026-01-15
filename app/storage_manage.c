/**
 ****************************************************************************************************
 * @file        stroage_manage.c
 * @author      xxxxxx
 * @version     V1.0
 * @date        2025-12-31
 * @brief       存储管理功能模块
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

#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "Q468V1.h"
#include "BSP_EMMC.h"
#include "adc_sample.h"
#include "storage_manage.h"
#include "systick.h"
#include "msg_parse.h"

#include "rtt_log.h"


AdcObject_t g_AdcObject = {0};
AlarmObject_t g_AlarmObject = {0};
TaskHandle_t StorageTask_Handle; 
TaskHandle_t AlarmTask_Handle; 
static uint8_t g_adc_items[(ADC_ITEM_NUM_PER_SECOND + 100) * ADC_ITEM_SIZE] = {0};
static uint8_t g_adc_data[ADC_ITEM_NUM_PER_SECOND * ADC_ITEM_SIZE] = {0};

int8_t Get_AdcData_Time(AdcObject_t *adc, SystemTime_t *str_time, SystemTime_t *end_time)
{
    if (adc == NULL || str_time == NULL || end_time == NULL) {
        return -1;
    }

    if (xSemaphoreTake(adc->mutex, ADC_LOCK_TIMEOUT) != pdTRUE) {
        return -1;
    }

    timestamp_to_datetime(adc->str_time, str_time);
    timestamp_to_datetime(adc->end_time, end_time);

    xSemaphoreGive(adc->mutex);

    return 0;
}

int8_t Storage_Write_AdcData(AdcObject_t *adc, uint32_t timestamp, uint8_t *data, uint8_t block_num, uint8_t flag)
{
    if (adc == NULL || data == NULL) {
        return -1;
    }

    if (xSemaphoreTake(adc->mutex, ADC_LOCK_TIMEOUT) != pdTRUE) {
        return -1;
    }
    uint32_t addr = adc->addr;
    emmcerasewriteblocks(data, addr, block_num);
    adc->addr = addr + block_num;
    if (adc->addr >= (ADC_INDEX_START_ADDR / EMMC_BLOCK_SIZE)) {
        adc->addr = 0;
    }
    #if 0
    APP_PRINTF("Storage_Write_AdcData, timestamp:%d, block_num:%d, adc->addr:%d, adc->index_addr:%d, adc->item_index:%d, adc->curr_block:%d, adc->block_num:%d\r\n", 
                                    timestamp, block_num, adc->addr, adc->index_addr, adc->item_index, adc->curr_block, adc->block_num);
    #endif
    if (flag) {
        adc->item_tables[adc->item_index].timestamp = timestamp;
        adc->item_tables[adc->item_index].addr = addr;
        adc->item_index += 1;
        if (adc->item_index >= ADC_INDEX_NUM_PRE_BLOCK)
        {
            emmcerasewriteblocks((uint8_t *)&adc->item_tables[0], adc->index_addr, 1);
            if (adc->curr_block == 0) {
                if (adc->block_num == ADC_INDEX_BLOCK_NUM) {
                    emmcreadblocks((uint8_t *)&adc->item_tables[0], adc->index_addr + 1, 1);
                }
                adc->str_time = adc->item_tables[0].timestamp;
                #if 0
                APP_PRINTF("Storage_Write_AdcData, ts:%d, ts:%d\r\n", adc->item_tables[0].timestamp, adc->item_tables[1].timestamp);
                #endif
            }
            
            adc->end_time = adc->item_tables[ADC_INDEX_NUM_PRE_BLOCK - 1].timestamp;
            adc->item_index = 0;
            adc->index_addr += 1;
            adc->block_num += 1;
            adc->curr_block += 1;
            if (adc->curr_block >= ADC_INDEX_BLOCK_NUM) {
                adc->index_addr = ADC_INDEX_START_BLOCK;
                adc->curr_block = 0;
                adc->block_num = ADC_INDEX_BLOCK_NUM;
            }
            g_ConfigInfo.block_num = adc->block_num;
            g_ConfigInfo.curr_block = adc->curr_block;
            g_ConfigInfo.str_time = adc->str_time;
            g_ConfigInfo.end_time = adc->end_time;
            g_config_status = 1;
            #if 0
            APP_PRINTF("Storage_Write_AdcData, s_ts:%d, e_ts:%d\r\n", adc->str_time, adc->end_time);
            #endif
        }
    }

    xSemaphoreGive(adc->mutex);

    return 0;
}

int8_t Storage_Read_AdcData(AdcObject_t *adc, uint32_t timestamp, uint16_t block_off, uint8_t *data, uint8_t block_num)
{
    int8_t ret = -1;

    if (adc == NULL || data == NULL) {
        return ret;
    }

    if (xSemaphoreTake(adc->mutex, ADC_LOCK_TIMEOUT) != pdTRUE) {
        return ret; 
    }

    uint8_t i = 0;
    uint16_t num = 1;
    uint8_t index = adc->item_index;
    uint32_t index_addr = ADC_INDEX_START_BLOCK;
    AdcIndexItem_t item_tables[ADC_INDEX_NUM_PRE_BLOCK] = {0};

    memcpy(item_tables, adc->item_tables, sizeof(item_tables));
    for (i = 0; i < index; i ++) {
        if (timestamp == item_tables[i].timestamp) {
            #if 0
            APP_PRINTF("Storage_Read_AdcData, timestamp:%lu, block_num:%d, adc->block_num:%d, item_tables[i].addr:%d\r\n", timestamp, block_num, adc->block_num, item_tables[i].addr);
            #endif
            emmcreadblocks(data, item_tables[i].addr + block_off, block_num);
            ret = 0;
            goto exit;
        }
    }

    for (num = 0; num < adc->block_num; num++) {
        emmcreadblocks((uint8_t *)&item_tables[0], index_addr + num,  1);
        index = ADC_INDEX_NUM_PRE_BLOCK;
        
        for (i = 0; i < index; i++) {
            if (timestamp == item_tables[i].timestamp) {
                #if 0
                APP_PRINTF("Storage_Read_AdcData, timestamp:%lu, block_num:%d, adc->block_num:%d, item_tables[i].addr:%d\r\n", timestamp, block_num, adc->block_num, item_tables[i].addr);
                #endif
                emmcreadblocks(data, item_tables[i].addr + block_off, block_num);
                ret = 0;
                goto exit;
            }
        }      
    }
exit:
    xSemaphoreGive(adc->mutex);

    return ret;
}

int8_t Storage_Write_AlarmMsg(AlarmObject_t *alarm, uint32_t timestamp, uint8_t *data)
{
    if (alarm == NULL || data == NULL) {
        return -1;
    }

    if (xSemaphoreTake(alarm->mutex, ALARM_LOCK_TIMEOUT) != pdTRUE) {
        return -1;
    }

    gd55b01ge_write_data(alarm->addr, data, sizeof(AlarmMsg_t));
#if 0
    alarm->item_tables[alarm->item_index].timestamp = timestamp;
    alarm->item_tables[alarm->item_index].msg_addr = alarm->addr;
    alarm->addr += sizeof(AlarmMsg_t);
    if (alarm->addr >= ALARM_ITEM_TOATL_NUM * sizeof(AlarmMsg_t)) {
        alarm->addr = ALARM_MSG_START_ADDR;
    }
    alarm->item_index += 1;
#else
    alarm->index_item.timestamp = timestamp;
    alarm->index_item.msg_addr = alarm->addr;
    alarm->index_item.info_addr = alarm->info_addr;
    alarm->addr += sizeof(AlarmMsg_t);
#endif

    xSemaphoreGive(alarm->mutex);

    return 0;
}


int8_t Storage_Read_AlarmMsg(AlarmObject_t *alarm, uint16_t alarm_index, uint8_t *data, uint8_t alarm_num)
{
    int8_t ret = -1;

    if (alarm == NULL || data == NULL) {
        return -1;
    }

    if (xSemaphoreTake(alarm->mutex, ALARM_LOCK_TIMEOUT) != pdTRUE) {
        return -1; 
    }
#if 0
    uint8_t i = 0;
    uint16_t num = 0;
    uint8_t index = alarm->item_index;
    uint32_t index_addr = ALARM_INDEX_START_ADDR + alarm->curr_page * GD55B01GE_PAGE_SIZE;
    AlarmIndexItem_t item_tables[ALARM_INDEX_NUM_PRE_PAGE] = {0};

    for (num = 0; num < alarm->page_num; num ++) {
        if (index_addr >= ALARM_INDEX_END_ADDR) {
            index_addr = ALARM_INDEX_START_ADDR;
        }
        gd55b01ge_read_data(index_addr, (uint8_t *)&item_tables[0], sizeof(item_tables));
        index = ALARM_INDEX_NUM_PRE_PAGE;
        index_addr += sizeof(item_tables);
        for (i = 0; i < index; i++) {
            if (alarm_index == (num * ALARM_INDEX_NUM_PRE_PAGE + i)) {
                gd55b01ge_read_data(item_tables[i].msg_addr, data, sizeof(AlarmMsg_t));
                ret = 0;
                goto exit;
            }
        }
    }
    memcpy(item_tables, alarm->item_tables, sizeof(item_tables));
    for (i = 0; i < index; i++) {
        if (alarm_index == num * ALARM_INDEX_NUM_PRE_PAGE + i) {
            gd55b01ge_read_data(item_tables[i].msg_addr, data, sizeof(AlarmMsg_t));
            ret = 0;
            goto exit;
        }
    }
exit:
#else
    uint32_t addr = ALARM_MSG_START_ADDR + alarm_index * sizeof(AlarmMsg_t);

    gd55b01ge_read_data(addr, data, alarm_num * sizeof(AlarmMsg_t));

#endif
    xSemaphoreGive(alarm->mutex);

    return ret;
}

int8_t Storage_Write_AlarmInfo(AlarmObject_t *alarm, uint32_t timestamp, uint32_t off, uint8_t *data, uint32_t len, uint8_t flag)
{
    if (alarm == NULL || data == NULL) {
        return -1;
    }

    if (xSemaphoreTake(alarm->mutex, ALARM_LOCK_TIMEOUT) != pdTRUE) {
        return -1; 
    }
#if 0
    uint8_t i = 0;
    uint16_t index = alarm->item_index;
//    uint16_t num = 0;
    
    // for (num = 0; num < alarm->page_num; num++) {
        for (i = 0; i < index; i++) {
            if (timestamp == alarm->item_tables[index].timestamp) {
                gd55b01ge_write_data(alarm->info_addr + off, data, len);
                alarm->info_addr += ADC_ITEM_SIZE * ADC_ITEM_NUM_PER_SECOND;
                if (flag) {
                    if (alarm->item_index >= ALARM_INDEX_NUM_PRE_PAGE) {
                        gd55b01ge_write_data(alarm->index_addr, (uint8_t *)&alarm->item_tables[0], sizeof(alarm->item_tables));
                        alarm->item_index = 0;
                        alarm->page_num += 1;
                        alarm->curr_page += 1;
                        alarm->index_addr += sizeof(alarm->item_tables);
                        if (alarm->curr_page >= ALARM_INDEX_PAGE_NUM) {
                            alarm->index_addr = ALARM_INDEX_START_ADDR;
                            alarm->info_addr  = AlARM_INFO_START_ADDR;
                            alarm->addr = ALARM_MSG_START_ADDR;
                            alarm->curr_page = 0;
                            alarm->page_num = ALARM_INDEX_PAGE_NUM;
                        }
                        g_ConfigInfo.page_num = alarm->page_num;
                        g_ConfigInfo.curr_page = alarm->curr_page;
                        g_config_status = 1;
                    }
                }
                break;
            }
        }
    // }
#else
    // uint32_t info_addr = alarm->info_addr;
    if (timestamp != alarm->index_item.timestamp) {
        #if 0
        APP_PRINTF("Storage_Write_AlarmInfo, timestamp:%lu != alarm->index_item.timestamp:%lu\r\n", timestamp, alarm->index_item.timestamp);
        #endif
        xSemaphoreGive(alarm->mutex);
        return -1;
    }
    gd55b01ge_write_data(alarm->info_addr, data, len);
    alarm->info_addr += len;
    if (flag) {
        gd55b01ge_write_data(alarm->index_addr, (uint8_t *)&alarm->index_item, sizeof(AlarmIndexItem_t));
        alarm->index_addr += sizeof(AlarmIndexItem_t);
        alarm->index_num += 1;
        if (alarm->index_num >= ALARM_ITEM_TOATL_NUM) {
            alarm->index_num = ALARM_ITEM_TOATL_NUM;
            alarm->addr = ALARM_MSG_START_ADDR;
            alarm->info_addr = AlARM_INFO_START_ADDR;
            alarm->index_addr = ALARM_INDEX_START_ADDR;
        }
        g_ConfigInfo.alarm_num = alarm->index_num;
        g_config_status = 1;
    }

#endif
    xSemaphoreGive(alarm->mutex);

    return 0;
}

int8_t Storage_Read_AlarmInfo(AlarmObject_t *alarm, uint32_t timestamp, uint32_t off, uint8_t *data, uint32_t len)
{
    int8_t ret = -1;
    if (alarm == NULL || data == NULL) {
        return -1;
    }

    if (xSemaphoreTake(alarm->mutex, ALARM_LOCK_TIMEOUT) != pdTRUE) {
        return -1; 
    }
#if 0
    uint8_t i = 0;
    uint16_t num = 0;
    uint8_t index = alarm->item_index;
    AlarmIndexItem_t item_tables[ALARM_INDEX_NUM_PRE_PAGE] = {0};

    memcpy(item_tables, alarm->item_tables, sizeof(item_tables));
    for (num = 0; num < alarm->page_num; num ++) {
        if (num) {
            gd55b01ge_read_data(alarm->index_addr - num * sizeof(item_tables), (uint8_t *)&item_tables[0], sizeof(item_tables));
            index = ALARM_INDEX_NUM_PRE_PAGE;
        }
        for (i = 0; i < index; i++) {
            if (timestamp == item_tables[i].timestamp) {
                gd55b01ge_read_data(item_tables[i].info_addr, data, len);
                break;
            }
        }
    }
#else
    uint8_t buf[GD55B01GE_PAGE_SIZE] = {0};
    uint32_t index_addr = ALARM_INDEX_START_ADDR;
    uint8_t i = 0;
    uint16_t num = 0;
    AlarmIndexItem_t *item = NULL;
    
    do {
        gd55b01ge_read_data(index_addr, buf, sizeof(buf));
        for (i = 0; i < ALARM_INDEX_NUM_PRE_PAGE; i ++) {
            item = (AlarmIndexItem_t *)(buf + i * sizeof(AlarmIndexItem_t));
            if (timestamp == item->timestamp) {
                gd55b01ge_read_data(item->info_addr + off, data, len);
                ret = 0;
                goto exit;
            }
        }
        num += ALARM_INDEX_NUM_PRE_PAGE;
        index_addr += GD55B01GE_PAGE_SIZE;

    } while (num < alarm->index_num);
exit:
#endif
    xSemaphoreGive(alarm->mutex);

    return ret;
}

void Storage_Clear_AlarmData(AlarmObject_t *alarm)
{
    if (alarm == NULL) {
        return ;
    }

    if (xSemaphoreTake(alarm->mutex, ALARM_LOCK_TIMEOUT) != pdTRUE) {
        return ; 
    }

    alarm->addr = ALARM_MSG_START_ADDR;
    alarm->info_addr = AlARM_INFO_START_ADDR;
    alarm->index_addr = ALARM_INDEX_START_ADDR;
    alarm->index_num = 0;

    gd55b01ge_erase_chip();
    
    xSemaphoreGive(alarm->mutex);
}

void Storage_Clear_AdcData(AdcObject_t *adc)
{
    if (adc == NULL) {
        return ;
    }

    if (xSemaphoreTake(adc->mutex, ADC_LOCK_TIMEOUT) != pdTRUE) {
        return ;
    }

    adc->index_addr = ADC_INDEX_START_BLOCK;
    adc->addr = 0;
    adc->end_time = 0;
    adc->str_time = 0;
    adc->block_num = 0;
    adc->curr_block = 0;

    xSemaphoreGive(adc->mutex);
}

uint8_t Adc_Object_Init(AdcObject_t *object)
{
    if (object == NULL) {
        return 1;
    }

    memset(object, 0, sizeof(AdcObject_t));
    object->mutex = xSemaphoreCreateMutex();
    if (object->mutex == NULL) {
        return 1;
    }

    if (g_ConfigInfo.flag == DEV_FLAG_FIRST_ON) {
        object->index_addr = ADC_INDEX_START_BLOCK;
        object->addr = 0;
    } else if (g_ConfigInfo.flag == DEV_FLAG_NEXT_ON){
        object->block_num = g_ConfigInfo.block_num;
        object->curr_block = g_ConfigInfo.curr_block;
        object->addr = object->curr_block * ADC_INDEX_NUM_PRE_BLOCK * ADC_ITEM_BLOCK_NUM;
        object->index_addr = ADC_INDEX_START_BLOCK + object->curr_block;
        object->end_time = g_ConfigInfo.end_time;
        object->str_time = g_ConfigInfo.str_time;
    }
    
    return 0;
}


void storage_task(void *pvParameters)
{ 
    AdcItem_t *pItem = NULL;
    uint16_t read_len;
    uint32_t index = 0;
    uint16_t i = 0;
    
    for (;;)
    {
        if (Adc_Cache_Get_Count(&g_AdcItemCache) >= 100) {
            read_len = Adc_Cache_Read_Batch(&g_AdcItemCache, (AdcItem_t *)&g_adc_items[index], 100);
            if (read_len > 0) {
                for (i = 0; i < read_len; i ++) {
                    pItem = (AdcItem_t *)&g_adc_items[index];
                    #if 0
                    APP_PRINTF("storage_task, f_c:%lf, f_v:%lf, r_c:%lf, r_v:%lf\r\n", pItem->data[SENSOR_FRONT_CURRENT].f, pItem->data[SENSOR_FRONT_VOLTAGE].f, pItem->data[SNESOR_REAR_CURRENT].f, pItem->data[SNESOR_REAR_VOLTAGE].f);
                    #endif
                    if ((pItem->ts % ADC_ITEM_NUM_PER_SECOND) == 0 && index > ADC_ITEM_SIZE) {
                        Storage_Write_AdcData(&g_AdcObject, pItem->ts / 1000 - 1, &g_adc_items[0], (ADC_ITEM_SIZE * ADC_ITEM_NUM_PER_SECOND) / EMMC_BLOCK_SIZE, 1);
                        memcpy(&g_adc_items[0], &g_adc_items[index], (read_len - i) * ADC_ITEM_SIZE);
                        index = (read_len - i) * ADC_ITEM_SIZE;
                        break;
                    } else {
                        index += ADC_ITEM_SIZE;
                        if (index >= sizeof(g_adc_items)) {
                            index = 0;
                        }
                    }
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

uint8_t Alarm_Object_Init(AlarmObject_t *object)
{
    if (object == NULL) {
        return 1;
    }

    memset(object, 0, sizeof(AlarmObject_t));
    object->mutex = xSemaphoreCreateMutex();
    if (object->mutex == NULL) {
        return 1;
    }

    if (g_ConfigInfo.flag == DEV_FLAG_FIRST_ON) {
        object->addr = ALARM_MSG_START_ADDR;
        object->info_addr = AlARM_INFO_START_ADDR;
        object->index_addr = ALARM_INDEX_START_ADDR;
    } else if (g_ConfigInfo.flag == DEV_FLAG_NEXT_ON){
        #if 0
        object->page_num = g_ConfigInfo.page_num;
        object->curr_page = g_ConfigInfo.curr_page;
        object->addr = ALARM_MSG_START_ADDR + object->curr_page * ALARM_INDEX_NUM_PRE_PAGE * 32;
        object->info_addr = AlARM_INFO_START_ADDR + object->curr_page * ALARM_INDEX_NUM_PRE_PAGE * (ALARM_ITEM_SIZE - 32);
        object->index_addr = ALARM_INDEX_START_ADDR + object->curr_page * ADC_INDEX_NUM_PRE_BLOCK * ALARM_INDEX_ITEM_SIZE;
        #else
        object->index_num = g_ConfigInfo.alarm_num;
        object->addr = ALARM_MSG_START_ADDR + g_ConfigInfo.alarm_num * sizeof(AlarmMsg_t);
        object->info_addr = AlARM_INFO_START_ADDR + g_ConfigInfo.alarm_num * (ALARM_ITEM_SIZE - sizeof(AlarmMsg_t));
        object->index_addr = ALARM_INDEX_START_ADDR + g_ConfigInfo.alarm_num * sizeof(AlarmIndexItem_t);;
        #endif
    }

    return 0;
}

void alarm_task(void *pvParameters)
{
    AlarmMsg_t alarm_msg;
    int8_t ret = -1;
    uint32_t off = 0;

    for (;;)
    {
        if(xSemaphoreTake(g_AlarmNotify.alarm_sem, portMAX_DELAY) == pdPASS) {
            while(xQueueReceive(g_AlarmNotify.alarm_queue, &alarm_msg, 0) == pdPASS) {
                
                if (alarm_msg.type == BUS_ALARM_FRONT_VOL || alarm_msg.type == BUS_ALARM_FRONT_REAR_VOL || alarm_msg.type == BUS_ALARM_REAR_VOL) {
                    ALARM1_SHORT_CIRCUIT();
                    ALARM_LED_ON();
                    Storage_Write_AlarmMsg(&g_AlarmObject, alarm_msg.ts, (uint8_t *)&alarm_msg);
                } else if (alarm_msg.type == BUS_ALARM_SAVE) {
                    ret = Storage_Read_AdcData(&g_AdcObject, alarm_msg.ts, 0, g_adc_data, sizeof(g_adc_data) / EMMC_BLOCK_SIZE);
                    if (ret == 0) {
                        Storage_Write_AlarmInfo(&g_AlarmObject, g_AlarmNotify.alarm_ts + 1, off, g_adc_data, sizeof(g_adc_data), g_AlarmNotify.index >= 3 ? 1 : 0);
                    }
                }
            }
        }
    }
}


void storage_manage_start(void)
{
    Adc_Object_Init(&g_AdcObject);

    Alarm_Object_Init(&g_AlarmObject);

    xTaskCreate((TaskFunction_t)storage_task,              // 任务函数
                (const char*    )"storage_Task",           // 任务名称
                (uint16_t       )STORAGE_STK_SIZE,         // 任务栈大小
                (void*          )NULL,                     // 任务参数
                (UBaseType_t    )STORAGE_TASK_PRIO,        // 任务优先级（低于定时器中断优先级）
                (TaskHandle_t*  )&StorageTask_Handle);     // 任务句柄


    xTaskCreate((TaskFunction_t)alarm_task,                 // 任务函数
                (const char*    )"alarm_Task",              // 任务名称
                (uint16_t       )ALARM_STK_SIZE,            // 任务栈大小
                (void*          )NULL,                      // 任务参数
                (UBaseType_t    )ALARM_TASK_PRIO,           // 任务优先级（低于定时器中断优先级）
                (TaskHandle_t*  )&AlarmTask_Handle);        // 任务句柄
}
