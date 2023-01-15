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
 *	File:	ipc/ipc_entry.h
 *	Author:	Rich Draves
 *	Date:	1989
 *
 *	Definitions for translation entries, which represent
 *	tasks' capabilities for ports and port sets.
 */

#ifndef	_IPC_IPC_ENTRY_H_
#define _IPC_IPC_ENTRY_H_

#include <mach/port.h>
#include <mach/kern_return.h>
#include <kern/zalloc.h>
#include <ipc/port.h>
#include <ipc/ipc_table.h>
#include <ipc/ipc_types.h>

/*
 *	Spaces hold capabilities for ipc_object_t's (ports and port sets).
 *	Each ipc_entry_t records a capability.  Most capabilities have
 *	small names, and the entries are elements of a table.
 *	Capabilities can have large names, and a splay tree holds
 *	those entries.  The cutoff point between the table and the tree
 *	is adjusted dynamically to minimize memory consumption.
 *
 *	The ie_index field of entries in the table implements
 *	a ordered hash table with open addressing and linear probing.
 *	This hash table converts (space, object) -> name.
 *	It is used independently of the other fields.
 *
 *	Free (unallocated) entries in the table have null ie_object
 *	fields.  The ie_bits field is zero except for IE_BITS_GEN.
 *	The ie_next (ie_request) field links free entries into a free list.
 *
 *	The first entry in the table (index 0) is always free.
 *	It is used as the head of the free list.
 */

typedef unsigned int ipc_entry_bits_t;
typedef ipc_table_elems_t ipc_entry_num_t;	/* number of entries */

typedef struct ipc_entry {
	ipc_entry_bits_t ie_bits;
	struct ipc_object *ie_object;
	union {
		mach_port_index_t next;
		ipc_table_index_t request;
	} index;
	union {
		mach_port_index_t table;
		struct ipc_tree_entry *tree;
	} hash;
} *ipc_entry_t;

#define	IE_NULL		((ipc_entry_t) 0)

#define	ie_request	index.request
#define	ie_next		index.next
#define	ie_index	hash.table

#define	IE_BITS_UREFS_MASK	0x0000ffff	/* 16 bits of user-reference */
#define	IE_BITS_UREFS(bits)	((bits) & IE_BITS_UREFS_MASK)

#define	IE_BITS_TYPE_MASK	0x001f0000	/* 5 bits of capability type */
#define	IE_BITS_TYPE(bits)	((bits) & IE_BITS_TYPE_MASK)

#define	IE_BITS_MAREQUEST	0x00200000	/* 1 bit for msg-accepted */

#define	IE_BITS_COMPAT		0x00400000	/* 1 bit for compatibility */

#define	IE_BITS_COLLISION	0x00800000	/* 1 bit for collisions */
#define	IE_BITS_GEN_MASK	0xff000000U	/* 8 bits for generation */
#define	IE_BITS_GEN(bits)	((bits) & IE_BITS_GEN_MASK)
#define	IE_BITS_GEN_ONE		0x01000000	/* low bit of generation */
#define	IE_BITS_RIGHT_MASK	0x007fffff	/* relevant to the right */


typedef struct ipc_tree_entry {
	struct ipc_entry ite_entry;
	mach_port_t ite_name;
	struct ipc_space *ite_space;
	struct ipc_tree_entry *ite_lchild;
	struct ipc_tree_entry *ite_rchild;
} *ipc_tree_entry_t;

#define	ITE_NULL	((ipc_tree_entry_t) 0)

#define	ite_bits	ite_entry.ie_bits
#define	ite_object	ite_entry.ie_object
#define	ite_request	ite_entry.ie_request
#define	ite_next	ite_entry.hash.tree

extern zone_t ipc_tree_entry_zone;

#define ite_alloc()	((ipc_tree_entry_t) zalloc(ipc_tree_entry_zone))
#define	ite_free(ite)	zfree(ipc_tree_entry_zone, (vm_offset_t) (ite))

/*
 * Exported interfaces
 */

/* Search for entry in a space by name */
extern ipc_entry_t ipc_entry_lookup(
	ipc_space_t	space,
	mach_port_t	name);

/* Allocate an entry in a space */
extern kern_return_t ipc_entry_get(
	ipc_space_t	space,
	mach_port_t	*namep,
	ipc_entry_t	*entryp);

/* Allocate an entry in a space, growing the space if necessary */
extern kern_return_t ipc_entry_alloc(
	ipc_space_t	space,
	mach_port_t	*namep,
	ipc_entry_t	*entryp);

/* Allocate/find an entry in a space with a specific name */
extern kern_return_t ipc_entry_alloc_name(
	ipc_space_t	space,
	mach_port_t	name,
	ipc_entry_t	*entryp);

/* Deallocate an entry from a space */
extern void ipc_entry_dealloc(
	ipc_space_t	space,
	mach_port_t	name,
	ipc_entry_t	entry);

/* Grow the table in a space */
extern kern_return_t ipc_entry_grow_table(
	ipc_space_t	space,
	int		target_size);

#endif	/* _IPC_IPC_ENTRY_H_ */
