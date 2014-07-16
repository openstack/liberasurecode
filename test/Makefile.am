noinst_HEADERS = builtin/xor_codes/test_xor_hd_code.h
noinst_PROGRAMS = test_xor_hd_code alg_sig_test liberasurecode_test

test_xor_hd_code_SOURCES = \
	builtin/xor_codes/test_xor_hd_code.c \
	builtin/xor_codes/test_xor_hd_code.h
test_xor_hd_code_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/include/erasurecode -I$(top_srcdir)/include/xor_codes
test_xor_hd_code_LDFLAGS = -static-libtool-libs $(top_srcdir)/src/liberasurecode.la $(top_srcdir)/src/builtin/xor_codes/libXorcode.la
check_PROGRAMS = test_xor_hd_code

alg_sig_test_SOURCES = utils/chksum/test_alg_sig.c
alg_sig_test_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/include/erasurecode -I$(top_srcdir)/include/xor_codes
alg_sig_test_LDFLAGS = -static-libtool-libs $(top_srcdir)/src/liberasurecode.la -ldl
check_PROGRAMS += alg_sig_test

liberasurecode_test_SOURCES = liberasurecode_test.c
liberasurecode_test_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/include/erasurecode
liberasurecode_test_LDFLAGS = $(top_srcdir)/src/liberasurecode.la -ldl -lpthread
check_PROGRAMS += liberasurecode_test

if GF_COMPLETE_INSTALLED
  alg_sig_test_LDFLAGS += -lgf_complete
  liberasurecode_test_LDFLAGS += -lgf_complete 
  test_xor_hd_code_LDFLAGS += -lgf_complete
endif

