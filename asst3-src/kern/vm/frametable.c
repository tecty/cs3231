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
#define USED 1
#define FREE 0

static struct spinlock stealmem_lock = SPINLOCK_INITIALIZER;

// a global variable that always record the start of the usable ram
paddr_t start = 0;

//initialize the frame table
//the function should be called only once
void ft_initialize(void){
        //pointers for allocating address for the frame table
        struct frame_table_entry * curr_frame = NULL;
        struct frame_table_entry * prev_frame = NULL;

        //first find out how many entries the table can have
        long entryNum = ram_getsize() / PAGE_SIZE;

        //then start to steal memories for the file table
        long i = 0;

        //first steal memories for the head frame in the table
        spinlock_acquire(&stealmem_lock);
                ft = (struct frame_table_entry *)ram_stealmem(1);
                //refresh the prev pointing and prepare for the next new frame
                prev_frame = ft;
                //count the number of allocated frame addr
                i = i + 1;
        spinlock_release(&stealmem_lock);

        //then steal memories for the rest of the entries in the table
        while(i<entryNum){
                spinlock_acquire(&stealmem_lock);
                        //steal memories for a new frame
                        curr_frame = (struct frame_table_entry *)ram_stealmem(1);
                        //link the new allocated frame to the previous as a link list
                        prev_frame->next = curr_frame;
                        //refresh the prev pointing and prepare for the next new frame
                        prev_frame = curr_frame;
                        //count the number of allocated frame addr
                        i = i + 1;
                spinlock_release(&stealmem_lock);
        }

        //now use ram_getfirstfree() to find the start of the usable ram space
        //for following-up user process
        //this function can be called ONLY ONCE
        start = ram_getfirstfree();

        //now ram_stealman is no longer used

        //redirect the current examining frame to the start of ft
        curr_frame = ft;

        //initialize each frame in the table
        while(curr_frame->next!=NULL){
                //clean the state of each frame
                //stat: 0->free, 1->used
                curr_frame->stat = FREE;
                //go to the next frame
                curr_frame = curr_frame->next;
        }
}

//search a free frame from the start of the frame table
frame_table next_free_frame(){
        frame_table frame = ft;
        while(frame->next!=NULL){
                //if the stat of frame is free, than
                if(frame->stat == FREE) return frame;
                frame = frame->next;
        }
        //otherwise, not found
        return NULL;
}

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

        //return null if number of pages over 1
        // if(npages!=1)
        //     return (vaddr_t)NULL;

        // //define the result addr
        // frame_table result_frame;
        // //reuse the spinlock for stealmem
        // spinlock_acquire(&stealmem_lock);
        //         result_frame = next_free_frame();
        //         //all frames are full
        //         //out of memory ?
        //         if (result_frame == NULL)
        //             return (vaddr_t)NULL;
        //         //allocate the new physical memory to the
        //         //next free frame
        //         //and input the taken addr into a free frame
        //         result_frame->p_address = takemem(npages);
        //         result_frame->stat = USED;
        // spinlock_release(&stealmem_lock);

        // //return the address of the allocated frame
        // return PADDR_TO_KVADDR((paddr_t)result_frame);
        kprintf("alloc %u pages", npages);
        return 0;
}

//search a free memory block from the beginning
paddr_t
takemem(unsigned int npages){
        size_t size = npages * PAGE_SIZE;
        paddr_t addr = start;

        //search the suitable addr from the current addr til the one which
        //is not used by any current frame in the ft-table
        while (addressUsed(addr)==NULL && addr + size <= ram_getsize()) addr += size;

        if(addr+size <= ram_getsize()) return addr;
        else return -1; //this means no free mem is found, but should not happen ?
}

//check if the memory block starting from addr is already in use
frame_table addressUsed(paddr_t addr){
        struct frame_table_entry * frame = ft;
        while(frame->next!=NULL){
                if(frame->p_address == addr && frame->stat == USED){
                        //this memory block is already used
                        return frame;
                }
                frame = frame->next;
        }
        return NULL;
}


void free_kpages(vaddr_t addr)
{
        // frame_table frame = addressUsed(addr);
        // //if addr is not found in frame, just return
        // if(frame == NULL) return;
        // //otherwise, free this memory block;
        // frame->stat = FREE;
        addr = addr;
}

