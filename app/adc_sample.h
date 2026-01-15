#ifndef __ADC_SAMPLE_H
#define __ADC_SAMPLE_H

#include "utils.h"
#include <stdint.h>
#include "semphr.h"
#include "storage_manage.h"

/************************ 传感器变比基础配置 ************************/
#define BUS_VOLTAGE_FULL_SCALE_V     3000.0f   // 母线电压满量程（物理值）
#define BUS_VOLTAGE_SENSOR_FULL_V    7.5f      // 满量程对应的传感器电压
#define BUS_VOLTAGE_CONVERT_RATIO    (BUS_VOLTAGE_FULL_SCALE_V / BUS_VOLTAGE_SENSOR_FULL_V) // 电压换算系数（400）

#define BUS_CURRENT_FULL_SCALE_A     435.0f    // 母线电流满量程（物理值）
#define BUS_CURRENT_SENSOR_FULL_V    1.0f      // 满量程对应的传感器电压
#define BUS_CURRENT_CONVERT_RATIO    (BUS_CURRENT_FULL_SCALE_A / BUS_CURRENT_SENSOR_FULL_V) // 电流换算系数（435）

#define BUS_VOLTAGE_IDLE_THRESHOLD   200.0f    // 母线电压空置阈值
#define BUS_CURRENT_IDLE_THRESHOLD   200.0f    // 母线电流控制阈值

#define D1_LOGIC_HIGH_VOLTAGE_MAX    74.0f     // 逻辑高电平最大电压    
#define D1_LOGIC_HIGH_VOLTAGE_MIN    50.0f     // 逻辑高电平最小电压

#define D1_LOGIC_LOW_VOLTAGE_MAX     10.0f     // 逻辑低电平最大电压
#define D1_LOGIC_LOW_VOLTAGE_MIN     0.0f      // 逻辑低电平最小电压

/************************ 阈值上下限配置（软件检测） ************************/
#define BUS_VOLTAGE_LOWER_LIMIT_V    500.0f    // 母线电压下限阈值（软件）
#define BUS_VOLTAGE_UPPER_LIMIT_V    2000.0f   // 母线电压上限阈值（软件）

/************************ 传感器电压 -> 实际母线电压 ************************/
#define CONVERT_SENSOR_TO_BUS_VOLTAGE(sensor_v) ((sensor_v) * BUS_VOLTAGE_CONVERT_RATIO)
/************************ 传感器电压 -> 实际母线电流 ************************/
#define CONVERT_SENSOR_TO_BUS_CURRENT(sensor_v) ((sensor_v) * BUS_CURRENT_CONVERT_RATIO)

/************************ 动态阈值计算 ************************/
#define CALC_BUS_CURRENT_LOW_THRESHOLD(actual_bus_voltage, voltage_low_lim) \
    ((actual_bus_voltage < voltage_low_lim) ? voltage_low_lim : actual_bus_voltage)

#define CALC_BUS_CURRENT_UP_THRESHOLD(actual_bus_voltage, voltage_up_lim) \
    ((actual_bus_voltage < voltage_up_lim) ? actual_bus_voltage : voltage_up_lim)

/************************ 报警消除判定 ************************/
#define IS_CLEANR_ALARM(actual_bus_current) \
    ((actual_bus_current > (D1_LOGIC_HIGH_VOLTAGE_MIN * 0.091f) && actual_bus_current < (D1_LOGIC_HIGH_VOLTAGE_MAX * 0.091f)) ? 1 : 0)

#define SAMPLE_TASK_PRIO        (configMAX_PRIORITIES - 1)      /* 任务优先级 */
#define SAMPLE_STK_SIZE 	    1024                            /* 任务堆栈大小 */ 
#define TIME_SYNC_TASK_PRIO     (configMAX_PRIORITIES - 2)      /* 任务优先级 */
#define TIME_SYNC_STK_SIZE 	    1024                            /* 任务堆栈大小 */ 

#define ADC_SAMPLE_CHANNEL      4

typedef enum
{
    SENSOR_FRONT_CURRENT = 0,
    SENSOR_FRONT_VOLTAGE,
    SNESOR_REAR_CURRENT,
    SNESOR_REAR_VOLTAGE
} SensorIndex;

#define ADC_SAMPLE_CHANNEL   4

/* 采样数据 */
typedef struct 
{
    FloatUInt32_t f_cur;
    FloatUInt32_t f_vlt;
    FloatUInt32_t r_cur;
    FloatUInt32_t r_vlt;
    // FloatUInt32_t DI1;
} SampleData_t;

/* ADC数据格式 */
#pragma pack(1)
typedef struct 
{
    SystemTime_t time;
    uint64_t ts;
    uint16_t raw_data[4];
    FloatUInt32_t data[4];
    FloatUInt32_t temp;
    uint8_t rsvd[22];
} AdcItem_t;
#pragma pack()
/* ADC数据缓存 */
typedef struct
{
    AdcItem_t item[800];
    uint32_t wr_idx;                   // 写指针
    uint32_t rd_idx;                   // 读指针
    uint32_t count;                    // 当前缓存数据量
    SemaphoreHandle_t mutex;           // 缓存访问互斥锁
} AdcItemCache_t;
extern AdcItemCache_t g_AdcItemCache;

/* */
typedef enum 
{
    BUS_ALARM_NONE  = 0,
    BUS_ALARM_FRONT_VOL,
    BUS_ALARM_REAR_VOL,
    BUS_ALARM_FRONT_REAR_VOL,
    BUS_ALARM_CLEAR,
    BUS_ALARM_SAVE
} AlarmType;

/* 报警信息 */
#pragma pack(1)
typedef struct
{
    SystemTime_t time;
    uint32_t ts;
    uint16_t idx;
    AlarmType type;
    uint8_t ucRsvd[19];
} AlarmMsg_t;
#pragma pack()
typedef struct 
{
    FloatUInt32_t adc_data[4];
    uint16_t len;
} Ma_t;

typedef struct
{
    uint32_t data[4]; 
    uint8_t num;
} Cal_t;


#define ALARM_STA_RAISED   0xA0
#define ALARM_STA_CLEARED  0xB0
#define ALARM_STA_STR_SAVE 0xC0
#define ALARM_STA_END_SAVE 0xD0

typedef struct {
    QueueHandle_t alarm_queue;   // 告警消息队列（存储待处理的告警）
    SemaphoreHandle_t alarm_sem;
    uint32_t alarm_ts;           //
    AlarmType type;
    uint8_t alarm_sta;
    uint8_t save_sta;
    uint8_t index;
    SemaphoreHandle_t mutex;
} AlarmNotify_t;
extern AlarmNotify_t g_AlarmNotify;

// 同步结构体适配
typedef struct {
    uint32_t rtc_base_sec;  // 同步时的RTC秒级Unix时间
    uint32_t ms_offset;     // 同步时的毫秒偏移（0~999）
    uint32_t tick_base;     // 同步时的采样计数值
    uint8_t sync_valid;     // 同步标志
    SystemTime_t sys_time;  // 系统时间
    uint8_t flag;
    SemaphoreHandle_t mutex;
} TimeSync_t;
extern TimeSync_t g_TimeSync;

uint16_t Adc_Cache_Read_Batch(AdcItemCache_t *cache, AdcItem_t *data, uint16_t max_len);
uint16_t Adc_Cache_Get_Count(AdcItemCache_t *cache);

void sample_task(void *pvParameters);
void adc_sample_start(void);

#endif /* __ADC_SAMPLE_H */
