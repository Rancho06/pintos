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

/*
 * Copy a chunk of data of size length from source into the address ptr_dest points to, and increments the data ptr_dest points to by size.
 */
void copy_data(void* source, uint8_t** ptr_dest, int size) {
    bcopy(source, *ptr_dest, size);   
    *ptr_dest += size;
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

    /* Calculate the actual size of struct example it needs to be allocated by adding up the size of all the member variables in the struct. */
    int size_of_buf = 0;
    size_of_buf += sizeof(ex.byte);
    size_of_buf += sizeof(ex.int32);
    size_of_buf += sizeof(ex.int16);
    size_of_buf += sizeof(ex.some[i]) * 15;

    /* Make sure malloc is successful, otherwise print error message */
    if ( (buf = malloc(size_of_buf)) == NULL) fatal("malloc");

    /* Call copy_data on each member variable in the struct */
    copy_data(&(ex.byte), &buf, sizeof(ex.byte));
    copy_data(&(ex.int32), &buf, sizeof(ex.int32));
    copy_data(&(ex.int16), &buf, sizeof(ex.int16));
    for (i = 0; i < 15; i++) {
        copy_data(&(ex.some[i]), &buf, sizeof(ex.some[i]));
    }

    //bcopy(&ex, buf, sizeof(ex));
    
    /* Write to the file */
    buf -= size_of_buf;
    if ( write_buf(f, buf, size_of_buf)) fatal("write");
    close(f);

    /* C does not garbage collect buf.  All malloc-ed (or otherwise allocated)
     * memory must be freed. */
    free(buf);
}
