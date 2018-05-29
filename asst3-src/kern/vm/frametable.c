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
    struct frame_table_entry * next_free_frame;
};

typedef struct frame_table_entry * frame_table;

void clean_memory(vaddr_t mem);
vaddr_t find_next_free_frame(void);
//the global variable ft for the entire frame_table
//initialize with zero
frame_table ft = 0;
frame_table free_frame_list;
frame_table free_frame_list_tail;

unsigned int entry_size = sizeof(struct frame_table_entry);

//these numbers will be initialized when ft is initialized
unsigned int total_frame_num;
unsigned int ft_used_frame_num;

paddr_t mem_start;
paddr_t mem_end;

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
        total_frame_num = (unsigned int)(mem_end - mem_start)/PAGE_SIZE;

        ft_used_frame_num = total_frame_num * entry_size / PAGE_SIZE + 1;

        //get addr for frame table
        ft = (struct frame_table_entry *)PADDR_TO_KVADDR(ram_stealmem(ft_used_frame_num));

        //save the memory used by ft itself in the table 
        unsigned int num = 0;
        paddr_t curr_mem = mem_start;

        while(num < ft_used_frame_num){
            //save
            ft[num].stat = USED;
            ft[num].mem_addr = PADDR_TO_KVADDR(curr_mem);

            //iterate
            curr_mem = curr_mem + PAGE_SIZE;
            num = num + 1;
        }

        //relocate the starting memory addr for usable ram (except addr for ft table)
        mem_start = ram_getfirstfree();        
        
        int prev = -1;
        //save the memory usable for other processes
        while(num < total_frame_num){
            //save
            ft[num].stat = FREE;
            ft[num].mem_addr = PADDR_TO_KVADDR(curr_mem);
            if(prev!=-1){ //not at head
                ft[prev].next_free_frame = &ft[num];
            }
            free_frame_list_tail = &ft[num];
            //iterate1
            curr_mem = curr_mem + PAGE_SIZE;
            prev = num;
            num = num + 1;        
        }
        //the last frame points to no more free frame right now
        ft[num - 1].next_free_frame = 0;
        //now the free frame list starts at the head of ft
        free_frame_list = ft;
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
    if(free_frame_list==0){
        //no more free frames
        //out of memory
        return 0;
    }
    //otherwise, pick up a free frame out of the free list
    frame_table result = free_frame_list;
    if(result->next_free_frame==0){
        //this is the last free frame
        free_frame_list = free_frame_list_tail = 0;
    }
    else{
        free_frame_list = result->next_free_frame;
    }
    result->stat = USED;
    result->next_free_frame = 0;
    return result->mem_addr;
}

void clean_memory(vaddr_t mem){
    void * ptr;
    ptr = (void *)mem;
    //clean the memory byte by byte
    memset(ptr, 0, PAGE_SIZE);
    //bzero();
}

void free_kpages(vaddr_t addr)
{
    paddr_t paddr = KVADDR_TO_PADDR(addr);
    unsigned int pos = (paddr-mem_start)/PAGE_SIZE + ft_used_frame_num;
    ft[pos].stat = FREE;
    clean_memory(ft[pos].mem_addr);    
    spinlock_acquire(&stealmem_lock);
    if(free_frame_list==0){
        free_frame_list = free_frame_list_tail = &ft[pos];
    }
    else{
        free_frame_list_tail->next_free_frame = &ft[pos];
        free_frame_list_tail = &ft[pos];
    }
    spinlock_release(&stealmem_lock);
    return;
}

