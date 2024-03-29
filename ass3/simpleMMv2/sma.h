#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

//  Policies definition
#define WORST_FIT	1
#define NEXT_FIT	2

extern char *sma_malloc_error;

//  Public Functions declaration
void *sma_malloc(int size);
void sma_free(void* ptr);
void sma_mallopt(int policy);
void sma_mallinfo();
void *sma_realloc(void *ptr, int size);

//  Private Functions declaration
void *allocate_from_sbrk(int size);
void *allocate_from_freeList(int size);
void *allocate_worst_fit(int size);
void *allocate_next_fit(int size);
void *allocate_block_from_freeList(void *ptr, int size);  // allocate block from freeList
void replace_block_freeList(void *ptr);  // free an allocated block
void append_block_freeList(void* block);

void *get_largest_free_block();
void *get_next_fit_block();

int get_block_size(void *ptr);
void *get_free_block_prev(void *ptr);
void *get_free_block_next(void *ptr);

void set_block_header_footer(void *block, int size, int tag);
void set_free_block_next(void *block, void *next);
void set_free_block_prev(void *block, void *prev);
void merge_two_free_blocks(void *formerPtr, void *latterPtr);

void debug();
