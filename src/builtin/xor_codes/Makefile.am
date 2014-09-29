lib_LTLIBRARIES = libXorcode.la

# libXorcode params
libXorcode_la_SOURCES = xor_code.c xor_hd_code.c
libXorcode_la_CPPFLAGS = -I$(top_srcdir)/include/xor_codes $(SIMD_FLAGS) @GCOV_FLAGS@

# Version format  (C - A).(A).(R) for C:R:A input
libXorcode_la_LDFLAGS = @GCOV_LDFLAGS@ -rpath '$(libdir)' -version-info 1:1:0

MOSTLYCLEANFILES = *.gcda *.gcno *.gcov

