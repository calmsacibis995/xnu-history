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
 * Copyright (c) 1993-1987 Carnegie Mellon University
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
 *	File:	vm/vm_object.c
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Virtual memory object module.
 */

#include <norma_vm.h>
#include <mach_pagemap.h>

#if	NORMA_VM
#include <norma/xmm_server_rename.h>
#endif	/* NORMA_VM */

#include <mach/memory_object.h>
#include <mach/memory_object_default.h>
#include <mach/memory_object_user.h>
#include <mach/vm_param.h>
#include <ipc/ipc_port.h>
#include <ipc/ipc_space.h>
#include <kern/assert.h>
#include <kern/lock.h>
#include <kern/queue.h>
#include <kern/xpr.h>
#include <kern/zalloc.h>
#include <vm/memory_object.h>
#include <vm/vm_fault.h>
#include <vm/vm_map.h>
#include <vm/vm_object.h>
#include <vm/vm_page.h>
#include <vm/vm_pageout.h>

#include <mach/vm_policy.h>


void memory_object_release(
	ipc_port_t	pager,
	pager_request_t	pager_request,
	ipc_port_t	pager_name); /* forward */

void vm_object_deactivate_pages(vm_object_t);

/*
 *	Virtual memory objects maintain the actual data
 *	associated with allocated virtual memory.  A given
 *	page of memory exists within exactly one object.
 *
 *	An object is only deallocated when all "references"
 *	are given up.  Only one "reference" to a given
 *	region of an object should be writeable.
 *
 *	Associated with each object is a list of all resident
 *	memory pages belonging to that object; this list is
 *	maintained by the "vm_page" module, but locked by the object's
 *	lock.
 *
 *	Each object also records a "pager" routine which is
 *	used to retrieve (and store) pages to the proper backing
 *	storage.  In addition, objects may be backed by other
 *	objects from which they were virtual-copied.
 *
 *	The only items within the object structure which are
 *	modified after time of creation are:
 *		reference count		locked by object's lock
 *		pager routine		locked by object's lock
 *
 */

struct zone	*vm_object_zone;	/* vm backing store zone */

struct vm_object	kernel_object_store, vm_submap_object_store;
#define	VM_OBJECT_HASH_COUNT		1024
#define	VM_OBJECT_MEMORY_UNIT		(1024 * 1024)
#define	VM_OBJECTS_PER_MEMORY_UNIT	50
int		vm_cache_max;		/* set in vm_object_init */

queue_head_t	vm_object_hashtable[VM_OBJECT_HASH_COUNT];
struct zone	*object_hash_zone;

struct vm_object_hash_entry {
	queue_chain_t		hash_links;	/* hash chain links */
	vm_object_t		object;		/* object we represent */
};

typedef struct vm_object_hash_entry	*vm_object_hash_entry_t;

queue_head_t	vm_object_cached_list;	/* list of objects persisting */
int		vm_object_cached;	/* size of cached list */
simple_lock_data_t	vm_cache_lock;	/* lock for object cache */

queue_head_t	vm_object_list;		/* list of allocated objects */
long		vm_object_count;	/* count of all objects */
simple_lock_data_t	vm_object_list_lock;
					/* lock for object list and count */

vm_object_t	kernel_object;		/* the single kernel object */

#define	vm_object_cache_lock()		simple_lock(&vm_cache_lock)
#define	vm_object_cache_unlock()	simple_unlock(&vm_cache_lock)

struct vm_object	vm_object_template;

long	object_collapses = 0;
long	object_bypasses  = 0;

/*
 *	vm_object_init:
 *
 *	Initialize the VM objects module.
 */
void vm_object_init()
{
	register int	i;

	vm_object_zone = zinit((vm_size_t) sizeof(struct vm_object),
				round_page(512*1024),
				0, FALSE, "objects");

	object_hash_zone = zinit(
				(vm_size_t)sizeof(struct vm_object_hash_entry),
				100*1024, 0, FALSE,
				"object hash zone");

	queue_init(&vm_object_cached_list);
	queue_init(&vm_object_list);
	vm_object_count = 0;
	simple_lock_init(&vm_cache_lock);
	simple_lock_init(&vm_object_list_lock);

	for (i = 0; i < VM_OBJECT_HASH_COUNT; i++)
		queue_init(&vm_object_hashtable[i]);

	/*
	 *  Parameterize the size of the cache based on the amount of
	 *  physical memory available.
	 */
	vm_cache_max = mem_size / VM_OBJECT_MEMORY_UNIT;
	vm_cache_max *= VM_OBJECTS_PER_MEMORY_UNIT;
	 	
	if (vm_cache_max > 2500)
		vm_cache_max = 2500;

	/*
	 *	Fill in a template object, for quick initialization
	 */

	vm_object_template.ref_count = 1;
	vm_object_template.resident_page_count = 0;
	vm_object_template.size = 0;
	vm_object_template.can_persist = FALSE;
	vm_object_template.paging_in_progress = 0;
	vm_object_template.copy = VM_OBJECT_NULL;

	vm_object_template.pager = vm_pager_null;
	vm_object_template.pager_request = PORT_NULL;
	vm_object_template.pager_name = PORT_NULL;
	vm_object_template.pager_ready = FALSE;
	vm_object_template.pager_creating = FALSE;
	vm_object_template.internal = TRUE;
	vm_object_template.paging_offset = 0;
	vm_object_template.shadow = VM_OBJECT_NULL;
	vm_object_template.shadow_offset = (vm_offset_t) 0;
	vm_object_template.last_alloc = (vm_offset_t) 0;
	vm_object_template.policy = (vm_offset_t) VM_POLICY_SEQ_DEACTIVATE;

	/*
	 *	Initialize the "kernel object"
	 */

	kernel_object = &kernel_object_store;
	_vm_object_allocate(VM_MAX_KERNEL_ADDRESS - VM_MIN_KERNEL_ADDRESS,
			kernel_object);

	/*
	 *	Initialize the "submap object".  Make it as large as the
	 *	kernel object so that no limit is imposed on submap sizes.
	 */

	vm_submap_object = &vm_submap_object_store;
	_vm_object_allocate(VM_MAX_KERNEL_ADDRESS - VM_MIN_KERNEL_ADDRESS,
			vm_submap_object);
}

/*
 *	vm_object_allocate:
 *
 *	Returns a new object with the given size.
 */

vm_object_t vm_object_allocate(size)
	vm_size_t	size;
{
	register vm_object_t	result;

	result = (vm_object_t) zalloc(vm_object_zone);

	_vm_object_allocate(size, result);

	return(result);
}

_vm_object_allocate(size, object)
	vm_size_t		size;
	register vm_object_t	object;
{
	(*object) = vm_object_template;
	queue_init(&object->memq);
	vm_object_lock_init(object);
	object->size = size;
	simple_lock(&vm_object_list_lock);
	queue_enter(&vm_object_list, object, vm_object_t, object_list);
	vm_object_count++;
	simple_unlock(&vm_object_list_lock);
}

/*
 *	vm_object_reference:
 *
 *	Gets another reference to the given object.
 */
void vm_object_reference(object)
	register vm_object_t	object;
{
	if (object == VM_OBJECT_NULL)
		return;

	vm_object_lock(object);
	object->ref_count++;
	vm_object_unlock(object);
}

/*
 *	vm_object_deallocate:
 *
 *	Release a reference to the specified object,
 *	gained either through a vm_object_allocate
 *	or a vm_object_reference call.  When all references
 *	are gone, storage associated with this object
 *	may be relinquished.
 *
 *	No object may be locked.
 */
void vm_object_deallocate(object)
	register vm_object_t	object;
{
	vm_object_t	temp;

	while (object != VM_OBJECT_NULL) {

		/*
		 *	The cache holds a reference (uncounted) to
		 *	the object; we must lock it before removing
		 *	the object.
		 */

		vm_object_cache_lock();

		/*
		 *	Lose the reference
		 */
		vm_object_lock(object);
		if (--(object->ref_count) != 0) {

			/*
			 *	If there are still references, then
			 *	we are done.
			 */
			vm_object_unlock(object);
			vm_object_cache_unlock();
			return;
		}

		/*
		 *	See if this object can persist.  If so, enter
		 *	it in the cache, then deactivate all of its
		 *	pages.
		 */

		if (object->can_persist) {
			
			if (object->resident_page_count > 0) {
				queue_enter(&vm_object_cached_list, object,
					vm_object_t, cached_list);
				vm_object_cached++;
				vm_object_cache_unlock();
	
				vm_object_deactivate_pages(object);
				vm_object_unlock(object);
	
				vm_object_cache_trim();
				return;
			}
			else {
				object->can_persist = FALSE;
			}
		}

		/*
		 *	Make sure no one can look us up now.
		 */
		vm_object_remove(object->pager);
		vm_object_cache_unlock();

		temp = object->shadow;
		vm_object_terminate(object);
			/* unlocks and deallocates object */
		object = temp;
	}
}


#define		vm_object_pager_terminate(object) {			\
	if ((object)->pager != vm_pager_null) {				\
		port_release((object)->pager);				\
		(object)->pager = vm_pager_null;			\
	}								\
	if ((object)->pager_request != PORT_NULL) {			\
		port_deallocate(kernel_task, (object)->pager_request);	\
		(object)->pager_request = PORT_NULL;			\
	}								\
	if ((object)->pager_name != PORT_NULL) {			\
		port_deallocate(kernel_task, (object)->pager_name);	\
		(object)->pager_name = PORT_NULL;			\
	}								\
}

/*
 *	vm_object_terminate actually destroys the specified object, freeing
 *	up all previously used resources.
 *
 *	The object must be locked.
 */
void vm_object_terminate(object)
	register vm_object_t	object;
{
	register vm_page_t	p;
	vm_page_t		next_p;
	vm_object_t		shadow_object;

	/*
	 *	Detach the object from its shadow if we are the shadow's
	 *	copy.
	 */
	if ((shadow_object = object->shadow) != VM_OBJECT_NULL) {
		vm_object_lock(shadow_object);
		if (shadow_object->copy == object)
			shadow_object->copy = VM_OBJECT_NULL;
		else if (shadow_object->copy != VM_OBJECT_NULL)
			panic("vm_object_terminate: copy/shadow inconsistency");
		vm_object_unlock(shadow_object);
	}

	/*
	 *	Wait until the pageout daemon is through
	 *	with the object.
	 */

	while (object->paging_in_progress != 0) {
		vm_object_sleep(object, object, FALSE);
		vm_object_lock(object);
	}


	/*
	 *	While the paging system is locked,
	 *	pull the object's pages off the active
	 *	and inactive queues.  This keeps the
	 *	pageout daemon from playing with them
	 *	during vm_pager_deallocate.
	 *
	 *	We can't free the pages yet, because the
	 *	object's pager may have to write them out
	 *	before deallocating the paging space.
	 */

	p = (vm_page_t) queue_first(&object->memq);
	while (!queue_end(&object->memq, (queue_entry_t) p)) {
		VM_PAGE_CHECK(p);

		vm_page_lock_queues();
		if (p->active) {
			queue_remove(&vm_page_queue_active, p, vm_page_t,
						pageq);
			p->active = FALSE;
			vm_page_active_count--;
		}

		if (p->inactive) {
			queue_remove(&vm_page_queue_inactive, p, vm_page_t,
						pageq);
			p->inactive = FALSE;
			vm_page_inactive_count--;
		}
		/*
		 *	If we are on the free list just free ourselves now to make sure
		 *	noone else finds out about this page.
		 */
		next_p = (vm_page_t) queue_next(&p->listq);
		if (p->free) {
			vm_page_free(p);
		}
		vm_page_unlock_queues();
		p = next_p;
	}
				
	vm_object_unlock(object);

	/*
	 *	Let the pager know object is dead.
	 */

	if (object->pager != vm_pager_null)
		vm_pager_deallocate(object->pager);

	if (object->paging_in_progress != 0)
		panic("vm_object_deallocate: pageout in progress");

	/*
	 *	Free physical page resources.  All references to the object
	 *	are gone, so we don't need to lock it.
	 */

		while (!queue_empty(&object->memq)) {
			p = (vm_page_t) queue_first(&object->memq);

			VM_PAGE_CHECK(p);

			vm_page_lock_queues();
			vm_page_free(p);
			vm_page_unlock_queues();
		}

	simple_lock(&vm_object_list_lock);
	queue_remove(&vm_object_list, object, vm_object_t, object_list);
	vm_object_count--;
	simple_unlock(&vm_object_list_lock);

	/*
	 *	Free the space for the object.
	 */

	zfree(vm_object_zone, (vm_offset_t) object);
}

void vm_object_destroy(pager)
	port_t		pager;
{
	register
	vm_object_t	object = vm_object_lookup(pager);

	if (object == VM_OBJECT_NULL)
		return;

	vm_object_deallocate(object);
}

/*
 *	_vm_object_deactivate_pages
 *
 *	Internal routine to deactivate pages in the specified object.
 *	(Keep its pages in memory even though it is no longer referenced.)
 *
 *	The object must be locked.
 */
static void _vm_object_deactivate_pages(object, age)
	register vm_object_t	object;
	register boolean_t	age;
{
	register vm_page_t	p, next;

	vm_page_lock_queues();
	p = (vm_page_t) queue_first(&object->memq);
	while (!queue_end(&object->memq, (queue_entry_t) p)) {
		next = (vm_page_t) queue_next(&p->listq);
		if (!p->inactive) {
			if (age)
				vm_page_deactivate_first(p);
			else
				vm_page_deactivate(p);
		}
		p = next;
	}
	vm_page_unlock_queues();
}

/*
 *	vm_object_deactivate_pages
 *
 *	Deactivate pages in the specified object.
 *	(Keep its pages in memory even though it is no longer referenced.)
 *
 *	The object must be locked.
 */
void vm_object_deactivate_pages(object)
	register vm_object_t	object;
{
	_vm_object_deactivate_pages(object, FALSE);
}

/*
 *	vm_object_deactivate_pages_first
 *
 *	Deactivate pages in the specified object. Age them.
 *	(Keep its pages in memory even though it is no longer referenced.)
 *
 *	The object must be locked.
 */
void vm_object_deactivate_pages_first(object)
	register vm_object_t	object;
{
	_vm_object_deactivate_pages(object, TRUE);
}

/*
 *	Trim the object cache to size.
 */
vm_object_cache_trim()
{
	register vm_object_t	object;

	vm_object_cache_lock();
	while (vm_object_cached > vm_cache_max) {
		object = (vm_object_t) queue_first(&vm_object_cached_list);
		vm_object_cache_unlock();

		if (object != vm_object_lookup(object->pager))
			panic("vm_object_cache_trim: I'm sooo confused.");
		vm_object_cache_object(object, FALSE);

		vm_object_cache_lock();
	}
	vm_object_cache_unlock();
}

/*
* 
*/
kern_return_t
adjust_vm_object_cache(vm_size_t oldval, vm_size_t newval)
{
	vm_cache_max = newval;
	vm_object_cache_trim();
	return(KERN_SUCCESS);	
}

kern_return_t vm_object_cache_object(object, should_cache)
	vm_object_t	object;
	boolean_t	should_cache;
{
	if (object == VM_OBJECT_NULL)
		return(KERN_INVALID_ARGUMENT);

	vm_object_cache_lock();
	vm_object_lock(object);
	object->can_persist = should_cache;
	vm_object_unlock(object);
	vm_object_cache_unlock();

	vm_object_deallocate(object);

	return(KERN_SUCCESS);
}


/*
 *	vm_object_shutdown()
 *
 *	Shut down the object system.  Unfortunately, while we
 *	may be trying to do this, init is happily waiting for
 *	processes to exit, and therefore will be causing some objects
 *	to be deallocated.  To handle this, we gain a fake reference
 *	to all objects we release paging areas for.  This will prevent
 *	a duplicate deallocation.  This routine is probably full of
 *	race conditions!
 */

void vm_object_shutdown()
{
	/*
	 *	Clean up the object cache *before* we screw up the reference
	 *	counts on all of the objects.
	 */

	vm_object_cache_clear();
}

/*
 *	vm_object_pmap_copy:
 *
 *	Makes all physical pages in the specified
 *	object range copy-on-write.  No writeable
 *	references to these pages should remain.
 *
 *	The object must *not* be locked.
 */
void vm_object_pmap_copy(object, start, end)
	register vm_object_t	object;
	register vm_offset_t	start;
	register vm_offset_t	end;
{
	register vm_page_t	p;

	if (object == VM_OBJECT_NULL)
		return;

	vm_object_lock(object);
	p = (vm_page_t) queue_first(&object->memq);
	while (!queue_end(&object->memq, (queue_entry_t) p)) {
		if ((start <= p->offset) && (p->offset < end)) {
			if (!p->copy_on_write) {
				pmap_copy_on_write(VM_PAGE_TO_PHYS(p));
				p->copy_on_write = TRUE;
			}
		}
		p = (vm_page_t) queue_next(&p->listq);
	}
	vm_object_unlock(object);
}

/*
 *	vm_object_pmap_remove:
 *
 *	Removes all physical pages in the specified
 *	object range from all physical maps.
 *
 *	The object must *not* be locked.
 */
void vm_object_pmap_remove(object, start, end)
	register vm_object_t	object;
	register vm_offset_t	start;
	register vm_offset_t	end;
{
	register vm_page_t	p;

	if (object == VM_OBJECT_NULL)
		return;

	vm_object_lock(object);
	p = (vm_page_t) queue_first(&object->memq);
	while (!queue_end(&object->memq, (queue_entry_t) p)) {
		if ((start <= p->offset) && (p->offset < end)) {
			pmap_remove_all(VM_PAGE_TO_PHYS(p));
		}
		p = (vm_page_t) queue_next(&p->listq);
	}
	vm_object_unlock(object);
}

/*
 *	vm_object_copy:
 *
 *	Create a new object which is a copy of an existing
 *	object, and mark all of the pages in the existing
 *	object 'copy-on-write'.  The new object has one reference.
 *	Returns the new object.
 *
 *	May defer the copy until later if the object is not backed
 *	up by a non-default pager.
 */
void vm_object_copy(src_object, src_offset, size,
		    dst_object, dst_offset, src_needs_copy)
	register vm_object_t	src_object;
	vm_offset_t		src_offset;
	vm_size_t		size;
	vm_object_t		*dst_object;	/* OUT */
	vm_offset_t		*dst_offset;	/* OUT */
	boolean_t		*src_needs_copy;	/* OUT */
{
	register vm_object_t	new_copy;
	register vm_object_t	old_copy;
	vm_offset_t		new_start, new_end;

	register vm_page_t	p;

	if (src_object == VM_OBJECT_NULL) {
		/*
		 *	Nothing to copy
		 */
		*dst_object = VM_OBJECT_NULL;
		*dst_offset = 0;
		*src_needs_copy = FALSE;
		return;
	}

	/*
	 *	If the object's pager is null_pager or the
	 *	default pager, we don't have to make a copy
	 *	of it.  Instead, we set the needs copy flag and
	 *	make a shadow later.
	 */

	vm_object_lock(src_object);
	if (src_object->pager == vm_pager_null ||
	    src_object->internal) {

		/*
		 *	Make another reference to the object
		 */
		src_object->ref_count++;

		/*
		 *	Mark all of the pages copy-on-write.
		 */
		for (p = (vm_page_t) queue_first(&src_object->memq);
		     !queue_end(&src_object->memq, (queue_entry_t)p);
		     p = (vm_page_t) queue_next(&p->listq)) {
			if (src_offset <= p->offset &&
			    p->offset < src_offset + size)
				p->copy_on_write = TRUE;
		}
		vm_object_unlock(src_object);

		*dst_object = src_object;
		*dst_offset = src_offset;
		
		/*
		 *	Must make a shadow when write is desired
		 */
		*src_needs_copy = TRUE;
		return;
	}

	/*
	 *	Try to collapse the object before copying it.
	 */
	vm_object_collapse(src_object);

	/*
	 *	If the object has a pager, the pager wants to
	 *	see all of the changes.  We need a copy-object
	 *	for the changed pages.
	 *
	 *	If there is a copy-object, and it is empty,
	 *	no changes have been made to the object since the
	 *	copy-object was made.  We can use the same copy-
	 *	object.
	 */

    Retry1:
	old_copy = src_object->copy;
	if (old_copy != VM_OBJECT_NULL) {
		/*
		 *	Try to get the locks (out of order)
		 */
		if (!vm_object_lock_try(old_copy)) {
			vm_object_unlock(src_object);

			/* should spin a bit here... */
			vm_object_lock(src_object);
			goto Retry1;
		}

		if (old_copy->resident_page_count == 0 &&
		    old_copy->pager == vm_pager_null) {
			/*
			 *	Return another reference to
			 *	the existing copy-object.
			 */
			old_copy->ref_count++;
			vm_object_unlock(old_copy);
			vm_object_unlock(src_object);
			*dst_object = old_copy;
			*dst_offset = src_offset;
			*src_needs_copy = FALSE;
			return;
		}
		vm_object_unlock(old_copy);
	}
	vm_object_unlock(src_object);

	/*
	 *	If the object has a pager, the pager wants
	 *	to see all of the changes.  We must make
	 *	a copy-object and put the changed pages there.
	 *
	 *	The copy-object is always made large enough to
	 *	completely shadow the original object, since
	 *	it may have several users who want to shadow
	 *	the original object at different points.
	 */

	new_copy = vm_object_allocate(src_object->size);

    Retry2:
	vm_object_lock(src_object);
	/*
	 *	Copy object may have changed while we were unlocked
	 */
	old_copy = src_object->copy;
	if (old_copy != VM_OBJECT_NULL) {
		/*
		 *	Try to get the locks (out of order)
		 */
		if (!vm_object_lock_try(old_copy)) {
			vm_object_unlock(src_object);
			goto Retry2;
		}

		/*
		 *	Consistency check
		 */
		if (old_copy->shadow != src_object ||
		    old_copy->shadow_offset != (vm_offset_t) 0)
			panic("vm_object_copy: copy/shadow inconsistency");

		/*
		 *	Make the old copy-object shadow the new one.
		 *	It will receive no more pages from the original
		 *	object.
		 */

		src_object->ref_count--;	/* remove ref. from old_copy */
		old_copy->shadow = new_copy;
		new_copy->ref_count++;		/* locking not needed - we
						   have the only pointer */
		vm_object_unlock(old_copy);	/* done with old_copy */
	}

	new_start = (vm_offset_t) 0;	/* always shadow original at 0 */
	new_end   = (vm_offset_t) new_copy->size; /* for the whole object */

	/*
	 *	Point the new copy at the existing object.
	 */

	new_copy->shadow = src_object;
	new_copy->shadow_offset = new_start;
	src_object->ref_count++;
	src_object->copy = new_copy;

	/*
	 *	Mark all the affected pages of the existing object
	 *	copy-on-write.
	 */
	p = (vm_page_t) queue_first(&src_object->memq);
	while (!queue_end(&src_object->memq, (queue_entry_t) p)) {
		if ((new_start <= p->offset) && (p->offset < new_end)) {
			p->copy_on_write = TRUE;
		}
		p = (vm_page_t) queue_next(&p->listq);
	}

	vm_object_unlock(src_object);

	*dst_object = new_copy;
	*dst_offset = src_offset - new_start;
	*src_needs_copy = FALSE;
}

/*
 *	vm_object_shadow:
 *
 *	Create a new object which is backed by the
 *	specified existing object range.  The source
 *	object reference is deallocated.
 *
 *	The new object and offset into that object
 *	are returned in the source parameters.
 */

void vm_object_shadow(object, offset, length)
	vm_object_t	*object;	/* IN/OUT */
	vm_offset_t	*offset;	/* IN/OUT */
	vm_size_t	length;
{
	register vm_object_t	source;
	register vm_object_t	result;

	source = *object;

	/*
	 *	Allocate a new object with the given length
	 */

	if ((result = vm_object_allocate(length)) == VM_OBJECT_NULL)
		panic("vm_object_shadow: no object for shadowing");

	/*
	 *	The new object shadows the source object, adding
	 *	a reference to it.  Our caller changes his reference
	 *	to point to the new object, removing a reference to
	 *	the source object.  Net result: no change of reference
	 *	count.
	 */
	result->shadow = source;
	
	/*
	 *	Store the offset into the source object,
	 *	and fix up the offset into the new object.
	 */

	result->shadow_offset = *offset;

	/*
	 *	Return the new things
	 */

	*offset = 0;
	*object = result;
}

/*
 *	Set the specified object's pager to the specified pager.
 */

void vm_object_setpager(object, pager, paging_offset,
			read_only)
	vm_object_t	object;
	vm_pager_t	pager;
	vm_offset_t	paging_offset;
	boolean_t	read_only;
{
#ifdef	lint
	read_only++;	/* No longer used */
#endif	lint

	vm_object_lock(object);			/* XXX ? */
	object->pager = pager;
	object->paging_offset = paging_offset;
	vm_object_unlock(object);			/* XXX ? */
}

/*
 *	vm_object_hash hashes the pager/id pair.
 */
/* Shift for sizeof(struct vm_object_hash_entry) */
#define VM_OBJECT_HASH_SHIFT	8
#define vm_object_hash(pager) \
	((((unsigned)pager) >> VM_OBJECT_HASH_SHIFT) % VM_OBJECT_HASH_COUNT)

/*
 *	vm_object_lookup looks in the object cache for an object with the
 *	specified pager and paging id.
 */

vm_object_t vm_object_lookup(pager)
	vm_pager_t	pager;
{
	register queue_t		bucket;
	register vm_object_hash_entry_t	entry;
	vm_object_t			object;

	bucket = &vm_object_hashtable[vm_object_hash(pager)];

	vm_object_cache_lock();

	entry = (vm_object_hash_entry_t) queue_first(bucket);
	while (!queue_end(bucket, (queue_entry_t) entry)) {
		object = entry->object;
		if (object->pager == pager) {
			vm_object_lock(object);
			if (object->ref_count == 0) {
				queue_remove(&vm_object_cached_list, object,
						vm_object_t, cached_list);
				vm_object_cached--;
			}
			object->ref_count++;
			vm_object_unlock(object);
			vm_object_cache_unlock();
			return(object);
		}
		entry = (vm_object_hash_entry_t) queue_next(&entry->hash_links);
	}

	vm_object_cache_unlock();
	return(VM_OBJECT_NULL);
}

/*
 *	vm_object_enter enters the specified object/pager/id into
 *	the hash table.
 */

void vm_object_enter(object, pager)
	vm_object_t	object;
	vm_pager_t	pager;
{
	register queue_t		bucket;
	register vm_object_hash_entry_t	entry;

	/*
	 *	We don't cache null objects, and we can't cache
	 *	objects with the null pager.
	 */

	if (object == VM_OBJECT_NULL)
		return;
	if (pager == vm_pager_null)
		return;

	bucket = &vm_object_hashtable[vm_object_hash(pager)];
	entry = (vm_object_hash_entry_t) zalloc(object_hash_zone);
	entry->object = object;
	object->can_persist = TRUE;

	vm_object_cache_lock();
	queue_enter(bucket, entry, vm_object_hash_entry_t, hash_links);
	vm_object_cache_unlock();
}

/*
 *	vm_object_remove:
 *
 *	Remove the pager from the hash table.
 *	Note:  This assumes that the object cache
 *	is locked.  XXX this should be fixed
 *	by reorganizing vm_object_deallocate.
 */
vm_object_remove(pager)
	register vm_pager_t	pager;
{
	register queue_t		bucket;
	register vm_object_hash_entry_t	entry;
	register vm_object_t		object;

	bucket = &vm_object_hashtable[vm_object_hash(pager)];

	entry = (vm_object_hash_entry_t) queue_first(bucket);
	while (!queue_end(bucket, (queue_entry_t) entry)) {
		object = entry->object;
		if (object->pager == pager) {
			queue_remove(bucket, entry, vm_object_hash_entry_t,
					hash_links);
			zfree(object_hash_zone, (vm_offset_t) entry);
			break;
		}
		entry = (vm_object_hash_entry_t) queue_next(&entry->hash_links);
	}
}

/*
 *	vm_object_cache_clear removes all objects from the cache.
 *
 */

void vm_object_cache_clear()
{
	register vm_object_t	object;

	/*
	 *	Remove each object in the cache by scanning down the
	 *	list of cached objects.
	 */
	vm_object_cache_lock();
	while (!queue_empty(&vm_object_cached_list)) {
		object = (vm_object_t) queue_first(&vm_object_cached_list);
		vm_object_cache_unlock();

		/* 
		 * Note: it is important that we use vm_object_lookup
		 * to gain a reference, and not vm_object_reference, because
		 * the logic for removing an object from the cache lies in 
		 * lookup.
		 */
		if (object != vm_object_lookup(object->pager))
			panic("vm_object_cache_clear: I'm sooo confused.");
		vm_object_cache_object(object, FALSE);

		vm_object_cache_lock();
	}
	vm_object_cache_unlock();
}

/*
 *	Remove some number of entries from the object cache.
 */
int
vm_object_cache_steal(num)
	register int num;
{
	register vm_object_t	object;
	register int		num_removed = 0;

	vm_object_cache_lock();
	while (num_removed < num && !queue_empty(&vm_object_cached_list)) {
		object = (vm_object_t) queue_first(&vm_object_cached_list);
		vm_object_cache_unlock();

		if (object != vm_object_lookup(object->pager))
			panic("vm_object_cache_steal: I'm sooo confused.");
		vm_object_cache_object(object, FALSE);
		num_removed++;

		vm_object_cache_lock();
	}
	vm_object_cache_unlock();

	return(num_removed);
}

boolean_t	vm_object_collapse_allowed = TRUE;
/*
 *	vm_object_collapse:
 *
 *	Collapse an object with the object backing it.
 *	Pages in the backing object are moved into the
 *	parent, and the backing object is deallocated.
 *
 *	Requires that the object be locked and the page
 *	queues be unlocked.
 *
 */
void vm_object_collapse(object)
	register vm_object_t	object;

{
	register vm_object_t	backing_object;
	register vm_offset_t	backing_offset;
	register vm_size_t	size;
	register vm_offset_t	new_offset;
	register vm_page_t	p, pp;

	if (!vm_object_collapse_allowed)
		return;

	while (TRUE) {
		/*
		 *	Verify that the conditions are right for collapse:
		 *
		 *	The object exists and no pages in it are currently
		 *	being paged out (or have ever been paged out).
		 */
		if (object == VM_OBJECT_NULL ||
		    object->paging_in_progress != 0 ||
		    object->pager != vm_pager_null)
			return;

		/*
		 *		There is a backing object, and
		 */
	
		if ((backing_object = object->shadow) == VM_OBJECT_NULL)
			return;
	
		vm_object_lock(backing_object);
		/*
		 *	...
		 *		The backing object is not read_only,
		 *		and no pages in the backing object are
		 *		currently being paged out.
		 *		The backing object is internal.
		 */
	
		if (!backing_object->internal ||
		    backing_object->paging_in_progress != 0) {
			vm_object_unlock(backing_object);
			return;
		}
	
		/*
		 *	The backing object can't be a copy-object:
		 *	the shadow_offset for the copy-object must stay
		 *	as 0.  Furthermore (for the 'we have all the
		 *	pages' case), if we bypass backing_object and
		 *	just shadow the next object in the chain, old
		 *	pages from that object would then have to be copied
		 *	BOTH into the (former) backing_object and into the
		 *	parent object.
		 */
		if (backing_object->shadow != VM_OBJECT_NULL &&
		    backing_object->shadow->copy != VM_OBJECT_NULL) {
			vm_object_unlock(backing_object);
			return;
		}

		/*
		 *	We know that we can either collapse the backing
		 *	object (if the parent is the only reference to
		 *	it) or (perhaps) remove the parent's reference
		 *	to it.
		 */

		backing_offset = object->shadow_offset;
		size = object->size;

		/*
		 *	If there is exactly one reference to the backing
		 *	object, we can collapse it into the parent.
		 */
	
		if (backing_object->ref_count == 1) {

			/*
			 *	We can collapse the backing object.
			 *
			 *	Move all in-memory pages from backing_object
			 *	to the parent.  Pages that have been paged out
			 *	will be overwritten by any of the parent's
			 *	pages that shadow them.
			 */

			while (!queue_empty(&backing_object->memq)) {

				p = (vm_page_t)
					queue_first(&backing_object->memq);

				new_offset = (p->offset - backing_offset);

				/*
				 *	If the parent has a page here, or if
				 *	this page falls outside the parent,
				 *	dispose of it.
				 *
				 *	Otherwise, move it as planned.
				 */

				if (p->offset < backing_offset ||
				    new_offset >= size) {
					vm_page_lock_queues();
					vm_page_free(p);
					vm_page_unlock_queues();
				} else {
				    pp = vm_page_lookup(object, new_offset);
				    if (pp != VM_PAGE_NULL) {
					vm_page_lock_queues();
					vm_page_free(p);
					vm_page_unlock_queues();
				    }
				    else {
					vm_page_rename(p, object, new_offset);
				    }
				}
			}

			/*
			 *	Move the pager from backing_object to object.
			 *
			 *	XXX We're only using part of the paging space
			 *	for keeps now... we ought to discard the
			 *	unused portion.
			 */

			object->pager = backing_object->pager;
			object->paging_offset =
				backing_object->paging_offset + backing_offset;

			backing_object->pager = vm_pager_null;
			backing_object->pager_request = PORT_NULL;
			backing_object->pager_name = PORT_NULL;

			/*
			 *	Object now shadows whatever backing_object did.
			 *	Note that the reference to backing_object->shadow
			 *	moves from within backing_object to within object.
			 */

			object->shadow = backing_object->shadow;
			object->shadow_offset += backing_object->shadow_offset;
			if (object->shadow != VM_OBJECT_NULL &&
			    object->shadow->copy != VM_OBJECT_NULL) {
				panic("vm_object_collapse: we collapsed a copy-object!");
			}
			/*
			 *	Discard backing_object.
			 *
			 *	Since the backing object has no pages, no
			 *	pager left, and no object references within it,
			 *	all that is necessary is to dispose of it.
			 */

			vm_object_unlock(backing_object);

			simple_lock(&vm_object_list_lock);
			queue_remove(&vm_object_list, backing_object,
						vm_object_t, object_list);
			vm_object_count--;
			simple_unlock(&vm_object_list_lock);

			zfree(vm_object_zone, (vm_offset_t) backing_object);

			object_collapses++;
		}
		else {
			/*
			 *	If all of the pages in the backing object are
			 *	shadowed by the parent object, the parent
			 *	object no longer has to shadow the backing
			 *	object; it can shadow the next one in the
			 *	chain.
			 *
			 *	The backing object must not be paged out - we'd
			 *	have to check all of the paged-out pages, as
			 *	well.
			 */

			if (backing_object->pager != vm_pager_null) {
				vm_object_unlock(backing_object);
				return;
			}

			/*
			 *	Should have a check for a 'small' number
			 *	of pages here.
			 */

			p = (vm_page_t) queue_first(&backing_object->memq);
			while (!queue_end(&backing_object->memq,
					  (queue_entry_t) p)) {

				new_offset = (p->offset - backing_offset);

				/*
				 *	If the parent has a page here, or if
				 *	this page falls outside the parent,
				 *	keep going.
				 *
				 *	Otherwise, the backing_object must be
				 *	left in the chain.
				 */

				if (p->offset >= backing_offset &&
				    new_offset <= size &&
				    (pp = vm_page_lookup(object, new_offset))
				      == VM_PAGE_NULL) {
					/*
					 *	Page still needed.
					 *	Can't go any further.
					 */
					vm_object_unlock(backing_object);
					return;
				}
				p = (vm_page_t) queue_next(&p->listq);
			}

			/*
			 *	Make the parent shadow the next object
			 *	in the chain.  Deallocating backing_object
			 *	will not remove it, since its reference
			 *	count is at least 2.
			 */

			vm_object_reference(object->shadow = backing_object->shadow);
			object->shadow_offset += backing_object->shadow_offset;

			/*	Drop the reference count on backing_object.
			 *	Since its ref_count was at least 2, it
			 *	will not vanish; so we don't need to call
			 *	vm_object_deallocate.
			 */
			backing_object->ref_count--;
			vm_object_unlock(backing_object);

			object_bypasses ++;

		}

		/*
		 *	Try again with this object's new backing object.
		 */
	}
}

/*
 *	vm_object_page_remove: [internal]
 *
 *	Removes all physical pages in the specified
 *	object range from the object's list of pages.
 *
 *	The object must be locked.
 */
void vm_object_page_remove(object, start, end)
	register vm_object_t	object;
	register vm_offset_t	start;
	register vm_offset_t	end;
{
	register vm_page_t	p, next;

	if (object == VM_OBJECT_NULL)
		return;

	p = (vm_page_t) queue_first(&object->memq);
	while (!queue_end(&object->memq, (queue_entry_t) p)) {
		next = (vm_page_t) queue_next(&p->listq);
		if ((start <= p->offset) && (p->offset < end)) {
			pmap_remove_all(VM_PAGE_TO_PHYS(p));
			vm_page_lock_queues();
			vm_page_free(p);
			vm_page_unlock_queues();
		}
		p = next;
	}
}

/*
 *	Routine:	vm_object_coalesce
 *	Function:	Coalesces two objects backing up adjoining
 *			regions of memory into a single object.
 *
 *	returns TRUE if objects were combined.
 *
 *	NOTE:	Only works at the moment if the second object is NULL -
 *		if it's not, which object do we lock first?
 *
 *	Parameters:
 *		prev_object	First object to coalesce
 *		prev_offset	Offset into prev_object
 *		next_object	Second object into coalesce
 *		next_offset	Offset into next_object
 *
 *		prev_size	Size of reference to prev_object
 *		next_size	Size of reference to next_object
 *
 *	Conditions:
 *	The object must *not* be locked.
 */
boolean_t vm_object_coalesce(prev_object, next_object,
			prev_offset, next_offset,
			prev_size, next_size)

	register vm_object_t	prev_object;
	vm_object_t	next_object;
	vm_offset_t	prev_offset, next_offset;
	vm_size_t	prev_size, next_size;
{
	vm_size_t	newsize;

#ifdef	lint
	next_offset++;
#endif	lint

	if (next_object != VM_OBJECT_NULL) {
		return(FALSE);
	}

	if (prev_object == VM_OBJECT_NULL) {
		return(TRUE);
	}

	vm_object_lock(prev_object);

	/*
	 *	Try to collapse the object first
	 */
	vm_object_collapse(prev_object);

	/*
	 *	Can't coalesce if:
	 *	. more than one reference
	 *	. paged out
	 *	. shadows another object
	 *	. has a copy elsewhere
	 *	(any of which mean that the pages not mapped to
	 *	prev_entry may be in use anyway)
	 */

	if (prev_object->ref_count > 1 ||
		prev_object->pager != vm_pager_null ||
		prev_object->shadow != VM_OBJECT_NULL ||
		prev_object->copy != VM_OBJECT_NULL) {
		vm_object_unlock(prev_object);
		return(FALSE);
	}

	/*
	 *	Remove any pages that may still be in the object from
	 *	a previous deallocation.
	 */

	vm_object_page_remove(prev_object,
			prev_offset + prev_size,
			prev_offset + prev_size + next_size);

	/*
	 *	Extend the object if necessary.
	 */
	newsize = prev_offset + prev_size + next_size;
	if (newsize > prev_object->size)
		prev_object->size = newsize;

	vm_object_unlock(prev_object);
	return(TRUE);
}

vm_object_t	vm_object_request_object(p)
	port_t		p;
{
#ifdef	lint
	p++;
#endif	lint
	printf("vm_object_request_object: called\n");
	return(VM_OBJECT_NULL);
}

/*
 *	Routine:	vm_object_name
 *	Purpose:
 *		Returns a reference to the "name" port associated
 *		with this object.
 */
port_t		vm_object_name(object)
	vm_object_t	object;
{
	return ((port_t)object);
}

#if	DEBUG
/*
 *	vm_object_print:	[ debug ]
 */
void vm_object_print(object)
	vm_object_t	object;
{
	register vm_page_t	p;
	extern indent;

	register int count;

	if (object == VM_OBJECT_NULL)
		return;

	iprintf("Object 0x%x: size=0x%x, ref=%d, pager=0x%x+0x%x, shadow=(0x%x)+0x%x\n",
		(int) object, (int) object->size, object->ref_count, (int) object->pager,
		(int) object->paging_offset,
		(int) object->shadow, (int) object->shadow_offset);

	indent += 2;

	count = 0;
	p = (vm_page_t) queue_first(&object->memq);
	while (!queue_end(&object->memq, (queue_entry_t) p)) {
		if (count == 0) iprintf("memory:=");
		else if (count == 6) {printf("\n"); iprintf(" ..."); count = 0;}
		else printf(",");
		count++;

		printf("(off=0x%x,page=0x%x)", p->offset, VM_PAGE_TO_PHYS(p));
		p = (vm_page_t) queue_next(&p->listq);
	}
	if (count != 0)
		printf("\n");
	indent -= 2;
}
#endif	DEBUG


