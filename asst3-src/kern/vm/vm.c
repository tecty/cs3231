#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <addrspace.h>
#include <vm.h>
#include <machine/tlb.h>


/* Place your page table functions here */
//lock for protecting the global resource pt
static struct spinlock pt_lock = SPINLOCK_INITIALIZER;

size_t page_entry_size = sizeof(struct page_table_entry);

//function for reset all page/frame linking
void reset_hpt(void){
        KASSERT(hpt!=0); //check the validity of pt
        // KASSERT(total_page_num!=0); //check the validity of total page number
        unsigned int num = 0;
        while(num < hpt_size){
			//clean
			spinlock_acquire(&pt_lock);
			hpt[num].next = NULL;
			hpt[num].stat = 0;
			spinlock_release(&pt_lock);
			//iterate
			num = num + 1;
        }
}


void vm_bootstrap(void)
{
        /* Initialise VM sub-system.  You probably want to initialise your 
           frame table here as well.
        */
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

