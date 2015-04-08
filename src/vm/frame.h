#ifndef VM_FRAME_H
#define VM_FRAME_H
#include "threads/thread.h"
#include "threads/palloc.h"
#include <list.h>

struct frame {
	void* frame_addr;
	void* page_addr;
	struct thread* thread;
	bool loaded;
	struct list_elem elem;
};

struct list frame_table;
struct lock file_lock;

void frame_table_init();
void* vm_alloc_frame(void* page_addr);
void vm_release_frame(struct thread* thread);
struct frame* get_frame_by_page(void*);
struct frame* get_frame_by_addr(void*);

#endif