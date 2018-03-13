#include <types.h>
#include <lib.h>
#include <synch.h>
#include <test.h>
#include <thread.h>

#include <lib.h>
#include "bar.h"
#include "bar_driver.h"



/*
 * **********************************************************************
 * YOU ARE FREE TO CHANGE THIS FILE BELOW THIS POINT AS YOU SEE FIT
 *
 */
/* Declare any globals you need here (e.g. locks, etc...) */

// create a pointer array to store all the bottle's lock
struct lock *bottle_lock[NBOTTLES];
// a lock for acquire bottle
struct lock *acquire_bottle_lock;

// create an array to store the bartender
struct lock *bartender_lock[NBARTENDERS];

// create an array to record all the order
struct barorder *order_queue[NCUSTOMERS];
// lock for editing queue
struct lock * order_lock;
// value for the order queue
int order_hi, order_lo;
// semaphore for the order queue
struct semaphore *order_sem;



char *get_lock_name(const char *main_name, int count);

char *get_lock_name(const char *main_name, int count){
        int str_len = 0;
        while (main_name[str_len]!= '\0'){
                str_len ++;
        }

        // malloc the return char's space
        char *ret  = kmalloc(str_len*4 + 12);
        for(int i= 0; i< str_len; i ++){
                // strcpy
                ret[i] = main_name[i];      
        }
        // get the count to the name
        ret[str_len] = '0' + count/10;
        ret[str_len+1] = '0' + count % 10;
        // get the null at the end
        ret[str_len+2] ='\0';
        return ret;
}


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
        // require semaphore for order
        P(order_sem);
        // require lock for order
        lock_acquire(order_lock);
                order_queue[order_hi] = order;
                // increment the hi for this order array
                order_hi = (order_hi+1 )% NCUSTOMERS; 
        // release the lock
        lock_release(order_lock);

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
        // increase the semaphore of order
        V(order_sem);

        // acquire lock for order
        lock_acquire(order_lock);
                // get the order from list
                struct barorder *ret = order_queue[order_lo];
                // increment the lo 
                order_lo = (order_lo +1) % NCUSTOMERS;
        // release the lock 
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
        lock_acquire(acquire_bottle_lock);
                for(int i = 0; i < DRINK_COMPLEXITY; i++)
                        // acquire all the bottles' lock
                        lock_acquire(bottle_lock[
                                        order->requested_bottles[i]
                                ]);
        lock_release(acquire_bottle_lock);


                        /* the call to mix must remain */
                        mix(order);

        for(int i = 0; i < DRINK_COMPLEXITY; i++)
                // release all the lock 
                lock_release(bottle_lock[
                                order->requested_bottles[i]
                        ]);

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
        
        // initial the lock for all the bottle
        for( int i = 0; i < NBOTTLES; i++){
                // create the lock by bottles' lock by its id 
                bottle_lock[i] = lock_create(
                        (const char *)get_lock_name("bottle_lock_",i)
                );
        }
        // initial lock for acquire bottle
        acquire_bottle_lock = lock_create("acquire_bottle_lock");


        // inital bartender's lock 
        for(int i = 0; i< NBARTENDERS; i ++){
                bartender_lock[i] = lock_create(
                        (const char *)get_lock_name("bartaner_lock_",i)
                );
        }

        // initial order queue's state
        order_hi = order_lo =0;
        // initial semaphore for the order queue
        order_sem = sem_create("order_sem",NCUSTOMERS); 
        // create lock for order
        order_lock = lock_create("order_lock");


}

/*
 * bar_close()
 *
 * Perform any cleanup after the bar has closed and everybody
 * has gone home.
 */

void bar_close(void)
{
        // destroy the lock for bottle
        for( int i = 0; i < NBOTTLES; i++){
                lock_destroy(bottle_lock[i]);
        }
        // destory the acquire lock 
        lock_destroy(acquire_bottle_lock);

        // destory bartender's lock 
        for(int i = 0; i< NBARTENDERS; i ++){
                lock_destroy(bartender_lock[i]);
        }

        // destory semahpore for the order queue
        sem_destroy(order_sem);
        // destory the lock for this orders
        lock_destroy(order_lock);
}

