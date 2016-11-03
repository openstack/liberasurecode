SUBDIRS = builtin/xor_codes builtin/null_code builtin/rs_vand

lib_LTLIBRARIES = liberasurecode.la

INCLUDES = \
		-I$(top_srcdir)/include/erasurecode \
		-I$(top_srcdir)/include/xor_codes \
		-I$(top_srcdir)/include/rs_vand \
		-I$(top_srcdir)/include/isa_l \
		-I$(top_srcdir)/include/shss

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
		backends/jerasure/jerasure_rs_cauchy.c \
		backends/isa-l/isa_l_common.c \
		backends/isa-l/isa_l_rs_vand.c \
		backends/isa-l/isa_l_rs_cauchy.c \
		backends/rs_vand/liberasurecode_rs_vand.c \
		builtin/rs_vand/rs_galois.c \
		backends/shss/shss.c

liberasurecode_la_CPPFLAGS = -Werror @GCOV_FLAGS@
liberasurecode_la_LIBADD = \
		builtin/null_code/libnullcode.la -lpthread -lm @GCOV_LDFLAGS@ \
		builtin/xor_codes/libXorcode.la -lpthread -lm @GCOV_LDFLAGS@ \
		builtin/rs_vand/liberasurecode_rs_vand.la -lpthread -lm @GCOV_LDFLAGS@

# Version format  (C - A).(A).(R) for C:R:A input
liberasurecode_la_LDFLAGS = -rpath '$(libdir)' -version-info @LIBERASURECODE_VERSION_INFO@

MOSTLYCLEANFILES = *.gcda *.gcno *.gcov utils/chksum/*.gcda utils/chksum/*.gcno utils/chksum/*.gcov \
                   backends/null/*.gcda backends/null/*.gcno backends/null/*.gcov  \
                   backends/xor/*.gcda backends/xor/*.gcno backends/xor/*.gcov  \
                   backends/jerasure/*.gcda backends/jerasure/*.gcno backends/jerasure/*.gcov \
                   backends/shss/*.gcda backends/shss/*.gcno backends/shss/*.gcov \
                   backends/rs_vand/*.gcda backends/rs_vand/*.gcno backends/rs_vand/*.gcov
