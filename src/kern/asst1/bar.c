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

//Semaphore array recording which customer is waiting for which bartender serving the order
struct semaphore *waiting_bartender[NBARTENDERS];

//Lock array recording which bottle is in used 
struct lock *bottle_lock[NBOTTLES];
//Array that records which bottle is in used
int bottle_usage[NBOTTLES];



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
	kprintf("Customer start ordering ..\n");
	while (orderList[hi] != NULL) {
		hi = (hi + 1) % NBARTENDERS;
	}
	order->serving_no = hi;
	orderList[hi] = order;
	hi = (hi + 1) % NBARTENDERS;
	lock_release(order_lock); //customer finishes adding his order
	V(empty_sem); //awake any bartender to handle the order if the list was empty

	//now wait the corresponding bartender to finish the order, then continue
	P(waiting_bartender[order->serving_no]);
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
	P(empty_sem); //if no order provided right now, the bartenders will wait
	lock_acquire(order_lock);  //bartender modifies the list to take an order
	kprintf("Bartender start taking orders ..\n");
	while (orderList[lo] == NULL) {
		lo = (lo + 1) % NBARTENDERS;
	}
	//take out the order from the list
	struct barorder *ret = orderList[lo];
	orderList[lo] = NULL;
	lo = (lo + 1) % NBARTENDERS;
	lock_release(order_lock); //bartender finishes taking an order
	V(full_sem); //awake any customer to continue adding new orders into the list
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
	// kprintf("Bartender start filling orders ..\n");
	// while (!mixable(order)) {
	// }
	/* the call to mix must remain */

	
	take_bottles(order);
	mix(order);
	return_bottles(order);
}



/*
* serve_order()
*
* Takes a filled order and makes it available to (unblocks) the
* waiting customer.
*/

void serve_order(struct barorder *order)
{
	//now awake the customer who is waiting for the order
	kprintf("Bartender start serving orders ..\n");
	V(waiting_bartender[order->serving_no]);
	return;
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
	//initialize the order list
	for (i = 0; i < NBARTENDERS; i++) {
		orderList[i] = NULL;
	}
	//initialize the two semaphores of the order list
	empty_sem = sem_create("empty_sem", 0);
	if (empty_sem == NULL) {
		panic("empty semaphore create failed");
	}
	full_sem = sem_create("full_sem", NBARTENDERS);
	if (full_sem == NULL) {
		panic("full semaphore create failed");
	}
	//initialize the permission lock for modifying the order list
	order_lock = lock_create("order_lock");
	KASSERT(order_lock != 0);
	//initialize the semaphore array for waiting orders
	for (i = 0; i < NBARTENDERS; i++) {
		waiting_bartender[i] = sem_create(get_name("bartender", i), 0);
		if (waiting_bartender[i] == NULL) {
			panic("waiting_bartender semaphore create failed");
		}
	}
	//initialize the permission lock for taking out bottles
	//initialize the lock array for bottles
	for (i = 0; i < NBOTTLES; i++) {
		bottle_lock[i] = lock_create(get_name("bottle", i));
		KASSERT(bottle_lock[i] != 0);
	}
	//initialize the control valuable for managing bottles
	//initialize the usage array of bottles;
	for (i = 0; i < NBOTTLES; i++) {
		bottle_usage[i] = 0;
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
	int i; //counter for looping
	sem_destroy(empty_sem);
	sem_destroy(full_sem);
	lock_destroy(order_lock);
	for (i = 0; i < NBARTENDERS; i++) {
		sem_destroy(waiting_bartender[i]);
	}
	for (i = 0; i < NBOTTLES; i++) {
		lock_destroy(bottle_lock[i]);
	}
}


void sort_requested_bottles(struct barorder *order){
	// simple bubble sort to meet the requirement of convenstion
	// which is require the bottle by accending order
	int swap =1;

	// set up the temp value to record bottle id
	unsigned int tmp_bot;

	while(swap){
		// reset the flag value
		swap = 0;
		for (int i =0 ; i < DRINK_COMPLEXITY -1; i++){
			if (order->requested_bottles[i] > 
				order->requested_bottles[i+1]){
				// swap the bottle require order
				tmp_bot 						  =
					order->requested_bottles[i];
				order->requested_bottles[i] =
					order->requested_bottles[i+1];
				order->requested_bottles[i] =
					tmp_bot;
			}
		}
	}
}

void take_bottles(struct barorder *order) {
	int i;
	for (i = 0; i < DRINK_COMPLEXITY; i++) {
		if (order->requested_bottles[i] != 0) {
			bottle_usage[order->requested_bottles[i]-1]++;
			lock_acquire(bottle_lock[order->requested_bottles[i] - 1]);
		}
	}
}

void return_bottles(struct barorder *order) {
	int i;
	for (i = 0; i < DRINK_COMPLEXITY; i++) {
		if (order->requested_bottles[i] != 0) {
			bottle_usage[order->requested_bottles[i] - 1]--;
			lock_release(bottle_lock[order->requested_bottles[i] - 1]);
		}
	}
}

int mixable(struct barorder *order) {
	int i;
	for (i = 0; i < DRINK_COMPLEXITY; i++) {
		if (order->requested_bottles[i] != 0 && bottle_usage[order->requested_bottles[i] - 1] > 0)
			return 0; //the bottle required in the order is being used by someone else, so this order need to wait
	}
	return 1; //all the bottles are available for mixing, so this order can mix right now
}
//function for generating names for semaphores and locks
char *get_name(const char *main_name, int count) {
	int str_len = 0;
	while (main_name[str_len] != '\0') {
		str_len++;
	}

	// malloc the return char's space
	char *ret = kmalloc(str_len * 4 + 12);
	for (int i = 0; i< str_len; i++) {
		// strcpy
		ret[i] = main_name[i];
	}
	// get the count to the name
	ret[str_len] = '0' + count / 10;
	ret[str_len + 1] = '0' + count % 10;
	// get the null at the end
	ret[str_len + 2] = '\0';
	return ret;
}

