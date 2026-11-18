#include <stdio.h>
#include <stdbool.h>
#include <string.h> // memcpy
#include <stdint.h> // MAX_SIZE
#include <stddef.h> //size_t
#include <sys/mman.h> // mmap
#include <unistd.h> 

#include "mymemory.h"

typedef struct metadata{
    size_t size; // size of the free block
    bool ismmap; // flag to recognise whether it was used mmap or sbrk
    bool isfree; // flag to recognise whether the memory block is free or used 
    struct metadata *next; // pointer to next metadata block
} metadata;

const size_t metasize=sizeof(struct metadata);
const size_t threshold= (128*1024); // 1024 byte size of a kb so if the need is higher than 128 kb mmap will be used
struct metadata *head_data=NULL;

struct metadata *free_list=NULL;
void *mymalloc(size_t value){
    struct metadata *block;
    if( value == 0 ) return NULL; //if the number of byte required is zero the programm can immediately finish

    value=((value + sizeof(void*) -1) & ~(sizeof(void*) -1)); //CPU alignment

    if(value >= threshold){
        // big quantity of byte required so we will use mmap
        void *ptr = mmap(NULL,value+metasize, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE,-1,0);
        
        if ( ptr == MAP_FAILED ){ 
            return NULL; } //MAP_FAILED is the value returned by mmap if the allocation wasn't successful
    
        block=(struct metadata *)ptr;

        block->size= value;
        block->ismmap = true;
        block->isfree = false;
        block->next = NULL;

    }

    else{
        if(!free_list){ // case 1: first allocation -> normal sbrk, "start" of the heap we are creating
            block=request_w_sbrk(NULL,value); 
            if(!block) return NULL;
            free_list=block;
        }
        else{
            struct metadata *last=free_list; 
            block=find_space(&last,value);
            if(block==NULL){
                block=request_w_sbrk(last,value);
                if(block == NULL) return NULL; 
            }
            else{
                splitting(block,value);
                block->isfree=false;
            }
        }
        return block+1;
    }    
}

struct metadata *request_w_sbrk(struct metadata *last,size_t value){
    void *ptr=sbrk(metasize+value);
    if(ptr == (void*)-1) return NULL;

    struct metadata *metablock=(struct metadata *)ptr;
    metablock->size= value;
    metablock->isfree= false;
    metablock->ismmap= false;
    metablock->next= NULL;

    if(last != NULL) last->next = metablock;

    return metablock;
}

struct metadata *find_space(struct metadata **last,size_t value){
    struct metadata *current=free_list;
    while(current != NULL){
        if(current->isfree == true && current->size>=value){
            return current;
        }
        *last=current;
        current=current->next;
    }

    return NULL;
}

void splitting(struct metadata *block, size_t value){
    if(block->size >= value + metasize + sizeof(void*)){
        struct metadata *new_block=(struct metadata *)((char*)(block+1)+value);
        new_block->size = block->size - metasize - value;
        block->size= value;
        new_block->next=block->next;
        block->next=new_block;
        block->isfree= false;
        block->ismmap = false;
        new_block->ismmap= false;
        new_block->isfree=true;
    }
}

void myfree(void *ptr){
    if(ptr == NULL) return;

    struct metadata *block=(struct metadata *)ptr-1;
    if(block->ismmap == true && block->isfree ==false){
        munmap(block,block->size + metasize);
        return;
    }
    else if(block->ismmap == false){
        block->isfree=true;

        // coalescing
        
        if(block->next != NULL && block->next->isfree == true){
        struct metadata *temp=block->next;
        block->size= block->size + temp->size + metasize;
        block->next=temp->next;
    }

        struct metadata *temp=free_list;
        while(temp!=NULL){
            if(temp->next==block){
                if(temp->isfree==true){
                    temp->size = temp->size + metasize + block->size;
                    temp->next=block->next;
                }
                return;
            }
            temp=temp->next;
        }
    }

}

void *mycalloc(size_t n,size_t value){
    if(n==0 || value==0) 
    return NULL;

    if(n<=SIZE_MAX/value){
    void *ptr=mymalloc(n*value);
    if(ptr==NULL) 
    return NULL;
    memset(ptr,0,n*value);
    return ptr;}

    return NULL;
}

size_t min_dim(size_t d1,size_t d2){
    if(d1<d2) return d1;
    else return d2;
}

void *myrealloc(void *ptr,size_t value){

    if(ptr == NULL && value == 0) 
    return NULL;

    if(ptr==NULL) 
    return mymalloc(value);

    if(value == 0) {
        myfree(ptr);
        return NULL;
    }

    struct metadata *block=(struct metadata *)ptr-1;
    value=((value + sizeof(void*) -1) & ~(sizeof(void*) -1));

    if(block->size >= value){
        splitting(block,value);
        return ptr;
    }
    else{
        if(block->next!= NULL && block->next->isfree == true && (block->next->size + block->size + metasize) >= value){
            struct metadata *temp=block->next;
            block->next=temp->next;
            block->size= block->size + temp->size + metasize;
            splitting(block,value);
            return block+1;
        }
        else{
            void *new_ptr=mymalloc(value);
            if(new_ptr == NULL) 
            return NULL;
            memcpy(new_ptr,ptr,min_dim(block->size,value));
            myfree(ptr);
            return new_ptr;
        }
    }

}
