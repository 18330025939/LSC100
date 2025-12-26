/************************** (C) COPYRIGHT 2025 banbridge  ***************************
* File Name          : dev_emmc.c
* Author             : banbridge Xtreme
* Version            : V1.0
* Date               : 10/18/2025
* Description        :eMMC read-write driver
*******************************************************************************/
#include "BSP_EMMC.h"
#include <string.h>
#include <stdio.h>

volatile u32 DMAEndOfTransfer = 0;
volatile u32 eMMCStopCondition = 0;
volatile u32 eMMCTransferEnd = 0;
volatile u32 eMMCDMAEndOfTransfer = 0;
volatile EmmcError eMMCTransferError = EMMC_OK;
EmmcCardInfo MyEmmcCardInfo = {0};
static u32 eMMC_Type =  _EMMC_STD_CAPACITY_SD_CARD_V1_1;
uint32_t count4=0;

__align(4) u8  sEmmcTempBuff[EMMC_BLOCK_SIZE] = {0};


/* Private function prototypes -----------------------------------------------*/
static void EmmcDeviceInit(void);
static void EmmcSdioCfg(u8 clk, u32 bus_wide);
static void EmmcSdioCmdCfg(u32 argument, u32 cmd, u32 mode);
static void EmmcLowLevelDMATxConfig(u8 *BufferSRC, u32 BufferSize);
static void EmmcLowLevelDMARxConfig(u8 *BufferDST, u32 BufferSize);
static void EmmcGpioConfiguration(void);
static void EmmcDmaIrq(void);
static void EmmcSdioIrq (void);

#define DEBUG(format,...)


//static u8 SDSTATUS_Tab[16];
/* Private functions ---------------------------------------------------------*/
EmmcError EmmcInit(void);
EmmcError EmmcDeInit(void);
EmmcError CmdError(void);
EmmcError CmdResp1Error(u8 cmd);
EmmcError CmdResp7Error(void);
EmmcError CmdResp3Error(void);
EmmcError CmdResp2Error(void);
EmmcError CmdResp6Error(u8 cmd, u16 *prca);
EmmcError EmmcEnWideBus(uint8_t NewState);
EmmcError IsCardProgramming(u8 *pstatus);
EmmcError EmmcPowerON(void);
EmmcError EmmcInitializeCards(EmmcCardInfo *E);
EmmcError EmmcSendStatus(u32 *pcardstatus);
EmmcError EmmcCMDStopTransfer(void);
EmmcError EmmcGetCardInfo(EmmcCardInfo *E, u32 *CSD_Tab,  u16 Rca);
EmmcError EmmcSelectDeselect(u32 addr);
EmmcError eMMC_Erase(u32 startaddr, u16 Block_num);
bool EmmcEraseBlocks(u32 WriteAddr, u32 NumberOfBlocks);

EmmcError EmmcWaitReadOperation(void);
EmmcError EmmcWaitWriteOperation(void);
EmmcError EmmcReadExtCsd(EmmcCardInfo *E);
EmmcError EmmcEnableWideBusOperation(u32 WideMode);

EmmcError EmmcReadMultiBlocks(u8 *readbuff, u32 ReadAddr, u16 BlockSize, u32 NumberOfBlocks);

EmmcError EmmcWriteMultiBlocks(u8 *writebuff, u32 WriteAddr, u16 BlockSize, u32 NumberOfBlocks);


EmmcCardState EmmcGetState(void);
EmmcTransferState EmmcGetStatus(void);
void EmmcProcessDMAIRQ(void);
u8 convert_from_bytes_to_power_of_two(u16 NumberOfBytes);

bool emmcerasewriteblocks(volatile u8 *writebuff, u32 WriteAddr, u32 NumberOfBlocks);
EmmcError emmc_erasemultiblock(u32 startaddr,u32 endaddr);
bool emmcreadblocks(volatile u8 *readbuff, u32 ReadAddr, u32 NumberOfBlocks);

// void nvicconfiguration(uint32_t offset_vectortable)
// {
//     __set_FAULTMASK(1);    // 关闭总中断

//     /* 设置中断向量表位置 */
//     nvic_vector_table_set(NVIC_VECTTAB_FLASH, offset_vectortable);

//     /* Enable SYSCFG clock */
//     rcu_periph_clock_enable(RCU_SYSCFG);
    
//     /* 设置中断优先级分组 */
//     nvic_priority_group_set(NVIC_PRIGROUP_PRE2_SUB2);
    
//     /* systick中断初始化 */
//     systick_drv_config();
	
//     /* SDIO 中断优先级 */
//     nvic_irq_enable(SDIO_IRQn, 1, 0);
    
//     /* EMMC_SDIO_DMA_IRQn 优先级 */
//     nvic_irq_enable(DMA1_Channel6_IRQn, 1, 1);

//     __set_FAULTMASK(0);    // 打开总中断
// }





// void systick_drv_config(void)
// {
// 	if (SysTick_Config(SystemCoreClock / 1000))
// 	{ 
// 	} 
// }

// void drv_bsp_config(void)
// {  
//     systick_drv_config(); 
// }

bool emmcdriveinit(void)
{
    u8 ret = 0xff;
    u8 EmmcTimeout = EmmcTimeOut;
    //一般情况下初始化第二次会成功
    while((ret != EMMC_OK)&&EmmcTimeout)
    {
        ret = EmmcInit();
        EmmcTimeout--;
    }
    if(EmmcTimeout == 0)
    {
        return false;
        // NovaPrintf("\r\nsd init fail!\r\n");
    }

    uint32_t sectors = emmcgetcapacity();
    uint32_t mb = sectors / 2048;
    printf("EMMC capacity: %u MB (%u sectors)\r\n", mb, sectors);

    return true;
}




/***************** 一块512个Byte,擦写任意块，地址块偏移 ************************/
bool EmmcEraseBlocks(u32 WriteAddr, u32 NumberOfBlocks)
{
//	 u8 i;
	 // 定义错误机会为三次
	 u8 Timeout = 3;
	 EmmcError errorstatus = EMMC_ERROR;

    if(NumberOfBlocks != 0)
    {
        // 连续擦N块
        errorstatus = EMMC_ERROR;
        Timeout = 3;
        while((errorstatus!=EMMC_OK)&&(Timeout))
        {
            errorstatus = emmc_erasemultiblock(WriteAddr,(WriteAddr+NumberOfBlocks-1));
            Timeout--;
        }
        if(!Timeout)
        {
            return false;
        }
    }
	 return true;
}

bool emmcerasewriteblocks(volatile u8 *writebuff, u32 WriteAddr, u32 NumberOfBlocks)
{
//	 u8 i;
	 // 定义错误机会为三次
	 u8 Timeout = 3;
	 EmmcError errorstatus = EMMC_ERROR;
    
    if(NumberOfBlocks != 0)
    {
        // 连续擦N块
        errorstatus = EMMC_ERROR;
        Timeout = 3;
        while((errorstatus!=EMMC_OK)&&(Timeout))
        {
            errorstatus = emmc_erasemultiblock(WriteAddr,(WriteAddr+NumberOfBlocks-1));
            Timeout--;
        }
        if(!Timeout)
        {
            return false;
        }
        // 连续写N块
        errorstatus = EMMC_ERROR;
        Timeout = 3;
        while((errorstatus!=EMMC_OK)&&(Timeout))
        {
            errorstatus = EmmcWriteMultiBlocks((u8*)writebuff,WriteAddr, 512, NumberOfBlocks);
            Timeout--;
        }
        if(!Timeout)
        {
            return false;
        }
    }
	 return true;
}

/***************** 一块512个Byte,读任意块，地址位块偏移 ************************/
bool emmcreadblocks(volatile u8 *readbuff, u32 ReadAddr, u32 NumberOfBlocks)
{
	 u8 Timeout = 3;
	 EmmcError errorstatus = EMMC_ERROR;

	
	
    if(NumberOfBlocks != 0)
    {
		

		
         errorstatus = EMMC_ERROR;
         Timeout = 3;
         while((errorstatus!=EMMC_OK)&&(Timeout))
         {
             errorstatus = EmmcReadMultiBlocks((u8*)readbuff, ReadAddr, 512, NumberOfBlocks);
             Timeout--;
         }
         if(!Timeout)
                return false;
     }
	 return true;
}


/**
  * @brief  Initializes the SD Card and put it into StandBy State (Ready for data 
  *         transfer).
  * @param  None
  * @retval EmmcError: SD Card Error code.
  */
EmmcError EmmcInit(void)
{
    volatile EmmcError Result = EMMC_OK; 

    /* 配置SDIO和DMA中断优先级 */
    EmmcDeviceInit();

    /* 开启SDIO电源、时钟，复位卡到空闲状态 */
    Result = EmmcPowerON();
    if (Result != EMMC_OK)
    {
        printf("PowerOn错误!\r\n");
    return(Result);
    }

    /* 初始化EMMC：CID(CMD2)、RCA(CMD3)、CSD(CMD9),将CID和CSD保存到MyEmmcCardInfo中 */
    Result = EmmcInitializeCards(&MyEmmcCardInfo);
    if (Result != EMMC_OK)
    {
        printf("Initialize错误!\r\n");
    return(Result);
    }

    if (Result == EMMC_OK)
    {   /* 重新设置总线位宽（默认位宽为1） */
        MyEmmcCardInfo.init_ret = 1;
        Result = EmmcEnableWideBusOperation(SDIO_BUSMODE_8BIT);
    } 

    return(Result);
}

/**
  * @brief  Gets the cuurent sd card data transfer status.
  * @param  None
  * @retval SDTransferState: Data Transfer state.
  *   This value can be: 
  *        - SD_TRANSFER_OK: No data transfer is acting
  *        - SD_TRANSFER_BUSY: Data transfer is acting
  */
EmmcTransferState EmmcGetStatus(void)
{
	
		EmmcCardState CardState =  EMMC_CARD_TRANSFER;
		CardState = EmmcGetState();

		if (CardState == EMMC_CARD_TRANSFER)//4
		{
			return(EMMC_TRANSFER_OK);
		}
		else if (CardState == EMMC_CARD_RECEIVING)//6
		{
			return(EMMC_TRANSFER_OK);
		}
		
		else if(CardState == EMMC_CARD_ERROR)//ff
		{
			return (EMMC_TRANSFER_ERROR);
		}
		else
		{
			return(EMMC_TRANSFER_BUSY);
		}
}

/**
  * @brief  Returns the current card's state.
  * @param  None
  * @retval EmmcCardState: EMMC Card Error or EMMC Card Current State.
  */
EmmcCardState EmmcGetState(void)
{
    u32 Resp = 0;
	
    if (EmmcSendStatus(&Resp) != EMMC_OK)
    {
      return EMMC_CARD_ERROR;
    }
    else
    {
      return (EmmcCardState)((Resp >> 9) & 0x0F);
    }
}



/**
  * @brief  Enquires cards about their operating voltage and configures 
  *   clock controls.
  * @param  None
  * @retval EmmcError: SD Card Error code.
  */
EmmcError EmmcPowerON(void)
{
  volatile EmmcError Result = EMMC_OK; 
  u32 Cnt = 0,i = 0;
  u32 response = 0;
  u16 MaxCnt = 0xFF;
  /*!< Power ON Sequence -----------------------------------------------------*/
  /*!< Configure the SDIO peripheral */

  EmmcSdioCfg(EMMC_SDIO_INIT_CLK_DIV, EMMC_SDIO_1BIT_BUS);       
    
  /*!< Set Power State to ON */
  sdio_power_state_set(SDIO_POWER_ON);

  /*!< Enable SDIO Clock */
  sdio_clock_enable();
for(i = 0;i<65535;i++);
    /* CMD0 : 内部复位到IDLE */
  EmmcSdioCmdCfg(0, EMMC_CMD_GO_IDLE_STATE, EMMC_SDIO_NO_RESPONSE);  

  /* 检测是否正确接收到 CMD0 */
  Result = CmdError();
  
  if (Result != EMMC_OK)
  {
    return(Result);
  }
  
  eMMC_Type = _EMMC_HIGH_CAPACITY_MMC_CARD; 
  do
  {
      /*!< SEND CMD1*/
    EmmcSdioCmdCfg(EMMC_OCR_REG, EMMC_CMD_SEND_OP_COND, EMMC_SDIO_SHORT_RESPONSE);  
    Result = CmdResp3Error();
    response = sdio_response_get(SDIO_RESPONSE0);
    response = (((response >> 31U) == 1U) ? 1U : 0U);
    Cnt++;
  }while((Cnt<MaxCnt)&&( (Result != EMMC_OK) || (response == 0)));

  
  return(Result);
}

/**
  * @brief  Intialises all cards or single card as the case may be Card(s) come 
  *         into standby state.
  * @param  None
  * @retval EmmcError: SD Card Error code.
  */
EmmcError EmmcInitializeCards(EmmcCardInfo *E)
{
  EmmcError Result = EMMC_OK;
	u32 CSD_Tab = 0;
//	u32 CID_Tab[4] = {0};
	u16 Rca = 0x01;
	u16 	i = 0;
  

  if (sdio_power_state_get() == SDIO_POWER_OFF){
    Result = EMMC_REQUEST_NOT_APPLICABLE;
		DEBUG("PowerState错误！\r\n");
    return(Result);
  }

  /* 发送CMD2 请求CID寄存器内容 */
  EmmcSdioCmdCfg(0x00, EMMC_CMD_ALL_SEND_CID, EMMC_SDIO_LONG_RESPONSE);

  EmmcSdioCmdCfg(0x00, EMMC_CMD_ALL_SEND_CID, EMMC_SDIO_LONG_RESPONSE);



	for(i = 0; i < 60000; i++); //延迟
  Result = CmdResp2Error();

  if (EMMC_OK != Result){
		//printf("CID请求响应错误！\r\n");
//    return(Result);
  }

 /* CMD3 */
  EmmcSdioCmdCfg(0x00, EMMC_CMD_SET_REL_ADDR, EMMC_SDIO_SHORT_RESPONSE);

  Result = CmdResp6Error(EMMC_CMD_SET_REL_ADDR, &Rca);					 /* 用Rca变量接收eMMC的相对地址 */

  if (EMMC_OK != Result){
		//printf("RCA请求响应错误！\r\n");
//    return(Result);
  }

	/* 请求CSD寄存器内容，如块长度、容量、最大时钟速率等 */  /* CMD9 */
   EmmcSdioCmdCfg((u32)(Rca << 16), EMMC_CMD_SEND_CSD, EMMC_SDIO_LONG_RESPONSE);


  Result = CmdResp2Error();

  if (EMMC_OK != Result){
		//printf("CSD请求响应错误！\r\n");
//    return(Result);
  }

	 /* 读取eMMC的CSD寄存器内容 */
  //CSD_Tab[0] = sdio_response_get(SDIO_RESPONSE0);										
  CSD_Tab = sdio_response_get(SDIO_RESPONSE1);
//  CSD_Tab[2] = sdio_response_get(SDIO_RESPONSE2);
//  CSD_Tab[3] = sdio_response_get(SDIO_RESPONSE3);	

	/* 如果系统中有多个eMMC器件，需选择被操作的特定eMMC器件 */
	Result = EmmcSelectDeselect((u32) (MyEmmcCardInfo.RCA << 16));
	if (EMMC_OK != Result){
//		DEBUG("eMMC选择错误！\r\n");
//		return(Result);
	}

	Result = EmmcReadExtCsd(&MyEmmcCardInfo);
	if (EMMC_OK != Result){
		//printf("ECSD读取错误！\r\n");
//		return(Result);
	}
  Result = EmmcGetCardInfo(E, &CSD_Tab,  Rca);
	if (EMMC_OK != Result) {
		//printf("填充eMMC信息结构体错误！\r\n");
		return(Result);
	}
  Result = EMMC_OK; /*!< All cards get intialized */
  return(Result);
}

/**
  * @brief  打包eMMC的所有信息到指定结构体
  * @param  E: eMMC信息结构体指针.
  * @retval EmmcError: 操作码.
  */
EmmcError EmmcGetCardInfo(EmmcCardInfo *E, u32 *CSD_Tab,  u16 Rca)
{
	
		EmmcError errorstatus = EMMC_OK;
		u8 tmp = 0;

	E->eMMC_Type = (u8)eMMC_Type;										/* eMMC_Type在PowerOn中指定为07 */
	E->RCA = (u16)Rca;
	tmp = (u8)((*CSD_Tab & 0xFF000000) >> 24);
	E->ExtCardComdClasses = tmp << 4;

	/*!< Byte 5 */
	tmp = (u8)((*CSD_Tab & 0x00FF0000) >> 16);
	E->ExtCardComdClasses |= (tmp & 0xF0) >> 4;
	E->eMMC_BlockSize = tmp & 0x0F;
	E->eMMC_BlockSize = 1 << (E->eMMC_BlockSize);
    // 注释掉错误的容量计算，保持eMMC_Capacity为扇区数（在EmmcReadExtCsd中已正确赋值）
    // E->eMMC_Capacity = ((E->eMMC_Capacity>>10) * E->eMMC_BlockSize)>>10;
  return(errorstatus);
}


/**
  * @brief  位宽设置.
  * @param  WideMode: 位宽模式. 
  *   This parameter can be one of the following values:
  *     @arg SDIO_BusWide_8b: 8-bit 模式 (仅为MMC)
  *     @arg SDIO_BusWide_4b: 4-bit 模式
  *     @arg SDIO_BusWide_1b: 1-bit 模式
  * @retval EmmcError: EmmcError code.
  */
EmmcError EmmcEnableWideBusOperation(u32 WideMode)
{
  EmmcError errorstatus = EMMC_OK;
		
  if (SDIO_BUSMODE_8BIT == WideMode) 
	{
		errorstatus = EmmcEnWideBus(1);

    if (EMMC_OK == errorstatus)	
    {
        EmmcSdioCfg(EMMC_SDIO_TRANSFER_CLK_DIV, EMMC_SDIO_8BIT_BUS);         
    }

  } 
	else if (SDIO_BUSMODE_4BIT == WideMode)
	{

    errorstatus = EmmcEnWideBus(1);

    if (EMMC_OK == errorstatus)	
	{
        EmmcSdioCfg(EMMC_SDIO_TRANSFER_CLK_DIV, EMMC_SDIO_4BIT_BUS);    
    }
  }
	else	
	{
    errorstatus = EmmcEnWideBus(0);

    if (EMMC_OK == errorstatus)
    {
        EmmcSdioCfg(EMMC_SDIO_TRANSFER_CLK_DIV, EMMC_SDIO_1BIT_BUS);   
    }
  }
  return(errorstatus);
}

/**
  * @brief  Selects od Deselects the corresponding card.
  * @param  addr: Address of the Card to be selected.
  * @retval EmmcError: SD Card Error code.
  */
EmmcError EmmcSelectDeselect(u32 addr)
{
  EmmcError errorstatus = EMMC_OK;

  /*!< Send CMD7 SDIO_SEL_DESEL_CARD */
  EmmcSdioCmdCfg(addr, EMMC_CMD_SEL_DESEL_CARD, EMMC_SDIO_SHORT_RESPONSE);


  errorstatus = CmdResp1Error(EMMC_CMD_SEL_DESEL_CARD);

  return(errorstatus);
}

/**
  * @brief  读取eMMC器件的ECSD寄存器内容.
  * @param  E：表征特定eMMC器件的信息结构体
  * @retval EmmcError: EMMC Error code.
  */  
EmmcError EmmcReadExtCsd(EmmcCardInfo *E)
{
	EmmcError Result = EMMC_OK;
	u32 count = 0;
  u32 *ExtCsdBuf = NULL;
	EMMCEXT_CSD eMMC_ECSD;
	ExtCsdBuf = (u32 *)(&(eMMC_ECSD.CsdBuf[0]));

  /* Configure data transfer */
  sdio_data_config(EMMC_DATATIMEOUT, (u32)512, SDIO_DATABLOCKSIZE_512BYTES);
  sdio_data_transfer_config(SDIO_TRANSMODE_BLOCK, SDIO_TRANSDIRECTION_TOSDIO);
  sdio_dsm_enable();
	
	/* CMD8 */ 
  EmmcSdioCmdCfg(0, EMMC_CMD_HS_SEND_EXT_CSD, EMMC_SDIO_SHORT_RESPONSE);

  Result = CmdResp1Error(EMMC_CMD_HS_SEND_EXT_CSD);
	
  if (EMMC_OK != Result) {
    return(Result);
  }


	while (!(SDIO_STAT &(SDIO_FLAG_RXORE | SDIO_FLAG_DTCRCERR | SDIO_FLAG_DTTMOUT | SDIO_FLAG_DTBLKEND | SDIO_FLAG_STBITE))){
		if (sdio_flag_get(SDIO_FLAG_RFH) != RESET){
			for (count = 0; count < 8; count++)	{
				*(ExtCsdBuf + count) = sdio_data_read();
			}
			ExtCsdBuf += 8;
		}
	}
  if (sdio_flag_get(SDIO_FLAG_DTTMOUT) != RESET) {
    sdio_flag_clear(SDIO_FLAG_DTTMOUT);
    Result = EMMC_DATA_TIMEOUT;
    return(Result);
  }
  else if (sdio_flag_get(SDIO_FLAG_DTCRCERR) != RESET){
    sdio_flag_clear(SDIO_FLAG_DTCRCERR);
    Result = EMMC_DATA_CRC_FAIL;
    return(Result);
  }
  else if (sdio_flag_get(SDIO_FLAG_RXORE) != RESET){
    sdio_flag_clear(SDIO_FLAG_RXORE);
    Result =EMMC_RX_OVERRUN;
    return(Result);
  }
  else if (sdio_flag_get(SDIO_FLAG_STBITE) != RESET){
    sdio_flag_clear(SDIO_FLAG_STBITE);
    Result = EMMC_START_BIT_ERR;
    return(Result);
  }
  count = EMMC_DATATIMEOUT;
	
  while ((sdio_flag_get(SDIO_FLAG_RXDTVAL) != RESET) && (count > 0)){
    *ExtCsdBuf = sdio_data_read();
    ExtCsdBuf++;
    count--;
  }
  
  /*!< Clear all the static flags */
  sdio_flag_clear(SDIO_FLAG_CCRCERR | SDIO_FLAG_DTCRCERR | SDIO_FLAG_CMDTMOUT | SDIO_FLAG_DTTMOUT | 
                  SDIO_FLAG_TXURE | SDIO_FLAG_RXORE | SDIO_FLAG_CMDRECV | SDIO_FLAG_CMDSEND | 
                  SDIO_FLAG_DTEND | SDIO_FLAG_STBITE | SDIO_FLAG_DTBLKEND);
	
    E->eMMC_Capacity = (eMMC_ECSD.EXT_CSD.SEC_COUNT[0] + (eMMC_ECSD.EXT_CSD.SEC_COUNT[1] << 8) + (eMMC_ECSD.EXT_CSD.SEC_COUNT[2] << 16) + (eMMC_ECSD.EXT_CSD.SEC_COUNT[3] << 24));
	
  return Result;
}


/**
  * @brief  Allows to read blocks from a specified address  in a card.  The Data
  *         transfer can be managed by DMA mode or Polling mode. 
  * @note   This operation should be followed by two functions to check if the 
  *         DMA Controller and SD Card status.
  *          - SD_ReadWaitOperation(): this function insure that the DMA
  *            controller has finished all data transfer.
  *          - EmmcGetStatus(): to check that the SD Card has finished the 
  *            data transfmer and it is ready for data.   
  * @param  readbuff: pointer to the buffer that will contain the received data.
  * @param  ReadAddr: Address from where data are to be read.
  * @param  BlockSize: the SD card Data block size. The Block size should be 512.
  * @param  NumberOfBlocks: number of blocks to be read.
  * @retval EmmcError: SD Card Error code.
  */
/**
  * @摘要:  允许从卡中的指定地址读多个块的数据
  * @参数  readbuff: 指向存放接收到数据的buffer 
  * @参数  ReadAddr: 将要读数据的地址
  * @参数  BlockSize: SD卡数据块的大小
  * @参数  NumberOfBlocks: 要读的块的数量
  * @返回值 SD_Error: SD卡错误代码
  */
EmmcError EmmcReadMultiBlocks(u8 *readbuff, u32 ReadAddr, u16 BlockSize, u32 NumberOfBlocks)
{
  EmmcError errorstatus = EMMC_OK;
//  u32 count = 0, *tempbuff = (u32 *)readbuff,debug0
	vu32 temp = 0;  
	EmmcTransferState TranResult = EMMC_TRANSFER_ERROR;
  eMMCTransferError = EMMC_OK;
  eMMCTransferEnd = 0;
  eMMCStopCondition = 1;
DMAEndOfTransfer = 0;    
    if(!NumberOfBlocks)
        return errorstatus;
	EmmcLowLevelDMARxConfig(readbuff, (NumberOfBlocks * BlockSize));
  sdio_dsm_disable();

  /* 设置Block的大小 */
  EmmcSdioCmdCfg((u32) BlockSize, EMMC_CMD_SET_BLOCKLEN, EMMC_SDIO_SHORT_RESPONSE);	 	

	temp = SDIO_STAT;
  errorstatus = CmdResp1Error(EMMC_CMD_SET_BLOCKLEN);

  if (EMMC_OK != errorstatus)
  {
    return(errorstatus);
  }
    /*!< Clear all the static flags */
  sdio_flag_clear(SDIO_FLAG_CCRCERR | SDIO_FLAG_DTCRCERR | SDIO_FLAG_CMDTMOUT | SDIO_FLAG_DTTMOUT | 
                  SDIO_FLAG_TXURE | SDIO_FLAG_RXORE | SDIO_FLAG_CMDRECV | SDIO_FLAG_CMDSEND | 
                  SDIO_FLAG_DTEND | SDIO_FLAG_STBITE | SDIO_FLAG_DTBLKEND);
 
  /* Configure data transfer */
  sdio_data_config(EMMC_DATATIMEOUT, NumberOfBlocks * BlockSize, SDIO_DATABLOCKSIZE_512BYTES);
  sdio_data_transfer_config(SDIO_TRANSMODE_BLOCK, SDIO_TRANSDIRECTION_TOSDIO);
  sdio_dsm_enable();

  /* pre-erased，调用CMD25之前，预写入操作的块数目 *//* CMD23 */
  EmmcSdioCmdCfg((u32) NumberOfBlocks, EMMC_CMD_SET_BLOCK_COUNT, EMMC_SDIO_SHORT_RESPONSE);	   	   


  errorstatus = CmdResp1Error(EMMC_CMD_SET_BLOCK_COUNT);

  if (errorstatus != EMMC_OK)
	{
    return(errorstatus);
  }	 
  
  /* 发送 CMD18（READ_MULT_BLOCK）命令，参数为要读数据的地址 */
  EmmcSdioCmdCfg((u32)ReadAddr, EMMC_CMD_READ_MULT_BLOCK, EMMC_SDIO_SHORT_RESPONSE);	   

  

  errorstatus = CmdResp1Error(EMMC_CMD_READ_MULT_BLOCK);
  if (errorstatus != EMMC_OK)
  {
    return(errorstatus);
  }

			 /* DMA模式 */
  sdio_interrupt_enable(SDIO_INT_DTCRCERR | SDIO_INT_DTTMOUT | SDIO_INT_DTEND | SDIO_INT_RXORE | SDIO_INT_STBITE);
  sdio_dma_enable();
	 
	errorstatus = EmmcWaitReadOperation();
	if(errorstatus !=  EMMC_OK)
	{
		return errorstatus;
	}
	
	do
	{			
			TranResult = EmmcGetStatus();
	}while(TranResult != EMMC_TRANSFER_OK);
			
  return(errorstatus);
}

/**
  * @brief  This function waits until the SDIO DMA data transfer is finished. 
  *         This function should be called after SDIO_ReadMultiBlocks() function
  *         to insure that all data sent by the card are already transferred by 
  *         the DMA controller.
  * @param  None.
  * @retval EmmcError: SD Card Error code.
  */
EmmcError EmmcWaitReadOperation(void)
{
  EmmcError errorstatus = EMMC_OK;
  u32 timeout = EMMC_DATATIMEOUT;
	

  /* 以下变量在中断中修改
	 * eMMCDMAEndOfTransfer（DMA）、eMMCTransferEnd（SDIO）、eMMCTransferError（SDIO）
	 */
  while ((DMAEndOfTransfer == 0x00) && (eMMCTransferEnd == 0) && (eMMCTransferError == EMMC_OK) && (timeout > 0))
  {
    timeout--;
  }

  timeout = EMMC_DATATIMEOUT;
  
  while(((SDIO_STAT & SDIO_FLAG_RXRUN)) && (timeout > 0))
  {
    timeout--;  
  }

  if (eMMCStopCondition == 1)
  {
		eMMCStopCondition = 0;
  }
  
  if ((timeout == 0) && (errorstatus == EMMC_OK))
  {
		//printf("传输超时");
    errorstatus = EMMC_DATA_TIMEOUT;
  }
  
  /*!< Clear all the static flags */
  sdio_flag_clear(SDIO_FLAG_CCRCERR | SDIO_FLAG_DTCRCERR | SDIO_FLAG_CMDTMOUT | SDIO_FLAG_DTTMOUT | 
                  SDIO_FLAG_TXURE | SDIO_FLAG_RXORE | SDIO_FLAG_CMDRECV | SDIO_FLAG_CMDSEND | 
                  SDIO_FLAG_DTEND | SDIO_FLAG_STBITE | SDIO_FLAG_DTBLKEND);

  if (eMMCTransferError != EMMC_OK)
  {
		//printf("传输错误");
    return(eMMCTransferError);
  }
  else
  {
    return(errorstatus);  
  }
}



/**
  * @brief  在指定的起始地址处，连续写入多块，多块读写只能在DMA模式下使用. 
  * @note   This operation should be followed by two functions to check if the 
  *         DMA Controller and SD Card status.
  *          - SD_ReadWaitOperation(): this function insure that the DMA
  *            controller has finished all data transfer.
  *          - SD_GetStatus(): to check that the SD Card has finished the 
  *            data transfer and it is ready for data.     
  * @param  WriteAddr: Address from where data are to be read.
  * @param  writebuff: pointer to the buffer that contain the data to be transferred.
  * @param  BlockSize: eMMC块大小. 固定为512.
  * @param  NumberOfBlocks: 写入的数据块儿数量.
  * @retval EmmcError: SD Card Error code.
  */
EmmcError EmmcWriteMultiBlocks(u8 *writebuff, u32 WriteAddr, u16 BlockSize, u32 NumberOfBlocks)
{
  EmmcError errorstatus = EMMC_OK;
	u32 i = 0;
	EmmcTransferState TranResult;
	u8 cardstate = 0;
  eMMCTransferError = EMMC_OK;
  eMMCTransferEnd = 0;
    DMAEndOfTransfer = 0;
  EmmcLowLevelDMATxConfig(writebuff, (NumberOfBlocks * BlockSize));
  sdio_dsm_disable();

	 if (eMMC_Type == _EMMC_HIGH_CAPACITY_SD_CARD) 
	 {
	 		 BlockSize = 512; 
			 WriteAddr /= 512; 
	 }
	
  /* Set Block Size for Card */ /* CMD16 */
	/* 如果缺少这个命令，很容易导致程序卡死在循环检测DMA传输结束的代码中 */
  EmmcSdioCmdCfg((u32) BlockSize, EMMC_CMD_SET_BLOCKLEN, EMMC_SDIO_SHORT_RESPONSE);	   	 


  errorstatus = CmdResp1Error(EMMC_CMD_SET_BLOCKLEN);

  if (EMMC_OK != errorstatus)
	{
    return(errorstatus);
  }

	/* pre-erased，调用CMD25之前，预写入操作的块数目 *//* CMD23 */
  EmmcSdioCmdCfg((u32) NumberOfBlocks, EMMC_CMD_SET_BLOCK_COUNT, EMMC_SDIO_SHORT_RESPONSE);	   	   


  errorstatus = CmdResp1Error(EMMC_CMD_SET_BLOCK_COUNT);

  if (errorstatus != EMMC_OK)
	{
    return(errorstatus);
  }	 

  /* 发送多块写命令：CMD25，参数：写起始地址 */
  EmmcSdioCmdCfg((u32)WriteAddr, EMMC_CMD_WRITE_MULT_BLOCK, EMMC_SDIO_SHORT_RESPONSE);  


  errorstatus = CmdResp1Error(EMMC_CMD_WRITE_MULT_BLOCK);

  if (EMMC_OK != errorstatus)
  {
    return(errorstatus);
  }
  
     /*!< Clear all the static flags */
  sdio_flag_clear(SDIO_FLAG_CCRCERR | SDIO_FLAG_DTCRCERR | SDIO_FLAG_CMDTMOUT | SDIO_FLAG_DTTMOUT | 
                  SDIO_FLAG_TXURE | SDIO_FLAG_RXORE | SDIO_FLAG_CMDRECV | SDIO_FLAG_CMDSEND | 
                  SDIO_FLAG_DTEND | SDIO_FLAG_STBITE | SDIO_FLAG_DTBLKEND);

  /* Configure data transfer */
  sdio_data_config(EMMC_DATATIMEOUT, NumberOfBlocks * BlockSize, SDIO_DATABLOCKSIZE_512BYTES);
  sdio_data_transfer_config(SDIO_TRANSMODE_BLOCK, SDIO_TRANSDIRECTION_TOCARD);
  sdio_dsm_enable();
	//小BUG,必须进一次中断，否则STA位读有问题
//	Delay(1);

  sdio_interrupt_enable(SDIO_INT_DTCRCERR | SDIO_INT_DTTMOUT | SDIO_INT_DTEND | SDIO_INT_RXORE | SDIO_INT_STBITE);
  sdio_dma_enable();

	errorstatus = EmmcWaitWriteOperation();

	if (EMMC_OK != errorstatus)
  {
    return(errorstatus);
  }
	
	errorstatus = IsCardProgramming(&cardstate);
	while ((errorstatus == EMMC_OK) && ((EMMC_CARD_PROGRAMMING == cardstate) || (EMMC_CARD_RECEIVING == cardstate)))
  {
    errorstatus = IsCardProgramming(&cardstate);
		for(i=0; i<10000; i++);
  }
	
		do
	{			
			TranResult = EmmcGetStatus();
	}while(TranResult != EMMC_TRANSFER_OK);
	
  return(errorstatus);
}

/**
  * @brief  This function waits until the SDIO DMA data transfer is finished. 
  *         This function should be called after SDIO_WriteBlock() and
  *         SDIO_WriteMultiBlocks() function to insure that all data sent by the 
  *         card are already transferred by the DMA controller.        
  * @param  None.
  * @retval EmmcError: SD Card Error code.
  */
EmmcError EmmcWaitWriteOperation(void)
{
  EmmcError errorstatus = EMMC_OK;
  u32 timeout;

  timeout = EMMC_DATATIMEOUT;
  
  while ((DMAEndOfTransfer == 0x00) && (eMMCTransferEnd == 0) && (eMMCTransferError == EMMC_OK) && (timeout > 0))
  {
    timeout--;
  }

  timeout = EMMC_DATATIMEOUT;
	
  while(((SDIO_STAT & SDIO_FLAG_TXRUN)) && (timeout > 0))
  {
    timeout--;  
  }
	/*!< Clear all the static flags */
  sdio_flag_clear(SDIO_FLAG_CCRCERR | SDIO_FLAG_DTCRCERR | SDIO_FLAG_CMDTMOUT | SDIO_FLAG_DTTMOUT | 
                  SDIO_FLAG_TXURE | SDIO_FLAG_RXORE | SDIO_FLAG_CMDRECV | SDIO_FLAG_CMDSEND | 
                  SDIO_FLAG_DTEND | SDIO_FLAG_STBITE | SDIO_FLAG_DTBLKEND);
  if ((timeout == 0) && (errorstatus == EMMC_OK))
  {
    errorstatus = EMMC_DATA_TIMEOUT;
  }
  if (eMMCTransferError != EMMC_OK)
  {
    return(eMMCTransferError);
  }
  else
  {
    return(errorstatus);
  }
}


/**
  * @brief  终止数据传输过程.
  * @param  None
  * @retval EmmcError: eMMC错误码.
  */
EmmcError EmmcCMDStopTransfer(void)
{
  EmmcError errorstatus = EMMC_OK;

  /* 发送CMD12，终止传输过程 */
  EmmcSdioCmdCfg((u32)0, EMMC_CMD_STOP_TRANSMISSION, EMMC_SDIO_SHORT_RESPONSE);  	


  errorstatus = CmdResp1Error(EMMC_CMD_STOP_TRANSMISSION);
  return(errorstatus);
}

EmmcError IsCardProgramming(u8 *pstatus);





/**
  * @摘要   允许擦除卡指定的内存区域
  * @参数   startaddr: 起始地址
  * @参数   endaddr: 结束地址
  * @返回值 SD_Error: SD卡错误代码
  */
EmmcError emmc_erasemultiblock(u32 startaddr,u32 endaddr)
{
  EmmcError errorstatus = EMMC_OK;
  u32 delay = 0;
  volatile u32 maxdelay = 0;
  u8 cardstate = 0x0F;
	

		if ((MyEmmcCardInfo.ExtCardComdClasses & EMMC_CCCC_ERASE) == 0)

		{
			errorstatus = EMMC_REQUEST_NOT_APPLICABLE;
			return(errorstatus);
		}

		maxdelay = 9200000 / ((SDIO_CLKCTL & 0xFF) + 2);

		if (sdio_response_get(SDIO_RESPONSE0) & EMMC_CARD_LOCKED)
		{
			errorstatus = EMMC_LOCK_UNLOCK_FAILED;
			//printf("应答1问题111");
			return(errorstatus);
		}
			/* CMD35:设置起始地址 */		
     EmmcSdioCmdCfg(startaddr, EMMC_CMD_ERASE_GRP_START, EMMC_SDIO_SHORT_RESPONSE);  		

     errorstatus = CmdResp1Error(EMMC_CMD_ERASE_GRP_START);
     if (errorstatus != EMMC_OK)
     {
			 //printf("应答1问题2222");
       return(errorstatus);
     }
	      /*!< Send CMD36 SD_ERASE_GRP_END with argument as addr  */
     EmmcSdioCmdCfg(endaddr, EMMC_CMD_ERASE_GRP_END, EMMC_SDIO_SHORT_RESPONSE);  			 



     errorstatus = CmdResp1Error(EMMC_CMD_ERASE_GRP_END);
     if (errorstatus != EMMC_OK)
     {
			 //printf("应答1问题333");
       return(errorstatus);
     }
	 	  /* CMD38:执行擦除 */
     EmmcSdioCmdCfg(0x00000001, EMMC_CMD_ERASE, EMMC_SDIO_SHORT_RESPONSE); 


   errorstatus = CmdResp1Error(EMMC_CMD_ERASE);

   if (errorstatus != EMMC_OK)
   {
		 //printf("应答1问题4444");
     return(errorstatus);
   }
	
	 
  /* 等待卡退出编程状态 */
  errorstatus = IsCardProgramming(&cardstate);
  delay = EMMC_DATATIMEOUT;
  while ((delay > 0) && (errorstatus == EMMC_OK) && ((EMMC_CARD_PROGRAMMING == cardstate) || (EMMC_CARD_RECEIVING == cardstate)))
  {
    errorstatus = IsCardProgramming(&cardstate);
  }
	
  return(errorstatus);
}


/**
  * @brief  Returns the current card's status.
  * @param  pcardstatus: pointer to the buffer that will contain the SD card 
  *         status (Card Status register).
  * @retval EmmcError: SD Card Error code.
  */
EmmcError EmmcSendStatus(u32 *pcardstatus)
{
  EmmcError errorstatus = EMMC_OK;

  if (pcardstatus == NULL)
  {
    errorstatus = EMMC_INVALID_PARAMETER;
    return(errorstatus);
  }

  EmmcSdioCmdCfg(0, EMMC_CMD_SEND_STATUS, EMMC_SDIO_SHORT_RESPONSE);


  errorstatus = CmdResp1Error(EMMC_CMD_SEND_STATUS);

  if (errorstatus != EMMC_OK)
  {
    return(errorstatus);
  }

  *pcardstatus = sdio_response_get(SDIO_RESPONSE0);

  return(errorstatus);
}		



/**
  * @brief  CMD0响应检测.
  * @param  None
  * @retval EmmcError: eMMC卡操作返回码.
  */
static EmmcError CmdError(void)
{
  EmmcError errorstatus = EMMC_OK;
  u32 timeout;

  timeout = SDIO_CMD0TIMEOUT; /* 0x10000 */

  while ((timeout > 0) && (sdio_flag_get(SDIO_FLAG_CMDSEND) == RESET))
  {
    timeout--;
  }

  if (timeout == 0)
  {	 
		/* CMD0超时返回 */
    errorstatus = EMMC_CMD_RSP_TIMEOUT;						
    return(errorstatus);
  }

  /* 清除标志 */
  sdio_flag_clear(SDIO_FLAG_CCRCERR | SDIO_FLAG_DTCRCERR | SDIO_FLAG_CMDTMOUT | SDIO_FLAG_DTTMOUT | 
                  SDIO_FLAG_TXURE | SDIO_FLAG_RXORE | SDIO_FLAG_CMDRECV | SDIO_FLAG_CMDSEND | 
                  SDIO_FLAG_DTEND | SDIO_FLAG_STBITE | SDIO_FLAG_DTBLKEND);

	/* 命令已发送 */
  return(errorstatus);
}

/**
  * @brief  Checks for error conditions for R7 response.
  * @param  None
  * @retval EmmcError: SD Card Error code.
  */
EmmcError CmdResp7Error(void)
{
  EmmcError errorstatus = EMMC_OK;
  u32 status;
  u32 timeout = SDIO_CMD0TIMEOUT;

  status = SDIO_STAT;

  while (!(status & (SDIO_FLAG_CCRCERR | SDIO_FLAG_CMDRECV | SDIO_FLAG_CMDTMOUT)) && (timeout > 0))
  {
    timeout--;
    status = SDIO_STAT;
  }

  if ((timeout == 0) || (status & SDIO_FLAG_CMDTMOUT))
  {
    /*!< Card is not V2.0 complient or card does not support the set voltage range */
    errorstatus = EMMC_CMD_RSP_TIMEOUT;
    sdio_flag_clear(SDIO_FLAG_CMDTMOUT);
    return(errorstatus);
  }

  if (status & SDIO_FLAG_CMDRECV)
  {
    /*!< Card is SD V2.0 compliant */
    errorstatus = EMMC_OK;
    sdio_flag_clear(SDIO_FLAG_CMDRECV);
    return(errorstatus);
  }
  return(errorstatus);
}

/**
  * @brief  Checks for error conditions for R1 response.
  * @param  cmd: The sent command index.
  * @retval EmmcError: SD Card Error code.
  */
EmmcError CmdResp1Error(u8 cmd)
{
  EmmcError errorstatus = EMMC_OK;
  u32 status = 0;
  u32 response_r1 = 0;

  status = SDIO_STAT;

  while (!(status & (SDIO_FLAG_CCRCERR | SDIO_FLAG_CMDRECV | SDIO_FLAG_CMDTMOUT)))
  {
    status = SDIO_STAT;
  }

  if (status & SDIO_FLAG_CMDTMOUT)
  {
    errorstatus = EMMC_CMD_RSP_TIMEOUT;
    sdio_flag_clear(SDIO_FLAG_CMDTMOUT);

    return(errorstatus);
  }
  else if (status & SDIO_FLAG_CCRCERR)
  {
    errorstatus = EMMC_CMD_CRC_FAIL;
    sdio_flag_clear(SDIO_FLAG_CCRCERR);
		//printf("Error1");
    return(errorstatus);
  }

  /*!< Check response received is of desired command */
  if (sdio_command_index_get() != cmd)
  {
    errorstatus = EMMC_ILLEGAL_CMD;
		//printf("Error2");
    return(errorstatus);
  }

  /*!< Clear all the static flags */
  sdio_flag_clear(SDIO_FLAG_CCRCERR | SDIO_FLAG_DTCRCERR | SDIO_FLAG_CMDTMOUT | SDIO_FLAG_DTTMOUT | 
                  SDIO_FLAG_TXURE | SDIO_FLAG_RXORE | SDIO_FLAG_CMDRECV | SDIO_FLAG_CMDSEND | 
                  SDIO_FLAG_DTEND | SDIO_FLAG_STBITE | SDIO_FLAG_DTBLKEND);

  /*!< We have received response, retrieve it for analysis  */
  response_r1 = sdio_response_get(SDIO_RESPONSE0);

  if ((response_r1 & EMMC_OCR_ERRORBITS) == EMMC_ALLZERO)
  {
    return(errorstatus);
  }

  if (response_r1 & EMMC_OCR_ADDR_OUT_OF_RANGE)
  {
		//printf("Error4");
    return(EMMC_ADDR_OUT_OF_RANGE);
  }

  if (response_r1 & EMMC_OCR_ADDR_MISALIGNED)
  {
		//printf("Error5");
    return(EMMC_ADDR_MISALIGNED);
  }

  if (response_r1 & EMMC_OCR_BLOCK_LEN_ERR)
  {
		//printf("Error6");
    return(EMMC_BLOCK_LEN_ERR);
  }

  if (response_r1 & EMMC_OCR_ERASE_SEQ_ERR)
  {
		//printf("Error7");
    return(EMMC_ERASE_SEQ_ERR);
  }

  if (response_r1 & EMMC_OCR_BAD_ERASE_PARAM)
  {
		//printf("Error8");
    return(EMMC_BAD_ERASE_PARAM);
  }

  if (response_r1 & EMMC_OCR_WRITE_PROT_VIOLATION)
  {
		//printf("Error9");
    return(EMMC_WRITE_PROT_VIOLATION);
  }

  if (response_r1 & EMMC_OCR_LOCK_UNLOCK_FAILED)
  {
		//printf("Error10");
    return(EMMC_LOCK_UNLOCK_FAILED);
  }

  if (response_r1 & EMMC_OCR_COM_CRC_FAILED)
  {
		//printf("Error11");
    return(EMMC_COM_CRC_FAILED);
  }

  if (response_r1 & EMMC_OCR_ILLEGAL_CMD)
  {
		//printf("Error012");
    return(EMMC_ILLEGAL_CMD);
  }

  if (response_r1 & EMMC_OCR_CARD_ECC_FAILED)
  {
		//printf("Error13");
    return(EMMC_CARD_ECC_FAILED);
  }

  if (response_r1 & EMMC_OCR_CC_ERROR)
  {
		//printf("Error14");
    return(EMMC_CC_ERROR);
  }

  if (response_r1 & EMMC_OCR_GENERAL_UNKNOWN_ERROR)
  {
		//printf("Error15");
    return(EMMC_GENERAL_UNKNOWN_ERROR);
  }

  if (response_r1 & EMMC_OCR_STREAM_READ_UNDERRUN)
  {
		//printf("Error16");
    return(EMMC_STREAM_READ_UNDERRUN);
  }

  if (response_r1 & EMMC_OCR_STREAM_WRITE_OVERRUN)
  {
		//printf("Error17");
    return(EMMC_STREAM_WRITE_OVERRUN);
  }

  if (response_r1 & EMMC_OCR_CARD_ECC_DISABLED)
  {
		//printf("Error18");
    return(EMMC_CARD_ECC_DISABLED);
  }

  if (response_r1 & EMMC_OCR_ERASE_RESET)
  {
		//printf("Error19");
    return(EMMC_ERASE_RESET);
  }

  if (response_r1 & EMMC_OCR_AKE_SEQ_ERROR)
  {
		//printf("Error20");
    return(EMMC_AKE_SEQ_ERROR);
  }
  return(errorstatus);
}

/**
  * @brief  Checks for error conditions for R3 (OCR) response.
  * @param  None
  * @retval EmmcError: SD Card Error code.
  */
EmmcError CmdResp3Error(void)
{
  EmmcError errorstatus = EMMC_OK;
  u32 status;

  status = SDIO_STAT;

  while (!(status & (SDIO_FLAG_CCRCERR | SDIO_FLAG_CMDRECV | SDIO_FLAG_CMDTMOUT)))
  {
    status = SDIO_STAT;
  }

  if (status & SDIO_FLAG_CMDTMOUT)
  {
    errorstatus = EMMC_CMD_RSP_TIMEOUT;
    sdio_flag_clear(SDIO_FLAG_CMDTMOUT);
    return(errorstatus);
  }
  /*!< Clear all the static flags */
  sdio_flag_clear(SDIO_FLAG_CCRCERR | SDIO_FLAG_DTCRCERR | SDIO_FLAG_CMDTMOUT | SDIO_FLAG_DTTMOUT | 
                  SDIO_FLAG_TXURE | SDIO_FLAG_RXORE | SDIO_FLAG_CMDRECV | SDIO_FLAG_CMDSEND | 
                  SDIO_FLAG_DTEND | SDIO_FLAG_STBITE | SDIO_FLAG_DTBLKEND);
  return(errorstatus);
}

/**
  * @brief  Checks for error conditions for R2 (CID or CSD) response.
  * @param  None
  * @retval EmmcError: SD Card Error code.
  */
EmmcError CmdResp2Error(void)
{
  EmmcError errorstatus = EMMC_OK;
  u32 status;

  status = SDIO_STAT;

  while (!(status & (SDIO_FLAG_CCRCERR | SDIO_FLAG_CMDTMOUT | SDIO_FLAG_CMDRECV)))
  {
    status = SDIO_STAT;
  }

  if (status & SDIO_FLAG_CMDTMOUT)
  {
    errorstatus = EMMC_CMD_RSP_TIMEOUT;
    sdio_flag_clear(SDIO_FLAG_CMDTMOUT);
		//printf("请求超时！\r\n");
    return(errorstatus);
  }
  else if (status & SDIO_FLAG_CCRCERR)
  {
    errorstatus = EMMC_CMD_CRC_FAIL;
    sdio_flag_clear(SDIO_FLAG_CCRCERR);
		//printf("命令校验出错！");
    return(errorstatus);
  }

  /*!< Clear all the static flags */
  sdio_flag_clear(SDIO_FLAG_CCRCERR | SDIO_FLAG_DTCRCERR | SDIO_FLAG_CMDTMOUT | SDIO_FLAG_DTTMOUT | 
                  SDIO_FLAG_TXURE | SDIO_FLAG_RXORE | SDIO_FLAG_CMDRECV | SDIO_FLAG_CMDSEND | 
                  SDIO_FLAG_DTEND | SDIO_FLAG_STBITE | SDIO_FLAG_DTBLKEND);

  return(errorstatus);
}

/**
  * @brief  Checks for error conditions for R6 (RCA) response.
  * @param  cmd: The sent command index.
  * @param  prca: pointer to the variable that will contain the SD card relative 
  *         address RCA. 
  * @retval EmmcError: SD Card Error code.
  */
EmmcError CmdResp6Error(u8 cmd, u16 *prca)
{
  EmmcError errorstatus = EMMC_OK;
  u32 status = 0;
  u32 response_r1 = 0;

  status = SDIO_STAT;

  while (!(status & (SDIO_FLAG_CCRCERR | SDIO_FLAG_CMDTMOUT | SDIO_FLAG_CMDRECV)))
  {
    status = SDIO_STAT;
  }

  if (status & SDIO_FLAG_CMDTMOUT)
  {
    errorstatus = EMMC_CMD_RSP_TIMEOUT;
    sdio_flag_clear(SDIO_FLAG_CMDTMOUT);
		//printf("RCA请求超时！\r\n");
    return(errorstatus);
  }
  else if (status & SDIO_FLAG_CCRCERR)
  {
    errorstatus = EMMC_CMD_CRC_FAIL;
    sdio_flag_clear(SDIO_FLAG_CCRCERR);
    return(errorstatus);
  }

  /*!< Check response received is of desired command */
  if (sdio_command_index_get() != cmd)
  {
    errorstatus = EMMC_ILLEGAL_CMD;
    return(errorstatus);
  }

  /*!< Clear all the static flags */
  sdio_flag_clear(SDIO_FLAG_CCRCERR | SDIO_FLAG_DTCRCERR | SDIO_FLAG_CMDTMOUT | SDIO_FLAG_DTTMOUT | 
                  SDIO_FLAG_TXURE | SDIO_FLAG_RXORE | SDIO_FLAG_CMDRECV | SDIO_FLAG_CMDSEND | 
                  SDIO_FLAG_DTEND | SDIO_FLAG_STBITE | SDIO_FLAG_DTBLKEND);

  /*!< We have received response, retrieve it.  */
  response_r1 = sdio_response_get(SDIO_RESPONSE0);

  if (EMMC_ALLZERO == (response_r1 & (EMMC_R6_GENERAL_UNKNOWN_ERROR | EMMC_R6_ILLEGAL_CMD | EMMC_R6_COM_CRC_FAILED)))
  {
    *prca = (u16) (response_r1 >> 16);
    return(errorstatus);
  }

  if (response_r1 & EMMC_R6_GENERAL_UNKNOWN_ERROR)
  {
    return(EMMC_GENERAL_UNKNOWN_ERROR);
  }

  if (response_r1 & EMMC_R6_ILLEGAL_CMD)
  {
    return(EMMC_ILLEGAL_CMD);
  }

  if (response_r1 & EMMC_R6_COM_CRC_FAILED)
  {
    return(EMMC_COM_CRC_FAILED);
  }

  return(errorstatus);
}

/**
  * @brief  SDIO总线宽度使能/禁用设置.
  * @param  NewState: 需要设置的总线宽度或者类型.
  *   This parameter can be: ENABLE or DISABLE.
  * @retval EmmcError: eMMC操作码.
	* @Reference MMC标准协议4.41 Page206-207
  */
EmmcError EmmcEnWideBus(uint8_t NewState)
{
  u32 i=0;
  EmmcError Result = EMMC_ERROR;

//  u32 scr[2] = {0, 0};

	/* 检查eMMC是否被锁定 */
  if (sdio_response_get(SDIO_RESPONSE0) & EMMC_CARD_LOCKED)
  {
    Result = EMMC_LOCK_UNLOCK_FAILED;
    return(Result);
  }

  /*!< If wide bus operation to be enabled */
  if (NewState == 1)
  {
			/* CMD6[POWER_CLASS]:设置电压等级 */ 
	        EmmcSdioCmdCfg(EMMC_POWER_REG, EMMC_CMD_HS_SWITCH, EMMC_SDIO_SHORT_RESPONSE);

			for(i=0; i<50000; i++);
			Result = CmdResp1Error(EMMC_CMD_HS_SWITCH);
		  if (EMMC_OK != Result)
			{
				return(Result);
			}
			
			/* CMD6[ ]:设置HIGHTSPEED模式 */ 
			EmmcSdioCmdCfg(EMMC_HIGHSPEED_REG, EMMC_CMD_HS_SWITCH, EMMC_SDIO_SHORT_RESPONSE);
	
			for(i=0; i<50000; i++);
			Result = CmdResp1Error(EMMC_CMD_HS_SWITCH);
		  if (EMMC_OK != Result)
			{
				return(Result);
			}
			
		  /* CMD6[BUS_WIDTH]:设置总线宽度（8位） */ 
		    EmmcSdioCmdCfg(EMMC_8BIT_REG, EMMC_CMD_HS_SWITCH, EMMC_SDIO_SHORT_RESPONSE);			

			for(i=0; i<50000; i++);
			Result = CmdResp1Error(EMMC_CMD_HS_SWITCH);
			
			if (EMMC_OK != Result)
			{
				return(Result);
			}
      return(Result);
  }   /*!< If wide bus operation to be disabled */
  else
  {
      Result = CmdResp1Error(EMMC_CMD_APP_CMD);

      if (Result != EMMC_OK)
      {
        return(Result);
      }

      if (Result != EMMC_OK)
      {
        return(Result);
      }

      return(Result);
  }
}

/**
  * @brief  Checks if the SD card is in programming state.
  * @param  pstatus: pointer to the variable that will contain the SD card state.
  * @retval EmmcError: SD Card Error code.
  */
EmmcError IsCardProgramming(u8 *pstatus)
{
  EmmcError errorstatus = EMMC_OK;
  volatile u32 respR1 = 0, status = 0;

  EmmcSdioCmdCfg(0, EMMC_CMD_SEND_STATUS, EMMC_SDIO_SHORT_RESPONSE);			

  status = SDIO_STAT;
  while (!(status & (SDIO_FLAG_CCRCERR | SDIO_FLAG_CMDRECV | SDIO_FLAG_CMDTMOUT)))	 /* CRC校验错误 */
  {
    status = SDIO_STAT;
  }

  if (status & SDIO_FLAG_CMDTMOUT)
  {
    errorstatus = EMMC_CMD_RSP_TIMEOUT;				 /* 命令响应超时 */
    sdio_flag_clear(SDIO_FLAG_CMDTMOUT);
    return(errorstatus);
  }
  else if (status & SDIO_FLAG_CCRCERR)
  {
    errorstatus = EMMC_CMD_CRC_FAIL;
    sdio_flag_clear(SDIO_FLAG_CCRCERR);
    return(errorstatus);
  }

  status = (u32)sdio_command_index_get();

  /*!< Check response received is of desired command */
  if (status != EMMC_CMD_SEND_STATUS)
  {
    errorstatus = EMMC_ILLEGAL_CMD;
    return(errorstatus);
  }

  /*!< Clear all the static flags */
  sdio_flag_clear(SDIO_FLAG_CCRCERR | SDIO_FLAG_DTCRCERR | SDIO_FLAG_CMDTMOUT | SDIO_FLAG_DTTMOUT | 
                  SDIO_FLAG_TXURE | SDIO_FLAG_RXORE | SDIO_FLAG_CMDRECV | SDIO_FLAG_CMDSEND | 
                  SDIO_FLAG_DTEND | SDIO_FLAG_STBITE | SDIO_FLAG_DTBLKEND);


  /*!< We have received response, retrieve it for analysis  */
  respR1 = sdio_response_get(SDIO_RESPONSE0);

  /*!< Find out card status */
  *pstatus = (u8) ((respR1 >> 9) & 0x0000000F);

  if ((respR1 & EMMC_OCR_ERRORBITS) == EMMC_ALLZERO)
  {
    return(errorstatus);
  }

  if (respR1 & EMMC_OCR_ADDR_OUT_OF_RANGE)
  {
    return(EMMC_ADDR_OUT_OF_RANGE);
  }

  if (respR1 & EMMC_OCR_ADDR_MISALIGNED)
  {
    return(EMMC_ADDR_MISALIGNED);
  }

  if (respR1 & EMMC_OCR_BLOCK_LEN_ERR)
  {
    return(EMMC_BLOCK_LEN_ERR);
  }

  if (respR1 & EMMC_OCR_ERASE_SEQ_ERR)
  {
    return(EMMC_ERASE_SEQ_ERR);
  }

  if (respR1 & EMMC_OCR_BAD_ERASE_PARAM)
  {
    return(EMMC_BAD_ERASE_PARAM);
  }

  if (respR1 & EMMC_OCR_WRITE_PROT_VIOLATION)
  {
    return(EMMC_WRITE_PROT_VIOLATION);
  }

  if (respR1 & EMMC_OCR_LOCK_UNLOCK_FAILED)
  {
    return(EMMC_LOCK_UNLOCK_FAILED);
  }

  if (respR1 & EMMC_OCR_COM_CRC_FAILED)
  {
    return(EMMC_COM_CRC_FAILED);
  }

  if (respR1 & EMMC_OCR_ILLEGAL_CMD)
  {
    return(EMMC_ILLEGAL_CMD);
  }

  if (respR1 & EMMC_OCR_CARD_ECC_FAILED)
  {
    return(EMMC_CARD_ECC_FAILED);
  }

  if (respR1 & EMMC_OCR_CC_ERROR)
  {
    return(EMMC_CC_ERROR);
  }

  if (respR1 & EMMC_OCR_GENERAL_UNKNOWN_ERROR)
  {
    return(EMMC_GENERAL_UNKNOWN_ERROR);
  }

  if (respR1 & EMMC_OCR_STREAM_READ_UNDERRUN)
  {
    return(EMMC_STREAM_READ_UNDERRUN);
  }

  if (respR1 & EMMC_OCR_STREAM_WRITE_OVERRUN)
  {
    return(EMMC_STREAM_WRITE_OVERRUN);
  }
  if (respR1 & EMMC_OCR_CARD_ECC_DISABLED)
  {
    return(EMMC_CARD_ECC_DISABLED);
  }

  if (respR1 & EMMC_OCR_ERASE_RESET)
  {
    return(EMMC_ERASE_RESET);
  }

  if (respR1 & EMMC_OCR_AKE_SEQ_ERROR)
  {
    return(EMMC_AKE_SEQ_ERROR);
  }

  return(errorstatus);
}
/**
  * @brief  Converts the number of bytes in power of two and returns the power.
  * @param  NumberOfBytes: number of bytes.
  * @retval Capacity /Mbytes
  */
uint32_t emmcgetcapacity(void)
{
    uint32_t Capacity = 0;
    if(1 == MyEmmcCardInfo.init_ret)
    {
        Capacity = MyEmmcCardInfo.eMMC_Capacity;
    }
    return Capacity;
}
/**
  * @摘要:  允许从卡中的指定地址读多个块的数据（此函数采用双缓存机制）
  * @参数  readbuff: 指向存放接收到数据的buffer
  * @参数  ReadAddr: 将要读数据的地址
  * @参数  BlockSize: SD卡数据块的大小
  * @参数  NumberOfBlocks: 要读的块的数量
  * @返回值 SD_Error: SD卡错误代码
  */
EmmcError EmmcReadMultiBlocksNoWait(u8 *readbuff, u32 ReadAddr, u16 BlockSize, u32 NumberOfBlocks)
{
  EmmcError errorstatus = EMMC_OK;
//  u32 count = 0, *tempbuff = (u32 *)readbuff,debug0
	vu32 temp = 0;  
  eMMCTransferEnd = 0;
    DMAEndOfTransfer = 0;
  eMMCStopCondition = 1;
    if(!NumberOfBlocks)
        return errorstatus;    
	EmmcLowLevelDMARxConfig(readbuff, (NumberOfBlocks * BlockSize));
  sdio_dsm_disable();

  /* 设置Block的大小 */
  EmmcSdioCmdCfg((u32) BlockSize, EMMC_CMD_SET_BLOCKLEN, EMMC_SDIO_SHORT_RESPONSE);	 	

	temp = SDIO_STAT;
  errorstatus = CmdResp1Error(EMMC_CMD_SET_BLOCKLEN);

  if (EMMC_OK != errorstatus)
  {
    return(errorstatus);
  }
  
   /*!< Clear all the static flags */
  sdio_flag_clear(SDIO_FLAG_CCRCERR | SDIO_FLAG_DTCRCERR | SDIO_FLAG_CMDTMOUT | SDIO_FLAG_DTTMOUT | 
                  SDIO_FLAG_TXURE | SDIO_FLAG_RXORE | SDIO_FLAG_CMDRECV | SDIO_FLAG_CMDSEND | 
                  SDIO_FLAG_DTEND | SDIO_FLAG_STBITE | SDIO_FLAG_DTBLKEND);
  
  /* Configure data transfer */
  sdio_data_config(EMMC_DATATIMEOUT, NumberOfBlocks * BlockSize, SDIO_DATABLOCKSIZE_512BYTES);
  sdio_data_transfer_config(SDIO_TRANSMODE_BLOCK, SDIO_TRANSDIRECTION_TOSDIO);
  sdio_dsm_disable();
  
  sdio_data_config(EMMC_DATATIMEOUT, NumberOfBlocks * BlockSize, SDIO_DATABLOCKSIZE_512BYTES);
  sdio_data_transfer_config(SDIO_TRANSMODE_BLOCK, SDIO_TRANSDIRECTION_TOSDIO);
  sdio_dsm_enable();

    /* pre-erased，调用CMD25之前，预写入操作的块数目 *//* CMD23 */
  EmmcSdioCmdCfg((u32) NumberOfBlocks, EMMC_CMD_SET_BLOCK_COUNT, EMMC_SDIO_SHORT_RESPONSE);	   	   


  errorstatus = CmdResp1Error(EMMC_CMD_SET_BLOCK_COUNT);

  if (errorstatus != EMMC_OK)
	{
    return(errorstatus);
  }	
  
  /* 发送 CMD18（READ_MULT_BLOCK）命令，参数为要读数据的地址 */
  EmmcSdioCmdCfg((u32)ReadAddr, EMMC_CMD_READ_MULT_BLOCK, EMMC_SDIO_SHORT_RESPONSE);	   

  errorstatus = CmdResp1Error(EMMC_CMD_READ_MULT_BLOCK);
  if (errorstatus != EMMC_OK)
  {
    return(errorstatus);
  }
  
#if defined (SD_POLLING_MODE)  			/* 轮询 */

   while (!(SDIO->STA &(SDIO_FLAG_RXOVERR | SDIO_FLAG_DCRCFAIL | SDIO_FLAG_DATAEND | SDIO_FLAG_DTIMEOUT | SDIO_FLAG_STBITERR)))
   {
     if (SDIO_GetFlagStatus(SDIO_FLAG_RXFIFOHF) != RESET)
     {
	   	 for (count = 0; count < EMMC_HALFFIFO; count++)
       {
         *(tempbuff + count) = SDIO_ReadData();
       }
       tempbuff += EMMC_HALFFIFO;
     }
   }

   if (SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != RESET)
   {
     SDIO_ClearFlag(SDIO_FLAG_DTIMEOUT);
     errorstatus = EMMC_DATA_TIMEOUT;
     return(errorstatus);
   }
   else if (SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != RESET)
   {
     SDIO_ClearFlag(SDIO_FLAG_DCRCFAIL);
     errorstatus = EMMC_DATA_CRC_FAIL;
     return(errorstatus);
   }
   else if (SDIO_GetFlagStatus(SDIO_FLAG_RXOVERR) != RESET)
   {
     SDIO_ClearFlag(SDIO_FLAG_RXOVERR);
     errorstatus = EMMC_RX_OVERRUN;
     return(errorstatus);
   }
   else if (SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != RESET)
   {
     SDIO_ClearFlag(SDIO_FLAG_STBITERR);
     errorstatus = EMMC_START_BIT_ERR;
     return(errorstatus);
   }
   while (SDIO_GetFlagStatus(SDIO_FLAG_RXDAVL) != RESET)
   {
     *tempbuff = SDIO_ReadData();
     tempbuff++;
   }

   if (SDIO_GetFlagStatus(SDIO_FLAG_DATAEND) != RESET)
   {
     /* In Case Of SD-CARD Send Command STOP_TRANSMISSION */
//     if ((_EMMC_STD_CAPACITY_SD_CARD_V1_1 == eMMC_Type) || (_EMMC_HIGH_CAPACITY_SD_CARD == eMMC_Type) || (_EMMC_STD_CAPACITY_SD_CARD_V2_0 == eMMC_Type))
//     {
       /* 发送 CMD12 STOP_TRANSMISSION */
	   EmmcSdioCmdCfg(0, EMMC_CMD_STOP_TRANSMISSION, EMMC_SDIO_SHORT_RESPONSE);	   


       errorstatus = CmdResp1Error(EMMC_CMD_STOP_TRANSMISSION);

       if (errorstatus != EMMC_OK)
       {
         return(errorstatus);
       }
//     }
   }   
  /*!< Clear all the static flags */
  SDIO_ClearFlag(SDIO_STATIC_FLAGS);
#endif

#if defined (SD_DMA_MODE)					 /* DMA模式 */ 
  sdio_interrupt_enable(SDIO_INT_DTCRCERR | SDIO_INT_DTTMOUT | SDIO_INT_DTEND | SDIO_INT_RXORE | SDIO_INT_STBITE);
  sdio_dma_enable();
	 
//	errorstatus = EmmcWaitReadOperation();
//	if(errorstatus !=  EMMC_OK)
//	{
//		return errorstatus;
//	}
//	
//	do
//	{			
//			TranResult = EmmcGetStatus();
//	}while(TranResult != EMMC_TRANSFER_OK);
	
#endif			
  return(errorstatus);
}



bool EmmcReadBlocksNoWait(vu8 *readbuff, u32 ReadAddr, u32 NumberOfBlocks)
{
  u8 Timeout;
  EmmcError errorstatus;

  errorstatus = EMMC_ERROR;
  Timeout = 3;
  while((errorstatus != EMMC_OK) && (Timeout))
  {
    errorstatus = EmmcReadMultiBlocksNoWait((u8*)readbuff, ReadAddr, 512, NumberOfBlocks);
    Timeout--;
  }
  if(!Timeout)
  {
    return false;
  }
  return true;
}
u8 readbyetsfromemmcnowait(u8 *buff, u32 addr, u32 size)
{    
     u8 ret = 0xff;
    u32 pack_num = 0;    
    u32 first_pack_size = 0;    
    u32 first_pack_unuse = 0;            
    u32 first_pack_addr = 0;              /*第一包（512对齐）的起始地址*/
    u32 last_pack_size = 0;               /*最后一包大小  0代表最后一包结尾是整512字节*/
    u32 last_pack_addr = 0;               /*最后一包（512对齐）的起始地址, 0代表最后一包结尾是整512字节  或者size小于512字节*/
    
    
    
    if(size != 0)
    {
        first_pack_unuse = addr%EMMC_BLOCK_SIZE;
        first_pack_size  = (EMMC_BLOCK_SIZE - first_pack_unuse)>size?size:(EMMC_BLOCK_SIZE - first_pack_unuse);
        first_pack_addr = addr - first_pack_unuse;
        
        last_pack_size = (addr + size)%EMMC_BLOCK_SIZE;
        /*如果收尾均在一包且，首不完整*/
        if(((last_pack_size + first_pack_size) > size)&&(first_pack_unuse != 0))
        {
            last_pack_addr = 0;
        }
		else
		{
 		    last_pack_addr = addr + size - last_pack_size;  
		}        
        
        if((first_pack_unuse == 0)&&(last_pack_size == 0))
        {        
            EmmcReadBlocksNoWait(buff, addr/EMMC_BLOCK_SIZE, size/EMMC_BLOCK_SIZE);   			
        }
        else
        {

            /*如果第一包头不完整，则特殊处理*/
			if(first_pack_unuse != 0)
		    {
                emmcreadblocks(sEmmcTempBuff, first_pack_addr/EMMC_BLOCK_SIZE, 1);
				memcpy(buff, &sEmmcTempBuff[first_pack_unuse], first_pack_size); 					
		    }
			else
			{

			}
            /*如果头尾均在一块内，则中间连续0块*/
			if(last_pack_addr == 0)
			{
			    pack_num = 0;
			}
			else
			{
				if(last_pack_size == 0)
			    {					
				    pack_num = 	(size - first_pack_size)/EMMC_BLOCK_SIZE;
				}
				else
			    {
					pack_num = (last_pack_addr - first_pack_addr)/EMMC_BLOCK_SIZE;
					if(first_pack_unuse != 0)
				    { 
				        pack_num--;
			        }
			    }
				
				
			}
			/*数据头和尾在同一个512块里面，且头不是整512字节之前已经处理过了*/
			if((first_pack_size == size)&&(first_pack_unuse != 0))
			{
				
			}
			else
		    {
				/*尾是整512字节的，连续块写中，也处理了*/
				if(last_pack_size != 0)
			    {
					emmcreadblocks(sEmmcTempBuff, last_pack_addr/EMMC_BLOCK_SIZE, 1); 
					memcpy( (u8 *)&buff[size - last_pack_size], &sEmmcTempBuff, last_pack_size);		
		        }
			}
            
			if(first_pack_unuse != 0)
	        {
			    EmmcReadBlocksNoWait(&buff[first_pack_size], first_pack_addr/EMMC_BLOCK_SIZE + 1, pack_num);	
		    }
			else
			{
			    EmmcReadBlocksNoWait(buff, first_pack_addr/EMMC_BLOCK_SIZE, pack_num);	
			}

        }                          
    }    
    
    
    return ret; 
}

bool readbyetsfromemmc(u8 *buff, u32 addr, u32 size)
{    
     u8 ret = true;
    u32 pack_num = 0;    
    u32 first_pack_size = 0;    
    u32 first_pack_unuse = 0;            
    u32 first_pack_addr = 0;              /*第一包（512对齐）的起始地址*/
    u32 last_pack_size = 0;               /*最后一包大小  0代表最后一包结尾是整512字节*/
    u32 last_pack_addr = 0;               /*最后一包（512对齐）的起始地址, 0代表最后一包结尾是整512字节  或者size小于512字节*/
    
    
    
    if(size != 0)
    {
        first_pack_unuse = addr%EMMC_BLOCK_SIZE;
        first_pack_size  = (EMMC_BLOCK_SIZE - first_pack_unuse)>size?size:(EMMC_BLOCK_SIZE - first_pack_unuse);
        first_pack_addr = addr - first_pack_unuse;
        
        last_pack_size = (addr + size)%EMMC_BLOCK_SIZE;
        /*如果收尾均在一包且，首不完整*/
        if(((last_pack_size + first_pack_size) > size)&&(first_pack_unuse != 0))
        {
            last_pack_addr = 0;
        }
		else
		{
 		    last_pack_addr = addr + size - last_pack_size;  
		}        
        
        if((first_pack_unuse == 0)&&(last_pack_size == 0))
        {        
            emmcreadblocks(buff, addr/EMMC_BLOCK_SIZE, size/EMMC_BLOCK_SIZE);   			
        }
        else
        {

            /*如果第一包头不完整，则特殊处理*/
			if(first_pack_unuse != 0)
		    {
                emmcreadblocks(sEmmcTempBuff, first_pack_addr/EMMC_BLOCK_SIZE, 1);
				memcpy(buff, &sEmmcTempBuff[first_pack_unuse], first_pack_size); 					
		    }
			else
			{

			}
            /*如果头尾均在一块内，则中间连续0块*/
			if(last_pack_addr == 0)
			{
			    pack_num = 0;
			}
			else
			{
				if(last_pack_size == 0)
			    {					
				    pack_num = 	(size - first_pack_size)/EMMC_BLOCK_SIZE;
				}
				else
			    {
					pack_num = (last_pack_addr - first_pack_addr)/EMMC_BLOCK_SIZE;
					if(first_pack_unuse != 0)
				    { 
				        pack_num--;
			        }
			    }
				
				
			}
            
			
            
			
			/*数据头和尾在同一个512块里面，且头不是整512字节之前已经处理过了*/
			if((first_pack_size == size)&&(first_pack_unuse != 0))
			{
				
			}
			else
		    {
				/*尾是整512字节的，连续块写中，也处理了*/
				if(last_pack_size != 0)
			    {
					emmcreadblocks(sEmmcTempBuff, last_pack_addr/EMMC_BLOCK_SIZE, 1); 
					memcpy( (u8 *)&buff[size - last_pack_size], &sEmmcTempBuff, last_pack_size);		
		        }
			}
			
            if(first_pack_unuse != 0)
	        {
			    emmcreadblocks(&buff[first_pack_size], first_pack_addr/EMMC_BLOCK_SIZE + 1, pack_num);	
		    }
			else
			{
			    emmcreadblocks(buff, first_pack_addr/EMMC_BLOCK_SIZE, pack_num);	
			}
        }                          
    }    
    
    
    return ret; 
    

}

u8 erasebyetstoemmc(u32 addr, u32 size)
{
    u8 ret = 0xff;
    u32 pack_num = 0;    
    u32 first_pack_size = 0;    
    u32 first_pack_unuse = 0;            
    u32 first_pack_addr = 0;              /*第一包（512对齐）的起始地址*/
    u32 last_pack_size = 0;               /*最后一包大小  0代表最后一包结尾是整512字节*/
    u32 last_pack_addr = 0;               /*最后一包（512对齐）的起始地址, 0代表最后一包结尾是整512字节  或者size小于512字节*/
    
    
    if(size != 0)
    {
        first_pack_unuse = addr%EMMC_BLOCK_SIZE;
        first_pack_size  = (EMMC_BLOCK_SIZE - first_pack_unuse)>size?size:(EMMC_BLOCK_SIZE - first_pack_unuse);
        first_pack_addr = addr - first_pack_unuse;
        
        last_pack_size = (addr + size)%EMMC_BLOCK_SIZE;
        /*如果收尾均在一包且，首不完整*/
        if(((last_pack_size + first_pack_size) > size)&&(first_pack_unuse != 0))
        {
            last_pack_addr = 0;
        }
		else
		{
		    last_pack_addr = addr + size - last_pack_size;  
		} 
        
        if((first_pack_unuse == 0)&&(last_pack_size == 0))
        {        
            EmmcEraseBlocks(addr/EMMC_BLOCK_SIZE, size/EMMC_BLOCK_SIZE);          
        }
        else
        {

            /*如果第一包头不完整，则特殊处理*/
			if(first_pack_unuse != 0)
		    {
                emmcreadblocks(sEmmcTempBuff, first_pack_addr/EMMC_BLOCK_SIZE, 1);
                memset(&sEmmcTempBuff[first_pack_unuse], 0xFF, first_pack_size); 
                emmcerasewriteblocks(sEmmcTempBuff, first_pack_addr/EMMC_BLOCK_SIZE, 1);					
		    }
			else
			{

			}
            /*如果头尾均在一块内，则中间连续0块*/
			if(last_pack_addr == 0)
			{
			    pack_num = 0;
			}
			else
			{
				if(last_pack_size == 0)
			    {					
				    pack_num = 	(size - first_pack_size)/EMMC_BLOCK_SIZE;
				}
				else
			    {
					pack_num = (last_pack_addr - first_pack_addr)/EMMC_BLOCK_SIZE;
					if(first_pack_unuse != 0)
				    { 
				        pack_num--;
			        }
			    }
			}
			if(first_pack_unuse != 0)
	        {
			    EmmcEraseBlocks(first_pack_addr/EMMC_BLOCK_SIZE + 1, pack_num);	
		    }
			else
			{
			    EmmcEraseBlocks(first_pack_addr/EMMC_BLOCK_SIZE, pack_num);	
			}
			/*数据头和尾在同一个512块里面，且头不是整512字节之前已经处理过了*/
			if((first_pack_size == size)&&(first_pack_unuse != 0))
			{
				
			}
			else
		    {
				/*尾是整512字节的，连续块写中，也处理了*/
				if(last_pack_size != 0)
			    {
					emmcreadblocks(sEmmcTempBuff, last_pack_addr/EMMC_BLOCK_SIZE, 1); 
					memset(&sEmmcTempBuff, 0xFF, last_pack_size);
					emmcerasewriteblocks(sEmmcTempBuff, last_pack_addr/EMMC_BLOCK_SIZE, 1);		
		        }
			}
        }                          
    }    
    
    
    return ret;    
}

bool writebyetstoemmc(u8 *data, u32 addr, u32 size)
{
    u8 ret = true;
    u32 pack_num = 0;    
    u32 first_pack_size = 0;    
    u32 first_pack_unuse = 0;            
    u32 first_pack_addr = 0;              /*第一包（512对齐）的起始地址*/
    u32 last_pack_size = 0;               /*最后一包大小  0代表最后一包结尾是整512字节*/
    u32 last_pack_addr = 0;               /*最后一包（512对齐）的起始地址, 0代表最后一包结尾是整512字节  或者size小于512字节*/
        
    if(size != 0)
    {
        first_pack_unuse = addr%EMMC_BLOCK_SIZE;
        first_pack_size  = (EMMC_BLOCK_SIZE - first_pack_unuse)>size?size:(EMMC_BLOCK_SIZE - first_pack_unuse);
        first_pack_addr = addr - first_pack_unuse;

        last_pack_size = (addr + size)%EMMC_BLOCK_SIZE;
        /*如果收尾均在一包且，首不完整*/
        if(((last_pack_size + first_pack_size) > size)&&(first_pack_unuse != 0))
        {
            last_pack_addr = 0;
        }
        else
        {
            last_pack_addr = addr + size - last_pack_size;  
        } 

        if((first_pack_unuse == 0)&&(last_pack_size == 0))
        {        
            emmcerasewriteblocks(data, addr/EMMC_BLOCK_SIZE, size/EMMC_BLOCK_SIZE);          
        }
        else
        {
            /*如果第一包头不完整，则特殊处理*/
            if(first_pack_unuse != 0)
            {
                emmcreadblocks(sEmmcTempBuff, first_pack_addr/EMMC_BLOCK_SIZE, 1);
                memcpy(&sEmmcTempBuff[first_pack_unuse], data, first_pack_size); 	
                emmcerasewriteblocks(sEmmcTempBuff, first_pack_addr/EMMC_BLOCK_SIZE, 1);					
            }
            else
            {

            }
                /*如果头尾均在一块内，则中间连续0块*/
            if(last_pack_addr == 0)
            {
                pack_num = 0;
            }
            else
            {
                if(last_pack_size == 0)
                {					
                    pack_num = 	(size - first_pack_size)/EMMC_BLOCK_SIZE;
                }
                else
                {
                    pack_num = (last_pack_addr - first_pack_addr)/EMMC_BLOCK_SIZE;
                    if(first_pack_unuse != 0)
                    { 
                        pack_num--;
                    }
                }
            }
            if(first_pack_unuse != 0)
            {
                emmcerasewriteblocks(&data[first_pack_size], first_pack_addr/EMMC_BLOCK_SIZE + 1, pack_num);	
            }
            else
            {
                emmcerasewriteblocks(data, first_pack_addr/EMMC_BLOCK_SIZE, pack_num);	
            }
            
            /*数据头和尾在同一个512块里面，且头不是整512字节之前已经处理过了*/
            if((first_pack_size == size)&&(first_pack_unuse != 0))
            {
            
            }
            else
            {
                /*尾是整512字节的，连续块写中，也处理了*/
                if(last_pack_size != 0)
                {
                    emmcreadblocks(sEmmcTempBuff, last_pack_addr/EMMC_BLOCK_SIZE, 1); 
                    memcpy(&sEmmcTempBuff, (u8 *)&data[size - last_pack_size], last_pack_size);
                    emmcerasewriteblocks(sEmmcTempBuff, last_pack_addr/EMMC_BLOCK_SIZE, 1);		
                }
            }
        }                          
    }    

    return ret;    
}
    
/**
  * @brief  emmc_test
  * @param  none
  * @retval 0 fail 1 success
  */
u8 emmc_test(void)
{
    u8 ret = 1;
    u32 i =0;
    u32 j = 0;
    u32 size = 1024;  // 减少测试数据量：从2048改为1024字节
    u32 addr = 0;
    u8 buff[1024] = {0}; 
    
    // 使用更合理的测试地址：从块地址1000开始测试        
    for(j = 0; j < 3 ; j++)  // 减少循环次数：从10次改为3次
    {
        addr = 1000 + j * 10;  // 使用简单的地址：1000, 1010, 1020...
        
        for(i = 0;i <size ; i++)
        {
             buff[i] = i+j;      
        }    
        
        emmcerasewriteblocks(buff, addr, size/512); 
        
        memset(buff, 0, size);
        
        emmcreadblocks(buff, addr, size/512);
        
        for(i =0; i<size; i++)
        {
            if(buff[i] != (u8)(i + j))
            {
                ret = 0;
                break;                
            }
        }
        if(ret == 0)
        {
            break;
        }
    }
    
    return ret;
}

/**
  * @brief  hw init
  * @param  
  * @param
  * @retval
  */
static void EmmcDeviceInit(void)
{
    /* 使能SDIO时钟 */
    rcu_periph_clock_enable(RCU_SDIO);
    // /* SDIO复位 */
    // rcu_periph_reset_enable(RCU_SDIORST);
    /* 使能DMA时钟 */
    rcu_periph_clock_enable(RCU_DMA1);
    
    EmmcGpioConfiguration();

    sdio_deinit();
    // /* 取消SDIO复位 */
    // rcu_periph_reset_disable(RCU_SDIORST);
}
/**
  * @brief  sdio set
  * @param  
  * @param
  * @retval
  */
static void EmmcSdioCfg(u8 clk, u32 bus_wide)
{
    uint32_t gd32_bus_wide;
    
    /* 转换总线宽度 */
    if(bus_wide == EMMC_SDIO_1BIT_BUS)
    {
        gd32_bus_wide = SDIO_BUSMODE_1BIT;
    }
    else if(bus_wide == EMMC_SDIO_4BIT_BUS)
    {
        gd32_bus_wide = SDIO_BUSMODE_4BIT;
    }
    else if(bus_wide == EMMC_SDIO_8BIT_BUS)
    {
        gd32_bus_wide = SDIO_BUSMODE_8BIT;
    }    
    else
    {
        gd32_bus_wide = SDIO_BUSMODE_1BIT;
    }
    
    /*!< SDIO_CK = SDIOCLK / (SDIO_INIT_CLK_DIV + 2) */
    /*!< on GD32F4xx devices, SDIOCLK is fixed to 48MHz */
    /*!< SDIO_CK for initialization should not exceed 400 KHz */     
    if(clk == 0xFF) {
        // 时钟旁路模式，直接使用48MHz
        sdio_clock_config(SDIO_SDIOCLKEDGE_RISING, SDIO_CLOCKBYPASS_ENABLE, 
                          SDIO_CLOCKPWRSAVE_DISABLE, 0);
    } else {
        // 正常分频模式
        sdio_clock_config(SDIO_SDIOCLKEDGE_RISING, SDIO_CLOCKBYPASS_DISABLE, 
                          SDIO_CLOCKPWRSAVE_DISABLE, clk);
    }
    sdio_bus_mode_set(gd32_bus_wide);
    sdio_hardware_clock_disable();
}
 
static void EmmcSdioCmdCfg(u32 argument, u32 cmd, u32 mode)
{
    uint32_t gd32_response_type;
    
    /* 转换响应类型 */
    if(mode == EMMC_SDIO_NO_RESPONSE)
    {
        gd32_response_type = SDIO_RESPONSETYPE_NO;
    }
    else if(mode == EMMC_SDIO_SHORT_RESPONSE)
    {
        gd32_response_type = SDIO_RESPONSETYPE_SHORT;
    }
    else if(mode == EMMC_SDIO_LONG_RESPONSE)
    {
        gd32_response_type = SDIO_RESPONSETYPE_LONG;
    }
    else
    {
        gd32_response_type = SDIO_RESPONSETYPE_NO;
    }
    
    /* 配置并发送命令 */
    sdio_command_response_config(cmd, argument, gd32_response_type);
    sdio_wait_type_set(SDIO_WAITTYPE_NO);
    sdio_csm_enable();
}
/**
  * @brief  Configures the DMA2 Channel4 for SDIO Tx request.
  * @param  BufferSRC: pointer to the source buffer
  * @param  BufferSize: buffer size
  * @retval None
  */

static void EmmcLowLevelDMATxConfig(u8 *BufferSRC, u32 BufferSize)
{
  dma_single_data_parameter_struct dma_init_struct;

  /* 清除DMA标志 */
  dma_flag_clear(DMA1, DMA_CH6, DMA_FLAG_FTF);
  dma_flag_clear(DMA1, DMA_CH6, DMA_FLAG_HTF);

  /* 禁用DMA通道 */
  dma_channel_disable(DMA1, DMA_CH6);
  dma_deinit(DMA1, DMA_CH6);

  /* DMA配置 */
  dma_init_struct.periph_addr = (uint32_t)&SDIO_FIFO;
  dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
  dma_init_struct.memory0_addr = (uint32_t)BufferSRC;
  dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
  dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_32BIT;
  dma_init_struct.circular_mode = DMA_CIRCULAR_MODE_DISABLE;
  dma_init_struct.direction = DMA_MEMORY_TO_PERIPH;
  dma_init_struct.number = BufferSize / 4;
  dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
  dma_single_data_mode_init(DMA1, DMA_CH6, &dma_init_struct);

  /* 配置DMA为外设流控制模式 */
  dma_channel_subperipheral_select(DMA1, DMA_CH6, DMA_SUBPERI4);

  /* 使能DMA中断 */
  dma_interrupt_enable(DMA1, DMA_CH6, DMA_CHXCTL_FTFIE);

  /* 使能DMA通道 */
  dma_channel_enable(DMA1, DMA_CH6);
}

/**
  * @brief  Configures the DMA2 Channel4 for SDIO Rx request.
  * @param  BufferDST: pointer to the destination buffer
  * @param  BufferSize: buffer size
  * @retval None
  */
static void EmmcLowLevelDMARxConfig(u8 *BufferDST, u32 BufferSize)
{
  dma_single_data_parameter_struct dma_init_struct;

  /* 清除DMA标志 */
  dma_flag_clear(DMA1, DMA_CH6, DMA_FLAG_FTF);
  dma_flag_clear(DMA1, DMA_CH6, DMA_FLAG_HTF);

  /* 禁用DMA通道 */
  dma_channel_disable(DMA1, DMA_CH6);
  dma_deinit(DMA1, DMA_CH6);

  /* DMA配置 */
  dma_init_struct.periph_addr = (uint32_t)&SDIO_FIFO;
  dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
  dma_init_struct.memory0_addr = (uint32_t)BufferDST;
  dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
  dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_32BIT;
  dma_init_struct.circular_mode = DMA_CIRCULAR_MODE_DISABLE;
  dma_init_struct.direction = DMA_PERIPH_TO_MEMORY;
  dma_init_struct.number = BufferSize / 4;
  dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
  dma_single_data_mode_init(DMA1, DMA_CH6, &dma_init_struct);

  /* 配置DMA为外设流控制模式 */
  dma_channel_subperipheral_select(DMA1, DMA_CH6, DMA_SUBPERI4);

  /* 使能DMA中断 */
  dma_interrupt_enable(DMA1, DMA_CH6, DMA_CHXCTL_FTFIE);

  /* 使能DMA通道 */
  dma_channel_enable(DMA1, DMA_CH6);
}

/**
  * @brief  gpio
  * @param  
  * @param
  * @retval
  */
static void EmmcGpioConfiguration(void)
{
    /* 使能GPIO时钟 */
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOC);
    rcu_periph_clock_enable(RCU_GPIOD);

    /* 配置复用功能 */
    gpio_af_set(_EMMC_D0_GPIO,  GPIO_AF_12, _EMMC_D0_GPIO_X);
    gpio_af_set(_EMMC_D1_GPIO,  GPIO_AF_12, _EMMC_D1_GPIO_X);
    gpio_af_set(_EMMC_D2_GPIO,  GPIO_AF_12, _EMMC_D2_GPIO_X);
    gpio_af_set(_EMMC_D3_GPIO,  GPIO_AF_12, _EMMC_D3_GPIO_X);
    gpio_af_set(_EMMC_D4_GPIO,  GPIO_AF_12, _EMMC_D4_GPIO_X);
    gpio_af_set(_EMMC_D5_GPIO,  GPIO_AF_12, _EMMC_D5_GPIO_X);
    gpio_af_set(_EMMC_D6_GPIO,  GPIO_AF_12, _EMMC_D6_GPIO_X);
    gpio_af_set(_EMMC_D7_GPIO,  GPIO_AF_12, _EMMC_D7_GPIO_X);
    gpio_af_set(_EMMC_CLK_GPIO, GPIO_AF_12, _EMMC_CLK_GPIO_X);
    gpio_af_set(_EMMC_CMD_GPIO, GPIO_AF_12, _EMMC_CMD_GPIO_X);

    /* 配置GPIO模式 */
    gpio_mode_set(_EMMC_D0_GPIO,  GPIO_MODE_AF, GPIO_PUPD_PULLUP, _EMMC_D0_GPIO_X);
    gpio_mode_set(_EMMC_D1_GPIO,  GPIO_MODE_AF, GPIO_PUPD_PULLUP, _EMMC_D1_GPIO_X);
    gpio_mode_set(_EMMC_D2_GPIO,  GPIO_MODE_AF, GPIO_PUPD_PULLUP, _EMMC_D2_GPIO_X);
    gpio_mode_set(_EMMC_D3_GPIO,  GPIO_MODE_AF, GPIO_PUPD_PULLUP, _EMMC_D3_GPIO_X);
    gpio_mode_set(_EMMC_D4_GPIO,  GPIO_MODE_AF, GPIO_PUPD_PULLUP, _EMMC_D4_GPIO_X);
    gpio_mode_set(_EMMC_D5_GPIO,  GPIO_MODE_AF, GPIO_PUPD_PULLUP, _EMMC_D5_GPIO_X);
    gpio_mode_set(_EMMC_D6_GPIO,  GPIO_MODE_AF, GPIO_PUPD_PULLUP, _EMMC_D6_GPIO_X);
    gpio_mode_set(_EMMC_D7_GPIO,  GPIO_MODE_AF, GPIO_PUPD_PULLUP, _EMMC_D7_GPIO_X);
    gpio_mode_set(_EMMC_CLK_GPIO, GPIO_MODE_AF, GPIO_PUPD_PULLUP, _EMMC_CLK_GPIO_X);
    gpio_mode_set(_EMMC_CMD_GPIO, GPIO_MODE_AF, GPIO_PUPD_PULLUP, _EMMC_CMD_GPIO_X);

    /* 配置输出类型和速度 */
    gpio_output_options_set(_EMMC_D0_GPIO,  GPIO_OTYPE_PP, GPIO_OSPEED_MAX, _EMMC_D0_GPIO_X);
    gpio_output_options_set(_EMMC_D1_GPIO,  GPIO_OTYPE_PP, GPIO_OSPEED_MAX, _EMMC_D1_GPIO_X);
    gpio_output_options_set(_EMMC_D2_GPIO,  GPIO_OTYPE_PP, GPIO_OSPEED_MAX, _EMMC_D2_GPIO_X);
    gpio_output_options_set(_EMMC_D3_GPIO,  GPIO_OTYPE_PP, GPIO_OSPEED_MAX, _EMMC_D3_GPIO_X);
    gpio_output_options_set(_EMMC_D4_GPIO,  GPIO_OTYPE_PP, GPIO_OSPEED_MAX, _EMMC_D4_GPIO_X);
    gpio_output_options_set(_EMMC_D5_GPIO,  GPIO_OTYPE_PP, GPIO_OSPEED_MAX, _EMMC_D5_GPIO_X);
    gpio_output_options_set(_EMMC_D6_GPIO,  GPIO_OTYPE_PP, GPIO_OSPEED_MAX, _EMMC_D6_GPIO_X);
    gpio_output_options_set(_EMMC_D7_GPIO,  GPIO_OTYPE_PP, GPIO_OSPEED_MAX, _EMMC_D7_GPIO_X);
    gpio_output_options_set(_EMMC_CLK_GPIO, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, _EMMC_CLK_GPIO_X);
    gpio_output_options_set(_EMMC_CMD_GPIO, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, _EMMC_CMD_GPIO_X);
}


/**
  * @brief  DMA IRQ process
  * @param  
  * @param
  * @retval
  */
//static void EmmcDmaIrq(void)
//{
//    if(_EMMC_SDIO_DMA->HISR & _EMMC_SDIO_DMA_FLAG_TCIF)
//    {
//        DMA_ClearFlag(_EMMC_SDIO_DMA_STREAM, _EMMC_SDIO_DMA_FLAG_TCIF /*| _EMMC_SDIO_DMA_FLAG_FEIF*/);
//        DMAEndOfTransfer = 0x01;
//    }
//    if(_EMMC_SDIO_DMA->HISR & _EMMC_SDIO_DMA_FLAG_FEIF)
//    {
//        DMA_ClearFlag(_EMMC_SDIO_DMA_STREAM, /*_EMMC_SDIO_DMA_FLAG_TCIF | */_EMMC_SDIO_DMA_FLAG_FEIF);
//		eMMCTransferError = EMMC_RX_OVERRUN;
//    }
//}
static void EmmcDmaIrq(void)
{
    if(dma_interrupt_flag_get(DMA1, DMA_CH6, DMA_INT_FLAG_FTF))
    {
        dma_interrupt_flag_clear(DMA1, DMA_CH6, DMA_INT_FLAG_FTF);
        DMAEndOfTransfer = 0x01;
    }
}

static void EmmcSdioIrq (void)
{ 
  /* 禁用所有中断 */
  sdio_interrupt_disable(SDIO_INT_DTCRCERR | SDIO_INT_DTTMOUT | SDIO_INT_DTEND |
                         SDIO_INT_TFH | SDIO_INT_RFH | SDIO_INT_TXURE |
                         SDIO_INT_RXORE | SDIO_INT_STBITE);
  
  if(sdio_interrupt_flag_get(SDIO_INT_FLAG_DTEND) != RESET)
  {
    eMMCTransferError = EMMC_OK;
    sdio_interrupt_flag_clear(SDIO_INT_FLAG_DTEND);
    eMMCTransferEnd = 1;
  }  
  else if (sdio_interrupt_flag_get(SDIO_INT_FLAG_DTCRCERR) != RESET)
  {
    //printf("CRC校验错误");
    sdio_interrupt_flag_clear(SDIO_INT_FLAG_DTCRCERR);
    eMMCTransferError = EMMC_DATA_CRC_FAIL;
  }
  else if (sdio_interrupt_flag_get(SDIO_INT_FLAG_DTTMOUT) != RESET)
  {
    //printf("超时");
    sdio_interrupt_flag_clear(SDIO_INT_FLAG_DTTMOUT);
    eMMCTransferError = EMMC_DATA_TIMEOUT;
  }
  else if (sdio_interrupt_flag_get(SDIO_INT_FLAG_RXORE) != RESET)
  {
    //printf("FIFO溢出");
    sdio_interrupt_flag_clear(SDIO_INT_FLAG_RXORE);
    eMMCTransferError = EMMC_RX_OVERRUN;
  }
  else if (sdio_interrupt_flag_get(SDIO_INT_FLAG_TXURE) != RESET)
  {
    sdio_interrupt_flag_clear(SDIO_INT_FLAG_TXURE);
    eMMCTransferError = EMMC_TX_UNDERRUN;
  }
  else if (sdio_interrupt_flag_get(SDIO_INT_FLAG_STBITE) != RESET)
  {
    //printf("sbit错误");
    sdio_interrupt_flag_clear(SDIO_INT_FLAG_STBITE);
    eMMCTransferError = EMMC_START_BIT_ERR;
  }
}
/**
    * @brief  Emmc校验块数据
    *   @param  original_buf : 原始数据缓冲区指针，作为标准对EMMC中存储的数据进行校准
    * @param  block_addr : Block起始地址
    * @param  block_num  : 待校验的数据块大小
  * @retval true:Verify is ok; false:Verify is error or parameter invalid
    */
bool EmmcVerifyBlocks(u8 *original_buf,u8 *read_buf, u32 block_addr, u32 block_num)
{
  u32 tmp_block_cnt = 0;;
  u16 tmp_byte_cnt = 0;
  u8  tmp_data_buf[512] = {0};

  if(!block_num)  /* 非法参数：block数目为0 */
  {
    return false;
  }

  for(tmp_block_cnt = 0; tmp_block_cnt < block_num; tmp_block_cnt++)
  {
    emmcreadblocks(tmp_data_buf, (block_addr + tmp_block_cnt), 1);
    for(tmp_byte_cnt = 0; tmp_byte_cnt < 512; tmp_byte_cnt++)
    {
      if(*original_buf != tmp_data_buf[tmp_byte_cnt])
      {
        return false;
      }
			*read_buf = tmp_data_buf[tmp_byte_cnt];
      original_buf++;
			read_buf++;
    }
  }
  return true;
}
bool emmcverifyreadbytes(uint8_t *buff, uint32_t addr, uint32_t size)
{
    uint32_t i = 0,pack_num = 0, byte_num = 0;    
    uint16_t first_pack_size = 0;    
    uint16_t first_pack_unuse = 0;            
    uint32_t first_pack_addr = 0;              /*第一包（512对齐）的起始地址*/
    uint16_t last_pack_size = 0;               /*最后一包大小  0代表最后一包结尾是整512字节*/
    uint32_t last_pack_addr = 0;               /*最后一包（512对齐）的起始地址, 0代表最后一包结尾是整512字节  或者size小于512字节*/
    uint16_t j = 0;
    
    if(size != 0)
    {
        first_pack_unuse = addr%EMMC_BLOCK_SIZE;
        first_pack_size  = (EMMC_BLOCK_SIZE - first_pack_unuse)>size?size:(EMMC_BLOCK_SIZE - first_pack_unuse);
        first_pack_addr = addr - first_pack_unuse;
        
        last_pack_size = (addr + size)%EMMC_BLOCK_SIZE;
        /*如果收尾均在一包且，首不完整*/
        if(((last_pack_size + first_pack_size) > size)&&(first_pack_unuse != 0))
        {
            last_pack_addr = 0;
        }
		else
		{
 		    last_pack_addr = addr + size - last_pack_size;  
		}        
        
        if((first_pack_unuse == 0)&&(last_pack_size == 0))
        {  
            pack_num = size/EMMC_BLOCK_SIZE;
            first_pack_addr = addr/EMMC_BLOCK_SIZE;
            for(i = 0; i < pack_num; i++)
            {
                emmcreadblocks(sEmmcTempBuff, first_pack_addr + i, 1);  
                for(j = 0; j < 512; j++)
                {
                    if(sEmmcTempBuff[j] != buff[byte_num])
                    {
                        return false;
                    }
                    byte_num++;
                }
            }  			
        }
        else
        {
            /*如果第一包头不完整，则特殊处理*/
			if(first_pack_unuse != 0)
		    {
                emmcreadblocks(sEmmcTempBuff, first_pack_addr/EMMC_BLOCK_SIZE, 1);
                for(j = 0; j < first_pack_size; j++)
                {
                    if(sEmmcTempBuff[j + first_pack_unuse] != buff[j])
                    {
                        return false;
                    }
                }                
		    }
			else
			{

			}
            /*如果头尾均在一块内，则中间连续0块*/
			if(last_pack_addr == 0)
			{
			    pack_num = 0;
			}
			else
			{
				if(last_pack_size == 0)
			    {					
				    pack_num = 	(size - first_pack_size)/EMMC_BLOCK_SIZE;
				}
				else
			    {
					pack_num = (last_pack_addr - first_pack_addr)/EMMC_BLOCK_SIZE;
					if(first_pack_unuse != 0)
				    { 
				        pack_num--;
			        }
			    }
				
				
			}
			if(first_pack_unuse != 0)
	        {
                byte_num = first_pack_size;
                for(i = 0; i < pack_num; i++)
                {
                    emmcreadblocks(sEmmcTempBuff, first_pack_addr/EMMC_BLOCK_SIZE + 1 + i, 1);  
                    for(j = 0; j < 512; j++)
                    {
                        if(sEmmcTempBuff[j] != buff[byte_num])
                        {
                            return false;
                        }
                        byte_num++;
                    }
                } 
		    }
			else
			{	
                byte_num = 0;
                for(i = 0; i < pack_num; i++)
                {
                    emmcreadblocks(sEmmcTempBuff, first_pack_addr/EMMC_BLOCK_SIZE + i, 1);  
                    for(j = 0; j < 512; j++)
                    {
                        if(sEmmcTempBuff[j] != buff[byte_num])
                        {
                            return false;
                        }
                        byte_num++;
                    }
                } 
			}
            
			
			/*数据头和尾在同一个512块里面，且头不是整512字节之前已经处理过了*/
			if((first_pack_size == size)&&(first_pack_unuse != 0))
			{
				
			}
			else
		    {
				/*尾是整512字节的，连续块写中，也处理了*/
				if(last_pack_size != 0)
			    {
                    byte_num = size - last_pack_size;
                    emmcreadblocks(sEmmcTempBuff, last_pack_addr/EMMC_BLOCK_SIZE, 1);  
                    for(j = 0; j < last_pack_size; j++)
                    {
                        if(sEmmcTempBuff[j] != buff[byte_num])
                        {
                            return false;
                        }
                        byte_num++;
                    }
		        }
			}
			

        }
        return true;        
    }    
    
    
    return false; 
    

}




/**
  * @brief  是否正在传输，如果上一个传输完成，发送停止命令(由于前面的EMMC驱动在读取时未发送读取块数的命令)
  * @param  None
  * @retval EmmcError: eMMC错误码.
  */
bool emmcistransing(void)
{
    if((SDIO_STAT & SDIO_FLAG_TXRUN) || (SDIO_STAT & SDIO_FLAG_RXRUN))
    {
        return true;
    }

	return false;
}

void emmcstoptransfer(void)
{
    u32 i = 0,time_out = 0x1FFFFFFF;
    EmmcError errorstatus = EMMC_OK;
    uint8_t cardstate = 0x0F;
    
    EmmcCMDStopTransfer(); /* 发送停止命令 */
    
    /* 这里就不用清除状态标志了，因为EMMC每次的数据传输均会重新初始化一次DMA */
    while(time_out--)
    {
        if((DMA_CHCTL(DMA1, DMA_CH6) & DMA_CHXCTL_CHEN) == 0)
        {
            break;
        }
        dma_channel_disable(DMA1, DMA_CH6);
    }
    if(time_out == 0)
    {
        return ;
    }
    dma_deinit(DMA1, DMA_CH6);
    
    /* 禁用数据传输 */
    sdio_dsm_disable();
    
    errorstatus = IsCardProgramming(&cardstate);
	while ((errorstatus == EMMC_OK) && ((EMMC_CARD_PROGRAMMING == cardstate) || (EMMC_CARD_RECEIVING == cardstate)))
    {
        errorstatus = IsCardProgramming(&cardstate);
        for(i=0; i<10000; i++);
    }
}

/* IRQ function ---------------------------------------------------------*/

void DMA1_Channel6_IRQHandler(void)	
{
    EmmcDmaIrq();
}

void SDIO_IRQHandler(void)
{
    EmmcSdioIrq();
}



