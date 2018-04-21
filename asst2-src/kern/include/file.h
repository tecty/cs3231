/*
 * Declarations for file handle and file table management.
 */

#ifndef _FILE_H_
#define _FILE_H_

/*
 * Contains some file-related maximum length constants
 */
#include <limits.h>


/*
 * Put your function declarations and data types here ...
 */
//return the count of bytes read() returns
int sys___read(int fd, void *buf, size_t buflen);
//return non-negative file handle or -1 for error
int sys___open(const char *filename, int flags);
//return the count of bytes write() writes
int sys___write(int fd, const void *buf, size_t nbytes);
//return 0 for success, 1 for error
int sys___close(int fd);
//return newfe or -1 for error
int dup2(int oldfd, int newfd);

#endif /* _FILE_H_ */
