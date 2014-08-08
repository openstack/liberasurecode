SUBDIRS = builtin/xor_codes builtin/null_code

lib_LTLIBRARIES = liberasurecode.la

INCLUDES = \
		-I$(top_srcdir)/include/erasurecode \
		-I$(top_srcdir)/include/xor_codes

# liberasurecode params
liberasurecode_la_SOURCES = \
		erasurecode.c \
		erasurecode_helpers.c \
		erasurecode_preprocessing.c \
		erasurecode_postprocessing.c \
		utils/chksum/crc32.c \
		utils/chksum/alg_sig.c \
		backends/null/null.c \
		backends/xor/flat_xor_hd.c \
		backends/jerasure/jerasure_rs_vand.c \
		backends/jerasure/jerasure_rs_cauchy.c

liberasurecode_la_CPPFLAGS = -Werror @GCOV_FLAGS@
liberasurecode_la_LIBADD = \
		builtin/xor_codes/libXorcode.la -lpthread -lm @GCOV_LDFLAGS@

# Version format  (C - A).(A).(R) for C:R:A input
liberasurecode_la_LDFLAGS = -rpath '$(libdir)' -version-info 9:10:9

MOSTLYCLEANFILES = *.gcda *.gcno *.gcov utils/chksum/*.gcda utils/chksum/*.gcno utils/chksum/*.gcov \
                   backends/null/*.gcda backends/null/*.gcno backends/null/*.gcov  \
                   backends/xor/*.gcda backends/xor/*.gcno backends/xor/*.gcov  \
                   backends/jerasure/*.gcda backends/jerasure/*.gcno backends/jerasure/*.gcov
