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
	File:		uslExpert.c

	Contains:	xxx put contents here xxx

	Version:	xxx put version here xxx

	Copyright:	� 1998 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				xxx put dri here xxx

		Other Contact:		xxx put other contact here xxx

		Technology:			xxx put technology here xxx

	Writers:

		(CJK)	Craig Keithley
		(DF)	David Ferguson
		(BT)	Barry Twycross

	Change History (most recent first):

	  <USB7>	 9/28/98	BT		Add device reset function
	  <USB6>	 6/11/98	CJK		add reserved params to USBExpertSetDevicePowerStatus. Wanted to
									get it into the USB.i before freezing C1
	  <USB5>	 6/11/98	BT		Add power state notification message
	  <USB4>	  6/8/98	CJK		add USBExpertSetDevicePowerStatus
	  <USB3>	  6/6/98	DF		Change parameter for USBExpertInstallInterfaceDriver from hubRef
									to deviceRef
	  <USB2>	 5/12/98	BT		New interface handling
	  <USB1>	 4/26/98	BT		first checked in
*/

//#include <errors.h>
#include "../USB.h"
#include "../USBpriv.h"
#include "../driverservices.h"
#include "uslpriv.h"
#include "../uimpriv.h"


static ExpertNotificationProcPtr expert;

void SetExpertFunction(void *exProc)
{
	expert = exProc;
}

OSStatus USBExpertNotify(void *note)
{
	if(expert == nil)
	{
		return(kUSBInternalErr);	// make up a not inited err
	}
	return(*expert)(note);
}

OSStatus USBExpertStatus(USBDeviceRef ref, void *pointer, UInt32 value)
{
ExpertNotificationData note;
	note.notification = kNotifyStatus;
	note.deviceRef = &ref;
	note.data = pointer;
	note.info2 = value;
	return(USBExpertNotify(&note));
}

OSStatus USBExpertFatalError(USBDeviceRef ref, OSStatus status, void *pointer, UInt32 value)
{
ExpertNotificationData note;
	note.notification = kNotifyFatalError;
	note.deviceRef = &ref;
	note.data = pointer;
	note.info1 = status;
	note.info2 = value;
	return(USBExpertNotify(&note));
}


OSStatus USBExpertInstallInterfaceDriver(USBDeviceRef ref, USBDeviceDescriptor *desc, 
							USBInterfaceDescriptor *interface,
							USBReference deviceRef, UInt32 busPowerAvailable)
{
ExpertNotificationData note;
	note.notification = kNotifyAddInterface;
	note.deviceRef = &ref;
	note.data = interface;
	note.info1 = deviceRef;
	note.info2 = (UInt32) desc;
	note.busPowerAvailable = busPowerAvailable;
	return(USBExpertNotify(&note));
}

OSStatus USBExpertInstallDeviceDriver(USBDeviceRef ref, USBDeviceDescriptor *desc, 
							USBReference hubRef, UInt32 port,
							UInt32 busPowerAvailable)
{
ExpertNotificationData note;
	note.notification = kNotifyAddDevice;
	note.deviceRef = &ref;
	note.data = desc;
	note.info1 = hubRef;
	note.info2 = port;
	note.busPowerAvailable = busPowerAvailable;
	return(USBExpertNotify(&note));
}

OSStatus USBExpertRemoveDeviceDriver(USBDeviceRef ref)
{
ExpertNotificationData note;
	note.notification = kNotifyRemoveDevice;
	note.deviceRef = &ref;
	return(USBExpertNotify(&note));
}

OSStatus USBExpertRemoveInterfaceDriver(USBDeviceRef ref)
{
ExpertNotificationData note;
	note.notification = kNotifyRemoveInterface;
	note.deviceRef = &ref;
	return(USBExpertNotify(&note));
}

OSStatus USBExpertSetDevicePowerStatus(USBDeviceRef ref, UInt32 reserved1, UInt32 reserved2, UInt32 powerStatus, UInt32 busPowerAvailable, UInt32 busPowerNeeded )
{
#pragma unused (reserved1)
#pragma unused (reserved2)

ExpertNotificationData note;
	note.notification = kNotifyPowerState;
	note.deviceRef = &ref;
	note.info1 = powerStatus;
	note.busPowerAvailable = busPowerAvailable;
	note.info2 = busPowerNeeded;
	USBExpertStatus(0,"USL - sending power note to expert", powerStatus);

	return(USBExpertNotify(&note));
}

OSStatus USBExpertNotifyParentMsg(USBReference reference, void *pointer)
{
ExpertNotificationData note;

	note.notification = kNotifyParentNotify;
	note.deviceRef = &reference;
	note.data = pointer;
	return(USBExpertNotify(&note));
}

