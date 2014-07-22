# Top-level liberasurecode automake configuration
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src test
EXTRA_DIST = autogen.sh

INCLUDE = -I$(abs_top_builddir)/include \
		  -I$(abs_top_builddir)/include/erasurecode \
		  -I$(abs_top_builddir)/include/xor_codes

AM_CPPFLAGS = $(CPPFLAGS) $(INCLUDE)
AM_CPPFLAGS += -Werror

AM_CFLAGS = -fPIC $(AM_CPPFLAGS) -L/usr/local/lib

include_HEADERS = \
	include/erasurecode/alg_sig.h \
	include/erasurecode/erasurecode.h \
	include/erasurecode/erasurecode_backend.h \
	include/erasurecode/erasurecode_helpers.h \
	include/erasurecode/erasurecode_log.h \
	include/erasurecode/erasurecode_preprocessing.h \
	include/erasurecode/erasurecode_postprocessing.h \
	include/erasurecode/erasurecode_stdinc.h \
	include/erasurecode/erasurecode_version.h \
	include/erasurecode/list.h \
	include/xor_codes/xor_hd_code_defs.h \
	include/xor_codes/xor_code.h

test: check
#	@./test/alg_sig_test
	@./test/test_xor_hd_code
	@./test/liberasurecode_test

VALGRIND_EXEC_COMMAND = $(LIBTOOL_COMMAND) valgrind --tool=memcheck \
	--error-exitcode=1 --leak-check=yes --track-fds=yes \
	--malloc-fill=A5 --free-fill=DE --fullpath-after=.

valgrind-test: check
	@$(VALGRIND_EXEC_COMMAND) ./test/liberasurecode_test
	@$(VALGRIND_EXEC_COMMAND) ./test/test_xor_hd_code
#	@$(VALGRIND_EXEC_COMMAND) ./test/alg_sig_test

CLEANFILES = cscope.in.out cscope.out cscope.po.out

.PHONY: cscope
cscope:
	find src include -name "*.[ch]" > cscope.files
	cscope -q -b

