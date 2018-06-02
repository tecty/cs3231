#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <spl.h>
#include <spinlock.h>
#include <proc.h>
#include <vm.h>
#include <machine/tlb.h>


/* Place your page table functions here */
//lock for protecting the global resource pt
static struct spinlock pt_lock = SPINLOCK_INITIALIZER;

size_t page_entry_size = sizeof(struct page_table_entry);

uint32_t hpt_next_free(struct addrspace *as, vaddr_t faultaddr, bool *result){
    //basic starting hash number
    uint32_t start = ((uint32_t)as)^(faultaddr >> PAGE_BITS);
    start %= hpt_size;
    //iterator
    uint32_t index = start;
    do{
        //check the validity
        if((hpt[index].frame_no & TLBLO_VALID) != TLBLO_VALID){
            //this is an invalid/empty binding
            *result = true;
            return index;
        }
        //iterate
        index = (index + 1)%hpt_size;
    }
    while(index!=start);
    //quit loop
    //no page allocatable
    *result = false;
    return index;
}

//function that create a space to save the deep copies pid/page_no/frame_no from 
//in old ass to new ass in hpt
//return 0 if all success
int hpt_copy(struct addrspace *old_as, struct addrspace *new_as){
    uint32_t i;
    //find all pages linked to old_as and copy them in new_as
    for(i = 0; i < hpt_size; i++){
        bool found_flag;
        if(hpt[i].pid==(uint32_t)old_as){
            //this should be a page to copy
            vaddr_t vaddr = hpt[i].page_no;
            paddr_t paddr = hpt[i].frame_no;
            uint32_t new_copy = hpt_next_free(new_as, vaddr, &found_flag);
            if(found_flag==false){
                //out of page
                return ENOMEM;
            }
            //allocate a frame for the new copy
            int dirty = paddr & TLBLO_DIRTY;
            int ret = hpt_fetch_frame(new_copy, dirty);
            if(ret){
                //no enough frame memory to allocate
                return ret;
            }
            hpt[new_copy].pid = (uint32_t)new_as;
            hpt[new_copy].page_no = vaddr&PAGE_FRAME;
            //now move the data from old to new
            memmove(
                (void *)PADDR_TO_KVADDR(hpt[new_copy].frame_no & PAGE_FRAME), 
                (const void *)PADDR_TO_KVADDR(hpt[i].frame_no & PAGE_FRAME),
                PAGE_SIZE
            );
        }
    }

    return 0;
}

void hpt_dirtybit_unset(struct addrspace *as, vaddr_t vaddr){
    uint32_t i;
    //find all pages linked to old_as and copy them in new_as
    for(i = 0; i < hpt_size; i++){
        if(hpt[i].pid==(uint32_t)as && (hpt[i].frame_no & PAGE_FRAME)==(vaddr & PAGE_FRAME)){
            //this should be a page to copy
            hpt[i].frame_no = hpt[i].frame_no & ~TLBLO_DIRTY;
            return;
        }
    }
}

//function for reset all page/frame linking
void hpt_reset(void){
    KASSERT(hpt!=0); //check the validity of pt
    // KASSERT(total_page_num!=0); //check the validity of total page number
    unsigned int num = 0;

    while(num < hpt_size){
        //clean
        //set all page-frame linking to be invalid
        hpt[num].frame_no &= ~ TLBLO_VALID;
        //iterate
        num = num + 1;
    }

}

//fetch a free frame to bind with new entry
int hpt_fetch_frame(int index, uint32_t dirty){

    vaddr_t new_frame = alloc_kpages(1);

    if(new_frame == 0){
        return ENOMEM;
    }
    paddr_t paddr = KVADDR_TO_PADDR(new_frame);

    hpt[index].frame_no = (paddr & PAGE_FRAME)|dirty|TLBLO_VALID;
    return 0;
}

//check if the frame can be retrived as free frames in hpt
//this is only prepared for those processes using the same frame memory
void clean_frame(paddr_t paddr){
    bool clean = true;
    uint32_t i;

    for(i = 0; i<hpt_size; i++){
        if((hpt[i].frame_no & PAGE_FRAME) == (paddr & PAGE_FRAME)){
            if((hpt[i].frame_no & TLBLO_VALID) == TLBLO_VALID){
                //this page is still using this frame
                clean = false;
            }
        }
    }

    if(clean==true){ 
        //no other page table needs this frame, clean it
        free_kpages(PADDR_TO_KVADDR(paddr & PAGE_FRAME));
    }
}


uint32_t hpt_find_hash(struct addrspace *as, vaddr_t faultaddr, bool *result){
    //basic starting hash number
    uint32_t start = ((uint32_t)as)^(faultaddr >> PAGE_BITS);
    start %= hpt_size;
    //iterator
    uint32_t index = start;
    do{
        if(hpt[index].pid==(uint32_t)as &&
            hpt[index].page_no== (faultaddr & PAGE_FRAME)){
            //found the right position
            //check the validity
            if((hpt[index].frame_no & TLBLO_VALID) == TLBLO_VALID){
                //this is a valid binding
                *result = true;
                return index;
            }
        }

        //iterate
        index = (index + 1)%hpt_size;
    }
    while(index!=start);
    //quit loop
    //must not found
    *result = false;
    return index; //not used
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
    //tlb entry high 
    uint32_t entry_hi;
    //tlb entry lo
    uint32_t entry_lo;
    //dirty bit
    uint32_t dirty = 0;

    //flag recording the tlb searching result
    bool found_flag = false;
    uint32_t result; //returning result

    //handling function
    //now it handles readonly, read and write fault
    switch(faulttype){
        case VM_FAULT_READONLY: 
            return EFAULT;
        case VM_FAULT_READ:
        case VM_FAULT_WRITE:
            break;
        default: 
            return EINVAL;
    }

    //check the validity of current process
    struct addrspace *as = proc_getas();
    if(as == NULL){
        return EFAULT;
    }

    //find this faultaddress in pagetable
    spinlock_acquire(&pt_lock);
    result = hpt_find_hash(as, faultaddress, &found_flag);
    if(found_flag){ //found in pagetable
        spinlock_release(&pt_lock);
        //Entry high is the virtual addr and ASID
        entry_hi = hpt[result].page_no & PAGE_FRAME;
        //Entry lo is physical frame, dirty bit and valid bit
        entry_lo = hpt[result].frame_no;

        /* Disable interrupts on this CPU while frobbing the TLB. */
        int spl = splhigh();
        tlb_random(entry_hi, entry_lo);
        splx(spl);
        return 0; //found in pagetable
    }

    //if not found
    //we first check if the address is a valid virtual addr inside a region
    struct region *ptr = as->regions;
    while(ptr!=NULL){
        //find which region it is in
        if(faultaddress >= ptr->vbase && faultaddress < (ptr->vbase + ptr->size)){
            //set the dirty bit if the region is writable
            if(ptr->writeable != 0){
                dirty = TLBLO_DIRTY;
            }
            else{
                dirty = 0;
            }
            break;
        }
        //iterate
        ptr = ptr->next;
    }
    //if not found, return bad memory error 
    if(ptr==NULL){
        spinlock_release(&pt_lock);
        return EFAULT;
    }
    //otherwise we put it into hpt as a new entry
    result = hpt_next_free(as, faultaddress, &found_flag);
    if(found_flag==true){ //find pagetable allocatable
        //allocate a frame entry
        int ret = hpt_fetch_frame(result, dirty);
        if(ret){
            spinlock_release(&pt_lock);
            //no enough frame memory to allocate
            return ret;
        }
        //otherwise continue
        hpt[result].pid = (uint32_t)as;
        hpt[result].page_no = faultaddress & PAGE_FRAME;
    }
    else{
        spinlock_release(&pt_lock);
        //no enough pagetable mem
        //stop input and return error
        return ENOMEM; 
    }

    spinlock_release(&pt_lock);
    //Entry high is the virtual addr and ASID
    entry_hi = hpt[result].page_no & PAGE_FRAME;
    //Entry lo is physical frame, dirty bit and valid bit
    entry_lo = hpt[result].frame_no;

    /* Disable interrupts on this CPU while frobbing the TLB. */
	int spl = splhigh();
    tlb_random(entry_hi, entry_lo);
	splx(spl);
	return 0;
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

//this function is used for flushing the entire TLB
void vm_tlbflush(void){
	int i;
	//flush the whole tlb by filling invalid data to each cell of TLB
	for(i = 0; i < NUM_TLB; i++){
		tlb_write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
	}
}

