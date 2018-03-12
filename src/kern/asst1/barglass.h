#ifndef BARGLASS_H
#define BARGLASS_H
/*
 * **********************************************************************
 *
 * YOU SHOULD NOT RELY ON ANY CHANGES YOU MAKE TO THIS FILE
 *
 * We will use our own version of this file for testing
 */




/*
 *
 * Define the number of drink types available and their symbolic constants. 
 *
 */
#define BEER 1
#define VODKA 2
#define RUM 3
#define GIN 4
#define TEQUILA 5
#define BRANDY 6
#define WHISKY 7
#define BOURBON 8
#define TRIPLE_SEC 9
#define ORANGE_JUICE 10
#define NBOTTLES 10


/*
 * The maximum number of drinks that can be mixed in a single glass
 */
#define DRINK_COMPLEXITY 3


/*
 * The data type representing a glass.
 */ 
struct glass {
        /* the actual contents of the can */
        unsigned int contents[DRINK_COMPLEXITY];
};

#endif
