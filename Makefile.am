# Top-level liberasurecode automake configuration
ACLOCAL_AMFLAGS = -I m4

if DEBUG
AM_CFLAGS = -g3 -O0
else
AM_CFLAGS = -O2
endif

AM_CPPFLAGS = $(CPPFLAGS) -I$(abs_top_builddir)/include/xor_codes
AM_CPPFLAGS += -I$(abs_top_builddir)/include/erasurecode -Werror

AM_CFLAGS += -fPIC $(AM_CPPFLAGS) -L/usr/local/lib

include_HEADERS = \
	include/erasurecode/list.h \
	include/erasurecode/erasurecode_stdinc.h \
	include/erasurecode/erasurecode_internal.h \
	include/erasurecode/erasurecode.h \
	include/erasurecode/erasurecode_version.h \
	include/xor_codes/xor_hd_code_defs.h \
	include/xor_codes/xor_code.h

lib_LTLIBRARIES = libXorcode.la liberasurecode.la

# libXorcode params
libXorcode_la_SOURCES = \
	src/builtin/xor_codes/xor_code.c \
	src/builtin/xor_codes/xor_hd_code.c

# Version format  (C - A).(A).(R) for C:R:A input
libXorcode_la_LDFLAGS = -rpath '$(libdir)' -version-info 1:1:0

# liberasurecode params
liberasurecode_la_SOURCES = \
	src/main.c \
	src/backends/xor/flat_xor_3.c \
	src/utils/chksum/galois.c \
	src/utils/chksum/crc32.c \
	src/utils/chksum/alg_sig.c

liberasurecode_la_LIBADD = -lXorcode -lgf_complete
# Version format  (C - A).(A).(R) for C:R:A input
liberasurecode_la_LDFLAGS = -rpath '$(libdir)' -version-info 9:4:9


noinst_HEADERS = test/builtin/xor_codes/test_xor_hd_code.h
noinst_PROGRAMS = test_xor_hd_code alg_sig_test

test_xor_hd_code_SOURCES = \
	test/builtin/xor_codes/test_xor_hd_code.c \
	test/builtin/xor_codes/test_xor_hd_code.h

test_xor_hd_code_LDFLAGS = -lerasurecode -lXorcode -lgf_complete
check_PROGRAMS = test_xor_hd_code

alg_sig_test_SOURCES = test/utils/chksum/test_alg_sig.c
alg_sig_test_LDFLAGS = -lerasurecode -lXorcode -lgf_complete
check_PROGRAMS += alg_sig_test

test: check
	@./alg_sig_test
	@./test_xor_hd_code

VALGRIND_EXEC_COMMAND = $(LIBTOOL_COMMAND) valgrind --tool=memcheck \
	--error-exitcode=1 --leak-check=yes --track-fds=yes \
	--malloc-fill=A5 --free-fill=DE --fullpath-after=.

valgrind-test: check
	@$(VALGRIND_EXEC_COMMAND) ./alg_sig_test
	@$(VALGRIND_EXEC_COMMAND) ./test_xor_hd_code

CLEANFILES = cscope.in.out cscope.out cscope.po.out

.PHONY: cscope
cscope:
	cscope -b -q -R -Iinclude -ssrc;

