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
 * Copyright (c) 1994 NeXT Computer, Inc.
 *
 * Exported interface for Kernel Bus Interrupt Object.
 *
 * HISTORY
 *
 * 30 Jun 1994 ? at NeXT
 *	Created.
 */

#ifdef	DRIVER_PRIVATE

#import <driverkit/KernBus.h>

@interface KernBusInterrupt : KernBusItem <KernBusInterrupt>
{
@private
    id		_attachedInterrupts;
    int		_attachedInterruptCount;
    id		_interruptLock;
    int		_suspendCount;
    id		_suspendLock;
    void	*_deviceHandler;
}

- initForResource: resource
	    item: (unsigned int)item
	withHandler: (void *)handler
	shareable: (BOOL)shareable;

@end

BOOL
KernBusInterruptDispatch(
	KernBusInterrupt	*interrupt,
	void			*state
);

void
KernBusInterruptSuspend(
	KernBusInterrupt	*interrupt
);

void
KernBusInterruptResume(
	KernBusInterrupt	*interrupt
);

#endif
