/*!
    \file    SD2505API_driver.c
    \brief   SD2505API driver implementation for GD32F4xx

    \version 2025-8-25, V1, firmware for GD32F4xx
*/

#include "BSP_SD2505API.h"
#include "gd32f4xx.h"
#include "systick.h"

/*********************************************
 * 函数名：I2C_delay_us
 * 描  述：基于循环的微秒延时函数（用于I2C通信，不依赖SysTick）
 * 输  入：count - 延时微秒数
 * 输  出：无
 * 说  明：此函数用于RTOS环境中的I2C通信，不依赖SysTick中断
 *         使用循环延时实现，延时精度取决于CPU频率
 *         每个循环大约消耗3-4个CPU周期（包含__NOP()）
 ********************************************/
//static void I2C_delay_us(uint32_t count)
//{
//    volatile uint32_t i, j;
//    for (i = 0; i < count; i++) {
//        // 每个循环约1微秒（需要根据实际CPU频率调整）
//        for (j = 0; j < 40; j++) {  // 40是经验值，可根据实际测试调整
//            __NOP();
//        }
//    }
//}

#define I2C_delay_us(us) delay_us_nop(us)

/*********************************************
 * 函数名：IIC_Init
 * 描  述：I2C端口初始化
 * 输  入：无
 * 输  出：无
 ********************************************/
void SD2505_Init(void)
{
    rcu_periph_clock_enable(SD2505API_IIC_RCU_RCC);//使能时钟

    gpio_mode_set(SD2505API_SCL_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, SD2505API_SCL_PIN); // 设置为输出模式，无上下拉
    gpio_output_options_set(SD2505API_SCL_PORT, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, SD2505API_SCL_PIN); // 开漏输出，50MHz速度

    gpio_mode_set(SD2505API_SDA_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, SD2505API_SDA_PIN); 
    gpio_output_options_set(SD2505API_SDA_PORT, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, SD2505API_SDA_PIN); 
}

/*********************************************
 * 函数名：I2CStart
 * 描  述：开启I2C总线
 * 输  入：无
 * 输  出：TRUE:操作成功，FALSE:操作失败
 ********************************************/
static uint8_t I2CStart(void)
{
	SDA_OUT();
	SDA(1);
	SCL(1); 
	I2C_delay_us(5);
	SDA(0);
	I2C_delay_us(5);
	SCL(0);
	I2C_delay_us(5);  
    
    return 0;
}

/*********************************************
 * 函数名：I2CStop
 * 描  述：释放I2C总线
 * 输  入：无
 * 输  出：无
 ********************************************/
static void I2CStop(void)
{
	SDA_OUT();
	SCL(0);
	SDA(0);
	SCL(1);
	I2C_delay_us(5);
	SDA(1);
	I2C_delay_us(5);
}

/*********************************************
 * 函数名：I2CAck
 * 描  述：发送ASK
 * 输  入：无
 * 输  出：无
 ********************************************/
static void I2CAck(void)
{
	SDA_OUT();
	SCL(0);
	SDA(1);
	SDA(0);
	SCL(1);
	I2C_delay_us(5);
	SCL(0);
	SDA(1);
}

/*********************************************
 * 函数名：I2CNoAck
 * 描  述：发送NOASK
 * 输  入：无
 * 输  出：无
 ********************************************/
static void I2CNoAck(void)
{
	SDA_OUT();
	SCL(0);
	SDA(0);
	SDA(1);
	SCL(1);
	I2C_delay_us(5);
	SCL(0);
	SDA(0);
}

/*********************************************
 * 函数名：I2CWaitAck
 * 描  述：读取ACK信号
 * 输  入：无
 * 输  出：TRUE=有ACK,FALSE=无ACK
 ********************************************/
uint8_t I2CWaitAck(void)
{
//	char ack = 0;
//    int i=0;
	unsigned char ack_flag = 10;
	SDA_OUT();
	SCL(0);
	SDA(1);
	SDA_IN();
	I2C_delay_us(5);
	SCL(1);
    I2C_delay_us(5);
	//printf(" test : %d  \r\n ", SDA_read());
	while((SDA_read()==1)  &&  ( ack_flag ) )
	{
		ack_flag--;
		I2C_delay_us(5);
	}
	if( ack_flag <= 0 )
	{
		I2CStop();
		return 0;
	}
	else//应答
	{
		SCL(0);
		SDA_OUT();
        return 1;
	}
}
/*********************************************
 * 函数名：I2CSendByte
 * 描  述：MCU发送一个字节
 * 输  入：无
 * 输  出：无
 ********************************************/
static void I2CSendByte(uint8_t SendByte) //数据从高位到低位
{
	int i = 0;
	SDA_OUT();
	SCL(0);
	for( i = 0; i < 8; i++ )
	{
		SDA( (SendByte & 0x80) >> 7 );
		I2C_delay_us(1);
		SCL(1);
		I2C_delay_us(5);
		SCL(0);
		I2C_delay_us(5);
		SendByte<<=1;
	}	
	I2C_delay_us(5);
}

/*********************************************
 * 函数名：I2CReceiveByte
 * 描  述：MCU读入一个字节
 * 输  入：无
 * 输  出：ReceiveByte
 ********************************************/
static uint8_t I2CReceiveByte(void)
{
	unsigned char i,receive=0;
	SDA_IN();
	for(i=0;i<8;i++ )
	{
		SCL(0);
		I2C_delay_us(5);
		SCL(1);
		I2C_delay_us(5);
		receive<<=1;
		if( SDA_read() )
		{	
			receive |= 1;   
		} 
	}
	SCL(0); 
	return receive;
}

/*********************************************
 * 函数名：WriteRTC_Enable
 * 描  述：RTC写允许程序
 * 输  入：无
 * 输  出：TRUE:操作成功，FALSE:操作失败
 ********************************************/
uint8_t WriteRTC_Enable(void)
{
    I2CStart();
    I2CSendByte(RTC_Address); 
    if(I2CWaitAck()== 0){I2CStop();return 0;}
    I2CSendByte(CTR2);      
    I2CWaitAck();	
    I2CSendByte(0x80);//置WRTC1=1      
    I2CWaitAck();
    I2CStop(); 

    I2CStart();
    I2CSendByte(RTC_Address);      
    I2CWaitAck();   
    I2CSendByte(CTR1);
    I2CWaitAck();	
    I2CSendByte(0x84);//置WRTC2,WRTC3=1      
    I2CWaitAck();
    I2CStop(); 
    return TRUE;
}

/*********************************************
 * 函数名：WriteRTC_Disable
 * 描  述：RTC写禁止程序
 * 输  入：无
 * 输  出：TRUE:操作成功，FALSE:操作失败
 ********************************************/
uint8_t WriteRTC_Disable(void)
{
    I2CStart();
    I2CSendByte(RTC_Address); 
    if(!I2CWaitAck()){I2CStop(); return FALSE;}
    I2CSendByte(CTR1);//设置写地址0FH      
    I2CWaitAck();	
    I2CSendByte(0x0) ;//置WRTC2,WRTC3=0      
    I2CWaitAck();
	I2CStop(); 
		
	I2CStart();
    I2CSendByte(RTC_Address);      
    I2CWaitAck();   
    I2CSendByte(CTR2);
	I2CWaitAck();	
    I2CSendByte(0x0) ;//置WRTC1=0(10H地址)      
    I2CWaitAck();
    I2CStop(); 
    return TRUE; 
}

/*********************************************
 * 函数名：RTC_WriteDate
 * 描  述：写RTC实时数据寄存器
 * 输  入：时间结构体指针
 * 输  出：TRUE:操作成功，FALSE:操作失败
 ********************************************/
uint8_t RTC_WriteDate(rtc_time	*psRTC)	//写时间操作要求一次对实时时间寄存器(00H~06H)依次写入，
{                               //不可以单独对7个时间数据中的某一位进行写操作,否则可能会引起时间数据的错误进位. 
                                //要修改其中某一个数据 , 应一次性写入全部 7 个实时时钟数据.

		WriteRTC_Enable();				//使能，开锁

		I2CStart();
		I2CSendByte(RTC_Address); 
		if(!I2CWaitAck()){I2CStop(); return FALSE;}
		I2CSendByte(0);			//设置写起始地址      
		I2CWaitAck();	
		I2CSendByte(psRTC->second);		//second     
		I2CWaitAck();	
		I2CSendByte(psRTC->minute);		//minute      
		I2CWaitAck();	
		I2CSendByte(psRTC->hour|0x80);//hour ,同时设置小时寄存器最高位（0：为12小时制，1：为24小时制）
		I2CWaitAck();	
		I2CSendByte(psRTC->week);		//week      
		I2CWaitAck();	
		I2CSendByte(psRTC->day);		//day      
		I2CWaitAck();	
		I2CSendByte(psRTC->month);		//month      
		I2CWaitAck();	
		I2CSendByte(psRTC->year);		//year      
		I2CWaitAck();	
		I2CStop();

		WriteRTC_Disable();				//关锁
		return	TRUE;
}

/*********************************************
 * 函数名：RTC_ReadDate
 * 描  述：读RTC实时数据寄存器
 * 输  入：时间结构体指针
 * 输  出：TRUE:操作成功，FALSE:操作失败
 ********************************************/
uint8_t RTC_ReadDate(rtc_time	*psRTC)
{
		I2CStart();
		I2CSendByte(RTC_Address);      
		if(!I2CWaitAck()){I2CStop(); return FALSE;}
		I2CSendByte(0);
		I2CWaitAck();
		I2CStart();	
		I2CSendByte(RTC_Address+1);
		I2CWaitAck();
		psRTC->second=I2CReceiveByte();
		I2CAck();
		psRTC->minute=I2CReceiveByte();
		I2CAck();
		psRTC->hour=I2CReceiveByte() & 0x7F;
		I2CAck();
		psRTC->week=I2CReceiveByte();
		I2CAck();
		psRTC->day=I2CReceiveByte();
		I2CAck();
		psRTC->month=I2CReceiveByte();
		I2CAck();
		psRTC->year=I2CReceiveByte();		
		I2CNoAck();
		I2CStop(); 
		return	TRUE;
}
/*********************************************
 * 函数名     ：I2CWriteSerial
 * 描  述     ：I2C在指定地址写一字节数据
 * Device_Addr：I2C设备地址
 * Address    ：内部地址
 * length     ：字节长度
 * ps         ：缓存区指针
 * 输出       ：TRUE 成功，FALSE 失败
 ********************************************/	
uint8_t I2CWriteSerial(uint8_t DeviceAddress, uint8_t Address, uint8_t length, uint8_t *ps)
{
		if(DeviceAddress == RTC_Address)  WriteRTC_Enable();

		I2CStart();
		I2CSendByte(DeviceAddress);   
		if(!I2CWaitAck()){I2CStop(); return FALSE;}
		I2CSendByte(Address);			
		I2CWaitAck();
		for(;(length--)>0;)
		{ 	
			I2CSendByte(*(ps++));		
			I2CAck();			
		}
		I2CStop(); 

		if(DeviceAddress == RTC_Address)  WriteRTC_Disable();
		return	TRUE;
}

/*********************************************
 * 函数名     ：I2CReadSerial
 * 描  述     ：I2C在指定地址写一字节数据
 * Device_Addr：I2C设备地址
 * Address    ：内部地址
 * length     ：字节长度
 * ps         ：缓存区指针
 * 输出       ：TRUE 成功，FALSE 失败
 ********************************************/	
uint8_t I2CReadSerial(uint8_t DeviceAddress, uint8_t Address, uint8_t length, uint8_t *ps)
{
		I2CStart();
		I2CSendByte(DeviceAddress); 
		//I2CWaitAck(); 
		if(!I2CWaitAck()){I2CStop();return 0;}
		I2CSendByte(Address);
		I2CWaitAck();
		I2CStart();	
		I2CSendByte(DeviceAddress+1);
		I2CWaitAck();
		for(;--length>0;ps++)
		{
			*ps = I2CReceiveByte();
			I2CAck();
		}
		*ps = I2CReceiveByte();	
		I2CNoAck();
		I2CStop(); 
		return	1;
}

/*********************************************
 * 函数名：Set_CountDown
 * 描  述：设置倒计时中断
 * 输  入：CountDown_Init 倒计时中断结构体指针 
 * 输  出：无
 ********************************************/
void Set_CountDown(CountDown_Def *CountDown_Init)					
{
		uint8_t buf[6];
		uint8_t tem=0xF0;
		buf[0] = (CountDown_Init->IM<<6)|0xB4;							//10H
		buf[1] = CountDown_Init->d_clk<<4;									//11H
		buf[2] = 0;																					//12H
		buf[3] = CountDown_Init->init_val & 0x0000FF;				//13H
		buf[4] = (CountDown_Init->init_val & 0x00FF00) >> 8;//14H
		buf[5] = (CountDown_Init->init_val & 0xFF0000) >> 16;//15H
		I2CWriteSerial(RTC_Address,CTR2,1,&tem);
		I2CWriteSerial(RTC_Address,CTR2,6,buf);
}

/*********************************************
 * 函数名：Set_Alarm
 * 描  述：设置报警中断（闹钟功能）
 * Enable_config：使能设置  
 * psRTC：报警时间的时间结构体指针
 * 输  出：无
 ********************************************/
void Set_Alarm(uint8_t Enable_config, rtc_time *psRTC)					
{
		uint8_t buf[10];
		buf[0] = psRTC->second;
		buf[1] = psRTC->minute;
		buf[2] = psRTC->hour;
		buf[3] = 0;
		buf[4] = psRTC->day;
		buf[5] = psRTC->month;
		buf[6] = psRTC->year;
		buf[7] = Enable_config;
		buf[8] = 0xFF;
		buf[9] = 0x92;	
		I2CWriteSerial(RTC_Address,Alarm_SC,10,buf);
}

/*********************************************
 * 函数名：SetFrq
 * 描  述：设置RTC频率中断，从INT脚输出频率方波
 * 输  入：频率值
 * 输  出：无
 ********************************************/
void SetFrq(enum Freq F_Out)					
{
		uint8_t buf[2];
		buf[0] = 0xA1;
		buf[1] = F_Out;
		I2CWriteSerial(RTC_Address,CTR2,2,buf);
}

/*********************************************
 * 函数名：ClrINT
 * 描  述：禁止中断
 * int_EN：中断类型 INTDE、INTDE、INTDE
 * 输  出：无
 ********************************************/
void ClrINT(uint8_t int_EN)         
{
		uint8_t buf;
		buf = 0x80 & (~int_EN);
		I2CWriteSerial(RTC_Address,CTR2,1,&buf);
}
/*********************************************END OF FILE**********************/
