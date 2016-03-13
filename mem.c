/* vim: set tabstop=8 shiftwidth=4 softtabstop=4 expandtab smarttab colorcolumn=80: */
/*
 * Copyright (c) 2015 Red Hat, Inc.
 * Author: Nathaniel McCallum <npmccallum@redhat.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define _GNU_SOURCE
#include "mem.h"

#include <dlfcn.h>
#include <stdalign.h>
#include <sys/mman.h>
#include <string.h>

#define containerof(ptr, type, field) \
    ((type *) (((char *) ptr) - offsetof(type, field)))

struct flags {
    int secure : 1;
};

struct mem {
    size_t size;
    struct flags flags;
    struct mem_link list;
    struct mem_link link;
    void (*destructor)(void *);
    alignas(16) unsigned char body[];
};

static __thread struct mem_scope *stack = NULL;
static typeof(malloc) *sys_malloc = NULL;
static typeof(calloc) *sys_calloc = NULL;
static typeof(realloc) *sys_realloc = NULL;
static typeof(free) *sys_free = NULL;

static void __attribute__((constructor))
loadsyms(void)
{
    sys_malloc = dlsym(RTLD_NEXT, "malloc");
    sys_calloc = dlsym(RTLD_NEXT, "calloc");
    sys_realloc = dlsym(RTLD_NEXT, "realloc");
    sys_free = dlsym(RTLD_NEXT, "free");
    if (!sys_malloc || !sys_calloc || !sys_realloc || !sys_free)
        abort();
}

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
mem_init(struct mem *m, size_t size, struct mem_link *parent)
{
    m->size = size;
    m->destructor = NULL;
    m->flags = (struct flags) {};
    m->list.prev = &m->list;
    m->list.next = &m->list;
    m->link.prev = &m->link;
    m->link.next = &m->link;

    if (parent)
        link_push(parent, &m->link);
    else if (stack)
        link_push(&stack->list, &m->link);
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
        free(containerof(stack->list.next, struct mem, link)->body);

    stack = stack->next;
}

void *
malloc(size_t size)
{
    struct mem *m = NULL;

    m = sys_malloc(size + sizeof(struct mem));
    if (!m)
        return NULL;

    mem_init(m, size, NULL);
    return m->body;
}

void *
calloc(size_t count, size_t size)
{
    struct mem *m = NULL;

    m = sys_calloc(1, count * size + sizeof(struct mem));
    if (!m)
        return NULL;

    mem_init(m, size, NULL);
    return m->body;
}

void *
realloc(void *ptr, size_t size)
{
    struct mem *m = containerof(ptr, struct mem, body);

    if (ptr)
        return sys_realloc(m, size + sizeof(struct mem));

    return malloc(size);
}

void
free(void *ptr)
{
    struct mem *m = containerof(ptr, struct mem, body);

    if (!ptr)
        return;

    link_pop(&m->link);

    if (m->destructor)
        m->destructor(m->body);

    while (m->list.next != &m->list)
        free(containerof(m->list.next, struct mem, link)->body);

    if (m->flags.secure) {
        memset(m->body, 0, m->size);
        munlock(m->body, m->size);
    }

    sys_free(m);
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

void *
mem_secure(void *ptr)
{
    struct mem *m = containerof(ptr, struct mem, body);

    if (!m->flags.secure && mlock(m->body, m->size) != 0) {
        free(ptr);
        return NULL;
    }

    return ptr;
}

void *
mem_destructor(void *ptr, void (*func)(void *))
{
    struct mem *m = containerof(ptr, struct mem, body);

    if (m)
        m->destructor = func;

    return ptr;
}

void *
mem_dup(const void *ptr, size_t size)
{
    void *tmp = NULL;

    tmp = malloc(size);
    if (!tmp)
        return NULL;

    memcpy(tmp, ptr, size);
    return tmp;
}

