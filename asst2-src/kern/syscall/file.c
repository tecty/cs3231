#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <kern/limits.h>
#include <kern/stat.h>
#include <kern/seek.h>
#include <lib.h>
#include <uio.h>
#include <thread.h>
#include <current.h>
#include <synch.h>
#include <vfs.h>
#include <vnode.h>
#include <file.h>
#include <syscall.h>
#include <copyinout.h>
#include <proc.h>

/*
 * Add your file-related functions here ...
 */

int sys__open(userptr_t filename, int flags, mode_t mode){
    // check whether the string given by the user is valid 
    copyinstr(filename,(char *)&str_buf,STR_BUF_SIZE, NULL);
    
    // the vnode for vfs call 
    struct vnode *vn;
    
    //the slot of the slot of open file table
    struct open_file_info **oft_slot; 


    // open by vfs call 
    int res = vfs_open((char *)&str_buf, flags,mode, &vn);
    
    if(res){
        // some error then early return
        return res;
    }

    // find a slot to store in the open file table 
    for(int i =0; i < __OPEN_MAX* __PID_MAX; i ++ ){
        if(of_table[i] == NULL){
            // a empty slot for the open file table
            // malloc a space for the file info 
            of_table[i] = kmalloc(sizeof(struct open_file_info));
            
            // unexpect no enough memory space 
            KASSERT(of_table[i]!= NULL);

            /*
             * Set Up: the information of a new opened file
             */
            // a offset for a new opened file is 0
            of_table[i]->f_offset = 0;
            // a reference count is setted to 1
            // the reference count will increase when dup2() or fork()
            // is used.
            of_table[i]->ref_count = 1;

            // set up the vnode 
            of_table[i]->vn = vn;
            // the open file is this flag
            of_table[i]->o_flags = flags;

            // get the pointer of this new slot
            oft_slot = &(of_table[i]);

            // Successful: break the loop and make a reference in
            // fd table 
            goto FD_REF;
        }
    }

    // Error Catch: No enough file table
    res = ENFILE;
    // go to free the vnode that acquire 
    goto CATCH_FREE_VNODE;



    // find an empty slot to store the pointer in fd_table
FD_REF:
    for(int i = 0;i < __OPEN_MAX;i++){
        if(curproc->fd_table[i]== NULL){
            // malloc a space to store the pin
            curproc->fd_table[i] =oft_slot;
            
            // early return with success
            return res;
        }
    }
    // overflow the fdtable
    res = EMFILE;

    // close the file because it would never been use
    vfs_close(vn);



    // free the vnode that acquire
CATCH_FREE_VNODE:
    vfs_close(vn);

    return res;
}

int sys__read(int fd, void * buf, size_t buflen){
    // uio operation needed structure 
	// struct iovec iov;
	// struct uio ku;
    
    // test code 
    kprintf("try to read %d \n",fd);
    buf = buf;
    kprintf("with buff len %u \n\n",(unsigned int)buflen);

    /*
    struct uio *un;
    if(res){
        vfs_close();
        return res;
    }
    uio_kinit(...);
    */
    return 0;
}

int sys__write(int fd, void * buf, size_t nbytes){
    kprintf("try to write %d \n",fd);
    buf = buf;
    kprintf("with write len %u \n\n",(unsigned int)nbytes);

    return 0;
}

int sys__lseek(int fd, int pos_lo,int pos_hi, int whence){
    kprintf("try to lseek  %d \n",fd);
    kprintf("pos_lo %d \n",pos_lo);
    kprintf("pos_hi %d \n",pos_hi);
    kprintf("with seek mode %d \n\n",whence);

    return 0;
}

int sys__close(int fd){
    kprintf("try to close  %d \n",fd);
    
    return 0;
}

int sys__dup2(int oldfd, int newfd){
    kprintf("try to copy file info from  %d  to %d \n",
        oldfd,newfd);

    return 0;
}

