#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(){
    void *pointer = NULL;
    char *string = "test";
    pointer = &string; // point to the address of string
    printf("The string address: %p\n", string);
    printf("The string: %s\n", string);
    printf("The pointer address: %p\n", pointer);
    printf("The pointer points to: %p\n", (char*)(*(char**)(pointer)));

    return 0;
}
