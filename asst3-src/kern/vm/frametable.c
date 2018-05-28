#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <addrspace.h>
#include <vm.h>

#define USED 1
#define FREE 0

/* Place your frametable data-structures here 
 * You probably also want to write a frametable initialisation
 * function and call it from vm_bootstrap
 */
struct frame_table_entry {
    unsigned int stat;
    vaddr_t mem_addr;
    // struct frame_table_entry * next;
};

typedef struct frame_table_entry * frame_table;

vaddr_t find_next_free_frame(void);
void clean_memory(vaddr_t mem);
//the global variable ft for the entire frame_table
//initialize with zero
frame_table ft = 0;

unsigned int entry_size = sizeof(struct frame_table_entry);

//these numbers will be initialized when ft is initialized
unsigned int total_frame_num;
unsigned int ft_used_frame_num;

paddr_t mem_start;
paddr_t mem_end;

unsigned int next_free;
unsigned int used_frame_num;

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

    if(ft == 0){
        //ft is not placed well in ram
        //initialize ft
        //use ram_stealmem
        //initializing cannot be synchronized hence don't need lock
        
        //find the top and bottom boundings of the usable memory
        mem_start = ram_getcurrentfirst();
        mem_end = ram_getsize();
        
        //find how many pages are needed to save the frame table
        total_frame_num = (unsigned int)(mem_end - mem_start)/(PAGE_SIZE + entry_size);

        ft_used_frame_num = total_frame_num * entry_size / PAGE_SIZE;

        //get addr for frame table
	    // spinlock_acquire(&stealmem_lock);
        ft = (struct frame_table_entry *)PADDR_TO_KVADDR(ram_stealmem(ft_used_frame_num));
        // spinlock_release(&stealmem_lock);

        //relocate the starting memory addr for usable ram (except addr for ft table)
        mem_start = ram_getfirstfree();
        paddr_t curr_mem = mem_start;
        unsigned int num = 0;
        
        // spinlock_acquire(&stealmem_lock);
        while(num < total_frame_num){
            //save
            ft[num].stat = FREE;
            
            ft[num].mem_addr = PADDR_TO_KVADDR(curr_mem);
            
            //iterate
            curr_mem = curr_mem + PAGE_SIZE;
            
            num = num + 1;        
        }
        // spinlock_release(&stealmem_lock);
        next_free = 0;
        used_frame_num = 0;
    }
    else{
        //assume one page at a time
        if(npages!=1) return 0;
        //ft is ready to use
        //use self-defined allocator
    }
    spinlock_acquire(&stealmem_lock);
    vaddr_t addr = find_next_free_frame();
    spinlock_release(&stealmem_lock);

    return addr;
}

//find next available frame
vaddr_t find_next_free_frame(void){
    unsigned int old_pos = next_free;
    if(used_frame_num == total_frame_num) return 0;
    do{
        if(ft[next_free].stat==FREE){
            //found
            ft[next_free].stat = USED;
            vaddr_t addr = ft[next_free].mem_addr;
            clean_memory(addr);
            //back one frame
            next_free = (next_free + 1) % total_frame_num;
            //add new used frame into the total number
            used_frame_num = used_frame_num + 1;
            return addr;
        }
        next_free = (next_free + 1) % total_frame_num;
    }
    while(next_free != old_pos);
    return 0;
}

void clean_memory(vaddr_t mem){
    void * ptr;
    ptr = (void *)mem;
    //clean the memory byte by byte
    memset(ptr, 0, PAGE_SIZE);
}

void free_kpages(vaddr_t addr)
{
    // spinlock_acquire(&stealmem_lock);
    paddr_t paddr = KVADDR_TO_PADDR(addr);
    unsigned int pos = (paddr-mem_start)/PAGE_SIZE;
    ft[pos].stat = FREE;
    spinlock_acquire(&stealmem_lock);
    used_frame_num = used_frame_num - 1;
    spinlock_release(&stealmem_lock);
    return;
    // spinlock_release(&stealmem_lock);
}

