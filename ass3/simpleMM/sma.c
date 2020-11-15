#include "sma.h"

#define MAX_TOP_FREE (128 * 1024) // Max top free block size = 128 Kbytes
//	TODO: Change the Header size if required
#define FREE_BLOCK_HEADER_SIZE 2 * sizeof(char *) + sizeof(int) // Size of the Header in a free memory block

//	Policy type definition
typedef enum {
	WORST,
	NEXT
} Policy;

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

void sma_free(void *ptr) {
    if (ptr == NULL) {
        puts("Error: Attempting to free NULL!");
    }
    else if (ptr > sbrk(0)) {
        puts("Error: Attempting to free unallocated space!");
    }
    else {
        //	Adds the block to the free memory list
		add_block_freeList(ptr);
    }
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

void *allocate_pBrk(int size) {
    void *newBlock = NULL;
    int excessSize;

	//	Allocates the Memory Block
	allocate_block(newBlock, size, excessSize, 0);
	return newBlock;
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
	
}

void *allocate_next_fit(int size) {

}

void allocate_block(void *newBlock, int size, int excessSize, int fromFreeList) {

}

void add_block_freeList(void *block) {
    
}
