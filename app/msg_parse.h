#ifndef __MSG_PARSE_H
#define __MSG_PARSE_H

#include "utils.h"
#include "storage_manage.h"
#include "tcp_server.h"
#include "adc_sample.h"

#define MSG_DATA_FRAM_HDR         0xAAAA    
#define MSG_DATA_FRAM_HDR_OFF     0
#define MSG_DATA_FTAM_HDR_SIZE    2

#define MSG_DATA_FRAM_LEN_OFF     2
#define MSG_DATA_FRAM_LEN_SIZE    2

#define MSG_DATA_FRAM_SIGN_OFF    4
#define MSG_DATA_FRAM_SIGN_SIZE   1

#define MSG_DATA_FRAM_DATA_OFF    5

#define MSG_DATA_FRAM_CRC_SIZE    2


#define MSG_SIGN_CONFIG_INFO_REQ    0x10
#define MSG_SIGN_CONFIG_INFO_RES    0x80

#define MSG_SIGN_SET_F_VMAX_REQ     0x11
#define MSG_SIGN_SET_F_VMAX_RES     0x81

#define MSG_SIGN_SET_F_VMIN_REQ     0x12
#define MSG_SIGN_SET_F_VMIN_RES     0x82

#define MSG_SIGN_SET_R_VMAX_REQ     0x13
#define MSG_SIGN_SET_R_VMAX_RES     0x83

#define MSG_SIGN_SET_R_VMIN_REQ     0x14
#define MSG_SIGN_SET_R_VMIN_RES     0x84

#define MSG_SIGN_SET_TIME_REQ       0x15
#define MSG_SIGN_SET_TIME_RES       0x85

#define MSG_SIGN_SET_F_VBIAS_REQ    0x16
#define MSG_SIGN_SET_F_VBIAS_RES    0x86

#define MSG_SIGN_SET_F_CBIAS_REQ    0x17
#define MSG_SIGN_SET_F_CBIAS_RES    0x87

#define MSG_SIGN_SET_R_VBIAS_REQ    0x18
#define MSG_SIGN_SET_R_VBIAS_RES    0x88

#define MSG_SIGN_SET_R_CBIAS_REQ    0x19
#define MSG_SIGN_SET_R_CBIAS_RES    0x89

#define MSG_SIGN_CLEAR_DATA_REQ     0x1A
#define MSG_SIGN_CLEAR_DATA_RES     0x8A

#define MSG_SIGN_CLEAR_CONFIG_REQ   0x1B
#define MSG_SIGN_CLEAR_CONFIG_RES   0x8B

#define MSG_SIGN_ALARM_INFO_REQ     0x20
#define MSG_SIGN_ALARM_INFO_RES     0x90  

#define MSG_SIGN_ALARM_DATA_REQ     0x21
#define MSG_SIGN_ALARM_DATA_RES     0x91

#define MSG_SIGN_SAMPLE_INFO_REQ    0x30
#define MSG_SIGN_SAMPLE_INFO_RES    0xA0

#define MSG_SIGN_SAMPLE_DATA_REQ    0x31
#define MSG_SIGN_SAMPLE_DATA_RES    0xA1

// #define MSG_SIGN_UPGRADE_HANDSHAKE        0x50
// #define MSG_SIGN_UPGRADE_HANDSHAKE_RESP   0x51
// #define MSG_SIGN_TRANS_FIRMWARE           0x52
// #define MSG_SIGN_TRANS_FIRMWARE_RESP      0x53
// #define MSG_SIGN_DO_UPGRADE_FIRMWARE      0x54
// #define MSG_SIGN_DO_UPGRADE_FIRMWARE_RESP 0x55

#define LConvert(x)  ((uint16_t)((*((uint8_t*)x)) | (*((uint8_t*)x+1) << 8)))
#define HConvert(x)  ((uint16_t)((*((uint8_t*)x+1)) | (*((uint8_t*)x) << 8)))

#define DW_HConvert(x)  ((uint32_t)((*((uint8_t*)(x) + 3)) | (*((uint8_t*)(x) + 2) << 8) | \
                                    (*((uint8_t*)(x) + 1) << 16) | (*((uint8_t*)(x)) << 24)))

#define QW_HConvert(x)  ((uint64_t)DW_HConvert(x) << 32 | DW_HConvert((uint8_t*)(x) + 4))


#define DEV_FLAG_FIRST_ON   0x5A
#define DEV_FLAG_NEXT_ON    0xA5
extern volatile uint8_t g_config_status;
#pragma pack(1)
typedef struct 
{
    uint8_t hw[3];
    uint8_t sw[3];
    uint8_t flag;
    FloatUInt32_t f_Vmax;
    FloatUInt32_t f_Vmin;
    FloatUInt32_t f_Cmax;
    FloatUInt32_t f_Cmin;
    FloatUInt32_t r_Vmax;
    FloatUInt32_t r_Vmin;
    FloatUInt32_t r_Cmax;
    FloatUInt32_t r_Cmin;
    FloatUInt32_t f_Vbias0;
    FloatUInt32_t f_Cbias0;
    FloatUInt32_t r_Vbias0;
    FloatUInt32_t r_Cbias0;
    FloatUInt32_t f_Vgain;
    FloatUInt32_t f_Cgain;
    FloatUInt32_t r_Vgain;
    FloatUInt32_t r_Cgain;
    uint8_t cal_flag;

    #if 0
    uint16_t page_num;
    uint16_t curr_page;
    #else
    uint16_t alarm_num;
    #endif
    uint16_t block_num;
    uint16_t curr_block;
    uint32_t str_time;
    uint32_t end_time;
} ConfigInfo_t;
#pragma pack()

extern ConfigInfo_t g_ConfigInfo;


/* 应用层协议报文格式 */
#pragma pack(1)
typedef struct 
{
    uint16_t usHdr;      /* 帧头 */
    uint16_t usLen;      /* 长度 */
    uint8_t ucSign;      /* 标识 */
} MsgFramHdr_t; 

typedef struct
{
    uint16_t usCrc;      /* 校验 */
} MsgFramCrc_t; 

typedef struct 
{
    uint8_t hw[3];
    uint8_t sw[3];
    FloatUInt32_t temp;
    FloatUInt32_t f_VBias;
    FloatUInt32_t f_CBias;
    FloatUInt32_t r_VBias;
    FloatUInt32_t r_CBias;
    FloatUInt32_t f_Vmax;
    FloatUInt32_t f_Vmin;
    FloatUInt32_t r_Vmax;
    FloatUInt32_t r_Vmin;
    SystemTime_t stTime;
    uint8_t Rsvd[9];
} ConfigInfoRes_t;

typedef struct 
{
    FloatUInt32_t uVmax;
    uint8_t Rsvd[5];
} SetFrontVmaxReq_t, SetRearVmaxReq_t;

typedef struct 
{
    FloatUInt32_t uVmin;
    uint8_t Rsvd[5];
} SetFrontVminReq_t, SetRearVminReq_t;

typedef struct 
{
    FloatUInt32_t uBias;
    uint8_t Rsvd[5];
} SetFrontVBiasReq_t, SetFrontCBiasReq_t, SetRearVBiasReq_t, SetRearCBiasReq_t;

typedef struct 
{
    SystemTime_t stTime;
    uint8_t usStatus;
    uint8_t Rsvd[2];
} ClearDataRes_t, ClearConfigRes_t;

//typedef struct
//{

//} AlarmInfoReq_t;

typedef struct
{
    uint16_t usTotalPack;
    uint16_t usCurrSequ;
    SystemTime_t stTime;
    uint32_t ulTimestamp;
    uint16_t usIdx;
    uint8_t ucType;
    uint8_t ucRsvd[8];
} AlarmInfoRes_t;

typedef struct
{
    SystemTime_t stTime;
    uint8_t ucRsvd[3];
} AlarmDataReq_t;

#if 0
typedef struct
{
    uint16_t usTotalPack;
    uint16_t usCurrSequ;
    SystemTime_t stTime;
    uint64_t ullTs;
    uint16_t usFrontC;
    uint16_t usFrontV;
    uint16_t usRearC;
    uint16_t usRearV;
    FloatUInt32_t uFrontC;
    FloatUInt32_t uFrontV;
    FloatUInt32_t uRearC;
    FloatUInt32_t uRearV;
    uint8_t usRsvd[15];
} AlarmDataRes_t;
#else 
typedef struct
{
    uint16_t usTotalPack;
    uint16_t usCurrSequ;
    SystemTime_t stTime;
    uint64_t ullTs;
    FloatUInt32_t temp;
    SampleData_t data[50];
    uint8_t usRsvd[3];
} AlarmDataRes_t;
#endif
typedef struct
{
    SystemTime_t stStrTime;
    SystemTime_t stEndTime;
    uint8_t ucRsvd[13];
} SampleInfoRes_t;

typedef struct
{   
    SystemTime_t stStrTime;
    SystemTime_t stEndTime;
    uint8_t ucRsvd[13];
} SampleDataReq_t;
#if 0
typedef struct
{
    uint32_t ulTotalPack;
    uint32_t ulCurrSequ;
    SystemTime_t stTime;
    uint64_t ullTs;
    uint16_t usFrontC;
    uint16_t usFrontV;
    uint16_t usRearC;
    uint16_t usRearV;
    FloatUInt32_t uFrontC;
    FloatUInt32_t uFrontV;
    FloatUInt32_t uRearC;
    FloatUInt32_t uRearV;
    uint8_t usRsvd[11];
} SampleDataRes_t;
#else
typedef struct
{
    uint32_t ulTotalPack;
    uint32_t ulCurrSequ;
    SystemTime_t stTime;
    uint64_t ullTs;
    FloatUInt32_t temp;
    SampleData_t data[50];
    uint8_t usRsvd[15];
} SampleDataRes_t;
#endif

typedef struct
{
    SystemTime_t stTime;
    uint8_t usRsvd[3];
} SetTimeReq_t;
#pragma pack()


// #define MSG_ANS  0x00
// #define MSG_NANS 0x01


typedef uint8_t (*pfn_vp_vp_t)(void *, void *);
typedef void (*pfn_vp_t)(void *);

typedef struct
{
    uint8_t ucMsgSign;
    pfn_vp_t pFuncEntry;
    pfn_vp_t pFuncCb;
} MsgPraseInterface_t;


#define MSGPROC_TASK_PRIO     (configMAX_PRIORITIES - 3)
#define MSGPROC_STACK_SIZE    1024

#define MSGSEND_TASK_PRIO     (configMAX_PRIORITIES - 3)
#define MSGSEND_STACK_SIZE    1024

typedef struct
{
    TcpServerHandle_t *handle;
    TaskHandle_t proc_Handle;
    TaskHandle_t send_handle;
    ThreadSafeCirQue *cir_que;
} MsgProc_Typedef_t;



void system_init(void);
void msg_proc_task(void *pvParameters);
void msg_parse_start(void);
#endif /* __MSG_ANALYSIS_H */
