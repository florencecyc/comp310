#include "sma.h"

#define MAX_TOP_FREE (128 * 1024)  // Max top free block size = 128 Kbytes
#define BLOCK_HEADER_SIZE 2 * sizeof(int)  // state(0 for free, 1 for allocated) + length
#define FREE_BLOCK_EXTRA_HEADER_SIZE 2 * sizeof(char *)  // prev + next

#define max(X,Y) ((X) > (Y) ? (X) : (Y))

typedef enum __Policy {
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
        ptrMemory = allocate_from_sbrk(size);
    } else {
        // Allocate memory from the free memory list
        ptrMemory = allocate_from_freeList(size);
        if (ptrMemory == NULL) {
            ptrMemory = allocate_from_sbrk(size);
        }
    }
    // Validates memory allocation
    if (ptrMemory == NULL || ptrMemory < 0) {
        sma_malloc_error = "Error: Memory allocation failed!";
        return NULL;
    }
    // Updates SMA Info
    totalAllocatedSize += size;

    return ptrMemory;
}

void sma_free(void* ptr) {
    char str[100];

    if (ptr == NULL) {
		puts("Error: Attempting to free NULL!");
	}
	// Checks if the ptr is beyond Program Break
	else if (ptr > sbrk(0)) {
		puts("Error: Attempting to free unallocated space!");
	}
	else {
		//	Adds the block to the free memory list
		add_block_freeList(ptr);
        sprintf(str, "freeListHead after freeing: %p", freeListHead);
        puts(str);
        sprintf(str, "freeListTail after freeing: %p", freeListTail);
        puts(str);
	}
}

void add_block_freeList(void* block) {
    int blockSize = *(int *)get_block_size(block);

    if (freeListHead == NULL) {
        freeListHead = block;
    } else if (freeListTail == NULL) {
        // todo: check if you can merge
        freeListTail = block;
        *(char **)(freeListHead + sizeof(char *)) = freeListTail;
    } else {
        // todo: check if you can merge

        *(char **)(freeListTail + sizeof(char *)) = (char *)block;
        *(char **)block = freeListTail;
        freeListTail = block;
    }

	totalAllocatedSize -= blockSize;
	totalFreeSize += blockSize;
}

void *allocate_from_sbrk(int size) {
    char str[60];
    sprintf(str, "allocate_from_sbrk: %d", size);
    puts(str);

    void *newBlock = NULL;
    void *freeBlock = NULL;
    int excessSize = MAX_TOP_FREE;

    void *sbrkHead = sbrk(BLOCK_HEADER_SIZE * 2 + size + excessSize);

    newBlock = sbrkHead + BLOCK_HEADER_SIZE;
    set_new_block_config(newBlock, size);

    freeBlock = sbrkHead + BLOCK_HEADER_SIZE * 2 + size;
    set_free_block_config(freeBlock, excessSize, freeListTail, NULL);

    totalAllocatedSize += size;
    totalFreeSize += excessSize;
    return newBlock;
}

void *allocate_from_freeList(int size) {
    char str[60];
    sprintf(str, "allocate_from_freeList: %d", size);
    puts(str);

	void *newBlock = NULL;

    if (currentPolicy == WORST) {
		newBlock = allocate_worst_fit(size);
    }
    else if (currentPolicy == NEXT) {
        newBlock = allocate_next_fit(size);
    }
    else {
        newBlock = NULL;
    }

    return newBlock;
}

void *allocate_worst_fit(int size) {
    void *largestFreeBlock = get_largest_free_block();

    void *newBlock = NULL;
    newBlock = replace_block_freeList(largestFreeBlock, size);

    return newBlock;
}

void *allocate_next_fit(int size) {
    
}

void *replace_block_freeList(void *freeBlock, int newBlockSize) {
    int freeBlockSize = *(int *)get_block_size(freeBlock);

    if (freeBlockSize < newBlockSize) {
        return NULL;
    }

    void *newBlock = freeBlock;
    set_new_block_config(newBlock, newBlockSize);
    
    void *prev = (char *)newBlock;
    void *next = (char *)prev++;
    freeBlockSize = freeBlockSize - newBlockSize - BLOCK_HEADER_SIZE;

    if (freeListHead == freeBlock) {
        if (freeBlockSize < BLOCK_HEADER_SIZE + FREE_BLOCK_EXTRA_HEADER_SIZE) {
            freeListHead = get_free_block_next(freeListHead);
            *(char **)freeListHead = NULL;
        } else {
            freeBlock = freeBlock + newBlockSize + BLOCK_HEADER_SIZE;
            freeListHead = freeBlock;
            set_free_block_config(freeBlock, freeBlockSize, prev, next);
        }
    }
    else if (freeListTail == freeBlock) {
        if (freeBlockSize < BLOCK_HEADER_SIZE + FREE_BLOCK_EXTRA_HEADER_SIZE) {
            freeListTail = get_free_block_prev(freeListTail);
            *(char **)(freeListTail + sizeof(char *)) = NULL;
        } else {
            freeBlock = freeBlock + newBlockSize + BLOCK_HEADER_SIZE;
            freeListTail = freeBlock;
            set_free_block_config(freeBlock, freeBlockSize, prev, next);
        }
    }
    else {
        if (freeBlockSize >= BLOCK_HEADER_SIZE + FREE_BLOCK_EXTRA_HEADER_SIZE) {
            freeBlock = freeBlock + newBlockSize + BLOCK_HEADER_SIZE;
            set_free_block_config(freeBlock, freeBlockSize, prev, next);
        }
    }

    char str[60];
    sprintf(str, "freeListHead after replacing: %p", freeListHead);
    puts(str);
    sprintf(str, "freeListTail after replacing: %p", freeListTail);
    puts(str);

    totalAllocatedSize += newBlockSize;
    totalFreeSize -= newBlockSize;

    return newBlock;
}

void set_new_block_config(void *block, int size) {
    *(int *)(block - sizeof(int *)) = size;
    *(int *)(block - 2 * sizeof(int *)) = 1;
}

void set_free_block_config(void *block, int size, void *prev, void *next) {
    char str[100];

    *(int *)(block - sizeof(int)) = size;
    *(int *)(block - 2 * sizeof(int)) = 0;

    if (freeListHead == NULL) {
        freeListHead = block;
    }
    else if (freeListTail == NULL) {
        *(char **)(freeListHead) = (char *)prev;
        *(char **)(freeListHead + sizeof(char *)) = (char *)next;
    }
    else {
        
    }
}

void *get_largest_free_block() {
    if (freeListHead == NULL) {
        return NULL;
    } else {
        void *cursorBlock = freeListHead;
        int cursorBlockSize;
        int largestFreeBlockSize = *(int *)get_block_size(cursorBlock);
        void *largestFreeBlock = cursorBlock;

        while ((cursorBlock = get_free_block_next(cursorBlock)) != NULL) {
            if ((cursorBlockSize = *(int *)get_block_size(cursorBlock)) > largestFreeBlockSize) {
                largestFreeBlockSize = cursorBlockSize;
                largestFreeBlock = cursorBlock;
            }
        }

        return largestFreeBlock;
    }
}

void *get_block_size(void *ptr) {
    int *ptrSize;

    ptrSize = (int *)ptr;
    ptrSize--;
    return (int *)ptrSize;
}

void *get_free_block_prev(void *ptr) {
    char *ptrPrev;

    ptrPrev = (char *)ptr;
    return (char *)ptrPrev;
}

void *get_free_block_next(void *ptr) {
    char *ptrNext;

    ptrNext = (char *)ptr;
    ptrNext++;
    return (char *)ptrNext;
}
