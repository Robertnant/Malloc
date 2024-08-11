# Custom Malloc Implementation in C

This project is a custom implementation of the `malloc`, `free`, `calloc`, and `realloc` functions in C. It demonstrates a basic yet functional memory allocation strategy using mmap and explicit free lists for managing memory blocks.

## Features

- **Block Allocation:** Allocates memory blocks of varying sizes to fulfill user requests.
- **Free Lists:** Employs free lists to keep track of available memory blocks within allocated pages.
- **Page Management:** Uses `mmap` for requesting memory pages from the operating system and managing their allocation status.
- **Block Alignment:** Aligns allocated blocks to a specific size (long double) to simplify memory management.
- **Metadata Storage:** Stores metadata about allocated buckets and free lists.

## Implementation Details

The implementation utilizes the following key concepts:

- **Buckets:** Memory is divided into buckets, each containing blocks of a specific size.
- **Free List:** Each bucket maintains a free list to track available blocks. The free list is implemented as an array of flags within the bucket metadata.
- **Page Allocation:** When a new bucket is required, a new page is requested from the operating system using `mmap`.
- **Block Splitting:**  Currently not implemented, but could be added to divide large blocks into smaller ones if needed. 
- **Coalescing:** Currently not implemented, but could be added to merge adjacent free blocks upon freeing.

## Usage

The provided `malloc.h` header file exposes the custom `malloc`, `free`, `calloc`, and `realloc` functions. These functions can be used as drop-in replacements for the standard library versions, allowing you to observe the behavior of this custom allocator.

## Example

```c
#include <stdio.h>
#include "malloc.h" 

int main() {
    int *ptr = (int *)malloc(sizeof(int));
    *ptr = 10;
    printf("Value at allocated memory: %d\n", *ptr);
    free(ptr);
    return 0;
}
