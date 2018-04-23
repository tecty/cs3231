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

/*
 * Add your file-related functions here ...
 */

int sys__open(userptr_t filename, int flags, mode_t mode){
    // the vnode for vfs call 
    struct vnode *vn;
    
    // open by vfs call 
    int res = vfs_open(filename, flags,mode, &vn);
    
    if(res){
        // some error then early return
        return res;
    }

    for(int i = 0;i < __OPEN_MAX;i++){
        // find an empty slot to store the vnode 
        
        if(curproc->fd_table[i]== NULL){
            //add the vnode into fd table
            curproc->fd_table[i] =vn;
            // early return with success
            return res;
        }
    }
    // overflow the fdtable
    res = EMFILE;
    // close the file because it would never been use
    vfs_close(vn)

    return res;
}

int sys__read(int fd, void * buf, size_t buflen){
    // uio operation needed structure 
	struct iovec iov;
	struct uio ku;
    
    // test code 
    // kprintf("try to read %d \n",fd);
    // buf = buf;
    // kprintf("with buff len %u \n\n",(unsigned int)buflen);

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

