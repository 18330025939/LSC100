#ifndef __STORAGE_MANAGER_H
#define __STORAGE_MANAGER_H

#include "gd32f4xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "BSP_GD55B01.h"

#define EMMC_START_ADDR          0x00000000UL                          // 数据存储起始地址
#define EMMC_TOTAL_SIZE          29600                                 // 总容量（MB）
#ifndef EMMC_BLOCK_SIZE
#define EMMC_BLOCK_SIZE          512                                   // 单块字节数（Byte）
#endif
#define EMMC_BLOCK_NUM           60620800                              // 总块数

#define ADC_INDEX_START_ADDR     0x7270E0000ULL //0x730000000ULL 
#define ADC_ITEM_SIZE            sizeof(AdcItem_t)                               // 单条日志字节数
#define ADC_ITEM_NUM_PER_BLOCK   (EMMC_BLOCK_SIZE / ADC_ITEM_SIZE)               // 每块可存日志数：8条
#define ADC_ITEM_NUM_PER_SECOND  1000                                            // 每秒日志条数
#define ADC_ITEM_BLOCK_NUM       (ADC_ITEM_NUM_PER_SECOND / ADC_ITEM_NUM_PER_BLOCK)        // 每秒数据缓存块数：（1000/8=125）块
#define ADC_ITEM_TOTAL_NUM       (ADC_INDEX_START_ADDR / EMMC_BLOCK_SIZE / ADC_ITEM_BLOCK_NUM)    //480000

#define ADC_INDEX_ITEM_SIZE      8
#define ADC_INDEX_NUM_PRE_BLOCK  (EMMC_BLOCK_SIZE / ADC_INDEX_ITEM_SIZE)            // 每块可存索引数：64
#define ADC_INDEX_BLOCK_NUM      (ADC_ITEM_TOTAL_NUM / ADC_INDEX_NUM_PRE_BLOCK)     // 索引需要总块数：480000 / 64 = 7500
#define ADC_INDEX_START_BLOCK    (ADC_INDEX_START_ADDR / EMMC_BLOCK_SIZE)                          //
#define ADC_LOCK_TIMEOUT         pdMS_TO_TICKS(100) // 互斥锁超时（100ms）

#define ALARM_MSG_START_ADDR     0
#define AlARM_INFO_START_ADDR    20480
#define ALARM_INDEX_START_ADDR   0x7C00000UL
#define ALARM_ITEM_SIZE          ((ADC_ITEM_NUM_PER_SECOND * ADC_ITEM_SIZE * 3) + 32)
#define ALARM_ITEM_TOATL_NUM     640 //(ALARM_INDEX_START_ADDR / ALARM_ITEM_SIZE)
#define ALARM_MSG_END_ADDR       (ALARM_MSG_START_ADDR + (32 * ALARM_ITEM_TOATL_NUM))
#define AlARM_INFO_END_ADDR      (AlARM_INFO_START_ADDR + (ALARM_ITEM_SIZE - 32) * ALARM_ITEM_TOATL_NUM)

#define ALARM_INDEX_ITEM_SIZE    16
#define ALARM_INDEX_NUM_PRE_PAGE (GD55B01GE_PAGE_SIZE / ALARM_INDEX_ITEM_SIZE)
#define ALARM_INDEX_PAGE_NUM     (ALARM_ITEM_TOATL_NUM / 16) //40
#define ALARM_INDEX_END_ADDR     (ALARM_INDEX_START_ADDR + ALARM_INDEX_PAGE_NUM * GD55B01GE_PAGE_SIZE)
#define ALARM_LOCK_TIMEOUT       pdMS_TO_TICKS(100) // 互斥锁超时（100ms）

#define STORAGE_TASK_PRIO        (configMAX_PRIORITIES - 3)     /* 任务优先级 */
#define STORAGE_STK_SIZE         1024                           /* 任务堆栈大小 */

#define ALARM_TASK_PRIO          (configMAX_PRIORITIES - 2)     /* 任务优先级 */
#define ALARM_STK_SIZE           512                            /* 任务堆栈大小 */

/* 数据索引 */
//#pragma pack(1)
typedef struct 
{
    uint32_t addr;
    uint32_t timestamp;  
} AdcIndexItem_t;

typedef struct 
{
    uint32_t msg_addr;
    uint32_t info_addr;
    uint32_t timestamp;
    uint8_t rsvd[4];  
} AlarmIndexItem_t;
//#pragma pack()

typedef struct 
{
    uint32_t str_time;
    uint32_t end_time;
    uint32_t addr;
    SemaphoreHandle_t mutex;
    uint32_t index_addr;       //索引的块地址
    uint16_t block_num;        //块的数量
    uint16_t curr_block;       //当前索引的块
    uint8_t item_index;        //Adc_Item的索引
    AdcIndexItem_t item_tables[ADC_INDEX_NUM_PRE_BLOCK];
} AdcObject_t;
extern AdcObject_t g_AdcObject;

typedef struct 
{
    uint32_t addr;
    uint32_t info_addr;
    SemaphoreHandle_t mutex;
    uint32_t index_addr;
    #if 0
    uint16_t page_num;
    uint16_t curr_page;
    uint8_t item_index;
    AlarmIndexItem_t item_tables[ALARM_INDEX_NUM_PRE_PAGE];
    #else 
    uint16_t index_num;
    AlarmIndexItem_t index_item;
    #endif
} AlarmObject_t;
extern AlarmObject_t g_AlarmObject;

int8_t Get_AdcData_Time(AdcObject_t *adc, SystemTime_t *str_time, SystemTime_t *end_time);
int8_t Storage_Write_AdcData(AdcObject_t *adc, uint32_t timestamp, uint8_t *data, uint8_t block_num, uint8_t flag);
int8_t Storage_Read_AdcData(AdcObject_t *adc, uint32_t timestamp, uint16_t block_off, uint8_t *data, uint8_t block_num);
int8_t Storage_Write_AlarmMsg(AlarmObject_t *alarm, uint32_t timestamp, uint8_t *data);
int8_t Storage_Read_AlarmMsg(AlarmObject_t *alarm, uint16_t alarm_index, uint8_t *data, uint8_t alarm_num);
int8_t Storage_Write_AlarmInfo(AlarmObject_t *alarm, uint32_t timestamp, uint32_t off, uint8_t *data, uint32_t len, uint8_t flag);
int8_t Storage_Read_AlarmInfo(AlarmObject_t *alarm, uint32_t timestamp, uint32_t off, uint8_t *data, uint32_t len);

void storage_manage_start(void);
// int8_t Storage_Write_AlarmMsg(AlarmObject_t *alarm, uint32_t timestamp, uint8_t *data, uint32_t len);

#endif /* STORAGE_MANAGER_H */
