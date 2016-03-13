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

static volatile bool called = false;

static void
destructor(void *ptr)
{
    called = true;
}

int
main(int argc, char *argv[])
{
    void *tmp = NULL;

    called = false;
    assert(tmp = mem_destructor(malloc(1), destructor));
    free(tmp);
    assert(called);

    called = false;
    assert(tmp = mem_destructor(calloc(1, 1), destructor));
    free(tmp);
    assert(called);

    called = false;
    assert(tmp = mem_destructor(realloc(NULL, 1), destructor));
    free(tmp);
    assert(called);

    mem_scope(scope);

    /* Test allocations. */
    assert(malloc(1));
    assert(calloc(1, 1));
    assert(realloc(NULL, 1));

    /* Test mem_size() */
    assert(mem_size(malloc(1)) == 1);
    assert(mem_size(malloc(11)) == 11);

    /* Test scoped destruction. */
    called = false;
    {
        mem_scope(scope0);
        assert(mem_destructor(malloc(1), destructor));
    }
    assert(called);

    /* Test mem_steal(). */
    called = false;
    assert(tmp = malloc(1));
    {
        mem_scope(scope0);
        assert(mem_steal(mem_destructor(malloc(1), destructor), tmp));
    }
    assert(!called);
    free(tmp);
    assert(called);

    /* Test mem_new(). */
    assert(mem_size(mem_new(int)) == sizeof(int));

    return 0;
}
