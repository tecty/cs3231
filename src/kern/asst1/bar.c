#include <types.h>
#include <lib.h>
#include <synch.h>
#include <test.h>
#include <thread.h>

#include "bar.h"
#include "bar_driver.h"



/*
 * **********************************************************************
 * YOU ARE FREE TO CHANGE THIS FILE BELOW THIS POINT AS YOU SEE FIT
 *
 */

/* Declare any globals you need here (e.g. locks, etc...) */

// simple way to mix order by mix by customer himself
struct semaphore *mix_sem;
// lock the mixxing procedure for only one thread
struct lock *mix_lock;
// lock for reading the exist order in the system
struct lock *order_lock;



// semaphore for passing order
struct semaphore *order_sem_empty;
struct semaphore *order_sem_full;

// store the order as a long queue
struct barorder *order_queue[NCUSTOMERS];
// the pointer of the queue
int order_queue_hi, order_queue_lo; 



// lock for to get the order 
// struct lock *order_lock;


/*
 * **********************************************************************
 * FUNCTIONS EXECUTED BY CUSTOMER THREADS
 * **********************************************************************
 */

/*
 * order_drink()
 *
 * Takes one argument referring to the order to be filled. The
 * function makes the order available to staff threads and then blocks
 * until a bartender has filled the glass with the appropriate drinks.
 */

void order_drink(struct barorder *order)
{
    // wait until the system doesn't have order
    P(order_sem_full);

    // // CRITICAL REGION:: reading order in the sys.
    // lock_acquire(order_lock);

    // // push the order into queue
    // order_queue[order_queue_hi] = order;
    // order_queue_hi = (order_queue_hi +1 )%NCUSTOMERS;


    // // CRITICAL REGION END::
    // lock_release(order_lock);

    // the system has some order, wake up some bartender
    V(order_sem_empty);

    // // wait for the mix is finished
    // P(mix_sem);

    // mix by myself
    order->glass.contents[0] = 1;
    order->glass.contents[1] = 0;
    order->glass.contents[2] = 0;

}



/*
 * **********************************************************************
 * FUNCTIONS EXECUTED BY BARTENDER THREADS
 * **********************************************************************
 */

/*
 * take_order()
 *
 * This function waits for a new order to be submitted by
 * customers. When submitted, it returns a pointer to the order.
 *
 */

struct barorder *take_order(void)
{
    struct barorder *ret = NULL;
    // wait if the_order is empty
    P(order_sem_empty);


    // // CRITICAL REGION:: read order in the sys.
    // lock_acquire(order_lock);

    // // dequeue the order from the system
    // ret = order_queue[order_queue_lo];
    // order_queue_lo = (order_queue_lo +1)%NCUSTOMERS;

    // // CRITICAL REGION END ::
    // lock_release(order_lock);

    return ret;
}


/*
 * fill_order()
 *
 * This function takes an order provided by take_order and fills the
 * order using the mix() function to mix the drink.
 *
 * NOTE: IT NEEDS TO ENSURE THAT MIX HAS EXCLUSIVE ACCESS TO THE
 * REQUIRED BOTTLES (AND, IDEALLY, ONLY THE BOTTLES) IT NEEDS TO USE TO
 * FILL THE ORDER.
 */

void fill_order(struct barorder *order)
{

    /* add any sync primitives you need to ensure mutual exclusion
        holds as described */


    
    /* the call to mix must remain */
    // mix(order);
    (void) order;


}


/*
 * serve_order()
 *
 * Takes a filled order and makes it available to (unblocks) the
 * waiting customer.
 */

void serve_order(struct barorder *order)
{
    (void) order;
    // the mix is complete
    // BUG
    V(mix_sem); 


    // the system can take some order
    V(order_sem_full);
}



/*
 * **********************************************************************
 * INITIALISATION AND CLEANUP FUNCTIONS
 * **********************************************************************
 */


/*
 * bar_open()
 *
 * Perform any initialisation you need prior to opening the bar to
 * bartenders and customers. Typically, allocation and initialisation of
 * synch primitive and variable.
 */

void bar_open(void)
{

    // create the sem for mixxing.
    mix_sem = sem_create("mix_sem",0);
    KASSERT(mix_sem != NULL);

    // create the lock for mix
    mix_lock = lock_create("mix_lock");
    KASSERT(mix_lock != NULL);

    // create the lock for reading the order
    order_lock = lock_create("order_lock");
    KASSERT(order_lock!=NULL);
    // initial the order queue with null and 0, 0
    for(int i= 0; i< NCUSTOMERS; i ++){
        order_queue[i] = NULL;
    }
    order_queue_hi = order_queue_lo= 0;
    

    // create the order semaphore 
    order_sem_empty = sem_create("order_sem_empty",0);
    KASSERT(order_sem_empty);
    order_sem_full = sem_create("order_sem_full",1);
    KASSERT(order_sem_full);

}

/*
 * bar_close()
 *
 * Perform any cleanup after the bar has closed and everybody
 * has gone home.
 */

void bar_close(void)
{

    // destory the semaphore for mixxing
    sem_destroy(mix_sem);
    // destory the lock for mixxing procedure
    lock_destroy(mix_lock);
    // destory the lock for order
    lock_destroy(order_lock);

    // destory the semaphore for passing order
    sem_destroy(order_sem_empty);
    sem_destroy(order_sem_full);
}

