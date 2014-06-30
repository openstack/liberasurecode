ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS=-I./ -I../include -I/usr/local/include/jerasure
if DEBUG
AM_CFLAGS = -g3 -O0
else
AM_CFLAGS = -O2
endif
AM_CFLAGS += $(SIMD_FLAGS) -fPIC $(AM_CPPFLAGS) -L/usr/local/lib

VALGRIND_EXEC_COMMAND = $(LIBTOOL_COMMAND) valgrind --tool=memcheck --error-exitcode=1 --leak-check=yes --track-fds=yes --malloc-fill=A5 --free-fill=DE --fullpath-after=.

include_HEADERS = \
	include/erasurecode/list.h \
	include/erasurecode/erasurecode_stdinc.h \
	include/erasurecode/erasurecode_internal.h \
	include/erasurecode/erasurecode.h \
	include/erasurecode/erasurecode_version.h \
	include/xor_codes/xor_hd_code_defs.h \
	include/xor_codes/xor_code.h \

lib_LTLIBRARIES = libXorcode_la liberasurecode.la

libXorcode_la_SOURCES = \
	src/builtin/xor_codes/xor_code.c \
	src/builtin/xor_codes/xor_hd_code.c \

liberasurecode_la_SOURCES = \
	src/main.c \
	src/backends/xor/flat_xor_3.c \
	src/utils/chksum/crc32.c \
	src/utils/chksum/alg_sig.c \

liberasurecode_la_LIBADD = libXorcode_la

noinst_HEADERS = tests/test_xor_hd_code.h
noinst_PROGRAMS = test_xor_hd_code alg_sig_test

test_xor_hd_code_SOURCES = \
	test/builtin/xor_codes/test_xor_hd_code.c \
	test/builtin/xor_codes/test_xor_hd_code.h \

test_xor_hd_code_LDFLAGS = -lerasurecode -lXorcode
check_PROGRAMS = test_xor_hd_code

alg_sig_test_SOURCES = test/utils/chksum/test_alg_sig.c
alg_sig_test_LDFLAGS = -lerasurecode -lXorcode
check_PROGRAMS += alg_sig_test

test: check
	@./alg_sig_test
	@./test_xor_hd_code

valgrind-test: check
	@$(VALGRIND_EXEC_COMMAND) ./alg_sig_test
	@$(VALGRIND_EXEC_COMMAND) ./test_xor_hd_code

