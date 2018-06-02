/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *        The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <spl.h>
#include <spinlock.h>
#include <current.h>
#include <mips/tlb.h>
#include <addrspace.h>
#include <vm.h>
#include <proc.h>

/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 *
 * UNSW: If you use ASST3 config as required, then this file forms
 * part of the VM subsystem.
 *
 */

//global lock used in addrspace
static struct spinlock as_lock;

struct addrspace *
as_create(void)
{
	struct addrspace *as;

	as = kmalloc(sizeof(struct addrspace));
	if (as == NULL) {
		return NULL;
	}

	/*
	* Initialize as needed.
	*/
	as->regions = NULL;

	return as;
}

//as_copy creates a new space to store the data manipulated by ret
//and also add new pid/vaddr/paddr links to hpt
int
as_copy(struct addrspace *old, struct addrspace **ret)
{
	struct addrspace *newas;

	newas = as_create();
	if (newas==NULL) {
		return ENOMEM;
	}

	/*
	* Write this.
	*/

	//first copy the region vaddr in addrspace
	//set up iterators
	struct region *ptr = old->regions;
	struct region *new_regions = newas->regions;

	//copy region
	while(ptr!=NULL){
		//build up a new region
		struct region *new_reg = kmalloc(sizeof(struct region));
		//check if kmalloc success
		if(new_reg==NULL){
			//if not success, we cannot build the whole new addrspace
			as_destroy(newas);
			return ENOMEM;
		}
		//copy data
		new_reg->vbase = ptr->vbase;
		new_reg->size = ptr->size;
		new_reg->npages = ptr->npages;
		new_reg->readable = ptr->readable;
		new_reg->writeable = ptr->writeable;
		new_reg->executable = ptr->executable;
		//original records
		new_reg->orig_writeable = ptr->orig_writeable;
		//reset next 
		new_reg->next = NULL;
		//put the new region into the new addrspace
		if(new_regions == NULL){
			//at the head of the region list in newas
			newas->regions = new_reg;
			//relocate new_regions
			new_regions = newas->regions;
		}
		else{
			//not at the head, just add new_reg at the tail of the region list
			new_regions->next = new_reg;
		}

		//iterate
		if(new_regions!=newas->regions){ //move to the next region
			new_regions = new_regions->next;
		}
		ptr = ptr->next;
	}

	//create space for all deep copies of pid/page_no/frame_no from old as
	//now add corresponding new page-frame matching in hpt
	spinlock_acquire(&as_lock);
	int res = hpt_copy(old, newas);
	spinlock_release(&as_lock);

	if(res){
		as_destroy(newas);
		return ENOMEM;
	}

	*ret = newas;
	return 0;
}

void
as_destroy(struct addrspace *as)
{
	/*
	* Clean up as needed.
	*/
	//get the pid of current addrspace
	uint32_t pid = (uint32_t)as;
	//clean up all page tables under this pid
	uint32_t i;
	for(i = 0; i < hpt_size; i++){
		if(hpt[i].pid==pid){
			//set this block invalid
			hpt[i].frame_no = hpt[i].frame_no & ~TLBLO_VALID;
			//check if needs to free the linked frame
			// spinlock_acquire(&as_lock);
			clean_frame(hpt[i].frame_no);
			// spinlock_release(&as_lock);
		}
	}
	kfree(as);
}

void
as_activate(void)
{
	struct addrspace *as;

	as = proc_getas();
	if (as == NULL) {
		/*
		* Kernel thread without an address space; leave the
		* prior address space in place.
		*/
		return;
	}

	/*
	* Write this.
	*/
	//disable interrupts on this CPU while flushing the TLB
	int spl = splhigh();
	vm_tlbflush();
	splx(spl);
}

void
as_deactivate(void)
{
	/*
	* Write this. For many designs it won't need to actually do
	* anything. See proc.c for an explanation of why it (might)
	* be needed.
	*/
	//we still want to reflush TLB
	//so we just simply call as_activate again
	as_activate();
}

/*
 * Set up a segment at virtual address VADDR of size MEMSIZE. The
 * segment in memory extends from VADDR up to (but not including)
 * VADDR+MEMSIZE.
 *
 * The READABLE, WRITEABLE, and EXECUTABLE flags are set if read,
 * write, or execute permission should be set on the segment. At the
 * moment, these are ignored. When you write the VM system, you may
 * want to implement them.
 */
int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t memsize,
                 int readable, int writeable, int executable)
{
	/*
		* Write this.
		*/

	//decide how many pages are needed for this region
	size_t npages;

	//align the region starting by building the base addr
	memsize += vaddr & ~(vaddr_t)PAGE_FRAME;
	vaddr &= PAGE_FRAME;

	//and build until the end of its mem end
	memsize = (memsize + PAGE_SIZE - 1) & PAGE_FRAME;

	//now find the number of pages
	npages = memsize / PAGE_SIZE;

	//kmalloc a new space for the new region
	struct region *reg = kmalloc(sizeof(struct region));
	if(reg==NULL){
		return ENOMEM;
	}
	//input detail of reg
	reg->vbase = vaddr;
	reg->size = memsize;
	reg->npages = npages;
	reg->readable = readable;
	reg->writeable = writeable;
	reg->executable = executable;
	reg->next = NULL;

	//record writing permission again
	reg->orig_writeable = writeable;

	//add the new kmalloc'd region into as
	if(as->regions == NULL){
		as->regions = reg; //at the head of region list
	}
	else{
		//not at the head, find the end of the right region
		struct region * cur_reg = as->regions;
		struct region * prev_reg = as->regions;

		while(cur_reg != NULL && cur_reg->vbase < reg->vbase){
			//iterate to find the point
			prev_reg = cur_reg;
			cur_reg = cur_reg->next;
		}

		//locate the correct point
		prev_reg->next = reg;
	}
	return 0; //success adding and return
}

//as_prepare_load will set all regions to be writable temporarily
//so that loadelf can add data into each region later
int
as_prepare_load(struct addrspace *as)
{
	/*
	* Write this.
	*/
	struct region *cur_reg = as->regions;

	while(cur_reg!=NULL){
		cur_reg->writeable = 1;
		cur_reg = cur_reg->next;
	}

	return 0;
}

//After as_prepare_load completes and loadelf loads the data, turn all 
//read-only regions back to read-only stat
int
as_complete_load(struct addrspace *as)
{
	/*
	* Write this.
	*/

	struct region *cur_reg = as->regions;
	while(cur_reg!=NULL){
		cur_reg->writeable = cur_reg->orig_writeable;
		cur_reg = cur_reg->next;
	}

	//flush TLB again in case it still caches the wrong information of regions
	//always set up disable when flushing TLB
	int spl = splhigh();
	vm_tlbflush();
	splx(spl);

	return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	/*
	* Write this.
	*/

	//we use as_define_region to define each user's stack
	//the only difference is that no other users' regions will go above 
	//this region

	int res = as_define_region(as, USERSTACK - USERSTACKSIZE, USERSTACKSIZE, 
				1, 1, 1);

	//check if define succussful
	if(res){
		return res;
	}

	/* Initial user-level stack pointer */
	*stackptr = USERSTACK;

	return 0;
}

