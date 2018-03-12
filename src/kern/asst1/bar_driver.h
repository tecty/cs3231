/*
 * **********************************************************************
 *
 * Define function prototypes, types, and constants needed by both the
 * driver (bar_driver.c) and the code you need to write (bar.c)
 *
 * YOU SHOULD NOT RELY ON ANY CHANGES YOU MAKE TO THIS FILE
 *
 * We will use our own version of this file for testing
 */

#include "barglass.h"
#include "bar.h"


extern int run_bar(int, char**);

/*
 * FUNCTION PROTOTYPES FOR THE FUNCTIONS YOU MUST WRITE
 *
 * YOU CANNOT MODIFY THESE PROTOTYPES
 *  
 */

/* Customer functions */
extern void order_drink(struct barorder *);


/* Bar staff functions */ 
extern struct barorder * take_order(void);
extern void fill_order(struct barorder *);
extern void serve_order(struct barorder *);


/* Bar opening and closing functions */
extern void bar_open(void);
extern void bar_close(void);


/*
 * Function prototype for the supplied routine that mixes the various
 * bottle contents into a glass.
 *
 * YOU MUST USE THIS FUNCTION FOR MIXING
 *
 */
extern void mix(struct barorder *);


/*
 * THESE PARAMETERS CAN BE CHANGED BY US, so you should test various
 * combinations. NOTE: We will only ever set these to something
 * greater than zero.
 */ 

#define NCUSTOMERS 10 /* The number of customers drinking today */
#define NBARTENDERS 3 /* The number of bartenders working today */

