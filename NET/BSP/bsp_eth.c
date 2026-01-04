/*!
    \file  gd32f4xx_enet_eval.c
    \brief ethernet hardware configuration

    \version 2022-04-18, V2.0.0, demo for GD32F4xx
*/

/*
    Copyright (c) 2022, GigaDevice Semiconductor Inc.

    Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors
       may be used to endorse or promote products derived from this software without
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
*/

#include "gd32f4xx_enet.h"
#include "bsp_eth.h"
#include "systick.h"

#include "rtt_log.h"
static __IO uint32_t enet_init_status = 0;

static void enet_gpio_config(void);
static void enet_mac_dma_config(void);

// static void phy_delay_us(uint32_t count)
// {
//     volatile uint32_t i, j;
//     for (i = 0; i < count; i++) {

//         for (j = 0; j < 40; j++) {  
//             __NOP();
//         }
//     }
// }

#define phy_delay_us(us) delay_us_nop(us)

void phy_rst(void)
{
	PHY_RST_L;
	phy_delay_us(500*1000);
	PHY_RST_H;
	phy_delay_us(100*1000);
	PHY_RST_L;
	phy_delay_us(500*1000);
	PHY_RST_H;
	phy_delay_us(100*1000);

}
/*!
    \brief      setup ethernet system(GPIOs, clocks, MAC, DMA, systick)
    \param[in]  none
    \param[out] none
    \retval     none
*/

void enet_system_setup(void)
{
	APP_PRINTF("\r\n====== Ethernet hardware initialization begins ======\r\n");
	
	/* configure the GPIO ports for ethernet pins */
	enet_gpio_config();
	APP_PRINTF("[1/5] GPIO pin configuration completed.\r\n");
	
	phy_rst();
	APP_PRINTF("[2/5] PHY chip reset completed.\r\n");
	
	/* configure the ethernet MAC/DMA */
	enet_mac_dma_config();
	APP_PRINTF("[3/5] MAC/DMA configuration completed\r\n");

	if(0 == enet_init_status) {
		APP_PRINTF("[error] Ethernet initialization failed enet_init_status = 0\r\n");
		APP_PRINTF("Attempt to retry initialization...\r\n");
		
		// 重试机制：最多重试3次
		int retry_count = 0;
		int max_retries = 3;
		while((enet_init_status == 0) && (retry_count < max_retries)) {
			retry_count++;
			APP_PRINTF("[retry %d/%d] Wait for 2 seconds and retry...\r\n", retry_count, max_retries);
			phy_delay_us(2000*1000);
			
			// 重新复位PHY
			APP_PRINTF("Reset PHY chip again...\r\n");
			phy_rst();
			phy_delay_us(1000*1000);
			
			// 重新初始化
#ifdef CHECKSUM_BY_HARDWARE
			enet_init_status = enet_init(ENET_100M_FULLDUPLEX, ENET_AUTOCHECKSUM_DROP_FAILFRAMES, ENET_BROADCAST_FRAMES_PASS);
#else
			enet_init_status = enet_init(ENET_100M_FULLDUPLEX, ENET_NO_AUTOCHECKSUM, ENET_BROADCAST_FRAMES_PASS);
#endif
			
			if(enet_init_status == 1) {
				APP_PRINTF("[retry successful] Ethernet initialization successful!\r\n");
				break;
			} else {
				APP_PRINTF("[retry failed] Status value: %d\r\n", enet_init_status);
			}
		}
		
		// if(enet_init_status == 0) {
		// 	printf("[Serious Error] Ethernet initialization still failed after %d retries!\r\n", max_retries);
		// 	printf("[Tip] The system will continue to run, but network functionality may not be available\r\n");
		// 	printf("[Suggestion] Please check:\r\n");
		// 	printf("       1. PHY chip hardware connection (MDIO/MDC/reset pin)\r\n");
		// 	printf("       2. Is the network cable connected\r\n");
		// 	printf("       3. Does the PHY chip model support 100M full duplex\r\n");
		// 	printf("       4. Is the clock configuration correct (RMII requires a 50MHz reference clock)\r\n");
		// }
	}

	enet_interrupt_enable(ENET_DMA_INT_NIE);
	enet_interrupt_enable(ENET_DMA_INT_RIE);
	APP_PRINTF("[4/5] Interrupt enabled completion\r\n");
	
	APP_PRINTF("[5/5] Ethernet hardware initialization status: %s (Status value: %d)\r\n", 
	       (enet_init_status == 1) ? "OK" : "FAIL", enet_init_status);
	APP_PRINTF("========== Ethernet hardware initialization completed ==========\r\n\r\n");
}
/*!
    \brief      configures the ethernet interface
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void enet_mac_dma_config(void)
{
	//ErrStatus reval_state = ERROR;

	/* enable ethernet clock  */
	rcu_periph_clock_enable(RCU_ENET);
	rcu_periph_clock_enable(RCU_ENETTX);
	rcu_periph_clock_enable(RCU_ENETRX);

	/* reset ethernet on AHB bus */
	enet_deinit();
	 phy_delay_us(1000*1000);
	if(enet_software_reset())
		APP_PRINTF("enet_software_reset is OK!\r\n");
	else
	{
		while(1)
		{
			APP_PRINTF("enet_software_reset is ERR!\r\n");
				 phy_delay_us(500*1000);
		};
	}
	phy_delay_us(500*1000);
	
	/* configure the parameters which are usually less cared for enet initialization */
	APP_PRINTF("      [3.3] Initialize Ethernet MAC parameters (fixed 100M full duplex mode)...\r\n");
	APP_PRINTF("      Wait for the PHY chip to fully stabilize (200ms)...\r\n");
	phy_delay_us(200*1000);  // 等待200ms让PHY芯片完全稳定，确保MDIO通信正常
	
#ifdef CHECKSUM_BY_HARDWARE
	APP_PRINTF("      Mode: 100M full duplex, hardware checksum\r\n");
	enet_init_status = enet_init(ENET_100M_FULLDUPLEX, ENET_AUTOCHECKSUM_DROP_FAILFRAMES, ENET_BROADCAST_FRAMES_PASS);
#else
	printf("      Mode: 100M full duplex, software checksum\r\n");
	enet_init_status = enet_init(ENET_100M_FULLDUPLEX, ENET_NO_AUTOCHECKSUM, ENET_BROADCAST_FRAMES_PASS);
#endif /* CHECKSUM_BY_HARDWARE */
	 
	APP_PRINTF("      [3.3] MAC initialization status: %s (status value: %d)\r\n", 
	       (enet_init_status == 1) ? "OK" : "FAIL", enet_init_status);
	if(enet_init_status == 0) {
		// printf("      [Diagnosis] MAC initialization failed, detailed information:\r\n");
		// printf("      - MAC configuration register: 0x%04X\r\n", ENET_MAC_CFG);
		// printf("      - Register value 0x8000 indicates: MAC partially initialized but PHY configuration failed\r\n");
		// printf("      [Possible reasons]:\r\n");
		// printf("      1. PHY chip not properly connected or damaged\r\n");
		// printf("      2. MDIO/MDC communication abnormality (check GPIO configuration)\r\n");
		// printf("      3. PHY chip requires more time for initialization\r\n");
		// printf("      4. Network cable not connected or link not established\r\n");
		// printf("      5. PHY chip does not support 100M full duplex mode\r\n");
		// printf("      6. RMII reference clock (50MHz) not configured correctly\r\n");
	}

	APP_PRINTF("      [3.3] MAC configuration register: 0x%04X\r\n", ENET_MAC_CFG);//???????cc00 ??????+100M
}
/*
phy_value is :7FC0
enet_init_flag2:1
ENET_MAC_CFG2:cc00
*/


/*!
    \brief      configures the different GPIO ports
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void enet_gpio_config(void)
{
	
	  /* ???????????? RMII???
     * ETH_MDIO -------------------------> PA2
     * ETH_MDC --------------------------> PC1
     * ETH_RMII_REF_CLK------------------> PA1
     * ETH_RMII_CRS_DV ------------------> PA7
     * ETH_RMII_RXD0 --------------------> PC4
     * ETH_RMII_RXD1 --------------------> PC5
     * ETH_RMII_TX_EN -------------------> PB11
     * ETH_RMII_TXD0 --------------------> PB12
     * ETH_RMII_TXD1 --------------------> PB13
     * ETH_RESET-------------------------> PE15
     */
	
	
	rcu_periph_clock_enable(RCU_GPIOA);
	rcu_periph_clock_enable(RCU_GPIOB);
	rcu_periph_clock_enable(RCU_GPIOC);
	rcu_periph_clock_enable(RCU_GPIOE);

	/* enable SYSCFG clock */
	rcu_periph_clock_enable(RCU_SYSCFG);


	/* choose DIV2 to get 50MHz from 200MHz on CKOUT0 pin (PA8) to clock the PHY */

	syscfg_enet_phy_interface_config(SYSCFG_ENET_PHY_RMII);

	/* PA1: ETH_RMII_REF_CLK */
	gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_1);
	gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_1);

	/* PA2: ETH_MDIO */
	gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_2);
	gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_2);

	/* PA7: ETH_RMII_CRS_DV */
	gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_7);
	gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_7);

	gpio_af_set(GPIOA, GPIO_AF_11, GPIO_PIN_1);
	gpio_af_set(GPIOA, GPIO_AF_11, GPIO_PIN_2);
	gpio_af_set(GPIOA, GPIO_AF_11, GPIO_PIN_7);

	/* PB11: ETH_RMII_TX_EN */
	gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_11);
	gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_11);

	/* PB12: ETH_RMII_TXD0 */
	gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_12);
	gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_12);

	/* PB13: ETH_RMII_TXD1 */
	gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_13);
	gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_13);

	gpio_af_set(GPIOB, GPIO_AF_11, GPIO_PIN_11);
	gpio_af_set(GPIOB, GPIO_AF_11, GPIO_PIN_12);
	gpio_af_set(GPIOB, GPIO_AF_11, GPIO_PIN_13);

	/* PC1: ETH_MDC */
	gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_1);
	gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_1);

	/* PC4: ETH_RMII_RXD0 */
	gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_4);
	gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_4);

	/* PC5: ETH_RMII_RXD1 */
	gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_5);
	gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_5);

	gpio_af_set(GPIOC, GPIO_AF_11, GPIO_PIN_1);
	gpio_af_set(GPIOC, GPIO_AF_11, GPIO_PIN_4);
	gpio_af_set(GPIOC, GPIO_AF_11, GPIO_PIN_5);

	//PHY_RST
	gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP,GPIO_PIN_4);
	gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_MAX,  GPIO_PIN_4);
	gpio_bit_set(GPIOA,GPIO_PIN_4);

}
