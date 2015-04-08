#ifndef VM_PAGE_H
#define VM_PAGE_H
#include "filesys/file.h"
#include "threads/thread.h"
#include <list.h>

enum src_type {
	FILE,
	SWAP,
	STACK
};

struct page {
	void* page_addr;
	enum src_type src;
	struct file* executable;
	off_t offset;
	uint32_t read_bytes;
	bool writable;
	int swap_num;
	bool locked;
	struct list_elem elem;
};

struct page* vm_get_page(void* page_addr, struct list* page_list);
bool vm_create_page(struct file* file, off_t ofs, void* upage, uint32_t read_bytes, bool writable);
bool vm_set_stack(void* page_addr);
void vm_release_page(struct list* page_list);

#endif