#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <addrspace.h>
#include <vm.h>
#include <machine/tlb.h>


/* Place your page table functions here */
//define the structure of the global page table
struct page_table_entry{
        uint32_t pid; //process identifier
        vaddr_t page_no; //page number
        paddr_t frame_no; //frame number
        uint32_t stat; //status
        struct page_table_entry * next; //link to handle collisions
};

typedef struct page_table_entry *page_table;

//lock for protecting the global resource pt
static struct spinlock pt_lock = SPINLOCK_INITIALIZER;

//the global page table
page_table pt;
//the global value 
size_t hpt_size;
size_t page_entry_size = sizeof(struct page_table_entry);

//function for reset all page/frame linking
void reset_pt(void){
        KASSERT(pt!=0); //check the validity of pt
        // KASSERT(total_page_num!=0); //check the validity of total page number
        unsigned int num = 0;
        while(num < 200){
			//clean
			spinlock_acquire(&pt_lock);
			pt[num].next = 0;
			pt[num].stat = 0;
			spinlock_release(&pt_lock);
			//iterate
			num = num + 1;
        }
}

// //function that find the mapping between addrspace's vaddr and used paddr
// uint32_t hpt_hash(struct addrspace *as, vaddr_t faultaddr){
// 	uint32_t index;

// 	index = (((uint32_t)as) ^ (faultaddr>>PAGE_BITS)) % hpt_size;
// 	return index;
// }

// struct page_table_entry *match_hpt(uint32_t index, uint32_t pid, vaddr_t page_no){
// 	struct page_table_entry *match = &pt[index]; //set result to be index by default
// 	//use internal chaining to find the right entry
// 	while(match!=NULL || match->pid!=pid || match->page_no!=page_no){
// 		match = match->next;
// 	}
// 	//quit the loop
// 	return match;
// }

// //remove vaddr/paddr bindings
// void remove_paddr(uint32_t index){
// 	spinlock_acquire(&pt_lock);
	
// 	spinlock_release(&pt_lock);
// }

void vm_bootstrap(void)
{
        /* Initialise VM sub-system.  You probably want to initialise your 
           frame table here as well.
        */
	   //double the size of frame table
	   hpt_size = get_total_frame_number() * 2;
		pt = kmalloc(hpt_size * sizeof(struct page_table_entry));
        reset_pt();
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

