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
//Order list that all bartenders can handle
struct barorder *orderList[NBARTENDERS]; 
//Semaphore controlling the limits of the list
struct semaphore *full_sem;
struct semaphore *empty_sem;
//Permission for modifying the order list
struct lock *order_lock;
int hi, lo; //queue pointer in the order list

//the semaphore array recording which customer is waiting for which bartender serving the order
struct semaphore *waiting_bartender[NBARTENDERS];



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
	P(full_sem); //if all bartenders are busy, the order will wait
	lock_acquire(order_lock); //customer modifies the list to add his order
	// add the order here .. 
	while (orderList[hi] != NULL) {
		hi = (hi + 1) % NBARTENDERS;
	}
	orderList[hi] = order;
	hi = (hi + 1) % NBARTENDERS;
	lock_release(order_lock); //customer finishes adding his order
	V(empty_sem); //awake any bartender to handle the order if the list was empty


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
	(void)order; /* avoid a compiler warning, remove when you
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
	int i; //counter for looping
	//initialize the two semaphores of the order list
	empty_sem = semaphore_create("empty_sem", NBARTENDERS);
	if (empty_sem == NULL) {
		panic("empty semaphore create failed");
	}
	full_sem = semaphore_create("full_sem", 0);
	if (full_sem == NULL) {
		panic("full semaphore create failed");
	}
	//initialize the permission lock for modifying the order list
	order_lock = lock_create("order_lock");
	KASSERT(order_lock != 0);
	//initialize the semaphore array for waiting orders
	for (i = 0; i < NBARTENDERS; i++) {
		waiting_bartender[i] = semaphore_create("bartender",)
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

