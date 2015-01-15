#include <stdio.h>

/* The size of the fixed array */
#define ASIZE 20

/* The array to average */
int a[ASIZE];

/* Average the a array of the given size.  Store the average in the double
 * pointed to by out_param. */
void average(int *a, int size, double *out_param) {
    int tot = 0;
    int i = 0;

    for (i = 0; i < size; i++)
	tot += a[i];
    /* Cast tot into a double so the result of the division is a double. */
    *out_param = (double) tot/size;
}

/* Initialize a with the integers from 0 to ASIZE-1 and call average on it.
 * Print the result. */
int main(int argc, char **argv) {
    int i = 0;
    double avg = 0.0;

    for (i=0; i< ASIZE; i++)
	a[i] = i;

    average(a, ASIZE, &avg);
    printf("Average: %g\n", avg);
}
