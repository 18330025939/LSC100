/**
 ****************************************************************************************************
 * @file        msg_parse.c
 * @author      xxxxxx
 * @version     V1.0
 * @date        2025-12-31
 * @brief       报文解析处理功能模块
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
#include <stdio.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "fm24cxx.h"
#include "sd2505.h"
#include "BSP_FM24C02.h"
#include "utils.h"
#include "app_task.h"
#include "tcp_server.h"
#include "adc_sample.h"
#include "storage_manage.h"
#include "msg_parse.h"
#include "Q468V1.h"

#include "rtt_log.h"


ConfigInfo_t g_ConfigInfo = {0};
// static ThreadSafeCirQue g_CirQue = {0};
TcpServerHandle_t g_TcpServerHandle = {0};

TaskHandle_t MsgProc_Task_Handle;
TaskHandle_t MsgSend_Task_Handle;
static uint8_t g_buf[1000 * ADC_ITEM_SIZE] = {0};
static uint8_t g_send[900] = {0};
volatile uint8_t g_config_status = 0;

static void kt_ConfigInfo(void *arg);
static void kt_SetFrontVmax(void *arg);
static void kt_SetFrontVmin(void *arg);
static void kt_SetRearVmax(void *arg);
static void kt_SetRearVmin(void *arg);
static void kt_AlarmInfo(void *arg);
static void kt_AlarmData(void *arg);
static void kt_SampleInfo(void *arg);
static void kt_SampleData(void *arg);
static void kt_SetSysTime(void *arg);
static void kt_ClearData(void *arg);

const static MsgPraseInterface_t g_MsgPraseInterTable[] = {
    {MSG_SIGN_CONFIG_INFO_REQ, kt_ConfigInfo    },
    {MSG_SIGN_SET_F_VMAX_REQ,  kt_SetFrontVmax  },
    {MSG_SIGN_SET_F_VMIN_REQ,  kt_SetFrontVmin  }, 
    {MSG_SIGN_SET_R_VMAX_REQ,  kt_SetRearVmax   },
    {MSG_SIGN_SET_R_VMIN_REQ,  kt_SetRearVmin   },  
    {MSG_SIGN_ALARM_INFO_REQ,  kt_AlarmInfo     },
    {MSG_SIGN_ALARM_DATA_REQ,  kt_AlarmData     },
    {MSG_SIGN_SAMPLE_INFO_REQ, kt_SampleInfo    },
    {MSG_SIGN_SAMPLE_DATA_REQ, kt_SampleData    },
    {MSG_SIGN_SET_TIME_REQ,    kt_SetSysTime    },
    {MSG_SIGN_CLEAR_DATA_REQ,  kt_ClearData     },
    {0xFF,                     NULL,            }
};

void sys_config_write(ConfigInfo_t *config)
{
    if (config == NULL) {
        return;
    }

    fm24c_write(0, (uint8_t *)config, sizeof(ConfigInfo_t));//fm24cxx_write_data(0, (uint8_t *)config, sizeof(ConfigInfo_t));
}

void sys_config_read(ConfigInfo_t *config)
{
    if (config == NULL) {
        return;
    }

    fm24c_read(0, (uint8_t *)config, sizeof(ConfigInfo_t));
}


static uint16_t checkSum_8(uint8_t *buf, uint16_t len)
{
    uint16_t i;
    uint16_t ret = 0;
    for(i=0; i<len; i++)
    {
        ret += *(buf++);
    }

    return ret;
}

/* Zeller公式计算星期几 */
int calculateDayOfWeek(int year, int month, int day) 
{
    int y = year;
    int m = month;
    int w = 0;
    
    if (month < 3) {
        m += 12;
        y -= 1;
    }

    w = day + 13 * (m + 1) / 5 + (y % 100) + (y % 100) / 4 + (y / 100) / 4 - 2 * (y / 100);
    w = w % 7;
    if (w == 0) {
        w = 6; // 星期六
    } else {
        w--; // 其他情况减1，使得星期日为0
    }
    return w;
}


/**
 * @brief       获取系统配置信息
 * @param       arg : 指向获取系统配置信息报文的指针
 * @retval      无
 */
static void kt_ConfigInfo(void *arg)
{
    uint8_t buf[MAX_QUEUE_DATA_SIZE] = {0};
    ConfigInfoRes_t *pstConfig = NULL;
    MsgFramHdr_t *pstMsgHdr =NULL;
    MsgFramCrc_t *pstMsgCrc = NULL;
    uint16_t len = 0;

    if (arg == NULL) {
        return;
    }

    #if 0
    APP_PRINTF("kt_ConfigInfo\r\n");
    #endif
    pstMsgHdr = (MsgFramHdr_t*)buf;
    pstMsgHdr->usHdr = MSG_DATA_FRAM_HDR;
    len = sizeof(MsgFramHdr_t) + sizeof(ConfigInfoRes_t) + sizeof(MsgFramCrc_t);
    pstMsgHdr->usLen = HConvert(&len);
    pstMsgHdr->ucSign = MSG_SIGN_CONFIG_INFO_RES;
    pstConfig = (ConfigInfoRes_t*)(buf + sizeof(MsgFramHdr_t));
    memcpy(pstConfig->hw, g_ConfigInfo.hw, sizeof(g_ConfigInfo.hw));
    memcpy(pstConfig->sw, g_ConfigInfo.sw, sizeof(g_ConfigInfo.sw));
    pstConfig->temp.f = ds18b20_get_temp();
    pstConfig->temp.u = DW_HConvert(&pstConfig->temp.u);
    pstConfig->f_Vmax.u = DW_HConvert(&g_ConfigInfo.f_Vmax.u);
    pstConfig->f_Vmin.u = DW_HConvert(&g_ConfigInfo.f_Vmin.u);
    pstConfig->r_Vmax.u = DW_HConvert(&g_ConfigInfo.r_Vmax.u);
    pstConfig->r_Vmin.u = DW_HConvert(&g_ConfigInfo.r_Vmin.u);
    memcpy(&pstConfig->stTime, &g_TimeSync.sys_time, sizeof(SystemTime_t));
    pstMsgCrc = (MsgFramCrc_t*)(buf + sizeof(MsgFramHdr_t) + sizeof(ConfigInfoRes_t));
    pstMsgCrc->usCrc = checkSum_8(buf, len - sizeof(MsgFramCrc_t));
    pstMsgCrc->usCrc = HConvert(&pstMsgCrc->usCrc);
    // enQueue(&g_CirQue, buf, sizeof(buf));
    tcp_server_send_data(&g_TcpServerHandle, buf, len);
}

static void kt_SetFrontVmax(void *arg)
{
    SetFrontVmaxReq_t *pstFrontVmax = NULL;
    if (arg == NULL) {
        return; 
    }

    pstFrontVmax = (SetFrontVmaxReq_t *)((uint8_t *)arg + sizeof(MsgFramHdr_t));
    g_ConfigInfo.f_Vmax.u = DW_HConvert(&pstFrontVmax->uVmax.u);

    g_config_status = 1;
}

static void kt_SetFrontVmin(void *arg)
{
    SetFrontVminReq_t *pstFrontVmin = NULL;
    if (arg == NULL) {
        return; 
    }

    pstFrontVmin = (SetFrontVminReq_t *)((uint8_t *)arg + sizeof(MsgFramHdr_t));
    g_ConfigInfo.f_Vmin.u = DW_HConvert(&pstFrontVmin->uVmin.u);

    g_config_status = 1;
}

static void kt_SetRearVmax(void *arg)
{
    SetRearVmaxReq_t *pstRearVmax = NULL;
    if (arg == NULL) {
        return; 
    }
    
    pstRearVmax = (SetRearVmaxReq_t *)((uint8_t *)arg + sizeof(MsgFramHdr_t));
    g_ConfigInfo.r_Vmax.u = DW_HConvert(&pstRearVmax->uVmax.u);

    g_config_status = 1;
}

static void kt_SetRearVmin(void *arg)
{
    SetRearVminReq_t *pstRearVmin = NULL;

    if (arg == NULL) {
        return; 
    }

    pstRearVmin = (SetRearVminReq_t *)((uint8_t *)arg + sizeof(MsgFramHdr_t));
    g_ConfigInfo.r_Vmin.u = DW_HConvert(&pstRearVmin->uVmin.u);

    g_config_status = 1;
}

static void kt_ClearData(void *arg)
{
    ClearDataRes_t *pstClear = NULL;
    uint8_t buf[MAX_QUEUE_DATA_SIZE] = {0};
    MsgFramHdr_t *pstMsgHdr =NULL;
    MsgFramCrc_t *pstMsgCrc = NULL;
    uint16_t len = 0;

    if (arg == NULL) {
        return; 
    }

    g_ConfigInfo.block_num = 0;
    g_ConfigInfo.curr_block = 0;
    g_ConfigInfo.str_time = 0;
    g_ConfigInfo.end_time = 0;
    g_ConfigInfo.alarm_num = 0;
    Storage_Clear_AlarmData(&g_AlarmObject);
    Storage_Clear_AdcData(&g_AdcObject);

    pstMsgHdr = (MsgFramHdr_t *)buf;
    pstMsgHdr->usHdr = MSG_DATA_FRAM_HDR;
    len = sizeof(MsgFramHdr_t) + sizeof(ClearDataRes_t) + sizeof(MsgFramCrc_t);
    pstMsgHdr->usLen = HConvert(&len);
    pstMsgHdr->ucSign = MSG_SIGN_CLEAR_DATA_RES;
    pstClear = (ClearDataRes_t *)(buf + sizeof(MsgFramHdr_t));
    memcpy(&pstClear->stTime, &g_TimeSync.sys_time, sizeof(SystemTime_t));
    pstClear->usStatus = 1;
    pstMsgCrc = (MsgFramCrc_t*)(buf + sizeof(MsgFramHdr_t) + sizeof(ClearDataRes_t));
    pstMsgCrc->usCrc = checkSum_8(buf, len - sizeof(MsgFramCrc_t));
    pstMsgCrc->usCrc = HConvert(&pstMsgCrc->usCrc);
    tcp_server_send_data(&g_TcpServerHandle, buf, len);
    g_config_status = 1;
}

#if 0
static void kt_SetFrontCbias(void *arg)
{
    SetFrontCBiasReq_t *pstFrontCBias = NULL;

    if (arg == NULL) {
        return; 
    }

    pstFrontCBias = (SetFrontCBiasReq_t *)((uint8_t *)arg + sizeof(MsgFramHdr_t));
    g_ConfigInfo.f_Cbias.u = DW_HConvert(&pstFrontCBias->uBias.u);

    g_config_status = 1;
}

static void kt_SetRearVbias(void *arg)
{
    SetRearVBiasReq_t *pstRearVBias = NULL;

    if (arg == NULL) {
        return; 
    }

    pstRearVBias = (SetRearVBiasReq_t *)((uint8_t *)arg + sizeof(MsgFramHdr_t));
    g_ConfigInfo.r_Vbias.u = DW_HConvert(&pstRearVBias->uBias.u);

    g_config_status = 1;
}

static void kt_SetRearCbias(void *arg)
{
    SetRearCBiasReq_t *pstRearCBias = NULL;

    if (arg == NULL) {
        return; 
    }

    pstRearVBias = (SetRearVBiasReq_t *)((uint8_t *)arg + sizeof(MsgFramHdr_t));
    g_ConfigInfo.r_Cbias.u = DW_HConvert(&pstRearCBias->uVBias.u);

    g_config_status = 1;
}

static void kt_SetRearVbias(void *arg)
{
    SetRearVBiasReq_t *pstRearVBias = NULL;

    if (arg == NULL) {
        return; 
    }

    pstRearVBias = (SetRearVBiasReq_t *)((uint8_t *)arg + sizeof(MsgFramHdr_t));
    g_ConfigInfo.r_Vbias.u = DW_HConvert(&pstRearVBias->uVBias.u);

    g_config_status = 1;
}
#endif
/**
 * @brief       获取警告信息
 * @param       arg : 指向获取警告信息报文的指针
 * @retval      无
 */
static void kt_AlarmInfo(void *arg)
{
    uint8_t buf[MAX_QUEUE_DATA_SIZE] = {0};
    MsgFramHdr_t *pstMsgHdr =NULL;
    MsgFramCrc_t *pstMsgCrc = NULL;
    AlarmInfoRes_t *pstAlarmInfo = NULL;
    uint16_t len = 0;
    uint16_t packs = 0;
    AlarmMsg_t alarm = {0};

    if (arg == NULL) {
        return;
    }

    #if 0
    APP_PRINTF("kt_AlarmInfo\r\n");
    #endif
    pstMsgHdr = (MsgFramHdr_t *)buf;
    pstMsgHdr->usHdr = MSG_DATA_FRAM_HDR;
    len = sizeof(MsgFramHdr_t) + sizeof(AlarmInfoRes_t) + sizeof(MsgFramCrc_t);
    pstMsgHdr->usLen = HConvert(&len);
    pstMsgHdr->ucSign = MSG_SIGN_ALARM_INFO_RES;
    pstAlarmInfo = (AlarmInfoRes_t *)(buf + sizeof(MsgFramHdr_t)) ; 
    #if 0  
    packs = g_AlarmObject.page_num * ALARM_INDEX_NUM_PRE_PAGE + g_AlarmObject.item_index;
    #else
    packs = g_ConfigInfo.alarm_num;
    #endif
    pstAlarmInfo->usTotalPack = HConvert(&packs);
    pstMsgCrc = (MsgFramCrc_t *)(buf + sizeof(MsgFramHdr_t) + sizeof(AlarmInfoRes_t));

    for (uint16_t i = 0; i < packs; i++) {

        pstAlarmInfo->usCurrSequ = i;
        pstAlarmInfo->usCurrSequ = HConvert(&pstAlarmInfo->usCurrSequ);
        Storage_Read_AlarmMsg(&g_AlarmObject, i, (uint8_t *)&alarm, 1);
        memcpy(&pstAlarmInfo->stTime, &alarm.time, sizeof(SystemTime_t));
        pstAlarmInfo->ucType = alarm.type;
        pstAlarmInfo->ulTimestamp = DW_HConvert(&alarm.ts);
        pstAlarmInfo->usIdx = HConvert(&alarm.idx);
        pstMsgCrc->usCrc = checkSum_8(buf, len - sizeof(MsgFramCrc_t));
        pstMsgCrc->usCrc = HConvert(&pstMsgCrc->usCrc);
        // enQueue(&g_CirQue, buf, sizeof(buf));
        tcp_server_send_data(&g_TcpServerHandle, buf, len);
    }
}

/**
 * @brief       获取警告数据
 * @param       arg : 指向获取警告数据报文的指针
 * @retval      无
 */
static void kt_AlarmData(void *arg)
{
    MsgFramHdr_t *pstMsgHdr =NULL;
    MsgFramCrc_t *pstMsgCrc = NULL;
    AlarmDataReq_t *pstAlarmReq = NULL;
    AlarmDataRes_t *pstAlarmRes = NULL;
    SystemTime_t *pstSysTime = NULL;
    AdcItem_t *item = NULL;
    uint32_t ts = 0;
    uint16_t len = 0;
    int8_t ret = -1;
    uint32_t off = 0;
    uint16_t curr_index = 0;
    
    if (arg == NULL) {
        return;
    }

    pstAlarmReq = (AlarmDataReq_t *)((uint8_t *)arg + sizeof(MsgFramHdr_t));
    pstSysTime = (SystemTime_t *)&(pstAlarmReq->stTime);
    ts = datetime_to_timestamp(bcd_to_dec(pstSysTime->ucYear) + 2000, bcd_to_dec(pstSysTime->ucMonth), 
                               bcd_to_dec(pstSysTime->ucDay), bcd_to_dec(pstSysTime->ucHour), 
                               bcd_to_dec(pstSysTime->ucMin), bcd_to_dec(pstSysTime->ucScd));
    #if 0   
    APP_PRINTF("kt_AlarmData, ts:%lu\r\n", ts);
    #endif
    memset(g_send, 0, sizeof(g_send));
    pstMsgHdr = (MsgFramHdr_t *)g_send;
    pstMsgHdr->usHdr = MSG_DATA_FRAM_HDR;
    len = sizeof(MsgFramHdr_t) + sizeof(AlarmDataRes_t) + sizeof(MsgFramCrc_t);
    pstMsgHdr->usLen = HConvert(&len);
    pstMsgHdr->ucSign = MSG_SIGN_ALARM_DATA_RES;
    pstAlarmRes = (AlarmDataRes_t *)(g_send + sizeof(MsgFramHdr_t));
    pstMsgCrc = (MsgFramCrc_t*)(g_send + sizeof(MsgFramHdr_t) + sizeof(AlarmDataRes_t)); 
    pstAlarmRes->usTotalPack = ADC_ITEM_NUM_PER_SECOND * 3 / 50;
    pstAlarmRes->usTotalPack = HConvert(&pstAlarmRes->usTotalPack);

#if 0
    for (uint8_t index = 0; index < 15; index ++) {
        ret = Storage_Read_AlarmInfo(&g_AlarmObject, ts, off, g_buf, sizeof(g_buf));
        if (ret == 0) {
            for (uint8_t i = 0 ; i < 200; i++)
            {
                pstAlarmRes->usCurrSequ = i + index * 200;
                pstAlarmRes->usCurrSequ = HConvert(&pstAlarmRes->usCurrSequ);
                item = (AdcItem_t *)(g_buf + i * sizeof(AdcItem_t));
                memcpy(&pstAlarmRes->stTime, &item->time, sizeof(SystemTime_t));
                // memcpy(&pstAlarmRes->item, &alarm[i * sizeof(AdcItem_t)], sizeof(AdcItem_t));
                pstAlarmRes->ullTs = QW_HConvert(&item->ts);
                pstAlarmRes->usFrontC = HConvert(&item->raw_data[0]);
                pstAlarmRes->usFrontV = HConvert(&item->raw_data[1]);
                pstAlarmRes->usRearC = HConvert(&item->raw_data[2]);
                pstAlarmRes->usRearV = HConvert(&item->raw_data[3]);
                pstAlarmRes->uFrontC.u = DW_HConvert(&item->data[0].u);
                pstAlarmRes->uFrontV.u = DW_HConvert(&item->data[1].u);
                pstAlarmRes->uRearC.u = DW_HConvert(&item->data[2].u);
                pstAlarmRes->uRearV.u = DW_HConvert(&item->data[3].u);
                pstMsgCrc->usCrc = checkSum_8(buf, len - sizeof(MsgFramCrc_t));
                pstMsgCrc->usCrc = HConvert(&pstMsgCrc->usCrc);
                tcp_server_send_data(&g_TcpServerHandle, buf, len);
                // enQueue(&g_CirQue, buf, sizeof(buf));
            }
            off += sizeof(g_buf);
        }
    }
#else
    for (uint8_t i = 0; i < 15; i ++) {
        ret = Storage_Read_AlarmInfo(&g_AlarmObject, ts, off, g_buf, 12800);
        if (ret == 0) {
            for (uint8_t num = 0 ; num < (200 / 50); num++) {
                pstAlarmRes->usCurrSequ = HConvert(&curr_index);
                item = (AdcItem_t *)(g_buf + num * 50 * ADC_ITEM_SIZE);
                memcpy(&pstAlarmRes->stTime, &item->time, sizeof(SystemTime_t));
                pstAlarmRes->ullTs = QW_HConvert(&item->ts);
                pstAlarmRes->temp.u = DW_HConvert(&item->temp.u);
                for (uint8_t index = 0; index < 50; index ++) {
                    item = (AdcItem_t *)(g_buf + (index + num * 50) * ADC_ITEM_SIZE);
                    pstAlarmRes->data[index].f_cur.u = DW_HConvert(&item->data[0].u);
                    pstAlarmRes->data[index].f_vlt.u = DW_HConvert(&item->data[1].u);
                    pstAlarmRes->data[index].r_cur.u = DW_HConvert(&item->data[2].u);
                    pstAlarmRes->data[index].r_vlt.u = DW_HConvert(&item->data[3].u);  
                }
                pstMsgCrc->usCrc = checkSum_8(g_send, len - sizeof(MsgFramCrc_t));
                pstMsgCrc->usCrc = HConvert(&pstMsgCrc->usCrc);
                tcp_server_send_data(&g_TcpServerHandle, g_send, len);
                curr_index += 1;
            }
        }
        off += 12800;
    }
#endif
}

/**
 * @brief       获取采样信息
 * @param       arg : 指向获取采样信息报文的指针
 * @retval      无
 */
static void kt_SampleInfo(void *arg)
{
    uint8_t buf[MAX_QUEUE_DATA_SIZE] = {0};
    MsgFramHdr_t *pstMsgHdr =NULL;
    MsgFramCrc_t *pstMsgCrc = NULL;
    SampleInfoRes_t *pstInfoRes = NULL;
    uint16_t len = 0;
    SystemTime_t sTime, eTime;

    pstMsgHdr = (MsgFramHdr_t *)buf;
    pstMsgHdr->usHdr = MSG_DATA_FRAM_HDR;
    len = sizeof(MsgFramHdr_t) + sizeof(SampleInfoRes_t) + sizeof(MsgFramCrc_t);
    pstMsgHdr->usLen = HConvert(&len);
    pstMsgHdr->ucSign = MSG_SIGN_SAMPLE_INFO_RES;
    pstInfoRes = (SampleInfoRes_t *)(buf + sizeof(MsgFramHdr_t));
    timestamp_to_datetime(g_ConfigInfo.str_time, &sTime);
    timestamp_to_datetime(g_ConfigInfo.end_time, &eTime);
    pstInfoRes->stStrTime.ucYear = dec_to_bcd(sTime.ucYear); 
    pstInfoRes->stStrTime.ucMonth = dec_to_bcd(sTime.ucMonth); 
    pstInfoRes->stStrTime.ucDay = dec_to_bcd(sTime.ucDay); 
    pstInfoRes->stStrTime.ucHour = dec_to_bcd(sTime.ucHour); 
    pstInfoRes->stStrTime.ucMin = dec_to_bcd(sTime.ucMin); 
    pstInfoRes->stStrTime.ucScd = dec_to_bcd(sTime.ucScd); 
    pstInfoRes->stEndTime.ucYear = dec_to_bcd(eTime.ucYear); 
    pstInfoRes->stEndTime.ucMonth = dec_to_bcd(eTime.ucMonth); 
    pstInfoRes->stEndTime.ucDay = dec_to_bcd(eTime.ucDay); 
    pstInfoRes->stEndTime.ucHour = dec_to_bcd(eTime.ucHour); 
    pstInfoRes->stEndTime.ucMin = dec_to_bcd(eTime.ucMin); 
    pstInfoRes->stEndTime.ucScd = dec_to_bcd(eTime.ucScd); 
    #if 0
    APP_PRINTF("kt_SampleInfo, str_time:%d, end_time:%d, str:%x-%x-%x %x:%x:%x, end:%x-%x-%x %x:%x:%x\r\n", 
                                    g_ConfigInfo.str_time, g_ConfigInfo.end_time,
                                    pstInfoRes->stStrTime.ucYear, pstInfoRes->stStrTime.ucMonth, pstInfoRes->stStrTime.ucDay,
                                    pstInfoRes->stStrTime.ucHour, pstInfoRes->stStrTime.ucMin, pstInfoRes->stStrTime.ucScd, 
                                    pstInfoRes->stEndTime.ucYear, pstInfoRes->stEndTime.ucMonth, pstInfoRes->stEndTime.ucDay,
                                    pstInfoRes->stEndTime.ucHour, pstInfoRes->stEndTime.ucMin, pstInfoRes->stEndTime.ucScd);
    #endif    
    pstMsgCrc = (MsgFramCrc_t *)(buf + sizeof(MsgFramHdr_t) + sizeof(SampleInfoRes_t));
    pstMsgCrc->usCrc = checkSum_8(buf, len - MSG_DATA_FRAM_CRC_SIZE);
    pstMsgCrc->usCrc = HConvert(&pstMsgCrc->usCrc);
    // enQueue(&g_CirQue, buf, MAX_QUEUE_DATA_SIZE);
    tcp_server_send_data(&g_TcpServerHandle, buf, len);
}


/**
 * @brief       获取采样数据
 * @param       arg : 指向获取采样数据报文的指针
 * @retval      无
 */
static void kt_SampleData(void *arg)
{
    MsgFramHdr_t *pstMsgHdr =NULL;
    MsgFramCrc_t *pstMsgCrc = NULL;
    SampleDataReq_t *pstSamReq = NULL;
    SystemTime_t *pstSysTime = NULL;
    SampleDataRes_t *pstSampleRes = NULL;
    AdcItem_t *item = NULL;
    uint32_t str_ts = 0;
    uint32_t end_ts = 0;
    uint16_t len = 0;
    uint32_t curr_index = 0;
    uint16_t off = 0;
    int8_t ret = -1;

    if (arg == NULL) {
        return;
    }

    pstSamReq = (SampleDataReq_t *)((uint8_t *)arg + sizeof(MsgFramHdr_t));
    pstSysTime = (SystemTime_t *)&(pstSamReq->stStrTime);
    str_ts = datetime_to_timestamp(bcd_to_dec(pstSysTime->ucYear) + 2000, bcd_to_dec(pstSysTime->ucMonth), 
                                   bcd_to_dec(pstSysTime->ucDay), bcd_to_dec(pstSysTime->ucHour), 
                                   bcd_to_dec(pstSysTime->ucMin), bcd_to_dec(pstSysTime->ucScd));
    pstSysTime = (SystemTime_t *)&pstSamReq->stEndTime;
    end_ts = datetime_to_timestamp(bcd_to_dec(pstSysTime->ucYear) + 2000, bcd_to_dec(pstSysTime->ucMonth), 
                                   bcd_to_dec(pstSysTime->ucDay), bcd_to_dec(pstSysTime->ucHour), 
                                   bcd_to_dec(pstSysTime->ucMin), bcd_to_dec(pstSysTime->ucScd));

    #if 0
    APP_PRINTF("kt_SampleData, str_ts:%lu, end_ts:%lu\r\n", str_ts, end_ts);
    #endif
    memset(g_send, 0, sizeof(g_send));
    pstMsgHdr = (MsgFramHdr_t *)g_send;
    pstMsgHdr->usHdr = MSG_DATA_FRAM_HDR;
    len = sizeof(MsgFramHdr_t) + sizeof(SampleDataRes_t) + sizeof(MsgFramCrc_t);
    pstMsgHdr->usLen = HConvert(&len);
    pstMsgHdr->ucSign = MSG_SIGN_SAMPLE_DATA_RES;
    pstSampleRes = (SampleDataRes_t *)(g_send + sizeof(MsgFramHdr_t));
    pstSampleRes->ulTotalPack = (end_ts - str_ts) * ADC_ITEM_NUM_PER_SECOND / 50;
    pstSampleRes->ulTotalPack = DW_HConvert(&pstSampleRes->ulTotalPack);
    pstMsgCrc = (MsgFramCrc_t *)(g_send + sizeof(MsgFramHdr_t) + sizeof(SampleDataRes_t));
    #if 0
    do {
        for (uint8_t i = 0; i < (ADC_ITEM_BLOCK_NUM / 25); i++) {
            ret = Storage_Read_AdcData(&g_AdcObject, str_ts, off, g_buf, 25);//ADC_ITEM_BLOCK_NUM);
            if (ret == 0) {
                for (uint8_t index = 0; index < (25 * ADC_ITEM_NUM_PER_BLOCK); index++) {
                    pstSampleRes->ulCurrSequ = DW_HConvert(&curr_index);
                    item = (AdcItem_t *)(g_buf + index * sizeof(AdcItem_t));
                    memcpy(&pstSampleRes->stTime, &item->time, sizeof(SystemTime_t));
                    pstSampleRes->ullTs = QW_HConvert(&item->ts);
                    pstSampleRes->usFrontC = HConvert(&item->raw_data[0]);
                    pstSampleRes->usFrontV = HConvert(&item->raw_data[1]);
                    pstSampleRes->usRearC = HConvert(&item->raw_data[2]);
                    pstSampleRes->usRearV = HConvert(&item->raw_data[3]);
                    pstSampleRes->uFrontC.u = DW_HConvert(&item->data[0].u);
                    pstSampleRes->uFrontV.u = DW_HConvert(&item->data[1].u);
                    pstSampleRes->uRearC.u = DW_HConvert(&item->data[2].u);
                    pstSampleRes->uRearV.u = DW_HConvert(&item->data[3].u);
                    pstMsgCrc->usCrc = checkSum_8(buf, len - sizeof(MsgFramCrc_t));
                    pstMsgCrc->usCrc = HConvert(&pstMsgCrc->usCrc);
                    // enQueue(que, buf, MAX_QUEUE_DATA_SIZE);
                    tcp_server_send_data(&g_TcpServerHandle, buf, len);
                    curr_index += 1;
                }
                off += 25;
            }
        }
        str_ts += 1;
    } while (str_ts < end_ts);
    #else
    for (; str_ts < end_ts; str_ts++) {
        ret = Storage_Read_AdcData(&g_AdcObject, str_ts, off, g_buf, 125);
        if (ret == 0) {
            for (uint8_t num = 0; num < 20; num++) {
                pstSampleRes->ulCurrSequ = DW_HConvert(&curr_index);
                item = (AdcItem_t *)(g_buf + num * 50 * ADC_ITEM_SIZE);
                memcpy(&pstSampleRes->stTime, &item->time, sizeof(SystemTime_t));
                pstSampleRes->ullTs = QW_HConvert(&item->ts);
                pstSampleRes->temp.u = DW_HConvert(&item->temp.u);
                for (uint8_t index = 0; index < 50; index ++) {
                    item = (AdcItem_t *)(g_buf + (index + num * 50) * ADC_ITEM_SIZE);
                    pstSampleRes->data[index].f_cur.u = DW_HConvert(&item->data[0].u);
                    pstSampleRes->data[index].f_vlt.u = DW_HConvert(&item->data[1].u);
                    pstSampleRes->data[index].r_cur.u = DW_HConvert(&item->data[2].u);
                    pstSampleRes->data[index].r_vlt.u = DW_HConvert(&item->data[3].u);
                }  
                pstMsgCrc->usCrc = checkSum_8(g_send, len - sizeof(MsgFramCrc_t));
                pstMsgCrc->usCrc = HConvert(&pstMsgCrc->usCrc);
                tcp_server_send_data(&g_TcpServerHandle, g_send, len);
                curr_index += 1; 
            }
        } else {
            pstSampleRes->temp.f = ds18b20_get_temp();
            for (uint8_t index = 0; index < 20; index ++) {
                pstSampleRes->ulCurrSequ = DW_HConvert(&curr_index);
                memcpy(&pstSampleRes->stTime, pstSysTime, sizeof(SystemTime_t));
                pstSampleRes->stTime.ucScd += (curr_index / 200);
                pstSampleRes->ullTs = str_ts;
                pstSampleRes->ullTs = pstSampleRes->ullTs * 1000 + index * 50;
                pstSampleRes->ullTs = QW_HConvert(&pstSampleRes->ullTs);
                pstSampleRes->temp.u = DW_HConvert(&pstSampleRes->temp.u);
                pstMsgCrc->usCrc = checkSum_8(g_send, len - sizeof(MsgFramCrc_t));
                pstMsgCrc->usCrc = HConvert(&pstMsgCrc->usCrc);
                tcp_server_send_data(&g_TcpServerHandle, g_send, len);
                curr_index += 1; 
            }
        }
    }
    #endif
}

/**
 * @brief       设置系统时间
 * @param       arg : 指向设置系统时间报文的指针
 * @retval      无
 */
static void kt_SetSysTime(void *arg)
{
    SetTimeReq_t *pstSetTime = NULL;
    SystemTime_t *sys_time = NULL;
    rtc_time time;

    if (arg == NULL) {
        return;
    }

    pstSetTime = (SetTimeReq_t *)((uint8_t *)arg + sizeof(MsgFramHdr_t));
    sys_time = (SystemTime_t *)&pstSetTime->stTime;
    time.ucYear = sys_time->ucYear;
    time.ucMonth = sys_time->ucMonth;
    time.ucDay = sys_time->ucDay;
    time.ucHour = (sys_time->ucHour | 0x80);
    time.ucMinute = sys_time->ucMin;
    time.ucSecond = sys_time->ucScd;

    time.ucWeek = calculateDayOfWeek(bcd_to_dec(time.ucYear) + 2000, bcd_to_dec(time.ucMonth), bcd_to_dec(time.ucDay));
    time.ucWeek = dec_to_bcd(time.ucWeek);
    #if 0
    APP_PRINTF("kt_SetSysTime, time:%x-%x-%x %x %x:%x:%x\r\n",time.ucYear, time.ucMonth, time.ucDay,
                time.ucWeek, time.ucHour, time.ucMinute, time.ucSecond);
    #endif
    if (xSemaphoreTake(g_TimeSync.mutex, pdMS_TO_TICKS(10)) == pdPASS) {
        sd2505_set_time(&time);
        xSemaphoreGive(g_TimeSync.mutex);
    }
}

void system_init(void)
{
    // sys_config_write(&g_ConfigInfo);
    sys_config_read(&g_ConfigInfo);
    if (g_ConfigInfo.flag != DEV_FLAG_FIRST_ON && g_ConfigInfo.flag != DEV_FLAG_NEXT_ON) {
        g_ConfigInfo.hw[0] = HW_VERSION_MAJOR;
        g_ConfigInfo.hw[1] = HW_VERSION_MINOR;
        g_ConfigInfo.hw[2] = HW_VERSION_PATCH;
        g_ConfigInfo.sw[0] = SW_VERSION_MAJOR;
        g_ConfigInfo.sw[1] = SW_VERSION_MINOR;
        g_ConfigInfo.sw[2] = SW_VERSION_PATCH;
        g_ConfigInfo.f_Vmax.f = BUS_VOLTAGE_UPPER_LIMIT_V;
        g_ConfigInfo.f_Vmin.f = BUS_VOLTAGE_LOWER_LIMIT_V;
        g_ConfigInfo.r_Vmax.f = BUS_VOLTAGE_UPPER_LIMIT_V;
        g_ConfigInfo.r_Vmin.f = BUS_VOLTAGE_LOWER_LIMIT_V;
        g_ConfigInfo.flag = DEV_FLAG_FIRST_ON;
        g_ConfigInfo.r_Cbias0.f = 0.0f;
        g_ConfigInfo.f_Cbias0.f = 0.0f;
        g_ConfigInfo.r_Vbias0.f = 0.0f;
        g_ConfigInfo.f_Vbias0.f = 0.0f;
        g_ConfigInfo.f_Vgain.f = 1.0f;
        g_ConfigInfo.f_Cgain.f = 1.0f;
        g_ConfigInfo.r_Vgain.f = 1.0f;
        g_ConfigInfo.r_Cgain.f = 1.0f;
        g_ConfigInfo.cal_flag = 0;
        sys_config_write(&g_ConfigInfo); 
        rtc_time time = {0x00, 0x50, 0x08, 0x02, 0x30, 0x12, 0x25};
        sd2505_set_time(&time);
        gd55b01ge_erase_chip();
    } else if (g_ConfigInfo.flag == DEV_FLAG_FIRST_ON && g_ConfigInfo.cal_flag) {
        g_ConfigInfo.flag = DEV_FLAG_NEXT_ON;
    }
    APP_PRINTF("g_ConfigInfo.r_Cbias0.f:%lf, g_ConfigInfo.f_Cbias0.f:%lf, g_ConfigInfo.r_Vbias0.f:%lf, g_ConfigInfo.f_Vbias0.f:%lf\r\n",
                g_ConfigInfo.r_Cbias0.f, g_ConfigInfo.f_Cbias0.f, g_ConfigInfo.r_Vbias0.f, g_ConfigInfo.f_Vbias0.f);
    APP_PRINTF("g_ConfigInfo.f_Vgain.f:%lf, g_ConfigInfo.f_Cgain.f:%lf, g_ConfigInfo.r_Vgain.f:%lf, g_ConfigInfo.r_Cgain.f:%lf\r\n",       
                g_ConfigInfo.f_Vgain.f, g_ConfigInfo.f_Cgain.f, g_ConfigInfo.r_Vgain.f, g_ConfigInfo.r_Cgain.f);
    APP_PRINTF("g_ConfigInfo.flag:%d,  block_num:%d, curr_block:%d, str_time:%lu, end_time:%lu, alarm_num:%d\r\n",       
                g_ConfigInfo.flag,  g_ConfigInfo.block_num, g_ConfigInfo.curr_block, g_ConfigInfo.str_time, g_ConfigInfo.end_time, g_ConfigInfo.alarm_num);
}

/**
 * @brief  从数据队列收指令，处理数据
 */
void msg_proc_task(void *pvParameters)
{
    uint8_t msg[MAX_QUEUE_DATA_SIZE] = {0};
    MsgPraseInterface_t *pstInterface = (MsgPraseInterface_t *)g_MsgPraseInterTable;
    MsgFramHdr_t *pstMsgHdr = NULL;
    MsgFramCrc_t *pstMsgCrc = NULL;
    uint16_t len = 0;
    uint16_t check_crc = 0;
    uint8_t i = 0;
    
    if (pvParameters == NULL) {
        return;
    }

    TcpServerHandle_t *handle = (TcpServerHandle_t *)pvParameters;

    for (;;)
    {
        // 从“数据处理的消息队列”接收消息
        if(xQueueReceive(handle->queue, msg, portMAX_DELAY) == pdPASS)
        {
            pstMsgHdr = (MsgFramHdr_t *)msg;
            len = HConvert(&pstMsgHdr->usLen);
            pstMsgCrc = (MsgFramCrc_t *)(msg + len - MSG_DATA_FRAM_CRC_SIZE);
            check_crc = checkSum_8(msg, len - MSG_DATA_FRAM_CRC_SIZE);
            #if 0
            APP_PRINTF("stpMsg->usHdr 0x%x, wCrc 0x%x, wChkSum 0x%x, stpMsg->ucSign 0x%x\r\n", HConvert(&pstMsgHdr->usHdr), HConvert(&pstMsgCrc->usCrc), check_crc, pstMsgHdr->ucSign);
            #endif
            if (MSG_DATA_FRAM_HDR == HConvert(&pstMsgHdr->usHdr) && check_crc == HConvert(&pstMsgCrc->usCrc))
            {
                for (i = 0; pstInterface[i].pFuncEntry != NULL; i++)
                {
                    if (pstInterface[i].ucMsgSign == pstMsgHdr->ucSign)
                    {
                        pstInterface[i].pFuncEntry(msg);
                        break;
                    }
                }  
            }
        }
    }
}


/**
 * @brief  ：从“发送消息队列”收消息，执行发送
 */
void msg_send_task(void *pvParameters)
{
    // uint8_t msg[MAX_QUEUE_DATA_SIZE] = {0};
    // uint8_t ret = TRUE;
    // uint8_t front = 0;
    // uint16_t len = 0;
    // ThreadSafeCirQue *que = &g_CirQue;
    // MsgFramHdr_t *pstMsgHdr =NULL;

    // if (pvParameters == NULL) {
    //     return;
    // }
    uint8_t tmp_status = 0;
    // TcpServerHandle_t *handle = (TcpServerHandle_t *)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount(); 

    for (;;)
    {
        // for (front = que->ucFront; front <= que->ucRear; front++)
        // {
        //     ret = deQueue(que, msg);
        //     if (TRUE != ret)
        //     {
        //         break;
        //     }
        //     pstMsgHdr = (MsgFramHdr_t *)&msg;
        //     len = HConvert(&pstMsgHdr->usLen);
        //     tcp_server_send_data(handle, msg, len);
        // } 

        if (g_config_status) {
            g_config_status = 0;
            sys_config_write(&g_ConfigInfo);
        }
        if (tmp_status == 0) {
            ds18b20_start_convert();
            tmp_status = 1;
        } else {
            ds18b20_read_temp();
            tmp_status = 0;
        }

        // vTaskDelay(pdMS_TO_TICKS(10));  // 降低CPU占用
         vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000));
    }
}

void msg_parse_start(void)
{
    g_TcpServerHandle.queue = xQueueCreate(10, 128);
    // initCirQueue(&g_CirQue); 

    tcp_server_start(&g_TcpServerHandle);

    xTaskCreate((TaskFunction_t)msg_proc_task,            // 任务函数
                (const char*   )"msg_proc_task",          // 任务名称
                (uint16_t      )MSGPROC_STACK_SIZE,       // 任务栈大小
                (void*         )&g_TcpServerHandle,       // 任务参数
                (UBaseType_t   )MSGPROC_TASK_PRIO,        // 任务优先级
                (TaskHandle_t* )&MsgProc_Task_Handle);    // 任务句柄

    xTaskCreate((TaskFunction_t)msg_send_task,            // 任务函数
                (const char*   )"msg_send_task",          // 任务名称
                (uint16_t      )MSGSEND_STACK_SIZE,       // 任务栈大小
                (void*         )&g_TcpServerHandle,       // 任务参数
                (UBaseType_t   )MSGSEND_TASK_PRIO,        // 任务优先级
                (TaskHandle_t* )&MsgSend_Task_Handle);    // 任务句柄
}
