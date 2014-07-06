test_xor_hd_code_SOURCES = \
	builtin/xor_codes/test_xor_hd_code.c \
	builtin/xor_codes/test_xor_hd_code.h
test_xor_hd_code_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/include/erasurecode -I$(top_srcdir)/include/xor_codes
test_xor_hd_code_LDFLAGS = -lerasurecode -lgf_complete $(top_srcdir)/src/builtin/xor_codes/libXorcode.la
check_PROGRAMS = test_xor_hd_code

alg_sig_test_SOURCES = utils/chksum/test_alg_sig.c
alg_sig_test_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/include/erasurecode -I$(top_srcdir)/include/xor_codes
alg_sig_test_LDFLAGS = -lerasurecode -lgf_complete $(top_srcdir)/src/builtin/xor_codes/libXorcode.la
check_PROGRAMS += alg_sig_test

noinst_HEADERS = builtin/xor_codes/test_xor_hd_code.h
noinst_PROGRAMS = test_xor_hd_code alg_sig_test


