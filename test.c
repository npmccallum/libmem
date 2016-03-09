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

#include "mem.h"
#include <assert.h>
#include <stdbool.h>
#include <string.h>

static bool called = false;

static void
destructor(void *ptr)
{
    called = true;
}

int
main(int argc, char *argv[])
{
    void *tmp = NULL;

    assert(!mem_malloc(1));
    assert(!mem_calloc(1, 1));
    assert(!mem_realloc(NULL, 1));
    assert(!mem_dup(&(char) {'c'}, 1));
    assert(!mem_strdup("c"));
    assert(!mem_strndup("abc", 1));
    assert(!mem_asprintf("%s", "foo"));

    mem_scope(scope);

    /* Test allocations. */
    assert(mem_malloc(1));
    assert(mem_calloc(1, 1));
    assert(mem_realloc(NULL, 1));
    assert(mem_dup(&(char) {'c'}, 1));
    assert(mem_strdup("c"));
    assert(mem_strndup("abc", 1));
    assert(mem_asprintf("%s", "foo"));

    /* Test mem_size() */
    assert(mem_size(mem_malloc(1)) == 1);
    assert(mem_size(mem_malloc(11)) == 11);

    /* Test mem_destructor(). */
    {
        mem_scope(scope0);
        called = false;
        assert(mem_destructor(mem_malloc(1), destructor));
    }
    assert(called);

    /* Test mem_free(). */
    called = false;
    mem_free(mem_destructor(mem_malloc(1), destructor));
    assert(called);

    /* Test mem_steal(). */
    assert(tmp = mem_malloc(1));
    {
        mem_scope(scope0);
        called = false;
        assert(mem_steal(mem_destructor(mem_malloc(1), destructor), tmp));
    }
    assert(!called);
    mem_free(tmp);
    assert(called);

    /* Test mem_strdup() and mem_strndup(). */
    assert(strcmp("foo", mem_strdup("foo")) == 0);
    assert(strcmp("foo", mem_strndup("foo", 2)) != 0);
    assert(strcmp("fo", mem_strndup("foo", 2)) == 0);

    /* Test mem_asprintf() (and implicitly mem_vasprintf()). */
    assert(strcmp("foo", mem_asprintf("%s", "foo")) == 0);

    /* Test mem_new(). */
    assert(mem_size(mem_new(int)) == sizeof(int));

    return 0;
}
