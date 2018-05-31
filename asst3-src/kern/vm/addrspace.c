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

        // as->start_region = NULL;
        // as->heap_start = 0;
        // as->heap_end = 0;
        return as;
}

int
as_copy(struct addrspace *old, struct addrspace **ret)
{
        struct addrspace *newas;
        old = old;
        newas = as_create();
        if (newas==NULL) {
                return ENOMEM;
        }

        /*
         * Write this.
         */

        // struct region *region_orig_ptr = old->start_region;
        // struct region *region_copy_ptr = ret->start_region;

        // while(region_orig_ptr!=NULL){
        //         //malloc the copy's own memory space
        //         //at the head of all regions
        //         if(region_copy_ptr==NULL){
        //                 region_copy_ptr = kmalloc(sizeof(struct region));
        //                 if(region_copy_ptr==NULL){
        //                         return ENOMEM;
        //                 }
        //         }
        //         //not at the head, current pointer already have memory
        //         //move to the next pointer
        //         else{
        //                 region_copy_ptr->next = kmalloc(sizeof(struct region));
        //                 if(region_copy_ptr->next==NULL){
        //                         return ENOMEM;
        //                 }
        //                 region_copy_ptr = region_copy_ptr->next;
        //         }
        //         //do the full copy
        //         region_copy_ptr->mem_addr = region_orig_ptr->mem_addr;
        //         region_copy_ptr->size = region_orig_ptr->size;
        //         region_copy_ptr->npages = region_orig_ptr->npages;
        //         region_copy_ptr->read = region_orig_ptr->read;
        //         region_copy_ptr->writ = region_orig_ptr->writ;
        //         region_copy_ptr->exec = region_orig_ptr->exec;
        //         region_copy_ptr->next = NULL; //this is not assigned yet

        //         //iterate
        //         region_orig_ptr = region_orig_ptr->next;              
        // }

        // newas->heap_start = old->heap_start;
        // newas->heap_end = old->heap_end;

        *ret = newas;
        return 0;
}

void
as_destroy(struct addrspace *as)
{
        /*
         * Clean up as needed.
         */
        //clean all region malloc'd
        // struct region *reg_ptr = as->start_region;
        // struct region *prev_reg_ptr = NULL;
        // while(reg_ptr!=NULL){
        //         prev_reg_ptr = reg_ptr;
        //         reg_ptr = reg_ptr->next;
        //         //wipe up all connections in the page table
        //         size_t num = 0;
        //         while(num < npages){
                        
        //         }
        // }
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
}

void
as_deactivate(void)
{
        /*
         * Write this. For many designs it won't need to actually do
         * anything. See proc.c for an explanation of why it (might)
         * be needed.
         */
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

        (void)as;
        (void)vaddr;
        (void)memsize;
        (void)readable;
        (void)writeable;
        (void)executable;
        return ENOSYS; /* Unimplemented */
}

int
as_prepare_load(struct addrspace *as)
{
        /*
         * Write this.
         */

        (void)as;
        return 0;
}

int
as_complete_load(struct addrspace *as)
{
        /*
         * Write this.
         */

        (void)as;
        return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
        /*
         * Write this.
         */

        (void)as;

        /* Initial user-level stack pointer */
        *stackptr = USERSTACK;

        return 0;
}

