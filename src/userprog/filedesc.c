#include <stdbool.h>

#include <lib/stdio.h>
#include "userprog/filedesc.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "filesys/file.h"

/* File descriptor table info */
/* The size of the file descriptor table */
#define FDTABLESIZE 256
/* The Offset of the first file in the file descriptotr table */
#define FDBASE (STDOUT_FILENO + 1)

/*
 * Initialize the file descriptor data structure
 */
void
init_files (struct thread *cur)
{
  int i = 0;

  /* Allocate file descriptor table */
  cur->files = calloc (FDTABLESIZE, sizeof(struct file *));
  /* NULL all the pointers.  Calloc doesn't technically do that. */
  for ( i = 0 ; i < FDTABLESIZE; i++)
    cur->files[i] = NULL;
}

/*
 * Get the struct file associated with descriptor number fd.  This may be NULL
 * for a vairety of reasons - invalid fd or unopened/closed file for example.
 */
struct file *
get_file (struct thread *cur, int fd)
{
  struct file **files = (struct file **) cur->files;

  if ( fd < FDBASE )
    return NULL;

  fd -= FDBASE;

  if ( fd >= FDTABLESIZE)
    return NULL;

  if ( !files )
    return NULL;

  return files[fd];
}

/*
 * get a valid file descriptor to make an assignment to.
 */
int
get_new_file (struct thread *cur)
{
  struct file **files = (struct file **) cur->files;
  int fd = 0;

  /* Defensive driving */
  if ( !files )
    return -1;

  /* Find a file descriptor */
  while (fd < FDTABLESIZE ) {
    if ( !files[fd])
      break;
    fd++;
  }

  /* No file descriptors left */
  if ( fd == FDTABLESIZE )
    return -1;

  return fd + FDBASE;
}

/*
 * Assign to a file, returns true if the fd was valid and the assignment made.
 */
bool
set_file (struct thread *cur, int fd, struct file *f)
{
  struct file **files = (struct file **) cur->files;

  if ( fd < FDBASE )
    return false;

  fd -= FDBASE;

  if ( fd >= FDTABLESIZE)
    return false;

  if ( !files )
    return false;

  files[fd] = f;
  return true;
}

/*
 * Close all open files and remove the file descriptor infrastructure
 */

void
close_all_files (struct thread *cur)
{
  struct file **files = (struct file **) cur->files;
  int i = 0;

  /* Close any open files and release filedescriptor tables */
  for (i = 0; i < FDTABLESIZE; i++)
    if ( files[i] ) {
      file_close(files[i]);
      files[i] = NULL;
    }
  free (files);
  cur->files = NULL;
}

