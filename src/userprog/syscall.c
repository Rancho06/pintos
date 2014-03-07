#include <stdbool.h>

#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>

#include "filesys/file.h"
#include "filesys/filesys.h"

#include "devices/input.h"
#include "devices/shutdown.h"

#include "threads/interrupt.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

#include <userprog/filedesc.h>
#include <userprog/pagedir.h>
#include <userprog/process.h>

static void syscall_handler (struct intr_frame *);

#define MAX_SYSCALL_PARAMS 5
/* This will change for later projects */
#define MIN_SYSCALL SYS_HALT
#define MAX_SYSCALL SYS_CLOSE

/* Filesystem GKL (Giant Kernel Lock) */
struct lock fs_lock;

/* The value of a parameter to a system call, either an integer or a pointer */
union syscall_param_value {
  int ival;
  void *pval;
};

/* The type of a syscall parameter */
enum syscall_param_type {
  SYSCALL_INT, SYSCALL_PTR
};

/* A syscall parameter.  A typed value */
struct syscall_param {
  enum syscall_param_type type;
  union syscall_param_value value;
};

/* The signature of a system call.  The values of the params are filled in by
 * parsing. */
struct syscall_signature {
  int nparams;	  /* Number of parameters */
  struct syscall_param param[MAX_SYSCALL_PARAMS]; /* Types and values of
						     parameters */
  bool has_rv;	  /* True if the call returns a value */
};

/* The signatures of all the system calls */
struct syscall_signature sigs[MAX_SYSCALL+1] = {
  { 0 , {}, false},					/* SYS_HALT */
  { 1, { { SYSCALL_INT, { 0 } } }, false},		/* SYS_EXIT */
  { 1, { { SYSCALL_PTR, { 0 } } }, true},		/* SYS_EXEC */
  { 1, { { SYSCALL_INT, { 0 } } }, true},		/* SYS_WAIT */
  { 2, { { SYSCALL_PTR, { 0 } },
	 { SYSCALL_INT, { 0 } } }, true},		/* SYS_CREATE */
  { 1, { { SYSCALL_PTR, { 0 } } }, true},		/* SYS_REMOVE */
  { 1, { { SYSCALL_PTR, { 0 } } }, true},		/* SYS_OPEN */
  { 1, { { SYSCALL_PTR, { 0 } } }, true},		/* SYS_FILESIZE */
  { 3, { { SYSCALL_INT, { 0 } },
	 { SYSCALL_PTR, { 0 } },
	 { SYSCALL_INT, { 0 } } }, true},		/* SYS_READ */
  { 3, { { SYSCALL_INT, { 0 } },
	 { SYSCALL_PTR, { 0 } },
	 { SYSCALL_INT, { 0 } } }, true},		/* SYS_WRITE */
  { 2, { { SYSCALL_INT, { 0 } },
	 { SYSCALL_INT, { 0 } } }, false},		/* SYS_SEEK */
  { 1, { { SYSCALL_PTR, { 0 } } }, true},		/* SYS_TELL */
  { 1, { { SYSCALL_PTR, { 0 } } }, false},		/* SYS_CLOSE */
};

/* forward declarations of syscall implementations */

/* An unimplemented call */
static int
unimplemented_syscall (struct syscall_signature *sig, struct thread *cur);

/* Halt */
static int
halt_syscall (struct syscall_signature *sig, struct thread *cur);

/* Exit */
static int
exit_syscall (struct syscall_signature *sig, struct thread *cur);

/* Exec */
static int
exec_syscall (struct syscall_signature *sig, struct thread *cur);

/* Wait */
static int
wait_syscall (struct syscall_signature *sig, struct thread *cur);

/* Create */
static int
create_syscall (struct syscall_signature *sig, struct thread *cur);

/* Remove */
static int
remove_syscall (struct syscall_signature *sig, struct thread *cur);

/* Open */
static int
open_syscall (struct syscall_signature *sig, struct thread *cur);

/* Filesize */
static int
filesize_syscall (struct syscall_signature *sig, struct thread *cur);

/* Read */
static int
read_syscall (struct syscall_signature *sig, struct thread *cur);

/* Write */
static int
write_syscall (struct syscall_signature *sig, struct thread *cur);

/* Seek */
static int
seek_syscall (struct syscall_signature *sig, struct thread *cur);

/* Tell */
static int
tell_syscall (struct syscall_signature *sig, struct thread *cur);

/* Close */
static int
close_syscall (struct syscall_signature *sig, struct thread *cur);

/* Jump table of system calls, keyed by system call number */
typedef int (*syscall_impl)(struct syscall_signature *, struct thread *);
syscall_impl syscall_implementation[MAX_SYSCALL+1] = {
  halt_syscall,						/* SYS_HALT */
  exit_syscall,						/* SYS_EXIT */
  exec_syscall,						/* SYS_EXEC */
  wait_syscall,						/* SYS_WAIT */
  create_syscall,					/* SYS_CREATE */
  remove_syscall,					/* SYS_REMOVE */
  open_syscall,						/* SYS_OPEN */
  filesize_syscall,					/* SYS_FILESIZE */
  read_syscall,						/* SYS_READ */
  write_syscall,					/* SYS_WRITE */
  seek_syscall,						/* SYS_SEEK */
  tell_syscall,						/* SYS_TELL */
  close_syscall,					/* SYS_CLOSE */
};


/* Initialize the system call parts */
void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init (&fs_lock);
}

/* Return true if vaddr is a valid user address in cur's page directory */
static bool
valid_addr (struct thread *cur, void *vaddr)
{
  return ( is_user_vaddr (vaddr) &&
      pagedir_get_page (cur->pagedir, vaddr) != NULL);
}

/* Return true if vaddr points to a valid user buffer of at most lim characters
 * in cur's page directory.  If zeroed is false, all lim characters are
 * checked.  If it is true, the checking stops at the first NUL ('\0')
 * character.  If zeroed is true and the buffer is otherwise valid but contains
 * no NUL ('\0') false is returned.
 */
static bool
valid_buffer (struct thread *cur, char *vaddr, int lim, bool zeroed)
{
  int i = 0;
  bool found_zero = false;

  for (i = 0; i < lim; i++) {
    if ( !valid_addr (cur, vaddr+i))
      return false;
    if ( zeroed && vaddr[i] == '\0' ) {
      found_zero = true;
      break;
    }
  }
  return (zeroed) ? found_zero : true;
}

/*
 * Parse the signature from cur's address space starting from stack pointer
 * esp.  Each parameter must be valid.  Value entries in the signature are
 * overwritten.  Return true if the whole signature was parsed.
 */
static bool
get_args (struct syscall_signature *sig, void *esp, struct thread *cur)
{
  int nvar = sig->nparams;
  int i =0;

  for (i = 0; i < nvar; i++) {
    if (!valid_addr (cur, esp) ) return false;
    switch (sig->param[i].type) {
      case SYSCALL_PTR:
	{
	  /* Interpret esp as a pointer to a void *, get the void * into the
	   * current parameter value and advnace esp */
	  void **pesp = (void **) esp;
	  sig->param[i].value.pval = *pesp;
	  pesp++;
	  esp = (void *) pesp;
	  break;
	}
      case SYSCALL_INT:
	{
	  /* Interpret esp as a pointer to an int get the int into the
	   * current parameter value and advnace esp */
	  int *iesp = (int *) esp;
	  sig->param[i].value.ival = *iesp;
	  iesp++;
	  esp = (void *) iesp;
	  break;
	}
      default:
	/* Unknown parameter type */
	return false;
    }
  }
  return true;
}

/* Print the given parameter to the console for debugging */
static void
print_param (struct syscall_param *p)
{
  switch (p->type) {
    case SYSCALL_INT:
      printf ("int %d\n", p->value.ival);
      break;
    case SYSCALL_PTR:
      printf ("pointer %p\n", p->value.pval);
      break;
    default:
      printf ("unknown type %d\n", p->type);
      break;
  }
}

/* Unimplemented system call.  Print some diagnostics and bail */
static int
unimplemented_syscall (struct syscall_signature *sig,
    struct thread *cur UNUSED) {
  int i = 0;

  printf ("Unimplemented syscall with params\n");
  for (i = 0; i < sig->nparams; i++)
    print_param (&sig->param[i]);

  thread_exit ();
  NOT_REACHED ();
}

/* Halt syscall implementation */
static int
halt_syscall (struct syscall_signature *sig UNUSED, struct thread *cur UNUSED)
{
  shutdown_power_off ();
  NOT_REACHED ();
}

/* Exit syscall implementation */
static int
exit_syscall (struct syscall_signature *sig, struct thread *cur)
{
  cur->exit_status = sig->param[0].value.ival;
  thread_exit ();
  NOT_REACHED ();
}

/* Exec syscall implementation */
static int
exec_syscall (struct syscall_signature *sig, struct thread *cur)
{
  char *buf = (char *) sig->param[0].value.pval;

  if ( !valid_buffer (cur, buf, PGSIZE, true) )
    thread_exit ();
  return process_execute (buf);
}

/* Wait syscall implementation */
static int
wait_syscall (struct syscall_signature *sig, struct thread *cur UNUSED)
{
  return process_wait (sig->param[0].value.ival);
}

/* create syscall implementation */
static int
create_syscall (struct syscall_signature *sig, struct thread *cur)
{
  char *name = (char *) sig->param[0].value.pval;
  unsigned int size = (unsigned int) sig->param[1].value.ival;
  int rv = 0;

  /* Terminate the process on a bad pointer */
  if ( !valid_buffer (cur, name, PGSIZE, true) )
    thread_exit ();

  lock_acquire (&fs_lock);
  rv = (int) filesys_create (name, size);
  lock_release (&fs_lock);

  return rv;
}

/* remove syscall implementation */
static int
remove_syscall (struct syscall_signature *sig, struct thread *cur)
{
  char *name = (char *) sig->param[0].value.pval;
  int rv = 0;

  if ( !valid_buffer (cur, name, PGSIZE, true) )
    thread_exit ();

  lock_acquire (&fs_lock);
  rv = (int) filesys_remove (name);
  lock_release (&fs_lock);

  return rv;
}

/* open syscall implementation.  Stash the opened file in an entry in the files
 * table.  If no space, error. */
static int
open_syscall (struct syscall_signature *sig, struct thread *cur)
{
  char *name = (char *) sig->param[0].value.pval;
  struct file *opened = NULL;
  int fd = 0;

  if ( !valid_buffer (cur, name, PGSIZE, true) )
    thread_exit ();

  if ( (fd = get_new_file (cur)) == -1)
    return -1;

  lock_acquire (&fs_lock);
  opened = filesys_open (name);
  lock_release (&fs_lock);

  if ( !opened)
    return -1;

  set_file (cur, fd, opened);
  return fd;
}

/* Filesize implementation */
static int
filesize_syscall (struct syscall_signature *sig, struct thread *cur)
{
  int fd = sig->param[0].value.ival;
  struct file *f = get_file (cur, fd);
  int rv = -1;

  if ( !f )
    return -1;

  lock_acquire (&fs_lock);
  rv = file_length (f);
  lock_release (&fs_lock);

  return rv;
}

/* Read syscall implementation */
static int
read_syscall (struct syscall_signature *sig, struct thread *cur)
{
  int fd = sig->param[0].value.ival;
  char *buf = (char *) sig->param[1].value.pval;
  unsigned int lim = (unsigned int) sig->param[2].value.ival;
  struct file *f = NULL;
  int rv = -1;

  if ( !valid_buffer (cur, buf, lim, false) )
    thread_exit ();

  if ( fd == STDIN_FILENO ) {
    if ( lim == 0 ) return 0;

    buf[0] = input_getc ();
    return 1;
  }

  if ( !(f = get_file (cur, fd)))
      return -1;

  lock_acquire (&fs_lock);
  rv = file_read (f, buf, lim);
  lock_release (&fs_lock);

  return rv;
}

/* Write syscall implementation */
static int
write_syscall (struct syscall_signature *sig, struct thread *cur)
{
  int fd = sig->param[0].value.ival;
  char *buf = (char *) sig->param[1].value.pval;
  unsigned int lim = (unsigned int) sig->param[2].value.ival;
  struct file *f = NULL;
  int rv = -1;

  if ( !valid_buffer (cur, buf, lim, false) )
    thread_exit ();

  if ( fd == STDOUT_FILENO ) {
    putbuf (buf, lim);
    return lim;
  }

  if ( !(f = get_file (cur, fd)))
      return -1;

  lock_acquire (&fs_lock);
  rv = file_write (f, buf, lim);
  lock_release (&fs_lock);

  return rv;
}

/* Seek implementation */
static int
seek_syscall (struct syscall_signature *sig, struct thread *cur)
{
  int fd = sig->param[0].value.ival;
  unsigned int pos = (unsigned int) sig->param[1].value.ival;
  struct file *f = get_file (cur, fd);

  if ( !f )
    return 0;

  lock_acquire (&fs_lock);
  file_seek (f, pos);
  lock_release (&fs_lock);

  return 0;
}

/* Tell implementation */
static int
tell_syscall (struct syscall_signature *sig, struct thread *cur)
{
  int fd = sig->param[0].value.ival;
  struct file *f = get_file (cur, fd);
  int rv = -1;

  if ( !f )
    return -1;

  lock_acquire (&fs_lock);
  rv = file_tell (f);
  lock_release (&fs_lock);

  return rv;
}


/* open syscall implementation.  make sure to clear the file
 * descriptor entry.  */
static int
close_syscall (struct syscall_signature *sig, struct thread *cur)
{
  int fd = sig->param[0].value.ival;
  struct file *f = get_file (cur, fd);

  if ( !fd )
    return 0;

  lock_acquire (&fs_lock);
  file_close (f);
  lock_release (&fs_lock);
  set_file (cur, fd, NULL);

  return 0;
}

/* Look up the system call, parse the arguments from the user stack, and call
 * the implementation. */
static void
syscall_handler (struct intr_frame *f) 
{
  int *esp = (int *) f->esp;
  struct thread *cur = thread_current ();
  struct syscall_signature sig;
  int rv = 0;

  ASSERT (cur && cur->pagedir);

  if (!valid_addr (cur, esp))
    thread_exit ();

  int *iesp = (int *) esp;
  int sysnum = *iesp++;

  if ( sysnum < MIN_SYSCALL || sysnum > MAX_SYSCALL )
    thread_exit ();

  sig = sigs[sysnum];

  if (!get_args (&sig, iesp, cur))
    thread_exit ();
  rv = syscall_implementation[sysnum] (&sig, cur);

  if ( sig.has_rv ) f->eax = rv;
  return;

}
