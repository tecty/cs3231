#include <types.h>
#include <lib.h>
#include <synch.h>
#include <test.h>
#include <thread.h>
#include <ctype.h>

#include "bar.h"
#include "bar_driver.h"



/*
 * **********************************************************************
 * YOU ARE FREE TO CHANGE THIS FILE BELOW THIS POINT AS YOU SEE FIT
 *
 */

/* Declare any globals you need here (e.g. locks, etc...) */
//pointers used in the order list
int hi, lo;
//lock used for the order list
struct lock *order_lock;
//semaphore used for the order list
struct simaphore *order_sem;
//the order list
struct barorder *orders[NCUSTOMERS];
//array of locks for recording the status of each bottle
struct lock *bottle_lock[NBOTTLES];
//array of semaphores for customers ready to serve
struct semaphore *customer_sem[NCUSTOMERS];

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
	//decrement the semaphore -> add orders
	P(order_sem);
	//acquire the lock
	//Now this customer is making the order, the order list should not be used by anyone else
	lock_acquire(order_lock);
	
	//Now write down the order
	orders[hi] = order;
	hi = (hi + 1) % NCUSTOMERS;

	//release the order list
	lock_release(order_lock);

	//block until the drink is ready
	
	//some operations .. 

	return;
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
	//increment a semaphore -> take orders
	V(order_sem);

	//acquire the lock
	//Now the bartender chooses a order to take, no other people should interrupt him ..
	lock_acquire(order_lock);
    struct barorder *ret = orders[lo];
	lo = (lo - 1) % NCUSTOMERS;
	//Now release the order list
	lock_release(order_lock);

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
    mix(order);

}


/*
 * serve_order()
 *
 * Takes a filled order and makes it available to (unblocks) the
 * waiting customer.
 */

void serve_order(struct barorder *order)
{
        (void) order; /* avoid a compiler warning, remove when you
                         start */
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
		//initialize the order list and its relational tools;
		hi = lo = 0;
		order_lock = lock_create("order_lock");
		KASSERT(order_lock != 0);
		order_sem = sem_create("order_sem", NCUSTOMERS);
		if (order_sem == NULL) {
			panic("sem create failed");
		}
		//initialize the locks for all bottles
		int i;
		for (i = 0; i < NBOTTLES; i++) {
			bottle_lock[i] = lock_create(itoa(i));
		}
		//initialize the list of semaphores for customers
		for (i = 0; i < NCUSTOMERS; i++) {
			customer_sem[i] = sem_create(itoa)
		}

}

/*
 * bar_close()
 *
 * Perform any cleanup after the bar has closed and everybody
 * has gone home.
 */

void bar_close(void)
{

}

