/* This file will contain your solution. Modify it as you wish. */
#include <types.h>
#include <synch.h>
#include "producerconsumer_driver.h"

/* Declare any variables you need here to keep track of and
   synchronise your bounded. A sample declaration of a buffer is shown
   below. You can change this if you choose another implementation. */

static struct pc_data buffer[BUFFER_SIZE];

// state of the buffer
int hi,lo;
// lock use by this model
struct lock *pc_lock;
// semaphore of the buffer
struct semaphore * buff_sem ;

/* consumer_receive() is called by a consumer to request more data. It
   should block on a sync primitive if no data is available in your
   buffer. */

struct pc_data consumer_receive(void)
{
        struct pc_data thedata;

        // increment a semaphore
        V(buff_sem);

        // acquire the lock
        lock_acquire(pc_lock);
        
        // copy data from buffer
        thedata.item1 = buffer[lo].item1;
        thedata.item2 = buffer[lo].item2;

        // increatement the state of buffer
        lo = (lo +1) % BUFFER_SIZE;
        
        // return the lock
        lock_release(pc_lock);

        return thedata;
}

/* procucer_send() is called by a producer to store data in your
   bounded buffer. */

void producer_send(struct pc_data item)
{
        // decrement the semaphore
        P(buff_sem);
        // acquire the lock
        lock_acquire(pc_lock);

        // store the things to buffer
        buffer[hi].item1 = item.item1;
        buffer[hi].item2 = item.item2;
        
        // increatement the state of buffer
        hi = (hi +1) % BUFFER_SIZE;
        
        // return the lock
        lock_release(pc_lock);
}




/* Perform any initialisation (e.g. of global data) you need
   here. Note: You can panic if any allocation fails during setup */

void producerconsumer_startup(void)
{
        // initial the counter for buffer
        hi = lo = 0;

        // create lock use in this model
        pc_lock = lock_create("pc_lock");

        //semaphore used to record the storage count
        buff_sem = sem_create("storage", BUFFER_SIZE);
}

/* Perform any clean-up you need here */
void producerconsumer_shutdown(void)
{
        // destory the lock
        lock_destroy(pc_lock);

        // destroy semaphore
        sem_destroy(buff_sem);
}

