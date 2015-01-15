#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <strings.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/* Definition of the layout of an example structure to show padding and
 * alignment effects */
struct example {
    uint8_t byte;	/* 1-byte (8-bit) field */
    uint32_t int32;	/* 4-byte (32-bit) field */
    uint16_t int16;	/* 2-byte (16-bit) field */
    uint32_t some[15];	/* 15 x 4-byte (32-bit) fields */
};

/* Print an error message and exit with an error code */
void fatal(char *msg) {
    perror(msg);
    exit(20);
}

/* Print a usage message and exit with an error code */
void usage(const char *prog) {
    fprintf(stderr, "Usage: %s output_file\n", prog);
    exit(10);
}

/* Write size bytes from buf into file descriptor f (see open(1)).  Deal with
 * the possibility of write (2) not outputting all bytes requested. */
int write_buf(int f, void *buf, int size) {
    int tot = 0;

    while ( tot < size) {
	uint8_t *b = (uint8_t*) buf + tot;

	int w = write(f, b, size-tot);
	if (w < 0) return -1;
	tot += w;
    }
    return 0;
}

int main(int argc, char **argv) {
    int f = -1;		    /* File descriptor for output */
    struct example ex;	    /* An example structure */
    uint8_t *buf = NULL;    /* Formatted output buffer */
    int i = 0;		    /* Scratch */

    /* Initialize the example structure */
    ex.byte = 1;
    ex.int32 = 2;
    ex.int16 = 3;
    for (i=0; i< 15; i++)
	ex.some[i] = i+1;

    /* Get the first command line argument and open a file by that name. */
    if ( argc < 2 ) usage(argv[0]);

    if ( (f = open(argv[1], O_CREAT | O_WRONLY | O_TRUNC, 0644)) == -1 )
	fatal("open");

    /* Allocate the output buffer and use bcopy to directly copy the stucture.
     * You will need to modify this code. */
    if ( (buf = malloc(sizeof(ex))) == NULL) fatal("malloc");
    bcopy(&ex, buf, sizeof(ex));

    if ( write_buf(f, buf, sizeof(ex))) fatal("write");
    close(f);

    /* C does not garbage collect buf.  All malloc-ed (or otherwise allocated)
     * memory must be freed. */
    free(buf);
}
