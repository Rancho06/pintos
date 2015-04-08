#include "vm/page.h"

struct page* vm_get_page(void* page_addr, struct list* page_list) {
	struct list_elem* temp;
	for (temp = list_begin(page_list); temp != list_end(page_list); temp = list_next(temp)) {
		struct page* page = list_entry(temp, struct page, elem);
		if (page->page_addr == page_addr) {
			return page;
		}
	}
	return NULL;
}

bool vm_create_page(struct file* executable, off_t offset, void* page_addr, uint32_t read_bytes, bool writable) {
	struct page* page = malloc(sizeof(struct page));
	if (page == NULL) {
		return false;
	}
	page->page_addr = page_addr;
	page->src = FILE;
	page->map_id = -1;
	page->executable = executable;
	page->offset = offset;
	page->read_bytes = read_bytes;
	page->writable = writable;
	page->locked = false;
	list_push_back(&thread_current()->page_list, &page->elem);
	return true;
}

bool vm_set_stack(void* page_addr) {
	struct page* page = malloc(sizeof(struct page));
	if (page == NULL) {
		return false;
	}
	page->page_addr = page_addr;
	page->src = STACK;
	page->map_id = -1;
	page->writable = true;
	page->locked = false;
	list_push_back(&thread_current()->page_list, &page->elem);
	return true;
}

void vm_release_page(struct list* page_list) {
	struct list_elem* elem = list_begin(page_list);
	while (elem != list_end(page_list)) {
		struct page* page = list_entry(elem, struct page, elem);
		elem = list_next(elem);
		free(page);
	}
}


void vm_map_page(void* page_addr, struct file* file, int map_id, int ofs, struct map* map) {
	struct page* page = malloc(sizeof(struct page));
	if (page == NULL) {
		return false;
	}
	page->page_addr = page_addr;
	page->src = MAP;
	page->map_id = map->id;
	page->executable = file;
	page->offset = ofs;
	page->writable = true;
	page->locked = false;
	list_push_back(&thread_current()->page_list, &page->elem);
	list_push_back(&map->pages, &page->map_elem);
	return true;
}