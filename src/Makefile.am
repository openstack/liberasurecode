SUBDIRS = builtin/xor_codes

lib_LTLIBRARIES = liberasurecode.la

INCLUDES = \
		-I$(top_srcdir)/include/erasurecode \
		-I$(top_srcdir)/include/xor_codes

# liberasurecode params
liberasurecode_la_SOURCES = \
		erasurecode.c \
		utils/chksum/crc32.c \
		utils/chksum/alg_sig.c \
		utils/chksum/galois.c \
		backends/xor/flat_xor_3.c

liberasurecode_la_CPPFLAGS = -Werror
liberasurecode_la_LIBADD = \
		builtin/xor_codes/libXorcode.la -lpthread

if GF_COMPLETE_INSTALLED
  liberasurecode_la_LIBADD += -lgf_complete
endif

# Version format  (C - A).(A).(R) for C:R:A input
liberasurecode_la_LDFLAGS = -rpath '$(libdir)' -version-info 9:10:9

