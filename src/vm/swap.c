#include "vm/swap.h"
#include "devices/block.h"
#include "threads/vaddr.h"
#include "threads/synch.h"


static int find_empty_page();

static struct block* block;
const static int sectors_in_page = PGSIZE / BLOCK_SECTOR_SIZE;
static int pages_in_block;
static bool* usage;
static struct lock swap_lock;

void swap_init() {
	block = block_get_role(BLOCK_SWAP);
	if (!block) {
		PANIC("Block not found");
	}
	pages_in_block = block_size(block) / sectors_in_page;
	usage = malloc(sizeof(bool) * pages_in_block);
	memset(usage, 0, sizeof(bool) * pages_in_block);
	//lock_init(&swap_lock);
}

void load_data_from_swap(int page_num, void* frame_addr) {
	int i;
	//lock_acquire(&swap_lock);
	for (i = 0; i < sectors_in_page; ++i) {
		block_read(block, page_num * sectors_in_page + i, frame_addr + BLOCK_SECTOR_SIZE * i);
	}
	usage[page_num] = false;
	//lock_release(&swap_lock);
}

int write_data_to_swap(void* frame_addr) {
	//lock_acquire(&swap_lock);
	int page_num = find_empty_page();
	if (page_num == -1) {
		PANIC("NOT ENOUGH SPACE TO SWAP");
	}
	int i;
	for (i = 0; i < sectors_in_page; ++i) {
		block_write(block, page_num * sectors_in_page + i, frame_addr + BLOCK_SECTOR_SIZE * i);
	}
	//lock_release(&swap_lock);
	return page_num;
}

static int find_empty_page() {
	int i;
	for (i = 0; i < pages_in_block; ++i) {
		if (!usage[i]) {
			usage[i] = true;
			return i;
		}
	}
	return -1;
}