/**
* @file
* Ethernet Interface Skeleton
*
*/

/*
* Copyright (c) 2001-2004 Swedish Institute of Computer Science.
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
* 3. The name of the author may not be used to endorse or promote products
*    derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
* SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
* OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
* IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
* OF SUCH DAMAGE.
*
* This file is part of the lwIP TCP/IP stack.
*
* Author: Adam Dunkels <adam@sics.se>
*
*/

#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/timeouts.h"
#include "netif/etharp.h"
#include "err.h"
#include "ethernetif.h"
#include "udp_echo.h"


#include "gd32f4xx_enet.h"
#include <string.h>
#include "FreeRTOS.h"
#include "bsp_eth.h"
#include "semphr.h"
#include "rtt_log.h"

#define ETHERNETIF_INPUT_TASK_STACK_SIZE          (350)
#define ETHERNETIF_INPUT_TASK_PRIO                (configMAX_PRIORITIES - 1)
#define LOWLEVEL_OUTPUT_WAITING_TIME              (250)
/* The time to block waiting for input */
#define LOWLEVEL_INPUT_WAITING_TIME               ((portTickType )100)

/* define those to better describe your network interface */
#define IFNAME0 'G'
#define IFNAME1 'D'

/* ENET RxDMA/TxDMA descriptor */
extern enet_descriptors_struct  rxdesc_tab[ENET_RXBUF_NUM], txdesc_tab[ENET_TXBUF_NUM];

/* ENET receive buffer  */
extern uint8_t rx_buff[ENET_RXBUF_NUM][ENET_RXBUF_SIZE]; 

/* ENET transmit buffer */
extern uint8_t tx_buff[ENET_TXBUF_NUM][ENET_TXBUF_SIZE]; 

/*global transmit and receive descriptors pointers */
extern enet_descriptors_struct  *dma_current_txdesc;
extern enet_descriptors_struct  *dma_current_rxdesc;

/* preserve another ENET RxDMA/TxDMA ptp descriptor for normal mode */
enet_descriptors_struct  ptp_txstructure[ENET_TXBUF_NUM];
enet_descriptors_struct  ptp_rxstructure[ENET_RXBUF_NUM];


static struct netif *low_netif = NULL;
xSemaphoreHandle g_rx_semaphore = NULL;

/**
* In this function, the hardware should be initialized.
* Called from ethernetif_init().
*
* @param netif the already initialized lwip network interface structure
*        for this ethernetif
*/
static void low_level_init(struct netif *netif)
{
    uint32_t i;

    APP_PRINTF("\r\n========== Ethernet low-level initialization begins ==========\r\n");
    
    /* set netif MAC hardware address length */
    netif->hwaddr_len = ETHARP_HWADDR_LEN;

    /* set netif MAC hardware address */
    netif->hwaddr[0] =  MAC_ADDR0;
    netif->hwaddr[1] =  MAC_ADDR1;
    netif->hwaddr[2] =  MAC_ADDR2;
    netif->hwaddr[3] =  MAC_ADDR3;
    netif->hwaddr[4] =  MAC_ADDR4;
    netif->hwaddr[5] =  MAC_ADDR5;
    
    APP_PRINTF("[1/7] MAC address: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
           netif->hwaddr[0], netif->hwaddr[1], netif->hwaddr[2],
           netif->hwaddr[3], netif->hwaddr[4], netif->hwaddr[5]);

    /* set netif maximum transfer unit */
    netif->mtu = 1500;
    APP_PRINTF("[2/7] MTU: %d Byte\r\n", netif->mtu);

    /* accept broadcast address and ARP traffic */
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;
    APP_PRINTF("[3/7] Network interface flag: Broadcast | ARP | Link UP\r\n");

    low_netif =netif;

    /* create binary semaphore used for informing ethernetif of frame reception */
    APP_PRINTF("[4/7] Create Received Signal Quantity...\r\n");
    if (g_rx_semaphore == NULL){
        vSemaphoreCreateBinary(g_rx_semaphore);
        xSemaphoreTake( g_rx_semaphore, 0);
    }
    APP_PRINTF("[4/7] Received semaphore creation completed\r\n");

    /* initialize MAC address in ethernet MAC */ 
    APP_PRINTF("[5/7] Set MAC address to Ethernet hardware...\r\n");
    enet_mac_address_set(ENET_MAC_ADDRESS0, netif->hwaddr);
    APP_PRINTF("[5/7] MAC address setting completed\r\n");

    /* initialize descriptors list: chain/ring mode */
    printf("[6/7] Initialize DMA descriptor chain...\r\n");
#ifdef SELECT_DESCRIPTORS_ENHANCED_MODE
    enet_ptp_enhanced_descriptors_chain_init(ENET_DMA_TX);
    enet_ptp_enhanced_descriptors_chain_init(ENET_DMA_RX);
    printf("      Mode: Enhanced PTP descriptor chain\r\n");
#else
    enet_descriptors_chain_init(ENET_DMA_TX);
    enet_descriptors_chain_init(ENET_DMA_RX);
    APP_PRINTF("      Mode: Standard descriptor chain\r\n");
#endif /* SELECT_DESCRIPTORS_ENHANCED_MODE */  

    /* enable ethernet Rx interrrupt */
    {   int i;
        for(i=0; i<ENET_RXBUF_NUM; i++){ 
            enet_rx_desc_immediate_receive_complete_interrupt(&rxdesc_tab[i]);
        }
    }
    APP_PRINTF("      Number of receiving buffers: %d\r\n", ENET_RXBUF_NUM);
    APP_PRINTF("[6/7] DMA descriptor initialization completed\r\n");

#ifdef CHECKSUM_BY_HARDWARE
    /* enable the TCP, UDP and ICMP checksum insertion for the Tx frames */
    APP_PRINTF("      Enable hardware checksum (TCP/UDP/ICMP)...\r\n");
    for(i=0; i < ENET_TXBUF_NUM; i++){
        enet_transmit_checksum_config(&txdesc_tab[i], ENET_CHECKSUM_TCPUDPICMP_FULL);
    }
    APP_PRINTF("      Number of sending buffers: %d\r\n", ENET_TXBUF_NUM);
#else
    APP_PRINTF("      Use software to verify and sum up\r\n");
#endif /* CHECKSUM_BY_HARDWARE */

    /* create the task that handles the ETH_MAC */
    APP_PRINTF("[7/7] Create Ethernet input processing task...\r\n");
    xTaskCreate(ethernetif_input, "ETHERNETIF_INPUT", ETHERNETIF_INPUT_TASK_STACK_SIZE, NULL,
                ETHERNETIF_INPUT_TASK_PRIO,NULL);
    APP_PRINTF("      Task Name: ETHERNETIF_INPUT\r\n");
    APP_PRINTF("      Stack size : %d 字节\r\n", ETHERNETIF_INPUT_TASK_STACK_SIZE);
    APP_PRINTF("[7/7] Task creation completed\r\n");

    /* enable MAC and DMA transmission and reception */
    APP_PRINTF("      Enable MAC and DMA transmission and reception functions...\r\n");
    enet_enable();
    APP_PRINTF("      MAC and DMA enabled\r\n");
    
    APP_PRINTF("========== Ethernet bottom layer initialization completed ==========\r\n\r\n");
}


/**
* This function should do the actual transmission of the packet. The packet is
* contained in the pbuf that is passed to the function. This pbuf
* might be chained.
*
* @param netif the lwip network interface structure for this ethernetif
* @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
* @return ERR_OK if the packet could be sent
*         an err_t value if the packet couldn't be sent
*
* @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
*       strange results. You might consider waiting for space in the DMA queue
*       to become availale since the stack doesn't retry to send a packet
*       dropped because of memory failure (except for the TCP timers).
*/

static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
    static xSemaphoreHandle s_tx_semaphore = NULL;
    struct pbuf *q;
    uint8_t *buffer ;
    uint16_t framelength = 0;
    ErrStatus reval = ERROR;
  
    SYS_ARCH_DECL_PROTECT(sr);
    
    if (s_tx_semaphore == NULL){
        vSemaphoreCreateBinary (s_tx_semaphore);
    }

    if (xSemaphoreTake(s_tx_semaphore, LOWLEVEL_OUTPUT_WAITING_TIME)){    
        SYS_ARCH_PROTECT(sr);
      
        while((uint32_t)RESET != (dma_current_txdesc->status & ENET_TDES0_DAV)){
        }    
        buffer = (uint8_t *)(enet_desc_information_get(dma_current_txdesc, TXDESC_BUFFER_1_ADDR));

        for(q = p; q != NULL; q = q->next){ 
            memcpy((uint8_t *)&buffer[framelength], q->payload, q->len);
            framelength = framelength + q->len;
        }

       /* transmit descriptors to give to DMA */ 
#ifdef SELECT_DESCRIPTORS_ENHANCED_MODE
        reval = ENET_NOCOPY_PTPFRAME_TRANSMIT_ENHANCED_MODE(framelength, NULL);
#else
        reval = ENET_NOCOPY_FRAME_TRANSMIT(framelength);
#endif /* SELECT_DESCRIPTORS_ENHANCED_MODE */

        SYS_ARCH_UNPROTECT(sr);
        
        /* give semaphore and exit */
        xSemaphoreGive(s_tx_semaphore);
        
        if(SUCCESS == reval){
            return ERR_OK;
        } else {
            return ERR_IF;
        }
    } else {
        return ERR_TIMEOUT;
    }
    
}

/**
* Should allocate a pbuf and transfer the bytes of the incoming
* packet from the interface into the pbuf.
*
* @param netif the lwip network interface structure for this ethernetif
* @return a pbuf filled with the received packet (including MAC header)
*         NULL on memory error
*/
static struct pbuf * low_level_input(struct netif *netif)
{
    struct pbuf *p= NULL, *q;
    uint32_t l =0;
    u16_t len;
    uint8_t *buffer;

    /* obtain the size of the packet and put it into the "len" variable. */
    len = enet_desc_information_get(dma_current_rxdesc, RXDESC_FRAME_LENGTH);
    buffer = (uint8_t *)(enet_desc_information_get(dma_current_rxdesc, RXDESC_BUFFER_1_ADDR));
  
    if (len > 0){
        /* We allocate a pbuf chain of pbufs from the Lwip buffer pool */
        p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
    }
  
    if (p != NULL){
        for(q = p; q != NULL; q = q->next){
            memcpy((uint8_t *)q->payload, (u8_t*)&buffer[l], q->len);
            l = l + q->len;
        }
    }
#ifdef SELECT_DESCRIPTORS_ENHANCED_MODE
    ENET_NOCOPY_PTPFRAME_RECEIVE_ENHANCED_MODE(NULL);
  
#else
    
    ENET_NOCOPY_FRAME_RECEIVE();
#endif /* SELECT_DESCRIPTORS_ENHANCED_MODE */

    return p;
}


/**
* This function is the ethernetif_input task, it is processed when a packet 
* is ready to be read from the interface. It uses the function low_level_input() 
* that should handle the actual reception of bytes from the network
* interface. Then the type of the received packet is determined and
* the appropriate input function is called.
*
* @param netif the lwip network interface structure for this ethernetif
*/
void ethernetif_input( void * pvParameters )
{
    struct pbuf *p;
    SYS_ARCH_DECL_PROTECT(sr);
  
    for( ;; ){   
        if(pdTRUE == xSemaphoreTake(g_rx_semaphore, LOWLEVEL_INPUT_WAITING_TIME)){ 
TRY_GET_NEXT_FRAME:
            SYS_ARCH_PROTECT(sr);
            p = low_level_input( low_netif );
            SYS_ARCH_UNPROTECT(sr);
          
            if   (p != NULL){
                if (ERR_OK != low_netif->input( p, low_netif)){
                    pbuf_free(p);
                }else{
                    goto TRY_GET_NEXT_FRAME;
                }
            }
        }
    }
}

/**
* Should be called at the beginning of the program to set up the
* network interface. It calls the function low_level_init() to do the
* actual setup of the hardware.
*
* This function should be passed as a parameter to netif_add().
*
* @param netif the lwip network interface structure for this ethernetif
* @return ERR_OK if the loopif is initialized
*         ERR_MEM if private data couldn't be allocated
*         any other err_t on error
*/
err_t ethernetif_init(struct netif *netif)
{
    LWIP_ASSERT("netif != NULL", (netif != NULL));

    printf("\r\n========== Ethernet interface initialization begins ==========\r\n");
    
#if LWIP_NETIF_HOSTNAME
    /* initialize interface hostname */
    netif->hostname = "lwip";
    printf("主机名: %s\r\n", netif->hostname);
#endif /* LWIP_NETIF_HOSTNAME */

    netif->name[0] = IFNAME0;
    netif->name[1] = IFNAME1;
    printf("Interface name: %c%c\r\n", netif->name[0], netif->name[1]);

    netif->output = etharp_output;
    netif->linkoutput = low_level_output;
    printf("Output function: etharp_output, low_level_output\r\n");

    /* initialize the hardware */
    printf("Call underlying hardware initialization...\r\n");
    enet_system_setup();  // 先初始化硬件
    low_level_init(netif);

    printf("========== Ethernet interface initialization completed ==========\r\n\r\n");
    return ERR_OK;
}
