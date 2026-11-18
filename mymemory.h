#ifndef MYMEMORY_C
#define MYMEMORY_C

typedef struct metadata *meta; 

void *mymalloc(size_t value);
struct metadata *request_w_sbrk(struct metadata *last,size_t value);
struct metadata *find_space(struct metadata **last,size_t value);
void splitting(struct metadata *block, size_t value);
void myfree(void *ptr);
void *mycalloc(size_t n,size_t value);
void *myrealloc(void *ptr,size_t value);

#endif