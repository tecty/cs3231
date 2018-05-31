/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
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

#ifndef _VM_H_
#define _VM_H_

/*
 * VM system-related definitions.
 *
 * You'll probably want to add stuff here.
 */

#define PAGE_BITS 12

//the structure of frame table entry
struct frame_table_entry {
    unsigned int stat;
    vaddr_t mem_addr;
    struct frame_table_entry * next;
};
//frame table entry pointer
typedef struct frame_table_entry * frame_table;

//the structure of free frame list
struct free_frame_list {
    frame_table start;
    frame_table end;
    long count;
};

//the structure of page table entry
struct page_table_entry{
        uint32_t pid; //process identifier
        vaddr_t page_no; //page number
        paddr_t frame_no; //frame number
        uint32_t stat; //status
        struct page_table_entry * next; //link to handle collisions
};

//page table entry pointer
typedef struct page_table_entry *page_table;

//the global page table
//initialize later with ft
page_table hpt;
//the size of page table 
size_t hpt_size;

void reset_hpt(void);

#include <machine/vm.h>

/* Fault-type arguments to vm_fault() */
#define VM_FAULT_READ        0    /* A read was attempted */
#define VM_FAULT_WRITE       1    /* A write was attempted */
#define VM_FAULT_READONLY    2    /* A write to a readonly page was attempted*/


/* Initialization function */
void vm_bootstrap(void);

/* Fault handling function called by trap code */
int vm_fault(int faulttype, vaddr_t faultaddress);

/* Allocate/free kernel heap pages (called by kmalloc/kfree) */
vaddr_t alloc_kpages(unsigned npages);
void free_kpages(vaddr_t addr);
vaddr_t find_next_free_frame(void);
void clean_memory(vaddr_t mem);

/* TLB shootdown handling called from interprocessor_interrupt */
void vm_tlbshootdown(const struct tlbshootdown *);


#endif /* _VM_H_ */
