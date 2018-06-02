#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <spl.h>
#include <proc.h>
#include <vm.h>
#include <machine/tlb.h>


/* Place your page table functions here */
//lock for protecting the global resource pt
static struct spinlock pt_lock = SPINLOCK_INITIALIZER;

size_t page_entry_size = sizeof(struct page_table_entry);

uint32_t hpt_next_free(struct addrspace *as, vaddr_t faultaddr, bool *flag){
    uint32_t index = ((uint32_t)as)^(faultaddr >> PAGE_BITS);
    index %= hpt_size;
    while(index < hpt_size){
        //find the first invalid page to be the new page
        if((hpt[index].frame_no & TLBLO_VALID)!= TLBLO_VALID){
            *flag = true;
            return index;
        }
        //iterate
        index = index + 1;
    }
    //not found
    *flag = false;
    return index;
}

//function that create a space to save the deep copies pid/page_no/frame_no from 
//in old ass to new ass in hpt
//return 0 if all success
int hpt_copy(struct addrspace *old_as, struct addrspace *new_as){
    uint32_t i;
    for(i = 0; i < hpt_size; i++){
        bool found_flag;
        if(hpt[i].pid==(uint32_t)old_as){
            //this should be a page to copy
            vaddr_t vaddr = hpt[i].page_no;
            paddr_t paddr = hpt[i].frame_no;
            uint32_t new_copy = hpt_find_hash(new_as, vaddr, &found_flag);
            KASSERT(found_flag==false);
            uint32_t next = hpt_next_free(new_as, vaddr, &found_flag);
            if(found_flag==false){
                //out of page
                return ENOMEM;
            }
            hpt[next].pid = (uint32_t)new_as;
            hpt[next].page_no = vaddr;
            int dirty = paddr & TLBLO_DIRTY;
            hpt[next].frame_no = paddr | dirty | TLBLO_VALID;
            if(new_copy!=next){
                //there is an internal chaining happen
                hpt[new_copy].next = &hpt[next];
            }
        }
    }
    return 0;
}

//function for reset all page/frame linking
void hpt_reset(void){
    KASSERT(hpt!=0); //check the validity of pt
    // KASSERT(total_page_num!=0); //check the validity of total page number
    unsigned int num = 0;
    while(num < hpt_size){
        //clean
        spinlock_acquire(&pt_lock);
        //clean all internal chaining
        hpt[num].next = NULL;
        //set all page-frame linking to be invalid
        hpt[num].frame_no &= ~ TLBLO_VALID;
        spinlock_release(&pt_lock);
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

//hash and jump collision
//check existing hash
uint32_t hpt_find_hash(struct addrspace *as, vaddr_t faultaddr, bool *result){
    //basic starting hash number
    uint32_t index = ((uint32_t)as)^(faultaddr >> PAGE_BITS);
    index %= hpt_size;
    //prev_no is used to record where the internal chaining is from 
    //(what is the previous hash number)
    uint32_t prev_no = index;
    //use internal chaining to check the right position for as-faultaddr
    while(hpt[index].pid!=(uint32_t)as || 
        hpt[index].page_no!= (faultaddr & PAGE_FRAME)){
        //should find the next internal chaining
        if((hpt[index].frame_no & TLBLO_VALID) != TLBLO_VALID){
            //hit an invalid page-frame linking 
            //hpt entry not found
            *result = false;
            return index; 
        }
        if(hpt[index].next == NULL){
            //no further internal chaining
            //hpt entry not found
            prev_no = index;
            *result = false;
            return prev_no;
        }
        //go to next internal chaining to check
        //iterate
        prev_no = index;
        index = (hpt[index].next - hpt)/sizeof(struct page_table_entry);
    }
    //quit loop
    //the current entry should have the right index
    paddr_t paddr = hpt[index].frame_no;
    //check if it is valid
    if((paddr & TLBLO_VALID) == TLBLO_VALID){ //valid
        //copy offset from page_no
        *result = true;
        return index;
    }
    else{  //this addr is not valid
        *result = false;
        return prev_no;
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
    // (void) faulttype;
    // (void) faultaddress;

    // panic("vm_fault hasn't been written yet\n");

    // return EFAULT;	
    //tlb entry high 
    uint32_t entry_hi;
    //tlb entry lo
    uint32_t entry_lo;
    //dirty bit
    uint32_t dirty = 01;

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
    uint32_t next = hpt_next_free(as, faultaddress, &found_flag);
    if(found_flag==true){ //find pagetable allocatable
        //allocate a frame entry
        int ret = hpt_fetch_frame(next, dirty);
        if(ret){
            //no enough frame memory to allocate
            spinlock_release(&pt_lock);
            return ret;
        }
        //otherwise continue
        hpt[next].pid = (uint32_t)as;
        hpt[next].page_no = faultaddress & PAGE_FRAME;
        if(result!=next) {
            //there is an internal chaining happen
            //add this new page to the next pointer in the previoud page
            hpt[result].next = &hpt[next];
        }
        spinlock_release(&pt_lock);
    }
    else{
        //no enough pagetable mem
        //stop input and return error
        spinlock_release(&pt_lock);
        return ENOMEM; 
    }

    //Entry high is the virtual addr and ASID
    entry_hi = hpt[next].page_no & PAGE_FRAME;
    //Entry lo is physical frame, dirty bit and valid bit
    entry_lo = hpt[next].frame_no;

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

