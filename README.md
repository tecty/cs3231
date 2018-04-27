# cs3231
OS161 for cs3231 in UNSW. Written by Maggie and me.

# Have to Know asst2
change the ker_open to transfer in to proc

change the buf size to sopyin str with str_buf_size and the uio_kinit buf size to nbyte
retval in write is changed by the offset change after uio
<!-- from comment in syscall.c  -->
This means that
 * if the first argument is 32-bit and the second is 64-bit, a1 is unused.

Change to the tf->v0 to retval in the function of split64to32() since if we use tf->v0, then the tf->v0 will be assign to retval, which is not we want .

Have a try to not open the dir but the .h of type define could not included.


#TODOs
- [x] TRUNC APPEND in the open (TRUNC is in vfs_open)
- [x] Errno in the open (Kassert,
- [ ] oversize file name and path name (assume this is done in vfs_open)
- [x] check all the system call return values
- [x] lock acquire for atomic operation
- [x] Open would have third argument when O_CREATE provided in flag. (direct passthrough, no need to care)


#Unkown Error
try to replace copyinstr by copyin in sys__write with Error code 6

#Question
asynchronously of different process to read and write a file using system call
- No need to care by the followup discuss in https://piazza.com/class/jdwg14qxhhb4kp?cid=209

#No Need to Implement
- file premission control
- Symboliic link
