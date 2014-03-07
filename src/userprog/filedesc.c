#include <stdbool.h>

#include <lib/stdio.h>
#include "userprog/filedesc.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "filesys/file.h"

/* File descriptor table info */
/* The size of the file descriptor table */
#define FDCOUNT 31
/* The Offset of the first file in the file descriptotr table */
#define FDBASE (STDOUT_FILENO + 1)

struct fdtable {
  struct file *fd[FDCOUNT];
  struct fdtable *next;
};

static struct fdtable *
get_table (void)
{
  struct fdtable *rv = malloc (sizeof(struct fdtable));
  int i = 0;

  if (!rv) return NULL;

  /* Allocate file descriptor table */
  /* NULL all the pointers.  */
  for ( i = 0 ; i < FDCOUNT; i++)
    rv->fd[i] = NULL;
  rv->next = NULL;
  return rv;
}

/*
 * Initialize the file descriptor data structure
 */
void
init_files (struct thread *cur)
{
  /* Allocate the first file descriptor table */
  cur->files = get_table ();
}

/*
 * Get the struct file associated with descriptor number fd.  This may be NULL
 * for a vairety of reasons - invalid fd or unopened/closed file for example.
 */
struct file *
get_file (struct thread *cur, int fd)
{
  struct fdtable *fdtable = (struct fdtable *) cur->files;
  int table_no = 0;
  int table_off = 0;
  int tn = 0;

  if (!fdtable )
    return NULL;

  if ( fd < FDBASE )
    return NULL;

  table_no = (fd - FDBASE) / FDCOUNT;
  table_off = (fd - FDBASE) % FDCOUNT;


  /* Find the table */
  for ( tn = 0; tn < table_no; tn++ ) {
    fdtable = fdtable->next;
    if ( !fdtable)
      return NULL;
  }

  return fdtable->fd[table_off];
}

/*
 * get a valid file descriptor to make an assignment to.
 */
int
get_new_file (struct thread *cur)
{
  struct fdtable *fdtable = (struct fdtable *) cur->files;
  bool found = false;
  int fd = 0;

  /* Defensive driving */
  if ( !fdtable )
    return -1;

  while (fdtable && !found) {
    /* Current table number - leave the loop when we cross to the next tn */
    int tn = fd / FDCOUNT;

    /* Find a file descriptor in this table */
    while (fd / FDCOUNT == tn) {
      if ( !fdtable->fd[fd % FDCOUNT]) {
	found = true;
	break;
      }
      fd++;
    }

    /* Found a free fd, return it */
    if ( found )
      return fd + FDBASE;

    /* No empty fd in this table, on to the next */
    if (fdtable->next) {
      fdtable = fdtable->next;
    }
    else {
      /* No next table, try to allocate and attach an new one */
      struct fdtable *nt = get_table ();
      if (nt) {
	fdtable->next = nt;
	fdtable = nt;
      }
      else {
	/* No file descriptor tables left (!) */
	return -1;
      }
    }
  }

  return -1;
}

/*
 * Assign to a file, returns true if the fd was valid and the assignment made.
 */
bool
set_file (struct thread *cur, int fd, struct file *f)
{
  int table_no = 0;
  int table_off = 0;
  struct fdtable *fdtable = (struct fdtable *) cur->files;

  if (!fdtable )
    return NULL;

  if ( fd < FDBASE )
    return NULL;

  table_no = (fd - FDBASE) / FDCOUNT;
  table_off = (fd - FDBASE) % FDCOUNT;

  int tn = 0;

  /* Find the table */
  while ( tn < table_no ) {
    fdtable = fdtable->next;
    if ( !fdtable)
      return false;
    tn++;
  }

  fdtable->fd[table_off] = f;
  return true;
}

/*
 * Close all open files and remove the file descriptor infrastructure
 */
void
close_all_files (struct thread *cur)
{
  struct fdtable *fdtable = (struct fdtable *) cur->files;

  while (fdtable) {
    int i = 0;
    /* Close any open files and release filedescriptor tables */
    for (i = 0; i < FDCOUNT; i++)
      if ( fdtable->fd[i] ) {
	file_close(fdtable->fd[i]);
	fdtable->fd[i] = NULL;
      }

    struct fdtable *next = fdtable->next;
    free (fdtable);
    fdtable = next;
  }
  cur->files = NULL;
}

