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

#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

struct mem_link {
    struct mem_link *prev;
    struct mem_link *next;
};

struct mem_scope {
    struct mem_link list;
    struct mem_scope *next;
};

struct mem_scope
_mem_iscope(struct mem_scope *scope);

void
_mem_oscope(struct mem_scope *scope);

/**
 * Allocates zeroed memory into the current scope
 *
 * This function has the same contract as calloc().
 *
 * @param count the number of elements
 * @param size the element size
 *
 * @return newly allocated memory or NULL on failure
 */
void *
mem_calloc(size_t count, size_t size);

/**
 * Allocates memory into the current scope
 *
 * This function has the same contract as malloc().
 *
 * @param size the allocation size
 *
 * @return newly allocated memory or NULL on failure
 */
void *
mem_malloc(size_t size);

/**
 * Resizes an existing allocation
 *
 * This function has the same contract as realloc().
 *
 * @param ptr the pointer to the memory to be reallocated
 * @param size the new allocation size
 *
 * @return newly allocated memory or NULL on failure
 */
void *
mem_realloc(void *ptr, size_t size);

/**
 * Returns the size of the allocation
 *
 * @param ptr the pointer to an existing allocation
 *
 * @return the size of the allocation
 */
size_t
mem_size(void *ptr);

/**
 * Assigns an allocation to a new context
 *
 * This function removes the allocation from its existing context
 * and assigns it as a child of the parent allocation. When the parent
 * memory is freed, this memory will be freed as well.
 *
 * This function always succeeds if ptr != NULL.
 *
 * @param ptr the pointer to an existing allocation
 * @param parent the pointer to a parent allocation
 *
 * @return the ptr value
 */
void *
mem_steal(void *ptr, void *parent);

/**
 * Secures memory for use with sensitive data
 *
 * This function MUST be called before copying sensitive data into an
 * allocation. Failure to do so can result in security compromises.
 *
 * If the securing operation fails, the ptr is freed.
 *
 * @return the ptr value or NULL on failure
 */
void *
mem_secure(void *ptr);

/**
 * Sets a function to be called before an allocation is freed
 *
 * This function always succeeds if ptr != NULL.
 *
 * @param ptr the pointer to an existing allocation
 * @param func the destructor function to call
 *
 * @return the ptr value
 */
void *
mem_destructor(void *ptr, void (*func)(void *));

/**
 * Duplicates the input data into a new allocation in the current context
 *
 *
 * @param ptr the pointer to the memory to be copied
 * @param size the size of the memory to copy
 *
 * @return newly allocated memory or NULL on failure
 */
void *
mem_dup(const void *ptr, size_t size);

/**
 * Duplicates a string into a new allocation in the current context
 *
 * This function has the same contract as strdup().
 *
 * @param str the string to copy
 *
 * @return newly allocated memory or NULL on failure
 */
char *
mem_strdup(const char *str);

/**
 * Duplicates a partial string into a new allocation in the current context
 *
 * This function has the same contract as strndup().
 *
 * @param str the string to copy
 * @param size the maximum length of the string to copy
 *
 * @return newly allocated memory or NULL on failure
 */
char *
mem_strndup(const char *str, size_t size);

/**
 * Allocates a formatted string into a new allocation in the current context
 *
 * This function has a similar contract to vasprintf().
 *
 * @param fmt the string format
 * @param ap the argument vector
 *
 * @return newly allocated memory or NULL on failure
 */
char * __attribute__((format(__printf__, 1, 0)))
mem_vasprintf(const char *fmt, va_list ap);

/**
 * Allocates a formatted string into a new allocation in the current context
 *
 * This function has a similar contract to asprintf().
 *
 * @param fmt the string format
 * @param ap the argument vector
 *
 * @return newly allocated memory or NULL on failure
 */
char * __attribute__((format(__printf__, 1, 2)))
mem_asprintf(const char *fmt, ...);

/**
 * Defines the current memory scope
 *
 * This macro should be used at the start of every function that allocates
 * memory. If an application fails to use mem_scope() completely, all
 * allocations will fail.
 *
 * @param name the name of the scope variable
 */
#define mem_scope(name) \
    struct mem_scope __attribute__((cleanup(_mem_oscope))) \
        name = _mem_iscope(&name)

/**
 * Allocates a new instance of the type
 *
 * @param type the type to allocate
 *
 * @return newly allocated memory or NULL on failure
 */
#define mem_new(type) \
	((type *) mem_calloc(1, sizeof(type)))

