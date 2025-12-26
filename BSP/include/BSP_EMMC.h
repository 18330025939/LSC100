
#define __DEV_EMMC_H_	

#include "gd32f4xx.h"
#include "stdbool.h"

typedef uint32_t  u32;
typedef uint16_t u16;
typedef uint8_t  u8;
typedef __IO uint8_t  vu8;
typedef __IO uint32_t vu32;


/* GD32F4xx DMA宏定义 (注意: GD32使用DMA1 CH6用于SDIO) */
#define _EMMC_SDIO_DMA                   DMA1
#define _EMMC_SDIO_DMA_CLK               RCU_DMA1
#define _EMMC_SDIO_DMA_CHANNEL           DMA_CH6
#define _EMMC_SDIO_DMA_FLAG_FTF          DMA_FLAG_FTF
#define _EMMC_SDIO_DMA_FLAG_HTF          DMA_FLAG_HTF
#define _EMMC_SDIO_DMA_FLAG_ERR          DMA_FLAG_ERR 

#ifndef USE_SIMPLE_EMMC	 
	#define USE_SIMPLE_EMMC
#endif	 
/** 
  * @brief  SDIO Transfer state  
  */   
typedef enum
{
  EMMC_TRANSFER_OK  = 0,
  EMMC_TRANSFER_BUSY = 1,
  EMMC_TRANSFER_ERROR
} EmmcTransferState;

/** 
  * @brief  SD Card States 
  */   
typedef enum
{
  EMMC_CARD_READY                  = ((uint32_t)0x00000001),
  EMMC_CARD_IDENTIFICATION         = ((uint32_t)0x00000002),
  EMMC_CARD_STANDBY                = ((uint32_t)0x00000003),
  EMMC_CARD_TRANSFER               = ((uint32_t)0x00000004),
  EMMC_CARD_SENDING                = ((uint32_t)0x00000005),
  EMMC_CARD_RECEIVING              = ((uint32_t)0x00000006),
  EMMC_CARD_PROGRAMMING            = ((uint32_t)0x00000007),
  EMMC_CARD_DISCONNECTED           = ((uint32_t)0x00000008),
  EMMC_CARD_ERROR                  = ((uint32_t)0x000000FF)
}EmmcCardState;


/** 
  * @brief  Card Specific Data: CSD Register   
  */ 
typedef struct
{
  volatile uint8_t  CSDStruct;            /*!< CSD structure */
  volatile uint8_t  SysSpecVersion;       /*!< System specification version */
  volatile uint8_t  Reserved1;            /*!< Reserved */
  volatile uint8_t  TAAC;                 /*!< Data read access-time 1 */
  volatile uint8_t  NSAC;                 /*!< Data read access-time 2 in CLK cycles */
  volatile uint8_t  MaxBusClkFrec;        /*!< Max. bus clock frequency */
  volatile uint16_t CardComdClasses;      /*!< Card command classes */
  volatile uint8_t  RdBlockLen;           /*!< Max. read data block length */
  volatile uint8_t  PartBlockRead;        /*!< Partial blocks for read allowed */
  volatile uint8_t  WrBlockMisalign;      /*!< Write block misalignment */
  volatile uint8_t  RdBlockMisalign;      /*!< Read block misalignment */
  volatile uint8_t  DSRImpl;              /*!< DSR implemented */
  volatile uint8_t  Reserved2;            /*!< Reserved */
  volatile uint32_t DeviceSize;           /*!< Device Size */
  volatile uint8_t  MaxRdCurrentVDDMin;   /*!< Max. read current @ VDD min */
  volatile uint8_t  MaxRdCurrentVDDMax;   /*!< Max. read current @ VDD max */
  volatile uint8_t  MaxWrCurrentVDDMin;   /*!< Max. write current @ VDD min */
  volatile uint8_t  MaxWrCurrentVDDMax;   /*!< Max. write current @ VDD max */
  volatile uint8_t  DeviceSizeMul;        /*!< Device size multiplier */
  volatile uint8_t  EraseGrSize;          /*!< Erase group size */
  volatile uint8_t  EraseGrMul;           /*!< Erase group size multiplier */
  volatile uint8_t  WrProtectGrSize;      /*!< Write protect group size */
  volatile uint8_t  WrProtectGrEnable;    /*!< Write protect group enable */
  volatile uint8_t  ManDeflECC;           /*!< Manufacturer default ECC */
  volatile uint8_t  WrSpeedFact;          /*!< Write speed factor */
  volatile uint8_t  MaxWrBlockLen;        /*!< Max. write data block length */
  volatile uint8_t  WriteBlockPaPartial;  /*!< Partial blocks for write allowed */
  volatile uint8_t  Reserved3;            /*!< Reserded */
  volatile uint8_t  ContentProtectAppli;  /*!< Content protection application */
  volatile uint8_t  FileFormatGrouop;     /*!< File format group */
  volatile uint8_t  CopyFlag;             /*!< Copy flag (OTP) */
  volatile uint8_t  PermWrProtect;        /*!< Permanent write protection */
  volatile uint8_t  TempWrProtect;        /*!< Temporary write protection */
  volatile uint8_t  FileFormat;           /*!< File Format */
  volatile uint8_t  ECC;                  /*!< ECC code */
  volatile uint8_t  CSD_CRC;              /*!< CSD CRC */
  volatile uint8_t  Reserved4;            /*!< always 1*/
} EMMC_CSD;


/** 
  * @brief  Card Specific Data: EXTCSD Register   
  */ 
typedef union
{
		struct _EXT_CSD
		{
			volatile uint8_t  Reserved26[32];      		
			volatile uint8_t 	FLUSH_CACHE;	
			volatile uint8_t 	CACHE_CTRL; 	
			volatile uint8_t 	POWER_OFF_NOTIFICATION;	
			volatile uint8_t 	PACKED_FAILURE_INDEX;	
			volatile uint8_t 	PACKED_COMMAND_STATUS;	
			volatile uint8_t 	CONTEXT_CONF[15]; 	
			volatile uint8_t 	EXT_PARTITIONS_ATTRIBUTE[2];	
			volatile uint8_t 	EXCEPTION_EVENTS_STATUS[2];	
			volatile uint8_t 	EXCEPTION_EVENTS_CTRL[2];	
			volatile uint8_t 	DYNCAP_NEEDED; 	
			volatile uint8_t 	CLASS_6_CTRL; 	
			volatile uint8_t 	INI_TIMEOUT_EMU; 	
			volatile uint8_t 	DATA_SECTOR_SIZE; 	
			volatile uint8_t 	USE_NATIVE_SECTOR; 	
			volatile uint8_t 	NATIVE_SECTOR_SIZE;	
			volatile uint8_t 	VENDOR_SPECIFIC_FIELD[64];	
			volatile uint8_t  Reserved25;		
			volatile uint8_t 	PROGRAM_CID_CSD_DDR_SUPPORT;	
			volatile uint8_t 	PERIODIC_WAKEUP; 	
			volatile uint8_t 	TCASE_SUPPORT; 	
			volatile uint8_t  Reserved24;		
			volatile uint8_t 	SEC_BAD_BLK_MGMNT;	
			volatile uint8_t  Reserved23;		
			volatile uint8_t 	ENH_START_ADDR[4];	
			volatile uint8_t 	ENH_SIZE_MULT[3]; 	
			volatile uint8_t 	GP_SIZE_MULT[12]; 	
			volatile uint8_t 	PARTITION_SETTING_COMPLETED;	
			volatile uint8_t 	PARTITIONS_ATTRIBUTE; 	
			volatile uint8_t 	MAX_ENH_SIZE_MULT[3]; 	
			volatile uint8_t 	PARTITIONING_SUPPORT; 	
			volatile uint8_t 	HPI_MGMT; 	
			volatile uint8_t 	RST_n_FUNCTION; 	
			volatile uint8_t 	BKOPS_EN; 	
			volatile uint8_t 	BKOPS_START; 	
			volatile uint8_t 	SANITIZE_START; 	
			volatile uint8_t 	WR_REL_PARAM; 	
			volatile uint8_t 	WR_REL_SET; 	
			volatile uint8_t 	RPMB_SIZE_MULT; 	
			volatile uint8_t 	FW_CONFIG; 	
			volatile uint8_t  Reserved22;		
			volatile uint8_t 	USER_WP;	
			volatile uint8_t  Reserved21;		
			volatile uint8_t 	BOOT_WP;	
			volatile uint8_t 	BOOT_WP_STATUS; 	
			volatile uint8_t 	ERASE_GROUP_DEF; 	
			volatile uint8_t  Reserved20;		
			volatile uint8_t  BOOT_BUS_CONDITIONS;		
			volatile uint8_t  BOOT_CONFIG_PROT; 		
			volatile uint8_t 	PARTITION_CONFIG; 	
			volatile uint8_t  Reserved19;		
			volatile uint8_t  ERASED_MEM_CONT;		
			volatile uint8_t  Reserved18;		
			volatile uint8_t  BUS_WIDTH;		
			volatile uint8_t  Reserved17;		
			volatile uint8_t  HS_TIMING;		
			volatile uint8_t  Reserved16;		
			volatile uint8_t  POWER_CLASS;		
			volatile uint8_t  Reserved15;		
			volatile uint8_t  CMD_SET_REV;		
			volatile uint8_t  Reserved14;		
			volatile uint8_t  CMD_SET;		
			volatile uint8_t  Reserved13;		
			volatile uint8_t  EXT_CSD_REV ;		
			volatile uint8_t  Reserved12;		
			volatile uint8_t  Reserved11;		
			volatile uint8_t  Reserved10;		
			volatile uint8_t  DEVICE_TYPE;		
			volatile uint8_t  DRIVER_STRENGTH; 		
			volatile uint8_t  OUT_OF_INTERRUPT_TIME;		
			volatile uint8_t  PARTITION_SWITCH_TIME;		
			volatile uint8_t  PWR_CL_52_195; 		
			volatile uint8_t  PWR_CL_26_195; 		
			volatile uint8_t  PWR_CL_52_360; 		
			volatile uint8_t  PWR_CL_26_360; 		
			volatile uint8_t  Reserved9;		
			volatile uint8_t  MIN_PERF_R_4_26;		
			volatile uint8_t  MIN_PERF_W_4_26; 		
			volatile uint8_t  MIN_PERF_R_8_26_4_52; 		
			volatile uint8_t  MIN_PERF_W_8_26_4_52; 		
			volatile uint8_t  MIN_PERF_R_8_52; 		
			volatile uint8_t  MIN_PERF_W_8_52; 		
			volatile uint8_t  Reserved8;		
			volatile uint8_t  SEC_COUNT[4];		
			volatile uint8_t  Reserved7;		
			volatile uint8_t  S_A_TIMEOUT;		
			volatile uint8_t  Reserved6;		
			volatile uint8_t  S_C_VCCQ; 		
			volatile uint8_t  S_C_VCC; 		
			volatile uint8_t  HC_WP_GRP_SIZE; 		
			volatile uint8_t  REL_WR_SEC_C; 		
			volatile uint8_t  ERASE_TIMEOUT_MULT; 		
			volatile uint8_t  HC_ERASE_GRP_SIZE; 		
			volatile uint8_t  ACC_SIZE; 		
			volatile uint8_t  BOOT_SIZE_MULTI; 		
			volatile uint8_t  Reserved5;		
			volatile uint8_t  BOOT_INFO; 		
			volatile uint8_t  obsolete2;		
			volatile uint8_t  obsolete1;		
			volatile uint8_t  SEC_FEATURE_SUPPORT; 		
			volatile uint8_t  TRIM_MULT; 		
			volatile uint8_t  Reserved4;		
			volatile uint8_t  MIN_PERF_DDR_R_8_52;		
			volatile uint8_t  MIN_PERF_DDR_W_8_52; 		
			volatile uint8_t  PWR_CL_200_195; 		
			volatile uint8_t  PWR_CL_200_360; 		
			volatile uint8_t  PWR_CL_DDR_52_195; 		
			volatile uint8_t  PWR_CL_DDR_52_360; 		
			volatile uint8_t  Reserved3;		
			volatile uint8_t  INI_TIMEOUT_AP;		
			volatile uint8_t  CORRECTLY_PRG_SECTORS_NUM[4];		
			volatile uint8_t  BKOPS_STATUS[2];		
			volatile uint8_t  POWER_OFF_LONG_TIME;		
			volatile uint8_t  GENERIC_CMD6_TIME; 		
			volatile uint8_t  CACHE_SIZE[4]; 		
			volatile uint8_t  Reserved2[255];		
			volatile uint8_t  EXT_SUPPORT;		
			volatile uint8_t  LARGE_UNIT_SIZE_M1; 		
			volatile uint8_t  CONTEXT_CAPABILITIES; 		
			volatile uint8_t  TAG_RES_SIZE; 		
			volatile uint8_t  TAG_UNIT_SIZE; 		
			volatile uint8_t  DATA_TAG_SUPPORT; 		
			volatile uint8_t  MAX_PACKED_WRITES; 		
			volatile uint8_t  MAX_PACKED_READS; 		
			volatile uint8_t  BKOPS_SUPPORT;		
			volatile uint8_t  HPI_FEATURES; 		
			volatile uint8_t  S_CMD_SET;		
			volatile uint8_t  Reserved1[7];	         
		} EXT_CSD;
   volatile uint8_t CsdBuf[512];
} EMMCEXT_CSD;





/** 
  * @brief  eMMC Identification Data: CID Register   
  */
typedef struct
{
  volatile uint8_t  ManufacturerID;       /*!< ManufacturerID */
  volatile uint8_t  Reserved1;            /*!< Reserved1 */
  volatile uint8_t  CBX;            			/*!< Chip Package */
  volatile uint8_t  OEM_AppliID;          /*!< OEM/Application ID */
  volatile uint32_t ProdName1;            /*!< Product Name part1 */
  volatile uint16_t ProdName2;            /*!< Product Name part2*/
  volatile uint8_t  ProdRev;              /*!< Product Revision */
  volatile uint32_t ProdSN;               /*!< Product Serial Number */
  volatile uint8_t  ManufactDate;         /*!< Manufacturing Date */
  volatile uint8_t  CID_CRC;              /*!< CID CRC */
  volatile uint8_t  Reserved2;            /*!< always 1 */
} EMMC_CID;

/** 
  * @brief SD Card Status 
  */
typedef struct
{
  volatile uint8_t DAT_BUS_WIDTH;
  volatile uint8_t SECURED_MODE;
  volatile uint16_t SD_CARD_TYPE;
  volatile uint32_t SIZE_OF_PROTECTED_AREA;
  volatile uint8_t SPEED_CLASS;
  volatile uint8_t PERFORMANCE_MOVE;
  volatile uint8_t AU_SIZE;
  volatile uint16_t ERASE_SIZE;
  volatile uint8_t ERASE_TIMEOUT;
  volatile uint8_t ERASE_OFFSET;
} EmmcCardStatus;


/** 
  * @brief SD Card information 
  */
typedef struct
{

  uint32_t eMMC_Capacity;  /*!< Card Capacity */
  uint32_t eMMC_BlockSize; /*!< Card Block Size */
  uint16_t RCA;
  uint8_t eMMC_Type;
	volatile uint16_t ExtCardComdClasses;	
    uint8_t init_ret;

}EmmcCardInfo;

   
#define 	EMMC_BLOCK_SIZE    (512)
 

/** 
  * @brief SDIO Commands  Index 
  */
#define EMMC_CMD_GO_IDLE_STATE                       ((uint8_t)0)
#define EMMC_CMD_SEND_OP_COND                        ((uint8_t)1)
#define EMMC_CMD_ALL_SEND_CID                        ((uint8_t)2)
#define EMMC_CMD_SET_REL_ADDR                        ((uint8_t)3) /*!< SDIO_SEND_REL_ADDR for SD Card */
#define EMMC_CMD_SET_DSR                             ((uint8_t)4)
#define EMMC_CMD_SDIO_SEN_OP_COND                    ((uint8_t)5)
#define EMMC_CMD_HS_SWITCH                           ((uint8_t)6)
#define EMMC_CMD_SEL_DESEL_CARD                      ((uint8_t)7)
#define EMMC_CMD_HS_SEND_EXT_CSD                     ((uint8_t)8)
#define EMMC_CMD_SEND_CSD                            ((uint8_t)9)
#define EMMC_CMD_SEND_CID                            ((uint8_t)10)
#define EMMC_CMD_READ_DAT_UNTIL_STOP                 ((uint8_t)11) /*!< SD Card doesn't support it */
#define EMMC_CMD_STOP_TRANSMISSION                   ((uint8_t)12)
#define EMMC_CMD_SEND_STATUS                         ((uint8_t)13)
#define EMMC_CMD_HS_BUSTEST_READ                     ((uint8_t)14)
#define EMMC_CMD_GO_INACTIVE_STATE                   ((uint8_t)15)
#define EMMC_CMD_SET_BLOCKLEN                        ((uint8_t)16)
#define EMMC_CMD_READ_SINGLE_BLOCK                   ((uint8_t)17)
#define EMMC_CMD_READ_MULT_BLOCK                     ((uint8_t)18)
#define EMMC_CMD_HS_BUSTEST_WRITE                    ((uint8_t)19)
#define EMMC_CMD_WRITE_DAT_UNTIL_STOP                ((uint8_t)20) /*!< SD Card doesn't support it */
#define EMMC_CMD_SET_BLOCK_COUNT                     ((uint8_t)23) /*!< SD Card doesn't support it */
#define EMMC_CMD_WRITE_SINGLE_BLOCK                  ((uint8_t)24)
#define EMMC_CMD_WRITE_MULT_BLOCK                    ((uint8_t)25)
#define EMMC_CMD_PROG_CID                            ((uint8_t)26) /*!< reserved for manufacturers */
#define EMMC_CMD_PROG_CSD                            ((uint8_t)27)
#define EMMC_CMD_SET_WRITE_PROT                      ((uint8_t)28)
#define EMMC_CMD_CLR_WRITE_PROT                      ((uint8_t)29)
#define EMMC_CMD_SEND_WRITE_PROT                     ((uint8_t)30)
#define EMMC_CMD_ERASE_GRP_START                     ((uint8_t)35) /*!< To set the address of the first write block to be erased.
                                                                  (For MMC card only spec 3.31) */

#define EMMC_CMD_ERASE_GRP_END                       ((uint8_t)36) /*!< To set the address of the last write block of the
                                                                  continuous range to be erased. (For MMC card only spec 3.31) */

#define EMMC_CMD_ERASE                               ((uint8_t)38)
#define EMMC_CMD_FAST_IO                             ((uint8_t)39) /*!< SD Card doesn't support it */
#define EMMC_CMD_GO_IRQ_STATE                        ((uint8_t)40) /*!< SD Card doesn't support it */
#define EMMC_CMD_LOCK_UNLOCK                         ((uint8_t)42)
#define EMMC_CMD_APP_CMD                             ((uint8_t)55)
#define EMMC_CMD_GEN_CMD                             ((uint8_t)56)
#define EMMC_CMD_NO_CMD                              ((uint8_t)64)

#if !defined (SD_DMA_MODE) && !defined (SD_POLLING_MODE)
#define SD_DMA_MODE                                ((uint32_t)0x00000000)

#endif



#define NULL 0
#define SDIO_STATIC_FLAGS               ((uint32_t)0x000005FF)
#define SDIO_CMD0TIMEOUT                ((uint32_t)0x01000000)

/** 
  * @brief  Mask for errors Card Status R1 (OCR Register) 
  */
#define EMMC_OCR_ADDR_OUT_OF_RANGE        ((uint32_t)0x80000000)
#define EMMC_OCR_ADDR_MISALIGNED          ((uint32_t)0x40000000)
#define EMMC_OCR_BLOCK_LEN_ERR            ((uint32_t)0x20000000)
#define EMMC_OCR_ERASE_SEQ_ERR            ((uint32_t)0x10000000)
#define EMMC_OCR_BAD_ERASE_PARAM          ((uint32_t)0x08000000)
#define EMMC_OCR_WRITE_PROT_VIOLATION     ((uint32_t)0x04000000)
#define EMMC_OCR_LOCK_UNLOCK_FAILED       ((uint32_t)0x01000000)
#define EMMC_OCR_COM_CRC_FAILED           ((uint32_t)0x00800000)
#define EMMC_OCR_ILLEGAL_CMD              ((uint32_t)0x00400000)
#define EMMC_OCR_CARD_ECC_FAILED          ((uint32_t)0x00200000)
#define EMMC_OCR_CC_ERROR                 ((uint32_t)0x00100000)
#define EMMC_OCR_GENERAL_UNKNOWN_ERROR    ((uint32_t)0x00080000)
#define EMMC_OCR_STREAM_READ_UNDERRUN     ((uint32_t)0x00040000)
#define EMMC_OCR_STREAM_WRITE_OVERRUN     ((uint32_t)0x00020000)
#define EMMC_OCR_CID_CSD_OVERWRIETE       ((uint32_t)0x00010000)
#define HOCR_WP_ERASE_SKIP                ((uint32_t)0x00008000)
#define EMMC_OCR_CARD_ECC_DISABLED        ((uint32_t)0x00004000)
#define EMMC_OCR_ERASE_RESET              ((uint32_t)0x00002000)
#define EMMC_OCR_AKE_SEQ_ERROR            ((uint32_t)0x00000008)
#define EMMC_OCR_ERRORBITS                ((uint32_t)0xFDFFE008)

/** 
  * @brief  Masks for R6 Response 
  */
#define EMMC_R6_GENERAL_UNKNOWN_ERROR     ((uint32_t)0x00002000)
#define EMMC_R6_ILLEGAL_CMD               ((uint32_t)0x00004000)
#define EMMC_R6_COM_CRC_FAILED            ((uint32_t)0x00008000)

#define EMMC_VOLTAGE_WINDOW_SD            ((uint32_t)0x80100000)
#define EMMC_HIGH_CAPACITY                ((uint32_t)0x40000000)
#define EMMC_STD_CAPACITY                 ((uint32_t)0x00000000)
#define EMMC_CHECK_PATTERN                ((uint32_t)0x000001AA)

#define EMMC_MAX_VOLT_TRIAL               ((uint32_t)0x0000FFFF)
#define EMMC_ALLZERO                      ((uint32_t)0x00000000)

#define EMMC_WIDE_BUS_SUPPORT             ((uint32_t)0x00040000)
#define EMMC_SINGLE_BUS_SUPPORT           ((uint32_t)0x00010000)
#define EMMC_CARD_LOCKED                  ((uint32_t)0x02000000)

#define EMMC_DATATIMEOUT                  ((uint32_t)0xFFFFFFFF)
#define EMMC_0TO7BITS                     ((uint32_t)0x000000FF)
#define EMMC_8TO15BITS                    ((uint32_t)0x0000FF00)
#define EMMC_16TO23BITS                   ((uint32_t)0x00FF0000)
#define EMMC_24TO31BITS                   ((uint32_t)0xFF000000)
#define EMMC_MAX_DATA_LENGTH              ((uint32_t)0x01FFFFFF)

#define EMMC_HALFFIFO                     ((uint32_t)0x00000008)
#define EMMC_HALFFIFOBYTES                ((uint32_t)0x00000020)

/** 
  * @brief  Command Class Supported 
  */
#define EMMC_CCCC_LOCK_UNLOCK             ((uint32_t)0x00000080)
#define EMMC_CCCC_WRITE_PROT              ((uint32_t)0x00000040)
#define EMMC_CCCC_ERASE                   ((uint32_t)0x00000020)

 
/**
  * @}
  */ 

#define EMMC_OCR_REG             0x40FF8080
#define EMMC_POWER_REG           0x03BB0800
#define EMMC_HIGHSPEED_REG       0x03B90100
#define EMMC_4BIT_REG            0x03B70100
#define EMMC_8BIT_REG            0x03B70200

/* Private define ------------------------------------------------------------*/
/** 
  * @brief Supported SD Memory Cards 
  */
#define _EMMC_STD_CAPACITY_SD_CARD_V1_1             ((uint32_t)0x00000000)
#define _EMMC_STD_CAPACITY_SD_CARD_V2_0             ((uint32_t)0x00000001)
#define _EMMC_HIGH_CAPACITY_SD_CARD                 ((uint32_t)0x00000002)
#define _EMMC_MULTIMEDIA_CARD                       ((uint32_t)0x00000003)
#define _EMMC_SECURE_DIGITAL_IO_CARD                ((uint32_t)0x00000004)
#define _EMMC_HIGH_SPEED_MULTIMEDIA_CARD            ((uint32_t)0x00000005)
#define _EMMC_SECURE_DIGITAL_IO_COMBO_CARD          ((uint32_t)0x00000006)
#define _EMMC_HIGH_CAPACITY_MMC_CARD                ((uint32_t)0x00000007)

/** 
  * @brief  SDIO Intialization Frequency 
  */
//400KHz max
#define EMMC_SDIO_INIT_CLK_DIV               ((uint8_t)0x76)

//48MHz max (时钟旁路模式，最高频率)
#define EMMC_SDIO_TRANSFER_CLK_DIV           ((uint8_t)0xFF) 
/** 
  * @brief  SDIO width
  */
#define EMMC_SDIO_1BIT_BUS                  (1)
#define EMMC_SDIO_4BIT_BUS                  (2)
#define EMMC_SDIO_8BIT_BUS                  (3)
/** 
  * @brief  SDIO response
  */
#define EMMC_SDIO_NO_RESPONSE               (1)
#define EMMC_SDIO_SHORT_RESPONSE            (2)
#define EMMC_SDIO_LONG_RESPONSE             (3)

/* -S- configable define which is related to Hardware (GD32F4xx) */
#define _EMMC_D0_GPIO          GPIOC
#define _EMMC_D0_GPIO_X        GPIO_PIN_8

#define _EMMC_D1_GPIO          GPIOC
#define _EMMC_D1_GPIO_X        GPIO_PIN_9

#define _EMMC_D2_GPIO          GPIOC
#define _EMMC_D2_GPIO_X        GPIO_PIN_10

#define _EMMC_D3_GPIO          GPIOC
#define _EMMC_D3_GPIO_X        GPIO_PIN_11

#define _EMMC_D4_GPIO          GPIOB
#define _EMMC_D4_GPIO_X        GPIO_PIN_8

#define _EMMC_D5_GPIO          GPIOB
#define _EMMC_D5_GPIO_X        GPIO_PIN_9

#define _EMMC_D6_GPIO          GPIOC
#define _EMMC_D6_GPIO_X        GPIO_PIN_6

#define _EMMC_D7_GPIO          GPIOC
#define _EMMC_D7_GPIO_X        GPIO_PIN_7

#define _EMMC_CLK_GPIO         GPIOD
#define _EMMC_CLK_GPIO_X       GPIO_PIN_2

#define _EMMC_CMD_GPIO         GPIOC
#define _EMMC_CMD_GPIO_X       GPIO_PIN_12

/* GD32F4xx DMA定义 */
#define _EMMC_SDIO_DMA                   DMA1
#define _EMMC_SDIO_DMA_CLK               RCU_DMA1
#define _EMMC_SDIO_DMA_CHANNEL           DMA_CH6
#define _EMMC_SDIO_DMA_FLAG_FTF          DMA_FLAG_FTF
#define _EMMC_SDIO_DMA_FLAG_HTF          DMA_FLAG_HTF
#define _EMMC_SDIO_DMA_FLAG_ERR          DMA_FLAG_ERR
#define _EMMC_SDIO_DMA_IRQn              DMA1_Channel6_IRQn
#define _EMMC_SDIO_DMA_IRQHANDLER        DMA1_Channel6_IRQHandler

#define 	EmmcTimeOut     0x05
#define 	EmmcReset 	GPIO_WriteBit(GPIOG, GPIO_Pin_9, Bit_RESET); 	
#define 	EmmcSet     GPIO_WriteBit(GPIOG, GPIO_Pin_9, Bit_SET); 	 

/* -E- Congigable define which is related to Hardware */

/** 
  * @brief  SDIO specific error defines  
  */   
typedef enum
{
 
  EMMC_CMD_CRC_FAIL                    = (1), /*!< Command response received (but CRC check failed) */
  EMMC_DATA_CRC_FAIL                   = (2), /*!< Data bock sent/received (CRC check Failed) */
  EMMC_CMD_RSP_TIMEOUT                 = (3), /*!< Command response timeout */
  EMMC_DATA_TIMEOUT                    = (4), /*!< Data time out */
  EMMC_TX_UNDERRUN                     = (5), /*!< Transmit FIFO under-run */
  EMMC_RX_OVERRUN                      = (6), /*!< Receive FIFO over-run */
  EMMC_START_BIT_ERR                   = (7), /*!< Start bit not detected on all data signals in widE bus mode */
  EMMC_CMD_OUT_OF_RANGE                = (8), /*!< CMD's argument was out of range.*/
  EMMC_ADDR_MISALIGNED                 = (9), /*!< Misaligned address */
  EMMC_BLOCK_LEN_ERR                   = (10), /*!< Transferred block length is not allowed for the card or the number of transferred bytes does not match the block length */
  EMMC_ERASE_SEQ_ERR                   = (11), /*!< An error in the sequence of erase command occurs.*/
  EMMC_BAD_ERASE_PARAM                 = (12), /*!< An Invalid selection for erase groups */
  EMMC_WRITE_PROT_VIOLATION            = (13), /*!< Attempt to program a write protect block */
  EMMC_LOCK_UNLOCK_FAILED              = (14), /*!< Sequence or password error has been detected in unlock command or if there was an attempt to access a locked card */
  EMMC_COM_CRC_FAILED                  = (15), /*!< CRC check of the previous command failed */
  EMMC_ILLEGAL_CMD                     = (16), /*!< Command is not legal for the card state */
  EMMC_CARD_ECC_FAILED                 = (17), /*!< Card internal ECC was applied but failed to correct the data */
  EMMC_CC_ERROR                        = (18), /*!< Internal card controller error */
  EMMC_GENERAL_UNKNOWN_ERROR           = (19), /*!< General or Unknown error */
  EMMC_STREAM_READ_UNDERRUN            = (20), /*!< The card could not sustain data transfer in stream read operation. */
  EMMC_STREAM_WRITE_OVERRUN            = (21), /*!< The card could not sustain data programming in stream mode */
  EMMC_CID_CEMMC_OVERWRITE             = (22), /*!< CID/CSD overwrite error */
  EMMC_WP_ERASE_SKIP                   = (23), /*!< only partial address space was erased */
  EMMC_CARD_ECC_DISABLED               = (24), /*!< Command has been executed without using internal ECC */
  EMMC_ERASE_RESET                     = (25), /*!< Erase sequence was cleared before executing because an out of erase sequence command was received */
  EMMC_AKE_SEQ_ERROR                   = (26), /*!< Error in sequence of authentication. */
  EMMC_INVALID_VOLTRANGE               = (27),
  EMMC_ADDR_OUT_OF_RANGE               = (28),
  EMMC_SWITCH_ERROR                    = (29),
  EMMC_SDIO_DISABLED                   = (30),
  EMMC_SDIO_FUNCTION_BUSY              = (31),
  EMMC_SDIO_FUNCTION_FAILED            = (32),
  EMMC_SDIO_UNKNOWN_FUNCTION           = (33),

/** 
  * @brief  Standard error defines   
  */ 
  EMMC_INTERNAL_ERROR, 
  EMMC_NOT_CONFIGURED,
  EMMC_REQUEST_PENDING, 
  EMMC_REQUEST_NOT_APPLICABLE, 
  EMMC_INVALID_PARAMETER,  
  EMMC_UNSUPPORTED_FEATURE,  
  EMMC_UNSUPPORTED_HW,  
  EMMC_ERROR,  
  EMMC_OK = 0 
} EmmcError;

/** @defgroup SDIO_EMMC_Exported_Functions
  * @{
  */ 
bool emmcdriveinit(void);
bool writebyetstoemmc(uint8_t *data, uint32_t addr, uint32_t size);
bool readbyetsfromemmc(uint8_t *buff, uint32_t addr, uint32_t size);
bool emmcreadblocks(volatile u8 *readbuff, u32 ReadAddr, u32 NumberOfBlocks);
bool emmcerasewriteblocks(volatile u8 *writebuff, u32 WriteAddr, u32 NumberOfBlocks);
EmmcError emmc_erasemultiblock(uint32_t startaddr,uint32_t endaddr);
u8 erasebyetstoemmc(u32 addr, u32 size);
u8 readbyetsfromemmcnowait(u8 *buff, u32 addr, u32 size);
uint8_t emmc_test(void);
bool emmcverifyreadbytes(uint8_t *buff, uint32_t addr, uint32_t size);
void emmcstoptransfer(void);
bool emmcistransing(void);
uint32_t emmcgetcapacity(void);
bool EmmcEraseWriteBlocks(vu8 *writebuff, u32 WriteAddr, u32 NumberOfBlocks);

bool EmmcVerifyBlocks(u8 *original_buf,u8 *read_buf, u32 block_addr, u32 block_num);
void drv_bsp_config	  (void);
void nvicconfiguration(uint32_t offset_vectortable);

void 	 systick_drv_config(void);



