# 🧠 MyCMemory: A Custom Memory Allocator

A custom educational implementation of the core C memory allocation functions: `malloc`, `free`, `calloc`, and `realloc`. 

This project was built from scratch to deeply understand low-level memory management, heap organization, and memory alignment in C.

---

## 📁 Project Structure

The project is lightweight and modular, separated into implementation files, tests, and build automation:

**mycmemory/**
**├── .gitignore**
**├── main.c  # Test suite and usage examples**
**├── Makefile # Build automation**
**├── mymemory.c   # Core logic for memory functions**
**├── mymemory.h # Function prototypes and metadata definitions**
**└── README.md  # Project documentation**


## ⚙️ Core Concepts & Design Decisions

### 1. CPU Alignment

Modern processors read and write memory in chunks (e.g., 4 or 8 bytes at a time). Requesting unaligned memory can lead to performance hits or hardware crashes (Segmentation Faults).

To ensure every allocated block starts at a memory address that is a clean multiple of a pointer's size (`sizeof(void*)`), I used this bitwise rounding trick:

C

```
value = ((value + sizeof(void*) - 1) & ~(sizeof(void*) - 1));
```

This formula efficiently rounds up the requested size to the next multiple of 4 (on 32-bit systems) or 8 (on 64-bit systems) using fast bitwise operations instead of costly divisions.

### 2. The Metadata Struct

When a standard `free(ptr)` is called, the user only passes the pointer. **How does the allocator know how big the block is or where it belongs?**

The answer is the metadata header. Every allocated block is preceded by a hidden control structure:

C

```
typedef struct metadata {
    size_t size;            // Size of the usable memory block
    bool ismmap;            // Flag to distinguish between mmap and sbrk allocation
    bool isfree;            // Flag to track whether the block is free or used
    struct metadata *next;  // Pointer to the next metadata block in the list
} metadata;
```

- **How it works behind the scenes:** When `malloc` is called, it requests from the OS not just the bytes requested by the user, but `bytes_requested + sizeof(metadata)`.
    
- **The trick:** The function returns `metadata + 1`, which points precisely to the first usable memory address _after_ the metadata block. The metadata stays safely hidden just behind the pointer given to the user.
    

### 3. Sbrk, Mmap, and Why We Don't Always Shrink the Heap

Managing memory via system calls comes with strict performance considerations:

- **`mmap` blocks:** If a block was allocated via `mmap`, it is released back to the operating system immediately using `munmap(ptr, size)`.
    
- **`sbrk` blocks:** `sbrk` _can_ be used to shrink the heap, but doing so on every `free` introduces massive overhead and layout complications (you can only shrink the heap if the freed block is at the absolute end of it).
    

Instead, when an `sbrk` block is freed, we simply **mark it as free** (`isfree = true`) and rely on **coalescing** to make future allocations faster and cleaner without constantly resizing the heap boundaries.

### 4. Coalescing (Merging Free Blocks)

When we free a block allocated via `sbrk`, we don't shrink the heap. But wait: _how and when does memory actually become free, and what happens during coalescing?_

The memory becomes free the moment the user calls `my_free()`. Over time, freeing blocks scattered around the heap creates a fragmented list of alternating free and used spaces. The crucial point of **coalescing** is checking if the adjacent blocks in our linked list are _also_ free.

Seesing how it works in practice:
- **Forward Coalescing:** If the block right after ours is free (`block->next->isfree == true`), we can "merge" the two blocks simply by expanding the `size` of the current block and bypassing the next block using the `.next` pointer (`block->next = block->next->next`).
    
- **What happens to the old data?** Physically, it stays right there in RAM for a moment, but logically it no longer exists because the allocator has changed the boundaries of the block. It will only be overwritten when a new `malloc` comes along and reuses that space. This saves us from wasting time cleaning the memory byte by byte.
    

### 5. Malloc vs Calloc (and the use of `memset`)

While `malloc` simply allocates the requested space leaving "garbage" inside (the old data previously stored in that RAM), `calloc` does two extra things:

1. It allocates an array of elements (by calculating `number_of_elements * size`).
    
2. **It initializes all the memory to zero.**
    

To do this cleanly in my code, after obtaining the memory block (often by leveraging `malloc` itself), I use the standard `memset` function:

C

```
memset(ptr, 0, total_size);
```

`memset` takes the pointer, places `0` as the value for every single byte, and fills the entire range specified by `total_size`. This is crucial to avoid bugs caused by uninitialized variables.

### 6. Realloc and the use of `memcpy`

The `realloc` function is used to resize an already allocated memory block (making it bigger or smaller).

Handling `realloc` requires three key logical steps:

1. If the passed pointer is `NULL`, it acts exactly like a `malloc`.
    
2. If the new size is smaller than or equal to the old one, we can just keep the block as it is (or shrink it).
    
3. If instead there isn't enough space, we must allocate a **new, larger block**, **copy the old data** into the new space, and finally free the old block.
    

To copy the data safely and quickly without going crazy writing byte-by-byte `for` loops, we use `memcpy`:

C

```
memcpy(new_ptr, old_ptr, old_size);
```

`memcpy` takes the data from `old_ptr` and moves it straight over to `new_ptr` for the number of bytes indicated by `old_size`. Right after that, we call our `my_free(old_ptr)` and we're done!

## 🚀 Getting Started

### Prerequisites

You need a standard C compiler (like `gcc`) and `make` installed on a UNIX/Linux environment.
I suggest the use of valgrind as well to detect all the possible memory leaks.