#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <addrspace.h>
#include <vm.h>

/* Place your frametable data-structures here 
 * You probably also want to write a frametable initialisation
 * function and call it from vm_bootstrap
 */

static struct spinlock stealmem_lock = SPINLOCK_INITIALIZER;

/* Note that this function returns a VIRTUAL address, not a physical 
 * address
 * WARNING: this function gets called very early, before
 * vm_bootstrap().  You may wish to modify main.c to call your
 * frame table initialisation function, or check to see if the
 * frame table has been initialised and call ram_stealmem() otherwise.
 */

vaddr_t alloc_kpages(unsigned int npages)
{
    /*
        * IMPLEMENT ME.  You should replace this code with a proper
        *                implementation.
        */

    paddr_t addr;

    spinlock_acquire(&stealmem_lock);
    // calculate the address for return 
    addr = frame_start + next_free_frame_id*PAGE_SIZE;
    // set this page is used 
    frame_table[next_free_frame_id].status = USED;

    // not able to use steal meml, use frame table to find the slot 
    unsigned int old_next_free_frame_id = next_free_frame_id;

    
    // find another page for next allocation
    next_free_frame_id ++;
    while(old_next_free_frame_id!= next_free_frame_id){
        if(frame_table[next_free_frame_id].status== FREE]){
            // current next free frame is correct
            break;
        }
        next_free_frame_id++;
    }
    // TODO: Error handling. 

    spinlock_release(&stealmem_lock);

    if(addr == 0)
            return 0;

    return PADDR_TO_KVADDR(addr);
}

void free_kpages(vaddr_t addr)
{
    (void) addr;
}

