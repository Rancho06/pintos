#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

#include <userprog/pagedir.h>

static void syscall_handler (struct intr_frame *);

#define MAX_SYSCALL_PARAMS 5
/* This will change for later projects */
#define MIN_SYSCALL SYS_HALT
#define MAX_SYSCALL SYS_CLOSE

union syscall_param_value {
  int ival;
  void *pval;
};

enum syscall_param_type {
  SYSCALL_INT, SYSCALL_PTR
};

struct syscall_param {
  enum syscall_param_type type;
  union syscall_param_value value;
};

struct syscall_signature {
  int nparams;
  struct syscall_param param[MAX_SYSCALL_PARAMS];
  bool has_rv;
};

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

static int
unimplemented_syscall (struct syscall_signature *sig, struct thread *cur);

static int
write_syscall (struct syscall_signature *sig, struct thread *cur);

typedef int (*syscall_impl)(struct syscall_signature *, struct thread *);
syscall_impl syscall_implementation[MAX_SYSCALL+1] = {
  unimplemented_syscall,				/* SYS_HALT */
  unimplemented_syscall,				/* SYS_EXIT */
  unimplemented_syscall,				/* SYS_EXEC */
  unimplemented_syscall,				/* SYS_WAIT */
  unimplemented_syscall,				/* SYS_CREATE */
  unimplemented_syscall,				/* SYS_REMOVE */
  unimplemented_syscall,				/* SYS_OPEN */
  unimplemented_syscall,				/* SYS_FILESIZE */
  unimplemented_syscall,				/* SYS_READ */
  write_syscall,					/* SYS_WRITE */
  unimplemented_syscall,				/* SYS_SEEK */
  unimplemented_syscall,				/* SYS_TELL */
  unimplemented_syscall,				/* SYS_CLOSE */
};


void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static bool
valid_addr (struct thread *cur, void *vaddr)
{
  return ( is_user_vaddr (vaddr) &&
      pagedir_get_page (cur->pagedir, vaddr) != NULL);
}

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
	  void **pesp = (void **) esp;
	  sig->param[i].value.pval = *pesp;
	  pesp++;
	  esp = (void *) pesp;
	  break;
	}
      case SYSCALL_INT:
	{
	  int *iesp = (int *) esp;
	  sig->param[i].value.ival = *iesp;
	  iesp++;
	  esp = (void *) iesp;
	  break;
	}
      default:
	return false;
    }
  }
  return true;
}

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

static int
unimplemented_syscall (struct syscall_signature *sig,
    struct thread *cur UNUSED) {
  int i = 0;

  printf ("Unimplemented syscall with params\n");
  for (i = 0; i < sig->nparams; i++)
    print_param (&sig->param[i]);

  thread_exit ();
  return 0;
}

static int
write_syscall (struct syscall_signature *sig, struct thread *cur UNUSED) {
  if ( sig->param[0].ival == STDOUT_FILENO ) {
    char *buf = (char *) sig->param[1].pval;
    int lim = sig->param[2].ival;
    int i = 0;

    for ( i = 0 ; i < lim; i++) 
      if (!valid_addr (buf + i ) ) return -1;

    /* Good buffer, put it out */
    putbuf (buf, lim);
    return lim;
  }
  else {
    printf ("Not yet\n");
    return -1;
  }
}

static void
syscall_handler (struct intr_frame *f) 
{
  int *esp = (int *) f->esp;
  struct thread *cur = thread_current ();
  struct syscall_signature sig;
  int rv = 0;

  ASSERT (cur && cur->pagedir);

  if ( !valid_addr (cur, esp) ) {
    printf ("Bad stack!?! %p\n", esp);
    thread_exit ();
  }
  int *iesp = (int *) esp;
  int sysnum = *iesp++;

  if ( sysnum < MIN_SYSCALL || sysnum > MAX_SYSCALL ) {
    printf ("Unimplemented syscall %d\n", sysnum);
    thread_exit ();
  }

  printf ("Syscall %d\n", sysnum);

  sig = sigs[sysnum];

  get_args (&sig, iesp, cur);
  rv = syscall_implementation[sysnum] (&sig, cur);

  thread_exit ();
}
