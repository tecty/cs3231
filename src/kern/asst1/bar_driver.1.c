#include "opt-synchprobs.h"
#include <types.h>
#include <lib.h>
#include <synch.h>
#include <test.h>
#include <thread.h>

#include "bar_driver.h"


/*
 * DEFINE THIS MACRO TO SWITCH ON MORE PRINTING
 *
 * Note: Your solution should work whether printing is on or off
 *
 */

/* #define PRINT_ON */

/* this semaphore is for cleaning up at the end. */
static struct semaphore *alldone;

/*
 * Data type used to track number of doses each bottle gives
 */

struct bottle {
        int doses;
};

struct bottle bottles[NBOTTLES];

static int customers;
static struct lock *cust_lock;

/* A function used to manage staff leaving */

static void go_home(void);

/*
 * **********************************************************************
 * CUSTOMERS
 *
 * Customers are rather simple, they arrive and give their order to a
 * bartender and wait for their drink.
 *
 * Eventually their glass arrives with the requested contents (exactly
 * as requested), they drink until the glass is empty, take a short
 * break, and do it all again until they have emptied the desired
 * number of glasses (customers passing out is not simulated).
 *
 * Lastly, they indicate to the bar staff that they have
 * finished for the day by calling go_home()
 *
 */

static void customer(void *unusedpointer, unsigned long customernum)
{
        struct barorder order;
        int i,j;

        (void) unusedpointer; /* avoid compiler warning */

        order.go_home_flag = 0;

        i = 0; /* count number of iterations */
        do {


#ifdef PRINT_ON
                kprintf("C %ld is ordering\n", customernum);
#endif

                /* Clear away previously-requested ingredients and select a new drink */
                for (j = 0; j < DRINK_COMPLEXITY; j++) {
                        order.requested_bottles[j] = 0;
                }

                /* I'll have a beer. */
                order.requested_bottles[0] = BEER;

                /* order the drink, this blocks until the order is fulfilled */
                order_drink(&order);

#ifdef PRINT_ON
                kprintf("C %ld drinking %d, %d, %d\n",
                                customernum,
                                order.glass.contents[0],
                                order.glass.contents[1],
                                order.glass.contents[2]);
#endif

                /* Drink up */
                for (j = 0; j < DRINK_COMPLEXITY; j++) {
                        order.glass.contents[j] = 0;
                }


                /* I needed that break.... */
                thread_yield();

                i++;
        } while (i < 10); /* keep going until .... */

#ifdef PRINT_ON
        kprintf("C %ld going home\n", customernum);
#else
        (void)customernum;
#endif

        /*
         * Now we go home.
         */
        go_home();
        V(alldone);
}


/*
 * **********************************************************************
 * BARTENDERS
 *
 * bartenders are only slightly more complicated than the customers.
 * They take_orders, and if valid, they fill them and serve them.
 * When all the customers have left, the bartenders go home.
 *
 *
 */
static void bartender(void *unusedpointer, unsigned long staff)
{

        struct barorder *order;
        int i;

        /* avoid compiler warning */
        (void)unusedpointer;
        (void)staff;

        i = 0; /* count orders filled for stats */
        while (1) {

#ifdef PRINT_ON
                kprintf("S %ld taking order\n", staff);
#endif

                order = take_order();

                if (order->go_home_flag == 0) {

#ifdef PRINT_ON
                        kprintf("S %ld filling\n", staff);
#endif


                        i++;
                        fill_order(order);

#ifdef PRINT_ON
                        kprintf("S %ld serving\n", staff);
#endif

                        serve_order(order);
                } else {
                        /* Immediately return the order without filling, and then go home */
                        serve_order(order);
                        break;
                }

        };

#ifdef PRINT_ON
        kprintf("S %ld going home after mixing %d drinks\n", staff, i);
#endif
        V(alldone);
}


/*
 * **********************************************************************
 * RUN THE BAR
 *
 * This routine sets up the bar prior to opening and cleans up after
 * closing.
 *
 * It calls two routines (bar_open() and bar_close() in bar.c) in which
 * you can insert your own initialisation code.
 *
 * It also prints some statistics at the end.
 *
 */

int run_bar(int nargs, char **args)
{
        int i, result;

        (void) nargs; /* avoid compiler warnings */
        (void) args;

        /* this semaphore indicates everybody has gone home */
        alldone = sem_create("alldone", 0);
        if (alldone == NULL) {
                panic("run_bar: out of memory\n");
        }

        /* initialise the bottle doses to 0 */
        for (i = 0 ; i < NBOTTLES; i++) {
                bottles[i].doses = 0;
        }

        /* initialise the count of customers and create a lock to
           facilitate updating the counter by multiple threads */
        customers = NCUSTOMERS;

        cust_lock = lock_create("cust lock");
        if (cust_lock == NULL) {
                panic("no memory");
        }

        /**********************************************************************
         * call your routine that initialises the rest of the bar
         */
        bar_open();

        /* Start the bartenders */
        for (i = 0; i<NBARTENDERS; i++) {
                result = thread_fork("bartender thread", NULL,
                                     &bartender, NULL, i);
                if (result) {
                        panic("run_bar: thread_fork failed: %s\n",
                              strerror(result));
                }
        }

        /* Start the customers */

        for (i=0; i<NCUSTOMERS; i++) {
                result = thread_fork("customer thread", NULL,
                                     &customer, NULL, i);
                if (result) {
                        panic("run_bar: thread_fork failed: %s\n",
                              strerror(result));
                }
        }

        /* Wait for everybody to finish. */
        for (i = 0; i < NCUSTOMERS + NBARTENDERS; i++) {
                P(alldone);
        }

        for (i = 0; i < NBOTTLES; i++) {
                kprintf("Bottle %d used for %d doses\n", i + 1,
                        bottles[i].doses);
        }

        /***********************************************************************
         * Call your bar clean up routine
         */
        bar_close();

        lock_destroy(cust_lock);
        sem_destroy(alldone);
        kprintf("The bar is closed, bye!!!\n");
        return 0;
}



/*
 * **********************************************************************
 * MIX
 *
 * This function take a glass and an order and mixes the
 * drink as required. It does it such that the contents
 * EXACTLY matches the request.
 *
 * Yes, mix counts double and triple servings of the same tint.
 *
 * MIX NEEDS THE ROUTINE THAT CALLS IT TO ENSURE THAT MIX HAS EXCLUSIVE
 * ACCESS TO THE BOTTLES IT NEEDS. And ideally, only exclusive access to
 * the tints that are required in the mix.
 *
 * YOU MUST USE THIS MIX FUNCTION TO FILL GLASSES. We use it for
 * testing when marking.
 *
 */

void mix(struct barorder *order)
{
        int i;

        /*
         * add drinks to the glass in order given and increment number of
         * doses from particular bottle
         */

        for (i = 0; i < DRINK_COMPLEXITY; i++){
                int bottle;
                bottle = order->requested_bottles[i];
                order->glass.contents[i] = bottle;

                if (bottle > NBOTTLES) {
                        panic("Unknown bottle");
                }
                if (bottle > 0) {
                        bottles[bottle-1].doses++;
                }
        }
}

/*
 * go_home()
 *
 * This function is called by customers when they go home. It is used
 * to keep track of the number of remaining customers to allow bartender
 * threads to exit when no customers remain.
 */


static void go_home(void)
{

        lock_acquire(cust_lock);
        customers --;

        /* the last customer to leave tells the staff to go home */
        if (customers == 0) {
                struct barorder go_home_order;
                int i;
                lock_release(cust_lock); /* don't hold the lock longer than strictly needed */
                go_home_order.go_home_flag = 1;

                for (i = 0; i < NBARTENDERS; i++) {
                        order_drink(&go_home_order); /* returns without order being filled */
                }
        } else {
                lock_release(cust_lock);
        }
}


