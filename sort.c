#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/* ASIZE is the size of the array to sort (number of 32-bit integers) */
#define ASIZE 100
/* NFCNS is the number of different sort functions defined in the comp array */
#define NFCNS 1

/* defines the type compare_fcn to be a pointer to a function that takes two
 * constant void pointers and returns an integer. */
typedef int (*compare_fcn)(const void *, const void *);

/* randomly permute the elements in an array of size 32-bit integers.
 * Permutation is done in place. */
void shuffle(uint32_t *a, int size) {
    int i = 0;	/* Scratch */

    /* Seed the random number generator with the current time.  Since time(2)
     * returns a time_t, the code casts it properly. */
    srandom((unsigned int) time(NULL));

    /* Permute the elements */
    for (i = 0; i<size; i++) {
	int j = random() % size;
	int k = random() % size;
	int tmp = 0;

	if (j == k) continue;
	tmp = a[j];
	a[j] = a[k];
	a[k] = tmp;
    }
}

/* Print the 32-bit array (of size integers) to stdout.  Print the integers as
 * 3-digit, zero-padded integers, 10 integers to a line. */
void dump_array(uint32_t *a, int size) {
    int i = 0;	/* Scratch */

    for ( i = 0; i < size; i++) {
	if ( i % 10 == 0) printf("\n");
	printf("%03d ", a[i]);
    }
    if ((size -1) % 10 != 0 ) printf("\n");
}

/* 
 * qsort comparison functions compare the data pointed to by two pointers.  In
 * this program, the data is always interpreted as 32-bit, unsgned integers
 * (uint32_t's).  Comparison functions need to cast the pointers into the right
 * type and carry out the comparison.  The return value is <0 if the first data
 * should appear before the second, >0 if the first should appear after the
 * second, and 0 if there is no preference.
 */

/*
 * Compare the elements pointed to b a and b to each other as 32-bit integers.
 */
int compare_ab(const void *a, const void *b) {
    const uint32_t *aa = (const uint32_t *) a;
    const uint32_t *bb = (const uint32_t *) b;
    return *aa - *bb;
}

/* Definition of the array to sort */
uint32_t a[ASIZE];

/* Definition of the array of comparison functions */
compare_fcn comp[NFCNS];

/*
 * Main driver program.  Initialize an array with the integers from 0-ASIZE,
 * permute it, and sort it with different comparisons.  Print the initial
 * array, the permuted array, and each sort to stdout.
 */
int main(int argc, char **argv) {
    int i = 0;	    /* Scratch */

    /* Initialize array and functions */
    for ( i = 0 ; i < ASIZE; i++)
	a[i] = i;

    /* Initialize function table */
    comp[0] = compare_ab;

    dump_array(a, ASIZE);

    /* Permute */
    shuffle(a, ASIZE);
    dump_array(a, ASIZE);

    /* Sorts */
    for (i=0; i < NFCNS; i++) {
	qsort(a, ASIZE, sizeof(uint32_t), comp[i]);
	dump_array(a, ASIZE);
    }
}
