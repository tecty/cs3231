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

// the siple queue for order
struct barorder *order_queue[NBARTENDERS];
int order_hi, order_lo,drink_hi, drink_lo;

// lock for controlling changing queue
struct lock *order_lock;
// semaphore for consumer/ producer model of order list
struct semaphore *full_sem;
struct semaphore *empty_sem;

// lock array for custumer to wait for bartender to mix the drink
struct semaphore *drink_sem[NBARTENDERS];

// lock for permission to access bottles
struct lock *carbinet_lock;
// usage for each bottle
int bottle_usage[NBOTTLES];
// condition variable for waiting permission
struct cv *carbinet_cv;




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
	// kprintf("Customer start ordering ..\n");

    // wait if the order list is already full
    P(full_sem);

    // CRITICAL REGION:: changing and reading order list
    lock_acquire(order_lock);

    // push this order to the queue
    order_queue[order_hi] =order;

    // assign this drink to correspond bartender
    // by dequeue the drenk_sem
    order->wait_drink = drink_sem[drink_lo];
    // add up the drink lo
    drink_lo = (drink_lo + 1) % NBARTENDERS; 
    
    // increament the queue pointer
    order_hi  = (order_hi + 1 )% NBARTENDERS;

    // CRITICAL REGION END::
    lock_release(order_lock);

    // wake up any sleeping bartender to do this drink
    V(empty_sem);
    
    // waiting that bartender to finish the drink
    P(order->wait_drink);
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


    // Sleep until customer order a drink 
    P(empty_sem);

    // CRITICAL REGION:: Changing and reading order list
    lock_acquire(order_lock);

    // dequeue the order queue
    ret = order_queue[order_lo];
    order_lo = (order_lo +1) % NBARTENDERS;

    // CRITICAL REGION END::
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
    
    // sort the request order that meet the require of convension 
    // of takeing out bottle by same order (ascending order)
    // sort_requested_bottles(order);

    // // take the bottle form the carbinet
    // take_bottles(order);

    // /* the call to mix must remain */
    // mix(order);

    // // return the bottle to the carbinet
    // return_bottles(order);

    lock_acquire(carbinet_lock);
    while(!mixable(order)){
        cv_wait(carbinet_cv, carbinet_lock);
    }
    take_bottles(order);
    lock_release(carbinet_lock);
    mix(order);
    return_bottles(order);
    cv_signal(carbinet_cv, carbinet_lock);
}


/*
 * serve_order()
 *
 * Takes a filled order and makes it available to (unblocks) the
 * waiting customer.
 */

void serve_order(struct barorder *order)
{

    // CRITICAL REGION:: Changing the counter of drink queue
    lock_acquire(order_lock);

    // push drink queue by return the sem to drink_sem
    drink_sem[drink_hi] = order->wait_drink;
    
    // add up the counter of the drink queue
    drink_hi = (drink_hi + 1) % NBARTENDERS; 

    // wake up customer to enjoy the drink 
    // and this semaphore might be used by another customer
    V(order->wait_drink);

    // CRITICAL REGION END::
    lock_release(order_lock);

    // Some bartender can take order
    V(full_sem);
    
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
    // counter for looping
    int i;
    // tmp value to prevent memleak
    char *tmp_str ;

    // initial the order queue by reset order_hi and order_lo
    order_hi = order_lo = 0;
    // initial the drink sem queue by reset the drink_hi and drink_lo
    // Note: the inital queue is full of semaphore.
    drink_lo = drink_hi = 0;

    // continue: create order lock for order list
    order_lock = lock_create("order_lock");
    // continue: create carbinet lock 
    carbinet_lock = lock_create("carbinet_lock");
    // continue: create carbinet cv
    carbinet_cv = cv_create("carbinet_cv");
    // continue: create semaphore for consumer/ producer
    full_sem  = sem_create("order_full",NBARTENDERS);
    empty_sem = sem_create("order_empty",0);

    // initial the drink lock array to wait the mix is done
    for(i =0; i< NBARTENDERS; i ++){
        tmp_str = get_name("drink_sem",i);
        drink_sem[i] = sem_create(tmp_str,0);
        kfree(tmp_str);
    }

    // initial the current usage of each bottle
    for(i=0;i<NBOTTLES;i++){
        //0 means the bottle is not in use, 1 means the bottle is now in use
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
    // counter for the loop
    int i = 0;
    
    // destory the semaphore and lock use by order control
    lock_destroy(order_lock);
    sem_destroy(full_sem);
    sem_destroy(empty_sem);
    lock_destroy(carbinet_lock);

    // destory the drink lock array to wait the mix is done
    for(i =0; i< NBARTENDERS; i ++){
        sem_destroy(drink_sem[i]);
    }

    // destory the conditional varible of cv.
    cv_destroy(carbinet_cv);

    // // destory the bottle lock array
    // for(i=0; i < NBOTTLES; i++){
    //     lock_destroy(bottle_lock[i]);
    // }

}

/*
 * **********************************************************************
 * HELPER FUNCTION AREA
 * **********************************************************************
 */

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
				order->requested_bottles[i+1] =
					tmp_bot;
                swap = 1;
			}
		}
	}
}


void take_bottles(struct barorder * this_order){
    // counter for the loop
    int i;
    for(i=0; i < DRINK_COMPLEXITY; i ++){
        if(this_order->requested_bottles[i]){
            bottle_usage[
                this_order->requested_bottles[i]-1
            ] = 1;
        }
    }

}
void return_bottles(struct barorder * this_order){
    // counter for the loop
    int i;
    for(i=DRINK_COMPLEXITY -1; i >= 0; i --){
        // must use reverse order to release the lock
        if(this_order->requested_bottles[i]){
            bottle_usage[
                this_order->requested_bottles[i]-1
            ] = 0;
        }
    }

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

int mixable(struct barorder *order) {
	int i;
	for (i = 0; i < DRINK_COMPLEXITY; i++) {
		if (order->requested_bottles[i] != 0 && bottle_usage[order->requested_bottles[i] - 1] > 0)
			return 0; //the bottle required in the order is being used by someone else, so this order need to wait
	}
	return 1; //all the bottles are available for mixing, so this order can mix right now
}