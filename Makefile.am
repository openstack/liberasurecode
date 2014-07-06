# Top-level liberasurecode automake configuration
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src/builtin/xor_codes test
EXTRA_DIST = autogen.sh

if DEBUG
AM_CFLAGS = -g3 -O0 -D_GNU_SOURCE=1
else
AM_CFLAGS = -O2 -D_GNU_SOURCE=1
endif

INCLUDE = -I$(abs_top_builddir)/include \
		  -I$(abs_top_builddir)/include/erasurecode \
		  -I$(abs_top_builddir)/include/xor_codes

AM_CPPFLAGS = $(CPPFLAGS) $(INCLUDE)
AM_CPPFLAGS += -Werror

AM_CFLAGS += -fPIC $(AM_CPPFLAGS) -L/usr/local/lib

include_HEADERS = \
	include/erasurecode/list.h \
	include/erasurecode/erasurecode_stdinc.h \
	include/erasurecode/erasurecode_internal.h \
	include/erasurecode/erasurecode.h \
	include/erasurecode/erasurecode_version.h \
	include/xor_codes/xor_hd_code_defs.h \
	include/xor_codes/xor_code.h

lib_LTLIBRARIES = liberasurecode.la

# liberasurecode params
liberasurecode_la_SOURCES = \
	src/main.c \
	src/backends/xor/flat_xor_3.c \
	src/utils/chksum/galois.c \
	src/utils/chksum/crc32.c \
	src/utils/chksum/alg_sig.c

liberasurecode_la_LIBADD = src/builtin/xor_codes/libXorcode.la -lgf_complete
# Version format  (C - A).(A).(R) for C:R:A input
liberasurecode_la_LDFLAGS = -rpath '$(libdir)' -version-info 9:4:9


test: check
	@./test/alg_sig_test
	@./test/test_xor_hd_code

VALGRIND_EXEC_COMMAND = $(LIBTOOL_COMMAND) valgrind --tool=memcheck \
	--error-exitcode=1 --leak-check=yes --track-fds=yes \
	--malloc-fill=A5 --free-fill=DE --fullpath-after=.

valgrind-test: check
	@$(VALGRIND_EXEC_COMMAND) ./test/alg_sig_test
	@$(VALGRIND_EXEC_COMMAND) ./test/test_xor_hd_code

CLEANFILES = cscope.in.out cscope.out cscope.po.out

.PHONY: cscope
cscope:
	cscope -b -q -R -Iinclude -ssrc;

