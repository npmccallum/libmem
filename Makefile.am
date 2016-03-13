AM_CFLAGS = \
	-Wall \
	-Wextra \
	-Werror \
	-Wstrict-aliasing \
	-Wchar-subscripts \
	-Wformat-security \
	-Wmissing-declarations \
	-Wmissing-prototypes \
	-Wnested-externs \
	-Wpointer-arith \
	-Wshadow \
	-Wsign-compare \
	-Wstrict-prototypes \
	-Wtype-limits \
	-Wno-missing-field-initializers \
	-Wno-unused-parameter

include_HEADERS = mem.h
lib_LTLIBRARIES = libmem.la
libmem_la_SOURCES = mem.c
libmem_la_LDFLAGS = -ldl

check_PROGRAMS = test
test_LDADD = libmem.la

TESTS = $(check_PROGRAMS)
