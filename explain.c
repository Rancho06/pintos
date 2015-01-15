#include <stdio.h>
#include <stdint.h>

#define ASIZE 64
uint8_t a[ASIZE];

/*
 * Your mission: explain this output.
 */
int main(int argc, char **argv) {
    uint32_t *ip = NULL;
    uint16_t *sp = NULL;
    uint8_t *bp = NULL;
    int i = 0;

    for (i=0; i< ASIZE; i++)
	a[i] = i;

    for (i = 0, ip = (uint32_t *) a; i < sizeof(a)/sizeof(*ip); i++) {
	if ( i % 2 == 0) printf("\n");
	printf("%010d(%08x) ", ip[i], ip[i]);
    }
    printf("\nnext");

    for (i = 0, sp = (uint16_t *) a; i < sizeof(a)/sizeof(*sp); i++) {
	if ( i % 4 == 0) printf("\n");
	printf("%05d(%04x) ", sp[i], sp[i]);
    }
    printf("\nnext");

    for (i = 0, bp = (uint8_t *) a; i < sizeof(a)/sizeof(*bp); i++) {
	if ( i % 8 == 0) printf("\n");
	printf("%03d(%02x) ", bp[i], bp[i]);
    }
    printf("\n");
}
