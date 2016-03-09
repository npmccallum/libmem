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

#define __MEM_CLEANUP(f) __attribute__((cleanup(f)))
#define __MEM_PRINTF(a, b) __attribute__((format(__printf__, a, b)))

struct mem_link {
    struct mem_link *prev;
    struct mem_link *next;
};

struct mem_scope {
    struct mem_link list;
    struct mem_scope *next;
};

enum mem_flags {
  mem_flag_none = 0,
  mem_flag_secure = 1,
};

struct mem_scope
_mem_iscope(struct mem_scope *scope);

void
_mem_oscope(struct mem_scope *scope);


void *
mem_calloc(size_t count, size_t size);

void *
mem_malloc(size_t size);

void *
mem_realloc(void *ptr, size_t size);


size_t
mem_size(void *ptr);

void *
mem_steal(void *ptr, void *parent);

bool
mem_flags(void *ptr, enum mem_flags flags);


void *
mem_dup(const void *ptr, size_t size);

char *
mem_strdup(const char *str);

char *
mem_strndup(const char *str, size_t n);

char * __MEM_PRINTF(1, 0)
mem_vasprintf(const char *fmt, va_list ap);

char * __MEM_PRINTF(1, 2)
mem_asprintf(const char *fmt, ...);

#define mem_scope(name) \
    struct mem_scope __MEM_CLEANUP(_mem_oscope) name = _mem_iscope(&name)

#define mem_new(t) \
	((t *) mem_calloc(1, sizeof(t)))

