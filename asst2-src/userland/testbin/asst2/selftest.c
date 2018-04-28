#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <err.h>
#include <errno.h>
//additional header file
#include <fcntl.h>

#define MAX_BUF 500
char teststr[] = "These are some testing texts planned to be input into/output from a file.";
char buf[MAX_BUF];

int
main(int argc, char * argv[])
{
    int fd, r, i, j , k;
    (void) argc;
    (void) argv;


    printf("\n**********\n* Self-Made File Tester\n");

    //pre-check if stdout and stderr is linked well
    snprintf(buf, MAX_BUF, "**********\n* write() works for stdout\n");
    write(1, buf, strlen(buf));
    snprintf(buf, MAX_BUF, "**********\n* write() works for stderr\n");
    write(2, buf, strlen(buf));

    //skip the test for open() with three arguments (no need to consider in sys161)

    //first try to open a new file normally
    printf("**********\n* opening new file \"test2.file\"\n");
    fd = open("test2.file", O_RDWR | O_CREAT );
    //printf("* open() got fd %d\n", fd);
    //check the handle is greater or equal to 0
    if (fd < 0) {
        printf("ERROR opening file: %s\n", strerror(errno));
        exit(1);
    }
    printf("PASS\n");
    
    //Error: ENOTDIR, ENOENT, EISDIR, EIO are examined in vfslookup
    //So we skip the test

    //test writing stuff into the valid opened file
    printf("* normal writing test string into valid opened file\n");
    r = write(fd, teststr, strlen(teststr));
    printf("* wrote %d bytes\n", r);
    if (r < 0) {
        printf("ERROR writing file: %s\n", strerror(errno));
        exit(1);
    }
    printf("PASS\n");

    //test writing stuff into the invalid non-opened file
    printf("* writing test string into invalid non-opened file descriptor 80\n");
    r = write(80, teststr, strlen(teststr));
    if (r >= 0) {
        printf("INCORRECT writing: 80 should not be used as a fd yet\n");
        exit(1);
    }
    //else, check if print out correct error message: EBADF
    if(errno!=30){
        printf("INCORRECT error message: %s\n", strerror(errno));
        printf("The correct error message should be \"Bad file number\"\n");
        exit(1);
    }
    else{
        printf("PASS\n");
    }

    //close the modified file
    printf("* closing file\n");
    close(fd);

    //test if reopening the modified file is available
    printf("**********\n* opening old file \"test2.file\"\n");
    fd = open("test2.file", O_RDONLY);
    if (fd < 0) {
        printf("ERROR opening file: %s\n", strerror(errno));
        exit(1);
    }
    printf("PASS\n");

    //test normal reading the file into buffer
    //first test if reading-in works well
    printf("* reading entire file into buffer\n");
    i = 0;
    do  {
        printf("* attempting read of %d bytes\n", MAX_BUF -i);
        r = read(fd, &buf[i], MAX_BUF - i);
        printf("* read %d bytes\n", r);
        i += r;
    } while (i < MAX_BUF && r > 0);

    printf("* reading complete\n");
    if (r < 0) {
        printf("ERROR reading file: %s\n", strerror(errno));
        exit(1);
    }
    //then check if the readedcontent is correct
    k = j = 0;
    r = strlen(teststr);
    do {
        if (buf[k] != teststr[j]) {
            printf("ERROR  file contents mismatch\n");
            exit(1);
        }
        k++;
        j = k % r;
    } while (k < i);
    printf("PASS\n");

    //test reading from an invalid file descriptor
    printf("* reading from an invalid file descriptor 80\n");
    i = 0;
    do  {
        printf("* attempting read of %d bytes\n", MAX_BUF -i);
        r = read(fd, &buf[i], MAX_BUF - i);
        //check if print out correct error message: EBADF
        if(r<0){
            if(errno!=30){
                printf("INCORRECT error message: %s\n", strerror(errno));
                printf("The correct error message should be \"Bad file number\"\n");
                exit(1);
            }
            else{
                printf("PASS\n");
            }
            break;
        }
        printf("* read %d bytes\n", r);
        i += r;
    } while (i < MAX_BUF && r > 0);

    printf("* reading complete\n");
    if (r >= 0) {
        printf("INCORRECT reading file: the file descriptor should not exist\n");
        exit(1);
    }

    //test reading file into an invalid address
    printf("* reading into an invalid buffer address\n");
    i = 0;
    do  {
        printf("* attempting read of %d bytes\n", MAX_BUF -i);
        r = read(fd, &buf[i], MAX_BUF - i);
        //check if print out correct error message: EBADF
        if(r<0){
            if(errno!=6){
                printf("INCORRECT error message: %s\n", strerror(errno));
                printf("The correct error message should be \"Bad memory reference\"\n");
                exit(1);
            }
            else{
                printf("PASS\n");
            }
            break;
        }
        printf("* read %d bytes\n", r);
        i += r;
    } while (i < MAX_BUF && r > 0);
    printf("* reading complete\n");
    if (r >= 0) {
        printf("INCORRECT reading file: the reading buffer should be invalid\n");
        exit(1);
    }

    //check if normal lseek() is usable
    printf("**********\n* testing lseek\n");
    r = lseek(fd, 5, SEEK_SET);
    if (r < 0) {
        printf("ERROR lseek: %s\n", strerror(errno));
        exit(1);
    }
    printf("PASS\n");

    //check if lseek fails if fd is invalid
    printf("* seeking invalid fd 80\n");
    r = lseek(80, 5, SEEK_SET);
    if(r>=0){
        printf("INCORRECT lseek: fd 80 is not valid\n");
        exit(1);
    }
    //else, check  if print out correct error message: EBADF
    if(errno!=30){
        printf("INCORRECT error message: %s\n", strerror(errno));
        printf("The correct error message should be \"Bad file number\"\n");
        exit(1);
    }
    else{
        printf("PASS\n");
    }

    //check if seeking console will fail
    printf("* seeking console file\n");
    r = lseek(1, 5, SEEK_SET);
    if(r>=0){
        printf("INCORRECT lseek: console cannot be seeken\n");
        exit(1);
    }
    //else, check  if print out correct error message: ESPIPE
    if(errno!=33){
        printf("INCORRECT error message: %s\n", strerror(errno));
        printf("The correct error message should be \"Illegal seek\"\n");
        exit(1);
    }
    else{
        printf("PASS\n");
    }

    //check if seeking with invalid whence will fail
    printf("* seeking with invalid whence 9\n");
    r = lseek(fd, 5, 9);
    if(r>=0){
        printf("INCORRECT lseek: whence 9 is not defined\n");
        exit(1);
    }
    //else, check  if print out correct error message: EINVAL
    if(errno!=8){
        printf("INCORRECT error message: %s\n", strerror(errno));
        printf("The correct error message should be \"Invalid argument\"\n");
        exit(1);
    }
    else{
        printf("PASS\n");
    }

    //check if resulting seek position to be negative will fail
    printf("* seeking with resulting the final seek position to be negative\n");
    r = lseek(fd, -1, SEEK_SET);
    if(r>=0){
        printf("INCORRECT lseek: the final seek position is negative\n");
        exit(1);
    }
    //else, check  if print out correct error message: EINVAL
    if(errno!=8){
        printf("INCORRECT error message: %s\n", strerror(errno));
        printf("The correct error message should be \"Invalid argument\"\n");
        exit(1);
    }
    else{
        printf("PASS\n");
    }

    //check if lseek runs well with correct offset-changing
    //recheck starting offset is correct
    r = lseek(fd, 5, SEEK_SET);
    //now read some buffers and check if the result is as expected
    printf("* seeking and reading 10 bytes of file into buffer \n");
    i = 0;
    do  {
        printf("* attempting read of %d bytes\n", 10 - i );
        r = read(fd, &buf[i], 10 - i);
        printf("* read %d bytes\n", r);
        i += r;
    } while (i < 10 && r > 0);
    printf("* reading complete\n");
    if (r < 0) {
        printf("ERROR reading file: %s\n", strerror(errno));
        exit(1);
    }

    k = 0;
    j = 5;
    r = strlen(teststr);
    do {
        if (buf[k] != teststr[j]) {
            printf("ERROR  file contents mismatch\n");
            exit(1);
        }
        k++;
        j = (k + 5)% r;         
    } while (k < 5);

    printf("* PASS\n");
    printf("* closing file\n");
    close(fd);

    //skip the test for O_APPEND MODE (vfs_open does not require that)

    //test open() fails if opening an existing file with both the modes of O_EXCL and O_CREAT
    printf("**********\n* opening an existing file with both the modes O_EXCL and O_CREAT\n");
    fd = open("test2.file", O_RDWR | O_EXCL | O_CREAT);
    if(fd >= 0){
        printf("INCORRECT file opening: O_EXCL should force open() to fail because test2.file already exists\n");
        exit(1);
    }
    //else, check if print out correct error message: EEXIST
    if(errno!=22){
        printf("INCORRECT error message: %s\n", strerror(errno));
        printf("The correct error message should be \"File or object exists\"\n");
        exit(1);
    }
    else{
        printf("PASS\n");
    }

    //normal test if dup2() works
    printf("**********\n* duplicate valid fd number\n");
    r = dup2(fd,1);
    if(r<0){
        printf("ERROR dup2\n");
        exit(1);
    }
    if(r!=fd){
        printf("INCORRECT dup2: result != old fd\n");
        exit(1);
    }
    printf("PASS\n");

    //test if dup2() will fail when new fd is not a valid handler
    printf("**********\n* duplicate invalid fd number in new fd\n");
    r = dup2(fd, 9);
    if(r>=0){
        printf("INCORRECT dup2 operation: new file descriptor is not valid hence cannot be closed\n");
        exit(1);
    }
    //else, check if print out correct error message: EBADF
    if (errno != 30)
    {
        printf("INCORRECT error message: %s\n", strerror(errno));
        printf("The correct error message should be \"Bad file number\"\n");
        exit(1);
    }
    else
    {
        printf("PASS\n");
    }

    //test if dup2() will fail when old fd is not a valid handler
    printf("**********\n* duplicate invalid fd number in old fd\n");
    r = dup2(9, fd);
    if (r >= 0)
    {
        printf("INCORRECT dup2 operation: old file descriptor is not valid hence cannot be closed\n");
        exit(1);
    }
    //else, check if print out correct error message: EBADF
    if (errno != 30)
    {
        printf("INCORRECT error message: %s\n", strerror(errno));
        printf("The correct error message should be \"Bad file number\"\n");
        exit(1);
    }
    else
    {
        printf("PASS\n");
    }

    printf("ALL TESTS PASS\n");

}