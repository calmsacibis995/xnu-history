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
 * Copyright (c) 1995, 1994, 1993, 1992, 1991, 1990  
 * Open Software Foundation, Inc. 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation, and that the name of ("OSF") or Open Software 
 * Foundation not be used in advertising or publicity pertaining to 
 * distribution of the software without specific, written prior permission. 
 *  
 * OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL OSF BE LIABLE FOR ANY 
 * SPECIAL, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES 
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN 
 * ACTION OF CONTRACT, NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING 
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE 
 */
/*
 * OSF Research Institute MK6.1 (unencumbered) 1/31/1995
 */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 *	File:	ipc/ipc_splay.h
 *	Author:	Rich Draves
 *	Date:	1989
 *
 *	Declarations of primitive splay tree operations.
 */

#ifndef	_IPC_IPC_SPLAY_H_
#define _IPC_IPC_SPLAY_H_

#include <mach/port.h>
#include <kern/assert.h>
#include <kern/macro_help.h>
#include <ipc/ipc_entry.h>

typedef struct ipc_splay_tree {
	mach_port_t ist_name;		/* name used in last lookup */
	ipc_tree_entry_t ist_root;	/* root of middle tree */
	ipc_tree_entry_t ist_ltree;	/* root of left tree */
	ipc_tree_entry_t *ist_ltreep;	/* pointer into left tree */
	ipc_tree_entry_t ist_rtree;	/* root of right tree */
	ipc_tree_entry_t *ist_rtreep;	/* pointer into right tree */
} *ipc_splay_tree_t;

#define	ist_lock(splay)		/* no locking */
#define ist_unlock(splay)	/* no locking */

/* Initialize a raw splay tree */
extern void ipc_splay_tree_init(
	ipc_splay_tree_t	splay);

/* Pick a random entry in a splay tree */
extern boolean_t ipc_splay_tree_pick(
	ipc_splay_tree_t	splay,
	mach_port_t		*namep,
	ipc_tree_entry_t	*entryp);

/* Find an entry in a splay tree */
extern ipc_tree_entry_t ipc_splay_tree_lookup(
	ipc_splay_tree_t	splay,
	mach_port_t		name);

/* Insert a new entry into a splay tree */
extern void ipc_splay_tree_insert(
	ipc_splay_tree_t	splay,
	mach_port_t		name,
	ipc_tree_entry_t	entry);

/* Delete an entry from a splay tree */
extern void ipc_splay_tree_delete(
	ipc_splay_tree_t	splay,
	mach_port_t		name,
	ipc_tree_entry_t	entry);

/* Split a splay tree */
extern void ipc_splay_tree_split(
	ipc_splay_tree_t	splay,
	mach_port_t		name,
	ipc_splay_tree_t	entry);

/* Join two splay trees */
extern void ipc_splay_tree_join(
	ipc_splay_tree_t	splay,
	ipc_splay_tree_t	small);

/* Do a bounded splay tree lookup */
extern void ipc_splay_tree_bounds(
	ipc_splay_tree_t	splay,
	mach_port_t		name,
	mach_port_t		*lowerp, 
	mach_port_t		*upperp);

/* Initialize a symmetric order traversal of a splay tree */
extern ipc_tree_entry_t ipc_splay_traverse_start(
	ipc_splay_tree_t	splay);

/* Return the next entry in a symmetric order traversal of a splay tree */
extern ipc_tree_entry_t ipc_splay_traverse_next(
	ipc_splay_tree_t	splay,
	boolean_t		delete);

/* Terminate a symmetric order traversal of a splay tree */
extern void ipc_splay_traverse_finish(
	ipc_splay_tree_t	splay);

#endif	/* _IPC_IPC_SPLAY_H_ */
