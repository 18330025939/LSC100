/**
 ****************************************************************************************************
 * @file        app_task.c
 * @author      xxxxxx
 * @version     V1.0
 * @date        2025-12-31
 * @brief       应用任务初始化
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
 
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "adc_sample.h"
#include "msg_parse.h"
#include "storage_manage.h"
#include "Q468V1.h"
/******************************************************************************************************/


 /* START_TASK 任务 配置
  * 包括: 任务句柄 任务优先级 堆栈大小 创建任务
  */
 #define START_TASK_PRIO         5           /* 任务优先级 */
 #define START_STK_SIZE          512         /* 任务堆栈大小 */
 TaskHandle_t StartTask_Handler;             /* 任务句柄 */
 void start_task(void *pvParameters);        /* 任务函数 */

/******************************************************************************************************/

/**
 * @breif       freertos_demo
 * @param       无
 * @retval      无
 */
void app_task(void)
{
    /* start_task任务 */
    xTaskCreate((TaskFunction_t )start_task,
                (const char *   )"start_task",
                (uint16_t       )START_STK_SIZE,
                (void *         )NULL,
                (UBaseType_t    )START_TASK_PRIO,
                (TaskHandle_t * )&StartTask_Handler);

    vTaskStartScheduler(); /* 开启任务调度 */
}

/**
 * @brief       start_task
 * @param       pvParameters : 传入参数(未用到)
 * @retval      无
 */
void start_task(void *pvParameters)
{
    system_init();
    
    taskENTER_CRITICAL();           /* 进入临界区 */

    msg_parse_start();

    storage_manage_start();
    
    adc_sample_start();

    vTaskDelete(StartTask_Handler); /* 删除开始任务 */
    taskEXIT_CRITICAL();            /* 退出临界区 */
}




