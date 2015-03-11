#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "filesys/inode.h"
#include "threads/vaddr.h"

static void syscall_handler (struct intr_frame *);
static struct file* get_file(int fd);
static void validate(char* buffer, struct intr_frame *f);
static int count = 2;


void
syscall_init (void) 
{
	lock_init(&lock);

  	intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  //printf ("system call!\n");
  //thread_exit ();

	int handler = *(int*)(f->esp);

	switch(handler) {
		case SYS_HALT: {
			shutdown_power_off();
		}
		break;

		case SYS_EXIT: {
			int status = *(int*)(f->esp + 4);
			validate(f->esp+4, f);
			thread_current()->exit_code = status;
			printf("%s: exit(%d)\n", thread_current()->name, status);
			thread_exit();
		}
		break;

		case SYS_EXEC: {
			char* cmd_line = *((char **)(f->esp + 4));
			validate(cmd_line, f);
			f->eax = process_execute(cmd_line);
		}
		break;

		case SYS_WAIT: {
			int pid = *(int*)(f->esp + 4);
			validate(f->esp+4, f);
			f->eax = process_wait(pid);
		}
		break;

		case SYS_CREATE: {
			char* file = *(char **)(f->esp + 4);
			validate(file, f);
			unsigned int initial_size = *(unsigned int*)(f->esp + 8);
			validate(f->esp+8, f);
			if (!file || initial_size < 0) {
				f->eax = -1;
				printf("%s: exit(%d)\n", thread_current()->name, -1);
		      	thread_current()->exit_code = -1;
		      	thread_exit();
			}
			else {
				lock_acquire(&lock);
				f->eax = filesys_create(file, initial_size);
				lock_release(&lock);
			}
			
		}
		break;

		case SYS_REMOVE: {
			char* file = *(char **)(f->esp + 4);
			validate(file, f);
			lock_acquire(&lock);
			f->eax = filesys_remove(file);
			lock_release(&lock);
		}
		break;

		case SYS_OPEN: {
			char* name = *(char **)(f->esp + 4);
			validate(name, f);
			if (!name) {
				f->eax = -1;
			}
			else {
				lock_acquire(&lock);
				struct file* file = filesys_open(name);
				lock_release(&lock);
				if (file) {
					file->fd = count++;
					f->eax = file->fd;
					list_push_back(&thread_current()->files, &file->elem);
				}
				else {
					f->eax = -1;
				}
				
			}
			
		}
		break;

		case SYS_FILESIZE: {
			int fd = *(int *)(f->esp + 4);
			validate(f->esp+4, f);
	        f->eax = -1;
	        if (fd >= 2) {
	          struct file* file = get_file(fd);
	          if (file) {
	            lock_acquire(&lock);
	            f->eax = file_length(file);
	            lock_release(&lock);
	          }
	        }
		}
		break;

		case SYS_READ: {
			int fd = *(int*)(f->esp + 4);
			char* buffer = *(char**)(f->esp + 8);
			unsigned int size = *(unsigned int*)(f->esp + 12);
			validate(buffer, f);
			validate(f->esp+4, f);
			validate(f->esp+12, f);
			if (fd == 1 || size < 0 || !buffer) {
				f->eax = -1;
			} 
			else if (fd == 0) {
				f->eax = input_getc();
			}
			else {
				struct file* file = get_file(fd);
				if (file) {
					lock_acquire(&lock);
					f->eax = file_read(file, buffer, size);
					lock_release(&lock);
				}
				else {
					f->eax = -1;
				}
			}
		}
		break;

		case SYS_WRITE: {
			int fd = *(int*)(f->esp + 4);
			char* buffer = *(char**)(f->esp + 8);
			unsigned int size = *(unsigned int*)(f->esp + 12);
			validate(buffer, f);
			validate(f->esp+4, f);
			validate(f->esp+12, f);
			if (fd == 0 || size < 0 || !buffer) {
				f->eax = -1;
			}
			else if (fd == 1) {
				putbuf(buffer, size);
				f->eax = size;
			}
			else {
				struct file* file = get_file(fd);
				if (file) {
					lock_acquire(&lock);
					f->eax = file_write(file, buffer, size);
					lock_release(&lock);
				}
				else {
					f->eax = -1;
				}
			}
		}
		break;

		case SYS_SEEK: {
			int fd = *(int*)(f->esp + 4);
			unsigned int pos = *(unsigned int*)(f->esp + 8);
			validate(f->esp+4, f);
			validate(f->esp+8, f);
			struct file* file = get_file(fd);
			if (fd == 1 || fd == 2 || !file) {
				f->eax = -1;
			}
			else {
				lock_acquire(&lock);
	          	file_seek(file, pos);
	          	lock_release(&lock);
			}
		}
		break;

		case SYS_TELL: {
			int fd = *(int*)(f->esp + 4);
			validate(f->esp+4, f);
			struct file* file = get_file(fd);
			if (fd == 1 || fd == 2 || !file) {
				f->eax = -1;
			}
			else {
				lock_acquire(&lock);
	          	f->eax = file_tell(file);
	          	lock_release(&lock);
			}
		}
		break;

		case SYS_CLOSE: {
			int fd = *(int*)(f->esp + 4);
			validate(f->esp+4, f);
			struct file* file = get_file(fd);
			if (file) {
				lock_acquire(&lock);
		        list_remove(&file->elem);
		        file_close(file);
		        //file_allow_write(file);
		        lock_release(&lock);
			}
		}
		break;



	}
}


static struct file* get_file(int fd) {
  	struct list_elem *temp;
  	struct list* files = &(thread_current()->files);
  	for (temp = list_begin(files); temp != list_end(files); temp = list_next(temp)) {
    	struct file* file = list_entry(temp, struct file, elem);
    	if (file->fd == fd) {
    		return file;
    	}     	
  	}
  	return NULL;
}


static void validate(char* buffer, struct intr_frame *f) {
	if (!is_user_vaddr(buffer)) {
		f->eax = -1;
      	printf("%s: exit(%d)\n", thread_current()->name, -1);
      	thread_current()->exit_code = -1;
      	thread_exit();
	}
}