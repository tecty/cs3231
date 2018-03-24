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


// semaphore for passing order
struct semaphore *order_sem_empty;
struct semaphore *order_sem_full;

// temporary to store the order 
struct barorder *the_order;

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

    // pass a order into system 
    the_order = order;

    // the system has some order, wake up some bartender
    V(order_sem_empty);

    // wait for the mix is finished
    P(mix_sem);
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

    // get the only order in the system
    ret = the_order;


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

    // remove compile error 
    // kprintf("try to mix some order\n");
    lock_acquire(mix_lock);
    
    /* the call to mix must remain */
    mix(order);

    lock_release(mix_lock);

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

    // create the lock for mix
    mix_lock = lock_create("mix_lock");

    // create the order semaphore 
    order_sem_empty = sem_create("order_sem_empty",0);
    order_sem_full = sem_create("order_sem_full",1);



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


    // destory the semaphore for passing order
    sem_destroy(order_sem_empty);
    sem_destroy(order_sem_full);
}

