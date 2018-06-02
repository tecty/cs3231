#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <addrspace.h>
#include <vm.h>
#include <machine/tlb.h>

/* Place your page table functions here */


void vm_bootstrap(void)
{
    /* Initialise VM sub-system.  You probably want to initialise your 
        frame table here as well.
    */
    // get the first page location
    frame_start = ram_getfirstfree();
    //assign the framtable to the start of this address
    frame_table = (struct frame_slot *)frame_start;

    // calculate how many frame
    frame_count = (ram_getsize -frame_start)/PAGE_SIZE;
    // allocate the framtable's data structure 
    int frame_table_sizes_in_count =
        (frame_count*sizeof(struct frame_slot))/PAGE_SIZE  +1 ;
    frame_start += frame_table_sizes_in_count * PAGE_SIZE; 

    // recalculate the count of frames 
    frame_count -= frame_table_sizes_in_count;

    // initialize the frame table by mark all the palce iis free 
    for(unsigned int i = 0; i < frame_count; i++)
    {
        frame_table[i].status = FREE; 
    }
    // current free is 0
    next_free_frame_id = 0;     
}

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
    (void) faulttype;
    (void) faultaddress;

    panic("vm_fault hasn't been written yet\n");

    return EFAULT;
}

/*
 *
 * SMP-specific functions.  Unused in our configuration.
 */

void
vm_tlbshootdown(const struct tlbshootdown *ts)
{
    (void)ts;
    panic("vm tried to do tlb shootdown?!\n");
}

