#include "vm/frame.h"
#include "threads/thread.h"
#include "vm/page.h"
#include "userprog/pagedir.h"
#include "vm/swap.h"
#include <list.h>

static struct lock table_lock;
static struct frame* next_frame_to_evict();
static void* vm_evict_frame(void* page_addr);

void frame_table_init() {
	list_init(&frame_table);
	list_init(&maps);
	lock_init(&table_lock);
	lock_init(&file_lock);
}

void* vm_alloc_frame(void* page_addr) {
	void* frame_addr = palloc_get_page(PAL_USER);
	if (!frame_addr) {
		frame_addr = vm_evict_frame(page_addr);
		return frame_addr;
	}
	struct frame* frame;
	frame = malloc(sizeof(struct frame));
	frame->frame_addr = frame_addr;
	frame->page_addr = page_addr;
	frame->thread = thread_current();
	frame->loaded = false;
	lock_acquire(&table_lock);
	list_push_back(&frame_table, &frame->elem);
	lock_release(&table_lock);
	return frame_addr;
}


void vm_release_frame(struct thread* thread) {
	lock_acquire(&table_lock);
	struct list_elem* elem;
	for (elem = list_begin(&frame_table); elem != list_end(&frame_table); elem = list_next(elem)) {
		struct frame* frame = list_entry(elem, struct frame, elem);
		if (frame->thread == thread) {
			frame->thread = NULL;
			frame->page_addr = NULL;
		}
	}		
	vm_release_page(&thread->page_list);
	lock_release(&table_lock);
}

void* vm_evict_frame(void* page_addr) {
	lock_acquire(&table_lock);
	struct frame* frame = next_frame_to_evict();
	if (frame->thread != NULL) {
		pagedir_clear_page(frame->thread->pagedir, frame->page_addr);
		struct page* page = vm_get_page(frame->page_addr, &frame->thread->page_list);
		if (page->src == SWAP || (page->src != MAP && pagedir_is_dirty(frame->thread->pagedir, frame->page_addr))) {
			page->src = SWAP;
			page->swap_num = write_data_to_swap(frame->frame_addr);
		}
		if (page->src == MAP && pagedir_is_dirty(frame->thread->pagedir, frame->page_addr)) {
			struct frame* frame = get_frame_by_page(page->page_addr);
			lock_acquire(&file_lock);
			file_seek(page->executable, page->offset);
			file_write(page->executable, frame->frame_addr, 4096);
			lock_release(&file_lock);
		}
	}
	frame->page_addr = page_addr;
	frame->thread = thread_current();
	frame->loaded = false;
	list_push_back(&frame_table, &frame->elem);
	lock_release(&table_lock);
	return frame->frame_addr;
}


struct frame* get_frame_by_page(void *page_addr) {
	struct list_elem * temp;
	struct frame* frame = NULL;
	lock_acquire(&table_lock);
	for (temp = list_begin(&frame_table); temp != list_end(&frame_table); temp = list_next(temp)) {
		struct frame* f = list_entry(temp, struct frame, elem);
		if (f->page_addr == page_addr) {
			frame = f;
			break;
		}
	}
	lock_release(&table_lock);	
	return frame;
}

struct frame* get_frame_by_addr(void *frame_addr) {
	
	struct list_elem* temp;
	struct frame* frame = NULL;
	lock_acquire(&table_lock);
	for (temp = list_begin(&frame_table); temp != list_end(&frame_table); temp = list_next(temp)) {
		struct frame* f = list_entry(temp, struct frame, elem);
		if (f->frame_addr == frame_addr) {
			frame = f;
			break;
		}
	}
	lock_release(&table_lock);	
	return frame;
}


static struct frame* next_frame_to_evict() {
	struct frame* frame;
	frame = NULL;
	struct list_elem* elem;
	for (elem = list_begin(&frame_table); elem != list_end(&frame_table); elem = list_next(elem)) {
		struct frame* temp = list_entry(elem, struct frame, elem);
		if (!temp->thread || (temp->loaded && !pagedir_is_accessed(temp->thread->pagedir, temp->page_addr))) {
			frame = temp;
			break;
		}		
	}
	if (!frame) {
		for (elem = list_begin(&frame_table); elem != list_end(&frame_table); elem = list_next(elem)) {
			struct frame* temp = list_entry(elem, struct frame, elem);
			if (!temp->thread || temp->loaded) {
				frame = temp;
				break;
			}
		}
	}
 	list_remove(&frame->elem);
 	return frame;
}
/*
void vm_set_accessed() {
	if (!lock_try_acquire(&table_lock))
		return;
	if (list_size(&frame_table) == 0) {
		lock_release(&table_lock);
		return;
	}
	struct list_elem *temp;
	for (temp = list_begin(&frame_table); temp != list_end(&frame_table); temp = list_next(temp)) {
		struct frame* frame = list_entry(temp, struct frame, elem);
		struct thread * thread = frame->thread;
		if (thread != NULL)
			pagedir_set_accessed(thread->pagedir, frame->page_addr, false);
	}
	lock_release(&table_lock);
}*/
