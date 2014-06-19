ACLOCAL_AMFLAGS = -I m4
noinst_PROGRAMS=test_xor_hd_code alg_sig_test
AM_CPPFLAGS=-I./ -I../include -I/usr/local/include/jerasure
if DEBUG
AM_CFLAGS = -g3 -O0
else
AM_CFLAGS = -O2
endif
AM_CFLAGS += $(SIMD_FLAGS) -fPIC $(AM_CPPFLAGS) -L/usr/local/lib
VALGRIND_EXEC_COMMAND = $(LIBTOOL_COMMAND) valgrind --tool=memcheck --error-exitcode=1 --leak-check=yes --track-fds=yes --malloc-fill=A5 --free-fill=DE --fullpath-after=.
include_HEADERS = include/xor_hd_code_defs.h include/xor_code.h include/alg_sig.h
lib_LTLIBRARIES =liberasurecode.la
liberasurecode_la_SOURCES =\
                            src/alg_sig.c \
                            src/crc32.c \
                            src/xor_code.c \
                            src/xor_hd_code.c
liberasurecode_la_ADDLIB = -lgf_complete -lJerasure

noinst_HEADERS = tests/test_xor_hd_code.h

test_xor_hd_code_SOURCES = tests/test_xor_hd_code.c tests/test_xor_hd_code.h
test_xor_hd_code_CPPFLAGS = -I./tests
test_xor_hd_code_LDFLAGS = -static -lerasurecode -lgf_complete
check_PROGRAMS = test_xor_hd_code

alg_sig_test_SOURCES = tests/alg_sig_test.c
alg_sig_test_CPPFLAGS = -I./tests
alg_sig_test_LDFLAGS = -static -lerasurecode -lgf_complete -lJerasure
check_PROGRAMS += alg_sig_test

test: check
	@./alg_sig_test
	@./test_xor_hd_code

valgrind-test: check
	@$(VALGRIND_EXEC_COMMAND) ./alg_sig_test
	@$(VALGRIND_EXEC_COMMAND) ./test_xor_hd_code
