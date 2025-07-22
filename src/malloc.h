#ifndef MALLOC_H
#define MALLOC_H

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>

struct blk_meta
{
    struct blk_meta *previous;
    struct blk_meta *next;
    void *end_page;
    size_t size;
    char status;
};

void *malloc(size_t size);
void free(void *ptr);
void *realloc(void *ptr, size_t size);
void *calloc(size_t nmemb, size_t size);

#endif /* !MALLOC_H */
