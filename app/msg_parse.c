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
static ThreadSafeCirQue g_CirQue = {0};
TcpServerHandle_t g_TcpServerHandle = {0};

TaskHandle_t MsgProc_Task_Handle;
TaskHandle_t MsgSend_Task_Handle;
static uint8_t g_buf[25 * ADC_ITEM_NUM_PER_BLOCK * ADC_ITEM_SIZE] = {0};
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

    APP_PRINTF("kt_ConfigInfo\r\n");
    pstMsgHdr = (MsgFramHdr_t*)buf;
    pstMsgHdr->usHdr = MSG_DATA_FRAM_HDR;
    len = sizeof(MsgFramHdr_t) + sizeof(ConfigInfoRes_t) + sizeof(MsgFramCrc_t);
    pstMsgHdr->usLen = HConvert(&len);
    pstMsgHdr->ucSign = MSG_SIGN_CONFIG_INFO_RES;
    pstConfig = (ConfigInfoRes_t*)(buf + sizeof(MsgFramHdr_t));
    memcpy(pstConfig->hw, g_ConfigInfo.hw, sizeof(g_ConfigInfo.hw));
    memcpy(pstConfig->sw, g_ConfigInfo.sw, sizeof(g_ConfigInfo.sw));
    pstConfig->f_Vmax.u = DW_HConvert(&g_ConfigInfo.f_Vmax.u);
    pstConfig->f_Vmin.u = DW_HConvert(&g_ConfigInfo.f_Vmin.u);
    pstConfig->r_Vmax.u = DW_HConvert(&g_ConfigInfo.r_Vmax.u);
    pstConfig->r_Vmin.u = DW_HConvert(&g_ConfigInfo.r_Vmin.u);
    memcpy(&pstConfig->stTime, &g_TimeSync.sys_time, sizeof(SystemTime_t));
    pstMsgCrc = (MsgFramCrc_t*)(buf + sizeof(MsgFramHdr_t) + sizeof(ConfigInfoRes_t));
    pstMsgCrc->usCrc = checkSum_8(buf, len - sizeof(MsgFramCrc_t));
    pstMsgCrc->usCrc = HConvert(&pstMsgCrc->usCrc);
    enQueue(&g_CirQue, buf, sizeof(buf));
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
    AlarmMsg_t alarm;

    if (arg == NULL) {
        return;
    }

    APP_PRINTF("kt_AlarmInfo\r\n");
    pstMsgHdr = (MsgFramHdr_t *)buf;
    pstMsgHdr->usHdr = MSG_DATA_FRAM_HDR;
    len = sizeof(MsgFramHdr_t) + sizeof(AlarmInfoRes_t) + sizeof(MsgFramCrc_t);
    pstMsgHdr->usLen = HConvert(&len);
    pstMsgHdr->ucSign = MSG_SIGN_ALARM_INFO_RES;
    pstAlarmInfo = (AlarmInfoRes_t *)(buf + sizeof(MsgFramHdr_t)) ; 
    #if 0  
    packs = g_AlarmObject.page_num * ALARM_INDEX_NUM_PRE_PAGE + g_AlarmObject.item_index;
    #else
    packs = g_AlarmObject.index_num;
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
        enQueue(&g_CirQue, buf, sizeof(buf));
    }
}

/**
 * @brief       获取警告数据
 * @param       arg : 指向获取警告数据报文的指针
 * @retval      无
 */
static void kt_AlarmData(void *arg)
{
    uint8_t buf[MAX_QUEUE_DATA_SIZE] = {0};
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
    
    if (arg == NULL) {
        return;
    }

    // que = (ThreadSafeCirQue *)arg;
    pstAlarmReq = (AlarmDataReq_t *)((uint8_t *)arg + sizeof(MsgFramHdr_t));
    pstSysTime = (SystemTime_t *)&(pstAlarmReq->stTime);
    ts = datetime_to_timestamp(bcd_to_dec(pstSysTime->ucYear) + 2000, bcd_to_dec(pstSysTime->ucMonth), 
                               bcd_to_dec(pstSysTime->ucDay), bcd_to_dec(pstSysTime->ucHour), 
                               bcd_to_dec(pstSysTime->ucMin), bcd_to_dec(pstSysTime->ucScd));
    
    APP_PRINTF("kt_AlarmData, ts:%lu\r\n", ts);
    pstMsgHdr = (MsgFramHdr_t *)buf;
    pstMsgHdr->usHdr = MSG_DATA_FRAM_HDR;
    len = sizeof(MsgFramHdr_t) + sizeof(AlarmDataRes_t) + sizeof(MsgFramCrc_t);
    pstMsgHdr->usLen = HConvert(&len);
    pstMsgHdr->ucSign = MSG_SIGN_ALARM_DATA_RES;
    pstAlarmRes = (AlarmDataRes_t *)(buf + sizeof(MsgFramHdr_t));
    pstMsgCrc = (MsgFramCrc_t*)(buf + sizeof(MsgFramHdr_t) + sizeof(AlarmDataRes_t)); 
    pstAlarmRes->usTotalPack = ADC_ITEM_NUM_PER_SECOND * 3;
    pstAlarmRes->usTotalPack = HConvert(&pstAlarmRes->usTotalPack);

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
    // APP_PRINTF("kt_SampleInfo, str_time:%d, end_time:%d\r\n", g_ConfigInfo.str_time, g_ConfigInfo.end_time);
    // Get_AdcData_Time(&g_AdcObject, &sTime, &eTime);
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
    APP_PRINTF("kt_SampleInfo, str_time:%d, end_time:%d, str:%x-%x-%x %x:%x:%x, end:%x-%x-%x %x:%x:%x\r\n", 
                                    g_ConfigInfo.str_time, g_ConfigInfo.end_time,
                                    pstInfoRes->stStrTime.ucYear, pstInfoRes->stStrTime.ucMonth, pstInfoRes->stStrTime.ucDay,
                                    pstInfoRes->stStrTime.ucHour, pstInfoRes->stStrTime.ucMin, pstInfoRes->stStrTime.ucScd, 
                                    pstInfoRes->stEndTime.ucYear, pstInfoRes->stEndTime.ucMonth, pstInfoRes->stEndTime.ucDay,
                                    pstInfoRes->stEndTime.ucHour, pstInfoRes->stEndTime.ucMin, pstInfoRes->stEndTime.ucScd);
    pstMsgCrc = (MsgFramCrc_t *)(buf + sizeof(MsgFramHdr_t) + sizeof(SampleInfoRes_t));
    pstMsgCrc->usCrc = checkSum_8(buf, len - MSG_DATA_FRAM_CRC_SIZE);
    pstMsgCrc->usCrc = HConvert(&pstMsgCrc->usCrc);
    enQueue(&g_CirQue, buf, MAX_QUEUE_DATA_SIZE);
}


/**
 * @brief       获取采样数据
 * @param       arg : 指向获取采样数据报文的指针
 * @retval      无
 */
static void kt_SampleData(void *arg)
{
    uint8_t buf[MAX_QUEUE_DATA_SIZE] = {0};
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
//    ThreadSafeCirQue *que = (ThreadSafeCirQue *)&g_CirQue;
    // uint8_t adc[25 * ADC_ITEM_NUM_PER_BLOCK * ADC_ITEM_SIZE];
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

    APP_PRINTF("kt_SampleData, str_ts:%d, end_ts:%d\r\n", str_ts, end_ts);
    
    pstMsgHdr = (MsgFramHdr_t *)buf;
    pstMsgHdr->usHdr = MSG_DATA_FRAM_HDR;
    len = sizeof(MsgFramHdr_t) + sizeof(SampleDataRes_t) + sizeof(MsgFramCrc_t);
    pstMsgHdr->usLen = HConvert(&len);
    pstMsgHdr->ucSign = MSG_SIGN_SAMPLE_DATA_RES;
    pstSampleRes = (SampleDataRes_t *)(buf + sizeof(MsgFramHdr_t));
    pstSampleRes->ulTotalPack = (end_ts - str_ts) * ADC_ITEM_NUM_PER_SECOND;
    pstSampleRes->ulTotalPack = DW_HConvert(&pstSampleRes->ulTotalPack);
    pstMsgCrc = (MsgFramCrc_t *)(buf + sizeof(MsgFramHdr_t) + sizeof(SampleDataRes_t));
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
    for (; str_ts < end_ts; str_ts ++) {
        for (uint8_t i = 0; i < (ADC_ITEM_BLOCK_NUM / 25); i++) {
            ret = Storage_Read_AdcData(&g_AdcObject, str_ts, off, g_buf, 25);//ADC_ITEM_BLOCK_NUM);
            if (ret == 0) {
                for (uint8_t num = 0; num < ((25 * ADC_ITEM_NUM_PER_BLOCK) / 50); num++) {
                    pstSampleRes->ulCurrSequ = DW_HConvert(&curr_index);
                    item = (AdcItem_t *)g_buf;
                    memcpy(&pstSampleRes->stTime, &item->time, sizeof(SystemTime_t));
                    pstSampleRes->ullTs = QW_HConvert(&item->ts);
                    for (uint8_t index = 0; index < 50; index ++) {
                        item = (AdcItem_t *)(g_buf + (index + num * 50 * sizeof(AdcItem_t)));
                        pstSampleRes->data[i].f_cur.u = DW_HConvert(&item->data[0].u);
                        pstSampleRes->data[i].f_vlt.u = DW_HConvert(&item->data[1].u);
                        pstSampleRes->data[i].r_cur.u = DW_HConvert(&item->data[2].u);
                        pstSampleRes->data[i].r_vlt.u = DW_HConvert(&item->data[3].u);
                    }  
                    pstMsgCrc->usCrc = checkSum_8(buf, len - sizeof(MsgFramCrc_t));
                    pstMsgCrc->usCrc = HConvert(&pstMsgCrc->usCrc);
                    tcp_server_send_data(&g_TcpServerHandle, buf, len);
                    curr_index += 1; 
                }
            }
            off += 25;
        }
        off = 0;
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
    time.ucHour = (sys_time->ucHour & 0x7F);
    time.ucMinute = sys_time->ucMin;
    time.ucSecond = sys_time->ucScd;

    time.ucWeek = calculateDayOfWeek(bcd_to_dec(time.ucYear) + 2000, bcd_to_dec(time.ucMonth), bcd_to_dec(time.ucDay));
    time.ucWeek = dec_to_bcd(time.ucWeek);
    APP_PRINTF("kt_SetSysTime, time:%x-%x-%x %x %x:%x:%x\r\n",time.ucYear, time.ucMonth, time.ucDay,
                time.ucWeek, time.ucHour, time.ucMinute, time.ucSecond);
    if (xSemaphoreTake(g_TimeSync.mutex, pdMS_TO_TICKS(10)) == pdPASS) {
        // memcpy(&g_TimeSync.sys_time, sys_time, sizeof(SystemTime_t));
        // g_TimeSync.flag = 1;
        sd2505_set_time(&time);
        xSemaphoreGive(g_TimeSync.mutex);
    }
}

void system_init(void)
{
    memset(&g_ConfigInfo, 0, sizeof(ConfigInfo_t));
    sys_config_write(&g_ConfigInfo);
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
        sys_config_write(&g_ConfigInfo); 
        rtc_time time = {0x00, 0x21, 0x10, 0x04, 0x25, 0x12, 0x25};
        sd2505_set_time(&time);
        gd55b01ge_erase_chip();
    } else if (g_ConfigInfo.flag == DEV_FLAG_FIRST_ON) {
        g_ConfigInfo.flag = DEV_FLAG_NEXT_ON;
        sys_config_write(&g_ConfigInfo); 
    }
}

/**
 * @brief  从数据队列收指令，处理数据
 */
void msg_proc_task(void *pvParameters)
{
    uint8_t msg[MAX_QUEUE_DATA_SIZE] = {0};
    // uint8_t res[MAX_QUEUE_DATA_SIZE] = {0};
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

    while (1)
    {
        // 从“数据处理的消息队列”接收消息
        if(xQueueReceive(handle->queue, msg, portMAX_DELAY) == pdPASS)
        {
            pstMsgHdr = (MsgFramHdr_t *)msg;
            len = HConvert(&pstMsgHdr->usLen);
            pstMsgCrc = (MsgFramCrc_t *)(msg + len - MSG_DATA_FRAM_CRC_SIZE);
            check_crc = checkSum_8(msg, len - MSG_DATA_FRAM_CRC_SIZE);
            // APP_PRINTF("stpMsg->usHdr 0x%x, wCrc 0x%x, wChkSum 0x%x, stpMsg->ucSign 0x%x\r\n", HConvert(&pstMsgHdr->usHdr), HConvert(&pstMsgCrc->usCrc), check_crc, pstMsgHdr->ucSign);
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
    uint8_t msg[MAX_QUEUE_DATA_SIZE] = {0};
    uint8_t ret = TRUE;
    uint8_t front = 0;
    uint16_t len = 0;
    ThreadSafeCirQue *que = &g_CirQue;
    MsgFramHdr_t *pstMsgHdr =NULL;

    if (pvParameters == NULL) {
        return;
    }

    TcpServerHandle_t *handle = (TcpServerHandle_t *)pvParameters;

    while (1)
    {
        for (front = que->ucFront; front <= que->ucRear; front++)
        {
            ret = deQueue(que, msg);
            if (TRUE != ret)
            {
                break;
            }
            pstMsgHdr = (MsgFramHdr_t *)&msg;
            len = HConvert(&pstMsgHdr->usLen);
            tcp_server_send_data(handle, msg, len);
        } 
        if (g_config_status) {
            g_config_status = 0;
            sys_config_write(&g_ConfigInfo);
        }

        vTaskDelay(pdMS_TO_TICKS(10));  // 降低CPU占用
    }
}

void msg_parse_start(void)
{
    g_TcpServerHandle.queue = xQueueCreate(3, 128);
    initCirQueue(&g_CirQue); 

    tcp_server_start(&g_TcpServerHandle);

#if 1
    // 创建报文处理的任务
    xTaskCreate((TaskFunction_t)msg_proc_task,            // 任务函数
                (const char*   )"msg_proc_task",          // 任务名称
                (uint16_t      )MSGPROC_STACK_SIZE,       // 任务栈大小
                (void*         )&g_TcpServerHandle,       // 任务参数
                (UBaseType_t   )MSGPROC_TASK_PRIO,        // 任务优先级（低于定时器中断优先级）
                (TaskHandle_t* )&MsgProc_Task_Handle);    // 任务句柄

    // 创建报文相应的任务
    xTaskCreate((TaskFunction_t)msg_send_task,             // 任务函数
                (const char*   )"msg_send_task",          // 任务名称
                (uint16_t      )MSGSEND_STACK_SIZE,       // 任务栈大小
                (void*         )&g_TcpServerHandle,       // 任务参数
                (UBaseType_t   )MSGSEND_TASK_PRIO,        // 任务优先级（低于定时器中断优先级）
                (TaskHandle_t* )&MsgSend_Task_Handle);    // 任务句柄
#endif
}
