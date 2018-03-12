#ifndef PRODUCERCONSUMER_DRIVER_H
#define PRODUCERCONSUMER_DRIVER_H

/*  This file contains constants, types, and prototypes for the
 *  producer consumer problem. It is included by the driver and the
 *  file you modify so as to share the definitions between both.

 *  YOU SHOULD NOT CHANGE THIS FILE

 *  We will replace it in testing, so any changes will be lost.
 */


#define BUFFER_SIZE 10   /* The size of the bounded buffer */

/*  The buffer must be exactly the size of the constant defined here.
 *
 * The producer_send() will block if more than this size is sent to
 * the buffer, but won't block while there is space in the buffer.
 */



/* This is a type definition of the data that you will be passing
 * around in your own data structures
 */
struct pc_data {
        int item1;
        int item2;
};


extern int run_producerconsumer(int, char**);



/* These are the prototypes for the functions you need to write in
   producerconsumer.c */
struct pc_data consumer_receive(void); /* receive a data item, blocking
                                       if no item is available is the
                                       shared buffer */

void producer_send(struct pc_data); /* send a data item to the shared
                                       buffer */

void producerconsumer_startup(void); /* initialise your buffer and
                                        surrounding code */

void producerconsumer_shutdown(void); /* clean up your system at the
                                         end */
#endif 
