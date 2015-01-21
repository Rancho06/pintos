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

/* assign the value stored in ptr_address to the variable pointed to by out_ptr */
void assign(int ptr_address, int* out_ptr) {
    *out_ptr = ptr_address;
}



/* Old routine: Initialize a with the integers from 0 to ASIZE-1 and call average on it.
 * New routine: Declare two pointers pointing to different addresses, and call assign() to let them point to the same address.
 * Print the result. */
int main(int argc, char **argv) {
    /* Old routine */
    int i = 0;
    double avg = 0.0;

    for (i=0; i< ASIZE; i++)
	a[i] = i;

    average(a, ASIZE, &avg);
    printf("Average: %g\n", avg);

    /* New routine */
    int num = 5;
    int* num_ptr = &num;
    int* new_ptr = NULL;
    printf("Before copy pointers differ (%x, %x)\n", new_ptr, num_ptr);

    assign(num_ptr, &new_ptr);    
    printf("After copy pointers are the same (%x, %x)\n", new_ptr, num_ptr);

}
