#ifndef VM_SWAP_H
#define VM_SWAP_H

void swap_init();
void load_data_from_swap(int, void *);
int write_data_to_swap(void *);

#endif