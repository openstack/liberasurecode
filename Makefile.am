# Top-level liberasurecode automake configuration
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src test doc

EXTRA_DIST = autogen.sh

INCLUDE = -I$(abs_top_builddir)/include \
		  -I$(abs_top_builddir)/include/erasurecode \
		  -I$(abs_top_builddir)/include/xor_codes

AM_CPPFLAGS = $(CPPFLAGS) $(INCLUDE)
AM_CPPFLAGS += -Werror

AM_CFLAGS = -fPIC $(AM_CPPFLAGS) @GCOV_FLAGS@ -L/usr/local/lib

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
	include/xor_codes/xor_code.h \
	include/config_liberasurecode.h

test: check
	$(eval SONAMES := $(shell find $(abs_top_builddir) -name '*.so'))
	$(eval SODIRS := $(dir $(SONAMES)))
	$(eval LD_LIBRARY_PATH := LD_LIBRARY_PATH="$(subst / ,/:,$(SODIRS))")
	$(eval DYLD_LIBRARY_PATH := DYLD_LIBRARY_PATH="$(subst / ,/:,$(SODIRS))")
	$(eval DYLD_FALLBACK_LIBRARY_PATH := DYLD_FALLBACK_LIBRARY_PATH="$(subst / ,/:,$(SODIRS))")
	@$(LD_LIBRARY_PATH) $(DYLD_LIBRARY_PATH) $(DYLD_FALLBACK_LIBRARY_PATH) \
		./test/alg_sig_test
	@$(LD_LIBRARY_PATH) $(DYLD_LIBRARY_PATH) $(DYLD_FALLBACK_LIBRARY_PATH) \
		./test/test_xor_hd_code
	@$(LD_LIBRARY_PATH) $(DYLD_LIBRARY_PATH) $(DYLD_FALLBACK_LIBRARY_PATH) \
		./test/liberasurecode_test
	@$(LD_LIBRARY_PATH) $(DYLD_LIBRARY_PATH) $(DYLD_FALLBACK_LIBRARY_PATH) \
		./test/libec_slap
 
VALGRIND_EXEC_COMMAND = $(LIBTOOL_COMMAND) valgrind --tool=memcheck \
	--error-exitcode=1 --leak-check=yes --track-fds=yes \
	--malloc-fill=A5 --free-fill=DE --fullpath-after=.

valgrind-test: check
	@$(VALGRIND_EXEC_COMMAND) ./test/alg_sig_test
	@$(VALGRIND_EXEC_COMMAND) ./test/liberasurecode_test
	@$(VALGRIND_EXEC_COMMAND) ./test/test_xor_hd_code
	@$(VALGRIND_EXEC_COMMAND) ./test/libec_slap

CLEANFILES = cscope.in.out cscope.out cscope.po.out

.PHONY: cscope
cscope:
	find src include -name "*.[ch]" > cscope.files
	cscope -q -b

MOSTLYCLEANFILES = *.gcda *.gcno *.gcov 
