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

/* 	Copyright (c) 1992 NeXT Computer, Inc.  All rights reserved. 
 *
 * evsio.h - Get/Set parameter calls for Event Status Driver.
 *
 *	CAUTION: Developers should stick to the API exported in
 *		<drivers/event_status_driver.h> to guarantee
 *		binary compatability of their applications in future
 *		releases.
 *
 * HISTORY
 * 30 Jul 93    Frank Rowand
 *      Created. 
 */

// NOTES:
// * This module defines system specific additions to the get/set
//   parameter set defined in the machine independent evsio.h module.
// * At the moment, there are no extensions, so we don't have anything
//   in here except an import of that mi interface.

#ifdef	DRIVER_PRIVATE

#ifndef _PPC_DEV_EVIO_
#define _PPC_DEV_EVIO_

#import <bsd/dev/evio.h>

//
// Device/architecture specific controls, implemented within
// Event Sources follow.
//

#endif	/* DRIVER_PRIVATE */


#endif /* _PPC_DEV_EVIO_ */

