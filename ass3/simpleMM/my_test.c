#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    /*
    int *a = (int *)malloc(sizeof(int));
    *a = 1;

    printf("ptr: %p\n", a);
    printf("size ptr: %ld\n", sizeof(a));
    printf("size int: %ld\n", sizeof(*a));
    printf("ptr: %p\n", a + 1);
    */

    void *a = NULL;
    printf("ptr: %p\n", a);
    test(a);
    printf("ptr: %p\n", a);

}

void test(void *a) {
    a = (int *)sbrk(sizeof(int));
    *((int *)a) = 0;
    printf("ptr test: %p\n", a);
    printf("ptr test: %p\n", &a);

    printf("a test: %d\n", *((int *)a));

}