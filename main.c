#include <stdio.h>
#include <stdbool.h>
#include <stddef.h> //size_t
#include <sys/mman.h> // mmap
#include <unistd.h> 
#include <stdlib.h>

#include "mymemory.h"

int main(void){

    printf(" Allocation. \n");

    int *ptr=mymalloc(100*sizeof(int));

    for(int i=0;i<5;i++){
        ptr[i]=i+1;
    }
    for(int i=0;i<5;i++){
        printf(" Value %d: %d \n",i,ptr[i]);
    }

    printf(" Reallocation: \n");

    ptr=myrealloc(ptr,5*sizeof(int));

    /*for(int i=5;i<100;i++){
        ptr[i]=i+1;
    }*/

    for(int i=0;i<5;i++){
        printf(" Value %d: %d \n",i,ptr[i]);
    }

    myfree(ptr);

    printf(" Deallocation done. \n");
    
    return 0;
}