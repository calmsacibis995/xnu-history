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

/* Copyright (c) 1992, 1997-1998 Apple Computer, Inc.  All rights reserved.
 *
 *      File:  machdep/ppc/libc/ffs.s
 *
 *	int ffs(int value)
 *
 *	DESCRIPTION
 *		The ffs() function finds the first bit set in value and returns the
 *		index of that bit.  Bits are numbered starting from 1, starting at
 *		the right-most bit. A return value of 0 means that the argument was
 *
 * HISTORY
 *	14-Aug-1998	Umesh Vaishampayan	(umeshv@apple.com)
 *		Optimized!
 *
 *  10-Mar-1998 Matt Watson (mwatson@apple.com)
 *      Correctified
 *
 *  19-Jan-1998 Matt Watson (mwatson@apple.com)
 *      Simplified
 *
 *  24-Jan-1997 Umesh Vaishampayan (umeshv@NeXT.com)
 *      Ported to PPC. 
 */

.text
.align 4
.globl _ffs
_ffs:					/* Cycles */
	neg		r0,r3		/* 0 */
	and		r3,r0,r3	/* 1 */
	li		r4, 32		/* 1 */
	cntlzw	r3,r3		/* 2 */
	subf	r3,r3,r4	/* 3 */
	blr
