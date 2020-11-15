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
static void* allocate_pBrk(int size);
static void* allocate_freeList(int size);
static void* allocate_worst_fit(int size);
static void* allocate_next_fit(int size);
static void* allocate_block(int size, int excessSize, int fromFreeList);
static void replace_block_freeList(void *oldBlock, void *newBlock);
static void add_block_freeList(void *block);
static void remove_block_freeList(void *block);
static int get_largest_freeBlock();
static void *get_block_size(void *ptr);
static void *get_block_state(void *ptr);
static void *get_free_block_prev(void *ptr);
static void *get_free_block_next(void *ptr);
