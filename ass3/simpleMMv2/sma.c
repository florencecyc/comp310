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
unsigned long totalAllocatedSize = 0; //	Total Allocated memory in Bytes
unsigned long totalFreeSize = 0;	  //	Total Free memory in Bytes in the free memory list
Policy currentPolicy = WORST;		  //	Current Policy

void *sma_malloc(int size) {
    char str[100];
    sprintf(str, "\tsma_malloc");
    puts(str);

    debug();

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

void sma_free(void *ptr) {
    char str[100];
    sprintf(str, "\tsma_free");
    puts(str);

    debug();

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
}

void sma_mallopt(int policy)
{
	// Assigns the appropriate Policy
	if (policy == 1)
	{
		currentPolicy = WORST;
	}
	else if (policy == 2)
	{
		currentPolicy = NEXT;
	}
}

void *allocate_from_sbrk(int size) {
    char str[60];
    sprintf(str, "\tallocate_from_sbrk: %d", size);
    puts(str);

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

    return newBlock;
}

void *allocate_from_freeList(int size) {
    char str[60];
    sprintf(str, "\tallocate_from_freeList: %d", size);
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
    void *newBlock = NULL;
    void *largestFreeBlock = get_largest_free_block();

    if (largestFreeBlock != NULL) {
        newBlock = allocate_block_from_freeList(largestFreeBlock, size);
    }

    return newBlock;
}

void *allocate_next_fit(int size) {
    
}

void *allocate_block_from_freeList(void *freeBlock, int newBlockSize) {
    int freeBlockSize = get_block_size(freeBlock);
    
    char str[60];
    sprintf(str, "%d ? %d", newBlockSize, freeBlockSize);
    puts(str);
    
    if (freeBlockSize < newBlockSize) {
        return NULL;
    }

    void *freePrev = get_free_block_prev(freeBlock);
    void *freeNext = get_free_block_next(freeBlock);

    void *newBlock = freeBlock;

    set_block_header_footer(newBlock, newBlockSize, NOT_FREE);

    int newFreeBlockSize = freeBlockSize - newBlockSize - BLOCK_FOOTER_SIZE - BLOCK_HEADER_SIZE;

    sprintf(str, "\tnewFreeBlockSize %d", newFreeBlockSize);
    puts(str);

    if (newFreeBlockSize > 0) {
        void *newFreeBlock = freeBlock + newBlockSize + BLOCK_FOOTER_SIZE + BLOCK_HEADER_SIZE;
        set_free_block_prev(newFreeBlock, freePrev);
        set_free_block_next(newFreeBlock, freeNext);

        if (freePrev != NULL) {
            set_free_block_next(freePrev, newFreeBlock);
        }
        if (freeNext != NULL) {
            set_free_block_prev(freeNext, newFreeBlock);
        }

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

    return newBlock;
}

void *get_largest_free_block() {
    if (freeListHead == NULL) {
        return NULL;
    }
    else {
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
}

void replace_block_freeList(void *ptr) {  // change ptr from allocated to freed
    char str[120];
    sprintf(str, "replace_block_freeList ptr %p", ptr);
    puts(str);

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
    *(char **)block = (char *)prev;
}

void set_free_block_next(void *block, void *next) {
    *(char **)(block + sizeof(char *)) = (char *)next;
}

int get_block_size(void *ptr) {
    int *ptrSize = (int *)ptr;
    
    if (ptrSize == NULL)
        return -1;

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

    void *cursor = freeListHead;
    while (cursor != NULL) {
        if (cursor == freeListHead) {
            sprintf(str, "\tcursor %p size %d >>> freeListHead", cursor, get_block_size(cursor));
        } else if (cursor == freeListTail) {
            sprintf(str, "\tcursor %p size %d >>> freeListTail", cursor, get_block_size(cursor));
        } else {
            sprintf(str, "\tcursor %p size %d", cursor, get_block_size(cursor));
        }
        puts(str);
        cursor = get_free_block_next(cursor);
    }
}
