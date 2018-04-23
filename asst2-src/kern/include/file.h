/*
 * Declarations for file handle and file table management.
 */

#ifndef _FILE_H_
#define _FILE_H_

/*
 * Contains some file-related maximum length constants
 */
#include <limits.h>
#include <types.h>

/*
 * Put your function declarations and data types here ...
 */

// the struct of the openfile table
struct open_file_info{
    // current offset of this open file
    off_t f_offset;
    // how much time this slot has referenced
    unsigned int ref_count;
    // the pointer of this openfile vnode  
    struct vnode *vn;
    // how does this file opened
    mode_t o_flags;
}

// According to the piazza 
// https://piazza.com/class/jdwg14qxhhb4kp?cid=230
// the file table has construct in this number
struct open_file_info *of_table[__PID_MAX*__OPEN_MAX];


// a global buffer to record the filename or read info 
char str_buf[8192];



int sys__open(userptr_t filename, int flags, mode_t mode);
int sys__read(int fd, void * buf, size_t buflen);
int sys__write(int fd, void * buf, size_t nbytes);
int sys__lseek(int fd, int pos_lo,int pos_hi, int whence);
int sys__close(int fd);
int sys__dup2(int oldfd, int newfd);


#endif /* _FILE_H_ */
