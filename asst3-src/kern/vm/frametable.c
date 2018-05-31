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
    struct frame_table_entry * next;
};

typedef struct frame_table_entry * frame_table;

struct free_frame_list {
    frame_table start;
    frame_table end;
    long count;
};

vaddr_t find_next_free_frame(void);
void clean_memory(vaddr_t mem);
//the global variable ft for the entire frame_table
//initialize with zero
frame_table ft = 0;
//the global variable free_list recording which frames are free
//initialize later with ft
struct free_frame_list free_list;

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

        //number of free frames exclude the frames used by ft itself
        free_list.count = total_frame_num - ft_used_frame_num; 

        //relocate the starting memory addr for usable ram (except addr for ft table)
        mem_start = ram_getfirstfree();        
        
        unsigned int page_start = num;
        unsigned int previous = num;
        //save the memory usable for other processes
        while(num < total_frame_num){
            //save
            ft[num].stat = FREE;
            ft[num].mem_addr = PADDR_TO_KVADDR(curr_mem);
            //save into the free list
            if(num != page_start){ //this is not the head
                ft[previous].next = &ft[num];
            }
            //extend the end of free frame list
            free_list.end = &ft[num];
            //iterate
            previous = num;
            curr_mem = curr_mem + PAGE_SIZE;
            num = num + 1;        
        }
        //the last frame does not have any next frame link
        ft[num-1].next = 0;
        free_list.start = &ft[page_start];
        
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
    if(free_list.count == 0) return 0; //out of memory
    //otherwise pop up a free frame from free_list
    frame_table next_free = free_list.start;
    if(next_free->next==0){ //this is the last free frame
        free_list.start = free_list.end = 0;
    }
    else{
        free_list.start = next_free->next;
    }
    free_list.count = free_list.count - 1;
    next_free->stat = USED;
    next_free->next = 0;
    return next_free->mem_addr;
}

void clean_memory(vaddr_t mem){
    void * ptr;
    ptr = (void *)mem;
    //clean the memory byte by byte
    memset(ptr, 0, PAGE_SIZE);
    //bzero();
}

//this function can only be used after initialization
//it will give a reference size when boot functions decide the size of the 
//global page table
unsigned int get_total_frame_number(void){
    return total_frame_num;
}

void free_kpages(vaddr_t addr)
{
    paddr_t paddr = KVADDR_TO_PADDR(addr);
    unsigned int pos = (paddr-mem_start)/PAGE_SIZE + ft_used_frame_num;
    ft[pos].stat = FREE;
    clean_memory(ft[pos].mem_addr);    
    spinlock_acquire(&stealmem_lock);
    if(free_list.count==0){
        //this is the first newly added free frame
        free_list.start = free_list.end = &ft[pos];
    }
    else{
        free_list.end->next = &ft[pos];
        //extend free_list
        free_list.end = &ft[pos];
    }
    free_list.count = free_list.count + 1;  
    spinlock_release(&stealmem_lock);
    return;
}

