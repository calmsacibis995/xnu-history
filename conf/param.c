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
 * Copyright (c) 1980, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)param.c	8.3 (Berkeley) 8/20/94
 */

#import <confdep.h>
#import <sys/param.h>
#import <sys/systm.h>
#import <sys/socket.h>
#import <sys/vnode.h>
#import <sys/file.h>
#import <sys/callout.h>
#import <sys/clist.h>
#import <sys/mbuf.h>
#import <sys/domain.h>
#import <sys/kernel.h>
#import <ufs/ufs/quota.h>
#import <ufs/ufs/inode.h>
#import <miscfs/fifofs/fifo.h>

struct	timezone tz = { TIMEZONE, PST };
#define	NPROC (20 + 16 * MAXUSERS)
int	maxproc = NPROC;
int nprocs = 0; /* XXX */
#define	NTEXT (80 + NPROC / 8)			/* actually the object cache */
#define	NVNODE (NPROC + NTEXT + 300)
int	desiredvnodes = NVNODE + 350;
int	maxfiles = 4 * (3 * (NPROC + MAXUSERS) + 80);
unsigned int	ncallout = 16 + 2*NPROC;
int nmbclusters = NMBCLUSTERS;
int	nport = NPROC / 2;

/*
 * These have to be allocated somewhere; allocating
 * them here forces loader errors if this file is omitted
 * (if they've been externed everywhere else; hah!).
 */
struct 	callout *callout;
struct	cblock *cfree;
struct	cblock *cfreelist = 0;
int	cfreecount = 0;
struct	buf *buf;
char	*buffers;
struct	domain *domains;
