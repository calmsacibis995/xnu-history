/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * "Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.0 (the 'License').  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License."
 * 
 * @APPLE_LICENSE_HEADER_END@
 */

/*
 * Copyright (c) 1995-1996 NeXT Software, Inc.
 *
 * Interface definition for the Mace Ethernet chip 
 *
 * HISTORY
 *
 * 16-Sept-97	 
 *	Created.
 */

#import <driverkit/kernelDriver.h>
#import <driverkit/IOEthernet.h>
#import <driverkit/IONetbufQueue.h>
#import <driverkit/ppc/IOTreeDevice.h>
#import <driverkit/ppc/IODBDMA.h>
#import <driverkit/generalFuncs.h>
#import <driverkit/IOPower.h>
#import <machdep/ppc/proc_reg.h>		/* eieio */
#import <bsd/net/etherdefs.h>
#import <string.h>				/* bcopy */
#import <driverkit/IOEthernetPrivate.h>		/* debugger methods */
#import <kern/kdebug.h>				/* Performance tracepoints */


#import "MaceEnetRegisters.h"

#if 0
#define IOLog kprintf
#endif

typedef void  *		IOPPCAddress;

typedef struct enet_dma_cmd_t
{
    IODBDMADescriptor		desc_seg[2];
} enet_dma_cmd_t;

typedef struct enet_txdma_cmd_t
{
    IODBDMADescriptor		desc_seg[4];
} enet_txdma_cmd_t;

@interface MaceEnet:IOEthernet <IOPower>
{
    volatile IOPPCAddress       	ioBaseEnet;
    volatile IOPPCAddress		ioBaseEnetROM;
    volatile IODBDMAChannelRegisters 	*ioBaseEnetRxDMA;	
    volatile IODBDMAChannelRegisters 	*ioBaseEnetTxDMA;

    u_int16_t				chipId;	

    enet_addr_t				myAddress;
    IONetwork				*networkInterface;
    IONetbufQueue			*transmitQueue;
    BOOL				isPromiscuous;
    BOOL				multicastEnabled;

    BOOL				resetAndEnabled;
    netbuf_t				txNetbuf[TX_RING_LENGTH];
    netbuf_t				rxNetbuf[RX_RING_LENGTH];
    
    unsigned int			txCommandHead;		/* Transmit ring descriptor index */
    unsigned int			txCommandTail;
    unsigned int       			txMaxCommand;		
    unsigned int			rxCommandHead;		/* Receive ring descriptor index */
    unsigned int			rxCommandTail;
    unsigned int        		rxMaxCommand;		

    unsigned char *			dmaCommands;
    enet_txdma_cmd_t *			txDMACommands;		/* TX descriptor ring ptr */
    unsigned int			txDMACommandsPhys;

    enet_dma_cmd_t *			rxDMACommands;		/* RX descriptor ring ptr */
    unsigned int			rxDMACommandsPhys;

    u_int32_t				txWDInterrupts;
    u_int32_t				txWDTimeouts;
    BOOL				txWDForceReset;

    netbuf_t				debuggerPkt;
    u_int32_t                  		debuggerPktSize;
   
    u_int16_t				hashTableUseCount[64];
    u_int8_t         			hashTableMask[8];
}

+ (BOOL)probe:devDesc;
- initFromDeviceDescription:devDesc;

- free;
- (void)transmit:(netbuf_t)pkt;
- (void)serviceTransmitQueue;
- (BOOL)resetAndEnable:(BOOL)enable;

- (void)interruptOccurredAt:(int)irqNum;
- (void)timeoutOccurred;

- (BOOL)enableMulticastMode;
- (void)disableMulticastMode;
- (BOOL)enablePromiscuousMode;
- (void)disablePromiscuousMode;

/*
 * Kernel Debugger
 */
- (void)sendPacket:(void *)pkt length:(unsigned int)pkt_len;
- (void)receivePacket:(void *)pkt length:(unsigned int *)pkt_len timeout:(unsigned int)timeout;


/*
 * Power management methods. 
 */
- (IOReturn)getPowerState:(PMPowerState *)state_p;
- (IOReturn)setPowerState:(PMPowerState)state;
- (IOReturn)getPowerManagement:(PMPowerManagementState *)state_p;
- (IOReturn)setPowerManagement:(PMPowerManagementState)state;

/*
 * Queue interface
 */
- (int) transmitQueueSize;
- (int) transmitQueueCount;

@end

/*
 * Performance tracepoints
 *
 * DBG_MACE_RXIRQ     	- Receive  ISR run time
 * DBG_MACE_TXIRQ     	- Transmit ISR run time
 * DBG_MACE_TXQUEUE     - Transmit packet passed from network stack
 * DBG_MACE_TXCOMPLETE  - Transmit packet sent
 * DBG_MACE_RXCOMPLETE  - Receive packet passed to network stack
 */
#define DBG_MACE_ENET		0x0800
#define DBG_MACE_RXIRQ 		DRVDBG_CODE(DBG_DRVNETWORK,(DBG_MACE_ENET+1)) 	
#define DBG_MACE_TXIRQ	 	DRVDBG_CODE(DBG_DRVNETWORK,(DBG_MACE_ENET+2))	
#define DBG_MACE_TXQUEUE 	DRVDBG_CODE(DBG_DRVNETWORK,(DBG_MACE_ENET+3))	
#define DBG_MACE_TXCOMPLETE	DRVDBG_CODE(DBG_DRVNETWORK,(DBG_MACE_ENET+4))	
#define DBG_MACE_RXCOMPLETE	DRVDBG_CODE(DBG_DRVNETWORK,(DBG_MACE_ENET+5))	

