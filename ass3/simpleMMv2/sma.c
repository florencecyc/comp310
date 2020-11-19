#include <stdbool.h>
#include "sma.h"

#define MAX_TOP_FREE (128 * 1024)  // Max top free block size = 128 Kbytes
#define BLOCK_HEADER_SIZE 2 * sizeof(int)  // state (0 for free, 1 for allocated) + length
#define BLOCK_FOOTER_SIZE 2 * sizeof(int)  // length + state (0 for free, 1 for allocated)

#define FREE 1  // free block tag
#define NOT_FREE 2  // allocated block tag

typedef enum __Policy {
	WORST,
	NEXT
} Policy;

char *sma_malloc_error;
void *freeListHead = NULL;			  //	The pointer to the HEAD of the doubly linked free memory list
void *freeListTail = NULL;			  //	The pointer to the TAIL of the doubly linked free memory list
void *lastAllocatedPtr = NULL;        //    The pointer to the last allocated block
unsigned long totalAllocatedSize = 0; //	Total Allocated memory in Bytes
unsigned long totalFreeSize = 0;	  //	Total Free memory in Bytes in the free memory list
Policy currentPolicy = WORST;		  //	Current Policy

bool IS_DEBUG_MODE = false;

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

    lastAllocatedPtr = ptrMemory;
    
    if (IS_DEBUG_MODE) {
        char str[100];
        sprintf(str, "\tsma_malloc %d", size);
        puts(str);
        debug();
    }

    return ptrMemory;
}

void sma_free(void *ptr) {
    if (ptr == NULL) {
		puts("Error: Attempting to free NULL!");
	}
	// Checks if the ptr is beyond Program Break
	else if (ptr > sbrk(0)) {
		puts("Error: Attempting to free unallocated space!");
	}
    else {
		replace_block_freeList(ptr);
    }

    if (IS_DEBUG_MODE) {
        char str[100];
        sprintf(str, "\tsma_free %d", get_block_size(ptr));
        puts(str);
        debug();
    }
}

void *sma_realloc(void *ptr, int newSize) {
    if (ptr == NULL || newSize <= 0) {
        return NULL;
    }

    if (IS_DEBUG_MODE) {
        char str[60];
        sprintf(str, "sma_realloc %p new size %d", ptr, newSize);
        puts(str);
        sprintf(str, "original data %s", *(char **)ptr);
        puts(str);
        debug();
    }

    int ptrSize = get_block_size(ptr);

    if (newSize == ptrSize) {
        return ptr;
    }
    else if (newSize < ptrSize) {
        set_block_header_footer(ptr, newSize, NOT_FREE);
        int freeBlockSize = ptrSize - newSize - BLOCK_HEADER_SIZE - BLOCK_FOOTER_SIZE;
        if (freeBlockSize >= 2 * sizeof(char *)) {
            void *fakeAllocatedBlock = ptr + newSize + BLOCK_FOOTER_SIZE + BLOCK_HEADER_SIZE;
            set_block_header_footer(fakeAllocatedBlock, freeBlockSize, NOT_FREE);
            replace_block_freeList(fakeAllocatedBlock);
        }
        return ptr;
    }
    else {
        void *newPtr = sma_malloc(newSize);
        memcpy(newPtr, ptr, ptrSize);

        replace_block_freeList(ptr);
        return newPtr;
    }
}

void sma_mallopt(int policy)
{
	// Assigns the appropriate Policy
	if (policy == 1) {
		currentPolicy = WORST;
	}
	else if (policy == 2) {
		currentPolicy = NEXT;
        lastAllocatedPtr = NULL;
	}
}

void sma_mallinfo()
{
	//	Finds the largest Contiguous Free Space (should be the largest free block)
	void *largestFreeBlock = get_largest_free_block();
    int largestFreeBlockSize = get_block_size(largestFreeBlock);
	char str[60];

	//	Prints the SMA Stats
	sprintf(str, "Total number of bytes allocated: %lu", totalAllocatedSize);
	puts(str);
	sprintf(str, "Total free space: %lu", totalFreeSize);
	puts(str);
	sprintf(str, "Size of largest contigious free space (in bytes): %d", largestFreeBlockSize);
	puts(str);
}

void *allocate_from_sbrk(int size) {
    char str[60];
    /*
    sprintf(str, "\tallocate_from_sbrk: %d", size);
    puts(str);
    */

    void *newBlock = NULL;
    void *freeBlock = NULL;

    int tailSize = 0;
    if (freeListHead != NULL) {
        tailSize = freeListTail ? get_block_size(freeListTail) : get_block_size(freeListHead);
    }
    int excessSize = MAX_TOP_FREE;
    if (tailSize > 0) {
        excessSize += (size - tailSize);
    }

    void *sbrkHead = sbrk(2 * BLOCK_HEADER_SIZE + 2 * BLOCK_FOOTER_SIZE + size + excessSize);

    if (tailSize > 0) {
        if (freeListTail == NULL) {
            newBlock = freeListHead;
            freeListHead = NULL;
        } else {
            newBlock = freeListTail;
            void *tailPrev = get_free_block_prev(freeListTail);
            if (tailPrev == freeListHead) {
                freeListTail = NULL;
            } else {
                freeListTail = tailPrev;
            }
        }
    }
    else {
        newBlock = sbrkHead + BLOCK_HEADER_SIZE;
    }

    set_block_header_footer(newBlock, size, NOT_FREE);

    freeBlock = newBlock + size + BLOCK_HEADER_SIZE + BLOCK_FOOTER_SIZE;
    set_block_header_footer(freeBlock, MAX_TOP_FREE, FREE);
    append_block_freeList(freeBlock);

    if (newBlock != NULL) {
        totalAllocatedSize += size;
        totalFreeSize += excessSize;
    }

    return newBlock;
}

void *allocate_from_freeList(int size) {
	void *newBlock = NULL;

    if (currentPolicy == WORST) {
		newBlock = allocate_worst_fit(size);
    }
    else if (currentPolicy == NEXT) {
        newBlock = allocate_next_fit(size);
    }

    return newBlock;
}

void *allocate_worst_fit(int size) {
    void *newBlock = NULL;
    void *largestFreeBlock = get_largest_free_block();

    if (largestFreeBlock != NULL) {
        newBlock = allocate_block_from_freeList(largestFreeBlock, size);
    }

    return newBlock;
}

void *allocate_next_fit(int size) {
    void *newBlock = NULL;
    void *nextFreeBlock = get_next_fit_block(size);

    if (nextFreeBlock != NULL) {
        newBlock = allocate_block_from_freeList(nextFreeBlock, size);
    }

    return newBlock;
}

void *allocate_block_from_freeList(void *freeBlock, int newBlockSize) {
    int freeBlockSize = get_block_size(freeBlock);

    if (freeBlockSize < newBlockSize) {
        return NULL;
    }

    void *freePrev = get_free_block_prev(freeBlock);
    void *freeNext = get_free_block_next(freeBlock);

    void *newBlock = freeBlock;

    set_block_header_footer(newBlock, newBlockSize, NOT_FREE);

    int newFreeBlockSize = freeBlockSize - newBlockSize - BLOCK_FOOTER_SIZE - BLOCK_HEADER_SIZE;

    if (newFreeBlockSize >= 2 * sizeof(char *)) {
        void *newFreeBlock = freeBlock + newBlockSize + BLOCK_FOOTER_SIZE + BLOCK_HEADER_SIZE;
        set_free_block_prev(newFreeBlock, freePrev);
        set_free_block_next(newFreeBlock, freeNext);

        set_free_block_next(freePrev, newFreeBlock);
        set_free_block_prev(freeNext, newFreeBlock);

        if (freeListHead == freeBlock) {
            freeListHead = newFreeBlock;
        }
        else if (freeListTail == freeBlock) {
            freeListTail = newFreeBlock;
        }
        set_block_header_footer(newFreeBlock, newFreeBlockSize, FREE);
    }
    else {
        if (freeListHead == freeBlock) {
            freeListHead = NULL;
        }
        else if (freeListTail == freeBlock) {
            freeListTail = NULL;
            set_free_block_next(freeListHead, NULL);
        }
        else {
            set_free_block_prev(freeNext, freePrev);
            set_free_block_next(freePrev, freeNext);
        }
    }

    if (newBlock != NULL) {
        totalAllocatedSize += newBlockSize;

        if (newFreeBlockSize >= 2 * sizeof(char *)) {
            totalFreeSize -= newBlockSize;
        }
    }

    return newBlock;
}

void *get_largest_free_block() {
    if (freeListHead == NULL) {
        return NULL;
    }
    void *cursor = freeListHead;
    void *largestFreeBlock = cursor;
    int cursorSize = 0;
    int largestFreeBlockSize = 0;

    while (cursor != NULL) {
        cursorSize = get_block_size(cursor);
        if (cursorSize > largestFreeBlockSize) {
            largestFreeBlockSize = cursorSize;
            largestFreeBlock = cursor;
        }
        cursor = get_free_block_next(cursor);
    }

    return largestFreeBlock;
}

void *get_next_fit_block(int newBlockSize) {
    if (freeListHead == NULL) {
        return NULL;
    }
    void *cursorPtr = freeListHead;
    void *nextFreeBlock = NULL, *restartFreeBlock = NULL;
    int cursorSize;

    while (cursorPtr != NULL) {
        cursorSize = get_block_size(cursorPtr);

        if (restartFreeBlock == NULL && cursorSize >= newBlockSize && cursorPtr < lastAllocatedPtr) {
            restartFreeBlock = cursorPtr;
        }
        if (nextFreeBlock == NULL && cursorSize >= newBlockSize && cursorPtr >= lastAllocatedPtr) {
            nextFreeBlock = cursorPtr;
            break;
        }
        cursorPtr = get_free_block_next(cursorPtr);
    }

    char str[100];
    if (IS_DEBUG_MODE) {
        sprintf(str, "\tlastAllocatedPtr %p, nextFreeBlock %p, restartFreeBlock %p", lastAllocatedPtr, nextFreeBlock, restartFreeBlock);
        puts(str);
    }

    return nextFreeBlock ? nextFreeBlock : restartFreeBlock;
}

// Replace allocated ptr to free ptr
void replace_block_freeList(void *ptr) {
    char str[120];
    /*
    sprintf(str, "replace_block_freeList ptr %p", ptr);
    puts(str);
    */

    int ptrSize = get_block_size(ptr);

    if (freeListHead == NULL) {
        set_block_header_footer(ptr, ptrSize, FREE);
        set_free_block_prev(ptr, NULL);
        set_free_block_next(ptr, NULL);
        freeListHead = ptr;
    }
    else {
        if (ptr < freeListHead) {
            if ((ptr + ptrSize + BLOCK_FOOTER_SIZE + BLOCK_HEADER_SIZE) == freeListHead) {
                merge_two_blocks(ptr, freeListHead);
            } else {
                set_block_header_footer(ptr, ptrSize, FREE);
                set_free_block_prev(ptr, NULL);
                set_free_block_next(ptr, freeListHead);
                set_free_block_prev(freeListHead, ptr);
                freeListTail = freeListHead;
                freeListHead = ptr;
            }
        }
        else {
            int prevBlockTag = *(int *)(ptr - BLOCK_HEADER_SIZE - BLOCK_FOOTER_SIZE);
            if (freeListTail == NULL) {
                if (prevBlockTag == FREE) {
                    merge_two_blocks(freeListHead, ptr);
                } else {
                    set_block_header_footer(ptr, ptrSize, FREE);
                    set_free_block_prev(ptr, freeListHead);
                    set_free_block_next(freeListHead, ptr);
                    freeListTail = ptr;
                }
            }
            else {
                if (ptr < freeListTail) {
                    void *freeCursor = freeListHead;
                    void *freeCursorNext = get_free_block_next(freeCursor);
                    while (freeCursor != NULL && freeCursorNext != NULL) {
                        if (freeCursor < ptr && ptr < freeCursorNext) {
                            int nextBlockTag = *(int *)(ptr + ptrSize + BLOCK_FOOTER_SIZE);

                            if (nextBlockTag == FREE) {
                                merge_two_blocks(ptr, freeCursorNext);
                            }
                            if (prevBlockTag == FREE) {
                                merge_two_blocks(freeCursor, ptr);
                            } 
                            if (nextBlockTag == NOT_FREE && prevBlockTag == NOT_FREE) {
                                set_block_header_footer(ptr, ptrSize, FREE);

                                set_free_block_prev(freeCursorNext, ptr);
                                set_free_block_prev(ptr, freeCursor);
                                set_free_block_next(freeCursor, ptr);
                                set_free_block_next(ptr, freeCursorNext);
                            }
                            break;
                        }
                        freeCursor = freeCursorNext;
                        freeCursorNext = get_free_block_next(freeCursorNext);
                    }
                } else {
                    if (prevBlockTag == FREE) {
                        merge_two_blocks(freeListTail, ptr);
                    } else {
                        set_free_block_next(freeListTail, ptr);
                        set_free_block_prev(ptr, freeListTail);
                        freeListTail = ptr;
                    }
                }
            }
        }
    }
    totalAllocatedSize -= ptrSize;
    totalFreeSize += ptrSize;
}

void append_block_freeList(void *ptr) {
    int ptrSize = get_block_size(ptr);
    set_block_header_footer(ptr, ptrSize, FREE);

    if (freeListHead == NULL) {
        freeListHead = ptr;
        set_free_block_prev(ptr, NULL);
        set_free_block_next(ptr, NULL);
    }
    else {
        if (freeListTail == NULL) {
            set_free_block_prev(ptr, freeListHead);
            set_free_block_next(ptr, NULL);
            set_free_block_next(freeListHead, ptr);
        } else {
            set_free_block_prev(ptr, freeListTail);
            set_free_block_next(ptr, NULL);
            set_free_block_next(freeListTail, ptr);
        }
        freeListTail = ptr;
    }
}

void merge_two_blocks(void *formerPtr, void *latterPtr) {    
    int formerSize = get_block_size(formerPtr);
    int latterSize = get_block_size(latterPtr);
    int latterTag = *(int *)(latterPtr - BLOCK_HEADER_SIZE);

    int mergeSize = formerSize + BLOCK_FOOTER_SIZE + BLOCK_HEADER_SIZE + latterSize;

    set_block_header_footer(formerPtr, mergeSize, FREE);

    if (latterTag == FREE) {
        void *latterPrev = get_free_block_prev(latterPtr);
        void *latterNext = get_free_block_next(latterPtr);

        if (latterPrev != NULL) {
            set_free_block_next(latterPrev, formerPtr);
        }
        set_free_block_next(formerPtr, latterNext);

        if (latterNext != NULL) {
            set_free_block_prev(latterNext, formerPtr);
        }
        set_free_block_prev(formerPtr, latterPrev);
        
    }
    
    if (latterPtr == freeListHead) {
        freeListHead = formerPtr;
    }
    else if (latterPtr == freeListTail) {
        freeListTail = formerPtr;
    }

    totalFreeSize += (BLOCK_FOOTER_SIZE + BLOCK_HEADER_SIZE);

    if (mergeSize > MAX_TOP_FREE) {
        set_block_header_footer(formerPtr, MAX_TOP_FREE, FREE);
        int brkState = brk(sbrk(0) - (mergeSize - MAX_TOP_FREE));

        if (IS_DEBUG_MODE) {
            char str[60];
            if (brkState == 0) {
                sprintf(str, "\tbrk() SUCCESS, ret val %d", brkState);
            } else {
                sprintf(str, "\tbrk() FAILURE, ret val %d", brkState);
            }
            puts(str);
        }

        totalFreeSize -= (mergeSize - MAX_TOP_FREE);
    }
}

void set_block_header_footer(void *block, int size, int tag) {
    // header
    *(int *)(block - 2 * sizeof(int)) = tag;
    *(int *)(block - sizeof(int)) = size;
    // footer
    *(int *)(block + size) = tag;
    *(int *)(block + size + sizeof(int)) = size;
}

void set_free_block_prev(void *block, void *prev) {
    if (block != NULL) {
        *(char **)block = (char *)prev;
    }
}

void set_free_block_next(void *block, void *next) {
    if (block != NULL) {
        *(char **)(block + sizeof(char *)) = (char *)next;
    }
}

int get_block_size(void *ptr) {
    if (ptr == NULL) {
        return -1;
    }
    int *ptrSize = (int *)ptr;
    ptrSize--;
    return *(int *)ptrSize;
}

void *get_free_block_prev(void *ptr) {
    char **ptrPrev = (char **)ptr;

    return *(char **)ptrPrev;
}

void *get_free_block_next(void *ptr) {
    char **ptrNext = (char **)ptr;
    ptrNext++;

    return *(char **)ptrNext;
}

void debug() {
    char str[120];

    sprintf(str, "------- FreeListDebug -------");
    puts(str);

    void *cursor = freeListHead;
    while (cursor != NULL) {
        if (cursor == freeListHead) {
            sprintf(str, "\t%p size %d >>> freeListHead", cursor, get_block_size(cursor));
        } else if (cursor == freeListTail) {
            sprintf(str, "\t%p size %d >>> freeListTail", cursor, get_block_size(cursor));
        } else {
            sprintf(str, "\t%p size %d", cursor, get_block_size(cursor));
        }
        puts(str);
        cursor = get_free_block_next(cursor);
    }

    sprintf(str, "\n\t%p >>> lastAllocatedPtr", lastAllocatedPtr);
    puts(str);
}
