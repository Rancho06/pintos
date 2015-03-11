#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);
struct lock lock;	/* lock used to control access to system calls */
#endif /* userprog/syscall.h */
