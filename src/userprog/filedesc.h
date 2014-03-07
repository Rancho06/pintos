#ifndef FILEDESC_H
#define FILEDESC_H

/* File descriptor manipulation */
struct thread;

void init_files (struct thread *);
struct file *get_file (struct thread *, int);
int get_new_file (struct thread *);
bool set_file (struct thread *, int, struct file *);
void close_all_files (struct thread *);

#endif /* userprog/filedesc.h */
