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
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
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

#ifndef	_PPC_TRAP_H_
#define	_PPC_TRAP_H_

/*
 * Hardware exception vectors for powerpc are in exception.h
 */

#ifndef	__ASSEMBLER__

#include <mach/thread_status.h>
#include <mach/boolean.h>
#include <ppc/thread.h>

extern void			doexception(int exc, int code, int sub, thread_t th);

extern void			thread_exception_return(void);

extern boolean_t 		alignment(unsigned long dsisr,
					  unsigned long dar,
					  struct ppc_saved_state *ssp);

extern struct ppc_saved_state*	trap(int trapno,
				     struct ppc_saved_state *ss,
				     unsigned int dsisr,
				     unsigned int dar);

extern struct ppc_saved_state* interrupt(int intno,
					 struct ppc_saved_state *ss,
					 unsigned int dsisr,
					 unsigned int dar);

extern int			syscall_error(int exception,
					      int code,
					      int subcode,
					      struct ppc_saved_state *ss);


extern int			procitab(unsigned, void (*)(int), int);

#endif	/* __ASSEMBLER__ */

#endif	/* _PPC_TRAP_H_ */
