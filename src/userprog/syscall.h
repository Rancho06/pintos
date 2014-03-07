#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);

/* File descriptor manipulation */
struct thread;
void init_files (struct thread *);
struct file *get_file (struct thread *, int);
int get_new_file (struct thread *);
bool set_file (struct thread *, int, struct file *);
void close_all_files (struct thread *);

#endif /* userprog/syscall.h */
