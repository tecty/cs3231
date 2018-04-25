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

int ker_open(char * filename, int flags, mode_t mode, int *retval,struct proc * to_proc ){
    // kprintf("try to open file %s\n",filename);
    int res =0;
    // the vnode for vfs call 
    struct vnode *vn;
    
    //the slot of the slot of open file table
    struct open_file_info **oft_slot; 

    // test message:
    // kprintf("try to open %s \n",filename);

    // open by vfs call 
    res = vfs_open(filename, flags,mode, &vn);
    
    if(res){
        // some error then early return
        // kprintf("Error here with %d \n",res);
        return res;
        
    }


    // kprintf("try to find a slot in oft \n");
    // find a slot to store in the open file table 
    for(int i =0; i <  __PID_MAX; i ++ ){
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

            // kprintf("found a slot in oft\n");

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
    // kprintf("try to find a slot in the fdt\n");
    for(int i = 0;i < __OPEN_MAX;i++){
        if(to_proc->fd_table[i]== NULL){
            // malloc a space to store the pin
            to_proc->fd_table[i] =oft_slot;

            // set the return value to fd 
            *retval  = i;
            
            // kprintf("sucessfully open a file %s\n",filename);

            // kprintf("the new fd for this file is %d\n", i );
            // early return with success
            return res;
        }
    }
    // *retval = 0;
    // kprintf("couldn't find a slot in the fd table \n ");
    // overflow the fdtable
    res = EMFILE;




    // free the vnode that acquire
CATCH_FREE_VNODE:
    // close the file because it would never been use
    vfs_close(vn);

    // kprintf("Error here %d \n",res);

    return res;
}

int ker__close(int fd, struct proc *to_proc ){
    // for kenel process to close other proc file table
    if(to_proc->fd_table[fd] == NULL){
        // no file is opened for this fd 
        return EBADF;
    }

    struct open_file_info *cur_ofi = (*to_proc->fd_table[fd]);

    // dereference from this fd_table
    cur_ofi->ref_count -- ;

    if(cur_ofi->ref_count == 0){
        /*
         * no more fd_table has reference to this openfile slot
         * close the flie and clean this slot 
         */        
        // vfs_close(using_vnode);
        vfs_close(cur_ofi->vn);        
        
        // free the slot of OFT 
        kfree(cur_ofi);
        // set that slot of OFT to NULL
        cur_ofi = NULL;

    }

    // clean the slot of fd table
    to_proc->fd_table[fd] = NULL;

    // successfully close the file
    return 0;
}


int sys__open(userptr_t filename, int flags, mode_t mode,int *retval){
    int res; 
    
    
    // check whether the string given by the user is valid 
    res = copyinstr(filename,(char *)&str_buf,STR_BUF_SIZE, NULL);
    
    if(res){
        // Error Catch: Invalid filename pointer
        // return the error code assigned by copyinstr()
        // kprintf("Error here %d \n",res);
        return res;
    }

    // call the ker_open to handle most of the job
    // which assume all the value and pointer is in kernel
    res = ker_open((char *)str_buf,flags,mode, retval,curproc);

    return res;
}

int sys__read(int fd, void * buf, size_t buflen,size_t *retval){
    // uio operation needed structure 
	// struct iovec iov;
	// struct uio ku;
    
    // test code 
    // kprintf("try to read %d \n",fd);
    // buf = buf;
    // kprintf("with buff len %u \n\n",(unsigned int)buflen);

    // a value to store the result 
    // success is 0 
    int res = 0; 
    // declear the var to do uio
    struct uio ku;
    struct iovec iov;

    if(curproc->fd_table[fd] == NULL){
        // no file is opened for this fd 
        return EBADF;
    }
    // dereference the oopen file info 
    struct open_file_info *cur_ofi = (* (curproc->fd_table[fd]));

    //early return if the file is not open for reading
    int how = cur_ofi->o_flags & O_ACCMODE;
    switch(how){
        case O_RDONLY:
        case O_RDWR: 
        break;
        default:
        return EBADF;
    }

    // Invariant: retval <= buflen 
    // init the uio block by the OFI 
    uio_kinit(&iov, &ku,
        str_buf,buflen, cur_ofi->f_offset,UIO_READ);
    res = VOP_READ(cur_ofi->vn , &ku);
    if(res){
        // Error Catch: VOP_READ Error 
        // return the error code from VOP_READ
        return res;
    }

    // Invariant: retval <= buflen 
    // get how much has read by difference of offset
    *retval = ku.uio_offset - cur_ofi->f_offset ;

    // push forward the current file position 
    // by refreshing the value 
    cur_ofi->f_offset = ku.uio_offset;
    if(*retval ==0 ){
        // didn't read anything, also a successful read
        // return 0
        return res;
    }


    // Invariant: retval <= buflen 
    // copy out the file  to cross the system boundary
    // copy out how much has read from file
    res = copyout((char *)str_buf,buf,*retval);
    
    if(res){
        // Error Catch: Invalid filename pointer
        // return the error code assigned by copyinstr()
        return res;
    }

    // return successfully
    return res;
}

int sys__write(int fd, void * buf, size_t nbytes,size_t *retval){
    // uio operation needed structure 
	// struct iovec iov;
	// struct uio ku;
    // a value to store the result 
    // success is 0 
    int res = 0; 
    // declear the var to do uio
    struct uio ku;
    struct iovec iov;


    if(curproc->fd_table[fd] == NULL){
        // no file is opened for this fd 
        // kprintf("error with the fd table is nothing there with fd %d \n",fd);
        return EBADF;
    }
    // dereference the oopen file info 
    struct open_file_info *cur_ofi = (* (curproc->fd_table[fd]));

    //early return if the file is not open for writing
    int how = cur_ofi->o_flags & O_ACCMODE;
    switch(how){
        case O_WRONLY:
	    case O_RDWR:
        break;
        default:
        return EBADF;
    }

    res = copyinstr(buf, str_buf, STR_BUF_SIZE,retval);

    // Unkown error: why can not use this ? 
    // res = copyin(buf, str_buf,nbytes);
    if(res){
        // kprintf("Copy in Error with %d \n",res);
        // Error Catch: Copyin error.
        return res;
    }
    
    // kprintf("try to print: \n%s\n",str_buf);
    // init the uio block by the OFI 
    uio_kinit(&iov, &ku,
        str_buf,nbytes, cur_ofi->f_offset,
        UIO_WRITE);
    res = VOP_WRITE(cur_ofi->vn , &ku);
    if(res){
        // kprintf("VOP WRITE error with %d\n", res);
        // Error Catch: VOP_WRITE Error 
        // return the error code from VOP_WRITE
        return res;
    }

    // refresh the retval by offset changes
    *retval =ku.uio_offset - cur_ofi->f_offset; 
    
    // push forward the current file position 
    // by refreshing the value 
    cur_ofi->f_offset = ku.uio_offset;
    // return successfully
    return res;
}

int sys__lseek(int fd,off_t pos, int whence,off_t *retval64){
    // kprintf("try to lseek  %d \n",fd);
    // kprintf("pos_lo %d \n",pos_lo);
    // kprintf("pos_hi %d \n",pos_hi);
    // kprintf("with seek mode %d \n\n",whence);

    // protect from bad fd
    if(curproc->fd_table[fd] == NULL){
        // no file is opened for this fd 
        return EBADF;
    }

    // dereference the oopen file info 
    struct open_file_info *cur_ofi = (* (curproc->fd_table[fd]));

    if (!VOP_ISSEEKABLE(cur_ofi->vn)){
        // this open file can not be process with seek 
        return ESPIPE;
    }
    
    // record the old offse in case it become negative
    // after seeking 
    off_t old_offset = cur_ofi->f_offset;

    // temporary struct to store file info 
    struct stat file_stat;
    
    
    // change the file offset by requirement 
    switch(whence){
        case SEEK_SET:
            cur_ofi->f_offset  = pos;
            break;
        case SEEK_CUR:
            cur_ofi->f_offset += pos;
            break;
        case SEEK_END:

            // get the file information by VOP_STAT
            VOP_STAT(cur_ofi->vn, &file_stat);
            // add up the offset by file stat
            cur_ofi->f_offset  = file_stat.st_size + pos;
            break;
        default:
            // error seeking by providing wrong whence
            return EINVAL;
            break;
    }

    // prevent current offset become negative
    if(cur_ofi->f_offset< 0){
        // current offset is negative, reset the position
        cur_ofi->f_offset = old_offset;
        return EINVAL;
    }


    retval64 = retval64;
    return 0;
}


int sys__close(int fd){
    // kprintf("try to close  %d \n",fd);
    // the reverse operation of sys__open

    // passing to ker__close to do the job
    return ker__close(fd, curproc);
}

int sys__dup2(int oldfd, int newfd){

    if(curproc->fd_table[oldfd] == NULL){
        // no file is opened for this fd 
        return EBADF;
    }
    if(curproc->fd_table[newfd] != NULL){
        // must has no file is opened for this fd 
        return EBADF;
    }


    curproc->fd_table[newfd] =curproc->fd_table[oldfd];

    return 0;
}

