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


struct file_info{
    struct vnode * vno;
    struct uio* uio;
};

int sys__open(userptr_t filename, int flags, mode_t mode);
int sys__read(int fd, void * buf, size_t buflen);
int sys__write(int fd, void * buf, size_t nbytes);
int sys__lseek(int fd, int pos_lo,int pos_hi, int whence);
int sys__close(int fd);
int sys__dup2(int oldfd, int newfd);


#endif /* _FILE_H_ */
