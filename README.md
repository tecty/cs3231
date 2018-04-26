# cs3231
OS161 for cs3231 in UNSW. Written by Maggie and me.

# Have to Know asst2
change the ker_open to transfer in to proc
change the buf size to sopyin str with str_buf_size and the uio_kinit buf size to nbyte
retval in write is changed by the offset change after uio
<!-- from comment in syscall.c  -->
This means that
 * if the first argument is 32-bit and the second is 64-bit, a1 is
 * unused.
#TODOs
- TRUNC in the open
- Errno in the open (Kassert )
- check all the system call return values
- lock acquire for atomic operation 


#Unkown Error
try to replace copyinstr by copyin in sys__write with Error code 6
