noinst_HEADERS = builtin/xor_codes/test_xor_hd_code.h
noinst_PROGRAMS = test_xor_hd_code alg_sig_test liberasurecode_test libec_slap rs_galois_test liberasurecode_rs_vand_test

test_xor_hd_code_SOURCES = \
	builtin/xor_codes/test_xor_hd_code.c \
	builtin/xor_codes/test_xor_hd_code.h
test_xor_hd_code_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/include/erasurecode -I$(top_srcdir)/include/xor_codes  @GCOV_FLAGS@
test_xor_hd_code_LDFLAGS = @GCOV_LDFLAGS@ -static-libtool-libs $(top_builddir)/src/liberasurecode.la $(top_builddir)/src/builtin/xor_codes/libXorcode.la -ldl
check_PROGRAMS = test_xor_hd_code

alg_sig_test_SOURCES = utils/chksum/test_alg_sig.c
alg_sig_test_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/include/erasurecode -I$(top_srcdir)/include/xor_codes  @GCOV_FLAGS@
alg_sig_test_LDFLAGS = @GCOV_LDFLAGS@ -static-libtool-libs $(top_builddir)/src/liberasurecode.la -ldl
check_PROGRAMS += alg_sig_test

liberasurecode_test_SOURCES = liberasurecode_test.c
liberasurecode_test_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/include/erasurecode  @GCOV_FLAGS@
liberasurecode_test_LDFLAGS = @GCOV_LDFLAGS@ $(top_builddir)/src/liberasurecode.la -ldl -lpthread
check_PROGRAMS += liberasurecode_test

libec_slap_SOURCES = libec_slap.c
libec_slap_CPPFLAGS = -I. -I$(top_srcdir)/include -I$(top_srcdir)/include/erasurecode  @GCOV_FLAGS@
libec_slap_LDFLAGS = @GCOV_LDFLAGS@ $(top_builddir)/src/liberasurecode.la -ldl -lpthread
check_PROGRAMS += libec_slap

rs_galois_test_SOURCES = builtin/rs_vand/rs_galois_test.c
rs_galois_test_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/include/rs_vand  @GCOV_FLAGS@
rs_galois_test_LDFLAGS = @GCOV_LDFLAGS@ -static-libtool-libs $(top_builddir)/src/builtin/rs_vand/liberasurecode_rs_vand.la
check_PROGRAMS += rs_galois_test

liberasurecode_rs_vand_test_SOURCES = builtin/rs_vand/liberasurecode_rs_vand_test.c
liberasurecode_rs_vand_test_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/include/rs_vand  @GCOV_FLAGS@
liberasurecode_rs_vand_test_LDFLAGS = @GCOV_LDFLAGS@ -static-libtool-libs $(top_builddir)/src/builtin/rs_vand/liberasurecode_rs_vand.la
check_PROGRAMS += liberasurecode_rs_vand_test

MOSTLYCLEANFILES = *.gcda *.gcno *.gcov \
                   ./builtin/xor_codes/*.gcda ./builtin/xor_codes/*.gcno ./builtin/xor_codes/*.gcov \
                   ./utils/chksum/*.gcda ./utils/chksum/*.gcno ./utils/chksum/*.gcov
