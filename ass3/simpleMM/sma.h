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
static void *allocate_from_sbrk(int size);
static void *allocate_from_freeList(int size);
static void set_new_block_config(void *block, int size);
static void set_free_block_config(void *block, int size, void *prev, void *next);
static void *allocate_worst_fit(int size);
static void *allocate_next_fit(int size);
static void *get_largest_free_block();
static void *replace_free_block(void *freeBlock, int newBlockSize);

static void *get_block_size(void *ptr);
static void *get_free_block_prev(void *ptr);
static void *get_free_block_next(void *ptr);
