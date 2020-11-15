#include "sma.h"

#define MAX_TOP_FREE (128 * 1024)  // Max top free block size = 128 Kbytes
#define BLOCK_HEADER_SIZE sizeof(State) + sizeof(int)  // state + length
#define FREE_BLOCK_EXTRA_HEADER_SIZE 2 * sizeof(void *)  // prev + next

#define max(X,Y) ((X) > (Y) ? (X) : (Y))

typedef enum {
	WORST,
	NEXT
} Policy;

typedef enum {
    FREE, 
    ALLOCATED
} State;

char *sma_malloc_error;
void *freeListHead = NULL;			  //	The pointer to the HEAD of the doubly linked free memory list
void *freeListTail = NULL;			  //	The pointer to the TAIL of the doubly linked free memory list
unsigned long totalAllocatedSize = 0; //	Total Allocated memory in Bytes
unsigned long totalFreeSize = 0;	  //	Total Free memory in Bytes in the free memory list
Policy currentPolicy = WORST;		  //	Current Policy

void *sma_malloc(int size) {
    void *ptrMemory = NULL;

    if (freeListHead == NULL) {
        // Allocate memory by increasing the Program Break
        ptrMemory = allocate_pBrk(size);
    } else {
        // Allocate memory from the free memory list
        ptrMemory = allocate_freeList(size);
        if (ptrMemory == (void *)-2) {
            ptrMemory = allocate_pBrk(size);
        }
    }
    // Validates memory allocation
    if (ptrMemory < 0 || ptrMemory == NULL) {
        sma_malloc_error = "Error: Memory allocation failed!";
        return NULL;
    }
    // Updates SMA Info
    totalAllocatedSize += size;
    return ptrMemory;
}

void *allocate_pBrk(int size) {
    void *newBlock = NULL;
    int excessSize = MAX_TOP_FREE;

	//	Allocate memory by incrementing the Program Break by calling sbrk() or brk()
	newBlock = allocate_block(size, excessSize, 0);
	return newBlock;
}

void *allocate_block(int size, int excessSize, int fromFreeList) {
    void *newBlock = NULL;
	void *excessFreeBlock = NULL;  // pointer for any excess free block
	int addFreeBlock;

    void *sbrkHead = sbrk(BLOCK_HEADER_SIZE + size + excessSize);

    newBlock = sbrkHead + BLOCK_HEADER_SIZE;
    int *newBlockSize = (int *)get_block_size(newBlock);
    *newBlockSize = size;
    State *newBlockState = (State *)get_block_state(newBlock);
    *newBlockState = ALLOCATED;

	// Checks if excess free size is big enough to be added to the free memory list
	// Helps to reduce external fragmentation
	addFreeBlock = excessSize > BLOCK_HEADER_SIZE + FREE_BLOCK_EXTRA_HEADER_SIZE;
    
	//	If excess free size is big enough
	if (addFreeBlock)
	{
		// Create a free block using the excess memory size, then assign it to the Excess Free Block
        excessFreeBlock = sbrkHead + BLOCK_HEADER_SIZE + size + BLOCK_HEADER_SIZE;
        int *excessFreeBlockSize = (int *)get_block_size(excessFreeBlock);
        *excessFreeBlockSize = excessSize - BLOCK_HEADER_SIZE;

		//	Checks if the new block was allocated from the free memory list
		if (fromFreeList)
		{
			//	Removes new block and adds the excess free block to the free list
			replace_block_freeList(newBlock, excessFreeBlock);
		}
		else
		{
			//	Adds excess free block to the free list
			add_block_freeList(excessFreeBlock);
		}
	}
	//	Otherwise add the excess memory to the new block
	else
	{
		//	TODO: Add excessSize to size and assign it to the new Block

		//	Checks if the new block was allocated from the free memory list
		if (fromFreeList)
		{
			//	Removes the new block from the free list
			remove_block_freeList(newBlock);
		}
	}
    
    return newBlock;
}

void sma_free(void *ptr) {
    if (ptr == NULL) {
        puts("Error: Attempting to free NULL!");
    }
    else if (ptr > sbrk(0)) {
        puts("Error: Attempting to free unallocated space!");
    }
    else {
        // Adds the block to the free memory list
		add_block_freeList(ptr);
    }
}

void add_block_freeList(void *block) {
   if (freeListHead == NULL) {
       freeListHead = block;
   }
   
}

void *allocate_freeList(int size) {
	void *ptrMemory = NULL;

    if (currentPolicy == WORST) {
		ptrMemory = allocate_worst_fit(size);
    }
    else if (currentPolicy == NEXT) {
        ptrMemory = allocate_next_fit(size);
    }
    else {
        ptrMemory = NULL;
    }
    return ptrMemory;
}

void *allocate_worst_fit(int size) {
	int largestFreeBlock = get_largest_freeBlock();
	
}

void *allocate_next_fit(int size) {

}

void sma_mallopt(int policy) {
    if (policy == 1) {
        currentPolicy = WORST;
    }
    else if (policy == 2) {
        currentPolicy = NEXT;
    }
}

void sma_mallinfo() {
	int largestFreeBlock = get_largest_freeBlock();
    char str[60];

    //	Prints the SMA Stats
	sprintf(str, "Total number of bytes allocated: %lu", totalAllocatedSize);
	puts(str);
	sprintf(str, "Total free space: %lu", totalFreeSize);
	puts(str);
	sprintf(str, "Size of largest contigious free space (in bytes): %d", largestFreeBlock);
	puts(str);
}

void *sma_realloc(void *ptr, int size) {

}

void *get_block_size(void *ptr) {
    int *ptrSize;

    ptrSize = (int *)ptr - sizeof(int);
    return (int *)ptrSize;
}

void *get_free_block_prev(void *ptr) {
    void *ptrPrev;

    ptrPrev = ptr;
    return (void *)ptrPrev;
}

void *get_free_block_next(void *ptr) {
    void *ptrNext;

    ptrNext = ptr + sizeof(void *);
    return (void *)ptrNext;
}

void *get_block_state(void *ptr) {
    void *ptrState;

    ptrState = ptr - sizeof(int) - sizeof(State);
    return (void *)ptrState;
}

int get_largest_freeBlock() {
	int largestBlockSize = 0;
    char str[300];

	// TODO: Iterate through the Free Block List to find the largest free block and return its size
    void *freeListCursor = freeListHead;
    void *freeListNext = NULL;
    int tmpSize;
    while ((freeListNext = get_free_block_next(freeListCursor)) != freeListTail) {
        tmpSize = *(int *)get_block_size(freeListCursor);
        sprintf(str, "cursor %p, next %p, tail %p", freeListCursor, freeListNext, freeListTail);
        puts(str);
        largestBlockSize = max(tmpSize, largestBlockSize);
        break;
    }
    largestBlockSize = *(int *)get_block_size(freeListCursor);

	return largestBlockSize;
}