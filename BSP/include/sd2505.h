#ifndef __SD2505_H
#define __SD2505_H

#include <stdint.h>


#ifndef False
#define False   1
#endif

#ifndef True
#define True    0
#endif


#define SD2505_DEV_ADDR              0x64       /* 0110 010x 8bit */

#define SD2505_CLOCK_SPEED           400000U
/* SD2505API-G寄存器地址定义 */ 
#define SD2505_ADDR_SECOND           0x00 // 秒
#define SD2505_ADDR_MINUTE           0x01 // 分钟
#define SD2505_ADDR_HOUR             0x02 // 小时
#define SD2505_ADDR_DAY              0x03 // 日
#define SD2505_ADDR_WEEK             0x04 // 星期
#define SD2505_ADDR_MONTH            0x05 // 月份
#define SD2505_ADDR_YEAR             0x06 // 年份
#define SD2505_ADDR_SECOND_ALARM     0x07 // 秒报警
#define SD2505_ADDR_MINUTE_ALARM     0x08 // 分钟报警
#define SD2505_ADDR_HOUR_ALARM       0x09 // 小时报警
#define SD2505_ADDR_WEEK_ALARM       0x0A // 星期报警
#define SD2505_ADDR_DAY_ALARM        0x0B // 日报警
#define SD2505_ADDR_MONTN_ALARM      0x0C // 月报警
#define SD2505_ADDR_YEAR_ALARM       0x0D // 年报警
#define SD2505_ADDR_PERMIT_ALARM     0x0E // 报警允许
#define SD2505_ADDR_CTR1             0x0F // 控制寄存器1
#define SD2505_ADDR_CTR2             0x10 // 控制寄存器2
#define SD2505_ADDR_CTR3             0x11 // 控制寄存器3

#define SD2505_ADDR_ALARM1           0x14 // 报警1
#define SD2505_ADDR_ALARM2           0x15 // 报警2
#define SD2505_ADDR_ALARM3           0x16 // 报警3
#define SD2505_ADDR_AGTC             0x17 // IIC控制寄存器
#define SD2505_ADDR_CHARGE           0x18 // 充电寄存器
#define SD2505_ADDR_CTR4             0x19 // CTR4
#define SD2505_ADDR_BAT_LVAL         0x1A // CTR5(电池电量bit8)
#define SD2505_ADDR_BAT_HVAL         0x1B // 电池电量bit0-bit7

#define SD2505_ADDR_ID                0x72
#define SD2505_LEN_ID                 8

//#define SD2505_ADDR_ALARM9           0x1C // 报警9
//#define SD2505_ADDR_ALARM10          0x1D // 报警10
//#define SD2505_ADDR_ALARM11          0x1E // 报警11
//#define SD2505_ADDR_ALARM12          0x1F // 报警12
//#define SD2505_ADDR_TEMP             0x20 // 温度传感器
//#define SD2505_ADDR_BATT_VOLT        0x21 // 电池电压
//#define SD2505_ADDR_CHARGE_CTRL      0x22 // 充电控制
//#define SD2505_ADDR_GENERAL_PURPOSE  0x23 // 通用SRAM寄存器

///* SD2505API-G控制位定义 */
//#define SD2505_CTR1_STOP            0x00 // 停止时钟
//#define SD2505_CTR1_RUN             0x80 // 运行时钟
//#define SD2505_CTR2_ALARM1_ENABLE   0x01 // 报警1使能
//#define SD2505_CTR2_ALARM2_ENABLE   0x02 // 报警2使能
//#define SD2505_CTR2_ALARM3_ENABLE   0x04 // 报警3使能
//#define SD2505_CTR2_ALARM4_ENABLE   0x08 // 报警4使能


typedef struct 
{
    uint8_t ucSecond;
    uint8_t ucMinute;
    uint8_t ucHour;
    uint8_t ucWeek;
    uint8_t ucDay;
    uint8_t ucMonth;
    uint8_t ucYear;

} rtc_time;

void sd2505_init(void);
uint8_t sd2505_set_time(rtc_time *time);
uint8_t sd2505_get_time(rtc_time *time);

#endif /* __SD2505_H */
