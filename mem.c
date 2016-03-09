#include "mem.h"
#undef mem_scope

#include <stdalign.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <stdio.h>

#define containerof(ptr, type, field) \
    (type *) (((char *) ptr) - offsetof(type, field))

struct mem {
    size_t size;
    enum mem_flags flags;
    struct mem_link list;
    struct mem_link link;
    alignas(16) unsigned char body[];
};

static __thread struct mem_scope *stack = NULL;

static void
link_push(struct mem_link *list, struct mem_link *item)
{
    item->next = list->next;
    item->prev = list;
    list->next->prev = item;
    list->next = item;
}

static void
link_pop(struct mem_link *item)
{
    item->prev->next = item->next;
    item->next->prev = item->prev;
}

static void
mem_free(struct mem *m)
{
    link_pop(&m->link);

    while (m->list.next != &m->list)
        mem_free(containerof(m->list.next, struct mem, link));

    if (m->flags & mem_flag_secure) {
        memset(m->body, 0, m->size);
        munlock(m, m->size + sizeof(struct mem));
    }

    free(m);
}

struct mem_scope
_mem_iscope(struct mem_scope *scope)
{
    if (!scope)
        return (struct mem_scope) {};

    scope->list.prev = &scope->list;
    scope->list.next = &scope->list;
    scope->next = stack;
    stack = scope;
    
    return *scope;
}

void
_mem_oscope(struct mem_scope *scope)
{
    if (scope != stack || !stack)
        return;

    while (stack->list.next != &stack->list)
        mem_free(containerof(stack->list.next, struct mem, link));

    stack = stack->next;
}

void *
mem_calloc(size_t count, size_t size)
{
    struct mem *m = NULL;

    if (!stack)
        return NULL;

    m = calloc(1, count * size + sizeof(struct mem));
    if (!m)
        return NULL;

    m->size = count * size;
    m->flags = mem_flag_none;
    link_push(&stack->list, &m->link);

    return &m->body;
}

void *
mem_malloc(size_t size)
{
    struct mem *m = NULL;

    if (!stack)
        return NULL;

    m = malloc(size + sizeof(struct mem));
    if (!m)
        return NULL;

    m->size = size;
    m->flags = mem_flag_none;
    link_push(&stack->list, &m->link);

    return &m->body;
}


void *
mem_realloc(void *ptr, size_t size)
{
    struct mem *m = containerof(ptr, struct mem, body);

    if (ptr)
        return realloc(m, size + sizeof(struct mem));

    return mem_malloc(size);
}

size_t
mem_size(void *ptr)
{
    struct mem *m = containerof(ptr, struct mem, body);
    return ptr ? m->size : 0;
}

void *
mem_steal(void *ptr, void *parent)
{
    struct mem *mparent = containerof(parent, struct mem, body);
    struct mem *mptr = containerof(ptr, struct mem, body);

    if (!ptr || !parent)
        return NULL;

    link_pop(&mptr->link);
    link_push(&mparent->list, &mptr->link);
    return ptr;
}

bool
mem_flags(void *ptr, enum mem_flags flags)
{
    struct mem *m = containerof(ptr, struct mem, body);

    if (flags & mem_flag_secure) {
        if (m->flags & mem_flag_secure)
            return true;

        if (mlock(m, m->size + sizeof(struct mem)) != 0)
            return false;

        m->flags |= mem_flag_secure;
    }

    return true;
}

void *
mem_dup(const void *ptr, size_t size)
{
    void *tmp = NULL;

    tmp = mem_malloc(size);
    if (!tmp)
        return NULL;

    memcpy(tmp, ptr, size);
    return tmp;
}

char *
mem_strdup(const char *str)
{
    char *tmp = NULL;

    if (!str)
        return NULL;

    tmp = mem_malloc(strlen(str) + 1);
    if (!tmp)
        return NULL;

    strcpy(tmp, str);
    return tmp;
}

char *
mem_strndup(const char *str, size_t n)
{
    char *tmp = NULL;

    if (!str)
        return NULL;

    tmp = mem_malloc(n + 1);
    if (!tmp)
        return NULL;

    strncpy(tmp, str, n);
    tmp[n] = '\0';
    return tmp;
}

char * __attribute__((format(__printf__, 1, 0)))
mem_vasprintf(const char *fmt, va_list ap)
{
    char *tmp = NULL;
    va_list copy;
    int len = 0;

    va_copy(copy, ap);
    len = vsnprintf(NULL, 0, fmt, copy);
    va_end(copy);
    if (len < 0)
        return NULL;

    tmp = mem_malloc(len + 1);
    if (!tmp)
        return NULL;

    len = vsnprintf(tmp, len + 1, fmt, ap);
    if (len < 0) {
        mem_free(containerof(tmp, struct mem, body));
        return NULL;
    }

    return tmp;
}

char * __attribute__((format(__printf__, 1, 2)))
mem_asprintf(const char *fmt, ...)
{
    char *tmp = NULL;
    va_list ap;

    va_start(ap, fmt);
    tmp = mem_vasprintf(fmt, ap);
    va_end(ap);

    return tmp;
}

