#include "mem.h"
#include "stdlib.h"
#include "string.h"
#include <pthread.h>
#include <stdio.h>

static BYTE _ram[RAM_SIZE];

static struct {
	uint32_t proc;	// ID of process currently uses this page

    
	int index;	// Index of the page in the list of pages allocated
	int next;	// The next page in the list. -1 if it is the last page.
} _mem_stat[NUM_PAGES];

static pthread_mutex_t mem_lock;

void init_mem(void) {
	memset(_mem_stat, 0, sizeof(*_mem_stat) * NUM_PAGES);
	memset(_ram, 0, sizeof(BYTE) * RAM_SIZE);
	pthread_mutex_init(&mem_lock, NULL);
}

/* Get offset of the virtual address */
static addr_t get_offset(addr_t addr) {
	return addr & ~((~0U) << OFFSET_LEN);
}

/* Get the first layer index */
static addr_t get_first_lv(addr_t addr) {
	return addr >> (OFFSET_LEN + PAGE_LEN);
}

/* Get the second layer index */
static addr_t get_second_lv(addr_t addr) {
	return (addr >> OFFSET_LEN) & ((1 << PAGE_LEN) - 1);
}

/* Search for page table from the segment table */
static struct trans_table_t *get_trans_table(
	addr_t index,
	struct page_table_t *page_table) {
	for (int i = 0; i < page_table->size; i++) {
		if (page_table->table[i].v_index == index) {
			return page_table->table[i].next_lv;
		}
	}
	return NULL;
}

/* Translate virtual address to physical address */
static int translate(
	addr_t virtual_addr,
	addr_t *physical_addr,
	struct pcb_t *proc) {
	addr_t offset = get_offset(virtual_addr);
	addr_t first_lv = get_first_lv(virtual_addr);
	addr_t second_lv = get_second_lv(virtual_addr);

	struct trans_table_t *trans_table = get_trans_table(first_lv, proc->page_table);
	if (trans_table == NULL) {
		return 0;
	}

	for (int i = 0; i < trans_table->size; i++) {
		if (trans_table->table[i].v_index == second_lv) {
			*physical_addr = (trans_table->table[i].p_index << OFFSET_LEN) | offset;
			return 1;
		}
	}
	return 0;
}

addr_t alloc_mem(uint32_t size, struct pcb_t *proc) {
    pthread_mutex_lock(&mem_lock);
    addr_t ret_mem = 0;
    uint32_t num_pages = (size + PAGE_SIZE - 1) / PAGE_SIZE; // Round up to pages
    int mem_avail = 0;

    printf("Requesting %d bytes (%d pages) for PID %d.\n", size, num_pages, proc->pid);

    // Calculate available pages
    uint32_t free_pages = 0;
    for (int i = 0; i < NUM_PAGES; i++) {
        if (_mem_stat[i].proc == 0) {
            free_pages++;
        }
    }
    printf("Free pages available: %d.\n", free_pages);

    // Check if enough memory is available
    if (free_pages >= num_pages &&
        proc->bp + num_pages * PAGE_SIZE <= (1U << ADDRESS_SIZE)) {
        mem_avail = 1;
    }

    if (mem_avail) {
        printf("Memory available. Allocating...\n");
        ret_mem = proc->bp;
        proc->bp += num_pages * PAGE_SIZE;

        int prev_page = -1;
        for (int i = 0, allocated = 0; i < NUM_PAGES && allocated < num_pages; i++) {
            if (_mem_stat[i].proc == 0) {
                _mem_stat[i].proc = proc->pid;
                _mem_stat[i].index = allocated;
                _mem_stat[i].next = -1;

                if (prev_page != -1) {
                    _mem_stat[prev_page].next = i;
                }
                prev_page = i;

                addr_t first_lv = get_first_lv(ret_mem + allocated * PAGE_SIZE);
                addr_t second_lv = get_second_lv(ret_mem + allocated * PAGE_SIZE);

                struct trans_table_t *trans_table = get_trans_table(first_lv, proc->page_table);
                if (!trans_table) {
                    proc->page_table->table[proc->page_table->size].v_index = first_lv;
                    proc->page_table->table[proc->page_table->size].next_lv = malloc(sizeof(struct trans_table_t));
                    trans_table = proc->page_table->table[proc->page_table->size].next_lv;
                    trans_table->size = 0;
                    proc->page_table->size++;
                }

                trans_table->table[trans_table->size].v_index = second_lv;
                trans_table->table[trans_table->size].p_index = i;
                trans_table->size++;
                allocated++;
            }
        }
    } else {
        printf("Insufficient memory for allocation.\n");
    }

    pthread_mutex_unlock(&mem_lock);
    return ret_mem;
}



int free_mem(addr_t address, struct pcb_t *proc) {
    pthread_mutex_lock(&mem_lock);
    addr_t physical_addr;

    if (!translate(address, &physical_addr, proc)) {
        pthread_mutex_unlock(&mem_lock);
        return 1;
    }

    int page_index = physical_addr >> OFFSET_LEN;
    while (page_index != -1) {
        _mem_stat[page_index].proc = 0;
        int next_page = _mem_stat[page_index].next;
        _mem_stat[page_index].next = -1;
        page_index = next_page;
    }

    // Update page table
    for (int i = 0; i < proc->page_table->size; i++) {
        if (proc->page_table->table[i].next_lv) {
            free(proc->page_table->table[i].next_lv);
            proc->page_table->table[i].next_lv = NULL;
        }
    }
    proc->page_table->size = 0;

    pthread_mutex_unlock(&mem_lock);
    return 0;
}


int read_mem(addr_t address, struct pcb_t *proc, BYTE *data) {
	addr_t physical_addr;
	if (translate(address, &physical_addr, proc)) {
		*data = _ram[physical_addr];
		return 0;
	}
	return 1;
}

int write_mem(addr_t address, struct pcb_t *proc, BYTE data) {
	addr_t physical_addr;
	if (translate(address, &physical_addr, proc)) {
		_ram[physical_addr] = data;
		return 0;
	}
	return 1;
}

void dump(void) {
	for (int i = 0; i < NUM_PAGES; i++) {
		if (_mem_stat[i].proc != 0) {
			printf("%03d: ", i);
			printf("%05x-%05x - PID: %02d (idx %03d, nxt: %03d)\n",
				i << OFFSET_LEN,
				((i + 1) << OFFSET_LEN) - 1,
				_mem_stat[i].proc,
				_mem_stat[i].index,
				_mem_stat[i].next);
			for (int j = i << OFFSET_LEN; j < ((i + 1) << OFFSET_LEN); j++) {
				if (_ram[j] != 0) {
					printf("\t%05x: %02x\n", j, _ram[j]);
				}
			}
		}
	}
}
