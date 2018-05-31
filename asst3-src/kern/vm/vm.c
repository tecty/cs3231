#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>

#include <vm.h>
#include <machine/tlb.h>


/* Place your page table functions here */
//lock for protecting the global resource pt
static struct spinlock pt_lock = SPINLOCK_INITIALIZER;

size_t page_entry_size = sizeof(struct page_table_entry);

uint32_t hpt_next_free(bool *flag){
    uint32_t index = 0;
    while(index < hpt_size){
        if(hpt[size].frame_no & ~ VALID_BYTE == 0){
            *flag = true;
            return index;
        }
        //iterate
        index = index + 1;
    }
    *flag = false;
    return index;
}

//function for reset all page/frame linking
void hpt_reset(void){
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

//function for finding the correct location in hpt
uint32_t hpt_hash(struct addrspace *as, vaddr_t faultaddr){
    uint32_t index;

    index = ((uint32_t)as)^(faultaddr >> PAGE_BITS) % hpt_size;
    return index;
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

//hash and jump collision
//check existing hash
uint32_t hpt_find_hash(struct addrspace *as, vaddr_t faultaddr, bool *result){
    uint32_t index = hpt_hash(as, faultaddr);
    while(hpt[index].pid!=(uint32_t)as || hpt[index].page_no!=faultaddr){
        //should find the next internal chaining
        if(hpt[index].next == NULL){
            //no further internal chaining
            //hpt entry not found
            *result = false;
            return index;
        }
        else{
            //go to next internal chaining to check
            //iterate
            index = (hpt[index].next - hpt)/sizeof(struct page_table_entry);
        }
    }
    //quit loop
    //the current entry should have the right index
    paddr_t paddr = hpt[index].frame_no;
    //check if it is valid
    if(paddr & ~VALID_BYTE == 1){ //valid
        //copy offset from page_no
        *result = true;
    }
    else{  //this addr is not valid
        *result = false;
    }
    return index;
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
    uint32_t dirty;
    //result paddr
    paddr_t paddr;

    //flag recording the tlb searching result
    bool found_flag = false;
    int result; //returning result

    //handling function
    switch(faulttype){
        case VM_FAULT_READONLY: return EFAULT;
        case VM_FAULT_READ:
        case VM_FAULT_WRITE: break;
        default: return EINVAL;
    }

    if(curproc==NULL){
        return EFAULT;
    }

    struct addrspace *as = proc_getas();
    if(cur_as == NULL){
        return EFAULT;
    }

    //now find the result
    paddr = KVADDR_TO_PADDR(faultaddress);

    result = hpt_find_hash(as, faultaddress, &found_flag);
    if(found_flag) return result; //found in pagetable

    //if not found
    //we first check if the address is a valid virtual addr inside a region
    struct region *ptr = as->regions;
    while(ptr!=NULL){
        //found
        if(faultaddress >= ptr->vbase && faultaddress < (ptr->vbase + pre->size){
            //set the dirty bit if the region is writable
            if(ptr->writable != 0){
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
    if(ptr==NULL) return EFAULT;
    //otherwise we put it into hpt as a new entry
    uint32_t next = hpt_next_free(found_flag);
    if(found_flag==true){ //find pagetable allocatable
        //allocate a frame entry
        int ret = hpt_fetch_frame(next, dirty);
        if(ret){
            return ret; //no frame allocatable
        }
        //otherwise continue
        hpt[next].pid = (uint32_t)as;
        hpt[next].page_no = faultaddress;
    }
    else{
        return ENOMEM; //no enough pagetable mem
    }

    //Entry high is the virtual addr and ASID
    entry_hi = faultaddress & PAGE_FRAME;
    //Entry lo is physical frame, dirty bit and valid bit
    entry_lo = hpt[next].frame_no;

    /* Disable interrupts on this CPU while frobbing the TLB. */
	spl = splhigh();
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

