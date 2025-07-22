#include "malloc.h"

#include <stddef.h>
#include <unistd.h>

struct blk_meta *head = NULL;

static size_t align(size_t size)
{
    size_t s = (size + sizeof(long double)) - (size % sizeof(long double));
    if (size % sizeof(long double) == 0)
        s = size;
    return s;
}

static void *check(struct blk_meta *b, size_t s, size_t al, struct blk_meta *bl)
{
    void *v = b;
    char *c = v;
    v = bl;
    char *c1 = v;
    size_t si = s - align(sizeof(struct blk_meta));
    if (b->status == 1 && b->size >= si)
    {
        b->status = 0;
        if ((b->size - si) >= align(sizeof(struct blk_meta)))
        {
            void *ptr = b;
            char *p = ptr;
            p = p + s;
            ptr = p;
            struct blk_meta *nxt = ptr;
            nxt->next = b->next;
            b->next = nxt;
            nxt->previous = b;
            nxt->end_page = b->end_page;
            nxt->status = 1;
            nxt->size = b->size - si - align(sizeof(struct blk_meta));
        }
        b->size = al;
        if (bl != NULL)
            memcpy(c + align(sizeof(struct blk_meta)),
                   c1 + align(sizeof(struct blk_meta)), al);
        return c + align(sizeof(struct blk_meta));
    }
    return NULL;
}

static void aux_modif(struct blk_meta *b, size_t len, size_t s)
{
    void *ptr = b;
    char *p = ptr;
    p = p + s;
    ptr = p;
    struct blk_meta *nxt = ptr;
    b->next = nxt;
    nxt->previous = b;
    nxt->next = head;
    nxt->end_page = b->end_page;
    nxt->size = len - align(sizeof(struct blk_meta));
    nxt->status = 1;
}

static int aux_modif_bis(struct blk_meta *b, long val)
{
    struct blk_meta *nxt = mmap(NULL, val, PROT_READ | PROT_WRITE,
                                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (nxt == MAP_FAILED)
        return -1;
    head->next = nxt;
    nxt->previous = b;
    nxt->next = head;
    void *ptr = nxt;
    char *p = ptr;
    p = p + val;
    nxt->end_page = p;
    nxt->size = val - align(sizeof(struct blk_meta));
    nxt->status = 1;
    return 1;
}

static void *end_aux(size_t a, struct blk_meta *bl, struct blk_meta *b)
{
    head = b;
    void *v = head;
    char *c = v;
    v = bl;
    char *c1 = v;
    if (bl != NULL)
        memcpy(c + align(sizeof(struct blk_meta)),
               c1 + align(sizeof(struct blk_meta)), a);
    return c + align(sizeof(struct blk_meta));
}

static void *aux_malloc(size_t s, size_t a, struct blk_meta *bl)
{
    struct blk_meta *b = head;
    while (b != NULL)
    {
        void *res = check(b, s, a, bl);
        if (res != NULL)
            return res;
        b = b->next;
    }
    long val = sysconf(_SC_PAGESIZE);
    b = mmap(NULL, (s + val) - (s % val), PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (b == MAP_FAILED)
        return NULL;
    b->previous = NULL;
    b->status = 0;
    b->size = a;
    void *ptr = b;
    char *p = ptr;
    p = p + ((s + val) - (s % val));
    b->end_page = p;
    size_t len = (s + val) - (s % val) - s;
    if (len >= align(sizeof(struct blk_meta)))
        aux_modif(b, len, s);
    else
    {
        if (aux_modif_bis(b, val) == -1)
            return NULL;
    }
    return end_aux(a, bl, b);
}

static void modif(size_t len, void *ptr, size_t s)
{
    char *p = ptr;
    p = p + s;
    ptr = p;
    struct blk_meta *nxt = ptr;
    head->next = nxt;
    nxt->previous = head;
    nxt->next = NULL;
    nxt->end_page = head->end_page;
    nxt->size = len - align(sizeof(struct blk_meta));
    nxt->status = 1;
}

static int modif_bis(long val, size_t s)
{
    struct blk_meta *nxt = mmap(NULL, val, PROT_READ | PROT_WRITE,
                                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (nxt == MAP_FAILED)
    {
        munmap(head, (s + val) - (s % val));
        head = NULL;
        return -1;
    }
    head->next = nxt;
    nxt->previous = head;
    nxt->next = NULL;
    void *ptr = nxt;
    char *p = ptr;
    p = p + val;
    nxt->end_page = p;
    nxt->size = val - align(sizeof(struct blk_meta));
    nxt->status = 1;
    return 1;
}

__attribute__((visibility("default"))) void *malloc(size_t size)
{
    size_t al = align(size);
    size_t s = al;
    s = s + align(sizeof(struct blk_meta));
    if (head == NULL)
    {
        long val = sysconf(_SC_PAGESIZE);
        head = mmap(NULL, (s + val) - (s % val), PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (head == MAP_FAILED)
        {
            head = NULL;
            return NULL;
        }
        head->previous = NULL;
        head->status = 0;
        head->size = al;
        void *ptr = head;
        char *p = ptr;
        p = p + ((s + val) - (s % val));
        head->end_page = p;
        size_t len = (s + val) - (s % val) - s;
        if (len >= align(sizeof(struct blk_meta)))
            modif(len, ptr, s);
        else
        {
            if (modif_bis(val, s) == -1)
                return NULL;
        }
        void *v = head;
        char *c = v;
        return c + align(sizeof(struct blk_meta));
    }
    return aux_malloc(s, al, NULL);
}

static void *aux_free(struct blk_meta *b)
{
    while (b->next != NULL)
    {
        if (b->next->end_page != b->end_page)
            break;
        if (b->next->status == 1)
        {
            b->size = b->size + align(sizeof(struct blk_meta)) + b->next->size;
            b->next = b->next->next;
            if (b->next != NULL)
                b->next->previous = b;
        }
        else
            break;
    }
    while (b->previous != NULL)
    {
        if (b->previous->end_page != b->end_page)
            break;
        if (b->previous->status == 1)
        {
            b = b->previous;
            b->size = b->size + align(sizeof(struct blk_meta)) + b->next->size;
            b->next = b->next->next;
            if (b->next != NULL)
                b->next->previous = b;
        }
        else
            break;
    }
    return b;
}

__attribute__((visibility("default"))) void free(void *ptr)
{
    if (head != NULL && ptr != NULL)
    {
        char *pt = ptr;
        struct blk_meta *b = head;
        while (b != NULL)
        {
            void *v = b;
            char *c = v;
            if (c + align(sizeof(struct blk_meta)) == pt)
            {
                b = aux_free(b);
                if (b->next == NULL)
                {
                    if (b->previous == NULL)
                    {
                        size_t s = align(sizeof(struct blk_meta));
                        int value = munmap(head, head->size + s);
                        if (value == 0)
                            head = NULL;
                        return;
                    }
                    else if (b->previous->end_page != b->end_page)
                    {
                        b->previous->next = NULL;
                        size_t s = align(sizeof(struct blk_meta));
                        munmap(b, b->size + s);
                        return;
                    }
                }
                return;
            }
            b = b->next;
        }
    }
}

__attribute__((visibility("default"))) void *realloc(void *ptr, size_t size)
{
    char *pt = ptr;
    size_t al = align(size);
    size_t s = al;
    s = s + align(sizeof(struct blk_meta));
    struct blk_meta *b = head;
    while (b != NULL)
    {
        void *v = b;
        char *c = v;
        if (c + align(sizeof(struct blk_meta)) == pt)
        {
            b->status = 1;
            void *res = check(b, s, al, NULL);
            if (res != NULL)
                return res;
            return aux_malloc(s, al, b);
        }
        b = b->next;
    }
    return malloc(size);
}

__attribute__((visibility("default"))) void *calloc(size_t nmemb, size_t size)
{
    size_t val = 1;
    if (__builtin_mul_overflow(nmemb, size, &val) != 0)
        return NULL;
    void *res = malloc(val);
    if (res == NULL)
        return NULL;
    memset(res, 0, val);
    return res;
}
