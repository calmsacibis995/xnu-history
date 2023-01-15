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

// 	Copyright (c) 1992 NeXT Computer, Inc.  All rights reserved. 
//
// PCPointer.h - Interface to Generic PC Pointer object
// 
//
// HISTORY
// 01-Dec-92	Joe Pasqua at NeXT
//      Created. 
// 5  Aug 1993	  Erik Kay at NeXT
//	added ivar space for expansion
//

#ifdef	DRIVER_PRIVATE

#import <driverkit/IODevice.h>
#import <driverkit/IODirectDevice.h>

#define RESOLUTION "Resolution"
#define INVERTED "Inverted"

int PCPatoi(char *p);

@interface PCPointer : IODirectDevice
{
    id target;			// Object to which we send events
    unsigned resolution;	// dots per inch
    BOOL 	inverted;	// inverted direction axes?
@private
    int		_reserved[4];	// reserved for future expansion
}

// Public methods

+ (id) activePointerDevice;

- (BOOL)setEventTarget:eventTarget;
// Description:	Called to inform us where to send mouse events. The
//		supplied object must implement the PCPointerTarget protocol.

- (int)getResolution;
// Description:	Returns the resolution, in dots per inch, of the pointer.

- (BOOL) getInverted;
// Description: Returns whether or not the mouse should have its axes inverted

- (BOOL)mouseInit:deviceDescription;
// Description:	Perform rodent specific initialization

// - (port_t) devicePort;
// Description:	Return the device port of the object.

- (IOReturn)getIntValues:(unsigned *)parameterArray
    forParameter:(IOParameterName)parameterName count:(unsigned *)count;
- (IOReturn)setIntValues:(unsigned *)parameterArray
    forParameter:(IOParameterName)parameterName count:(unsigned)count;

@end

#endif	/* DRIVER_PRIVATE */
