lib_LTLIBRARIES = liberasurecode_rsvand.la

# liberasurecode_rsvand params
liberasurecode_rsvand_la_SOURCES = rs_galois.c rs_vand_internal.c
liberasurecode_rsvand_la_CPPFLAGS = -I$(top_srcdir)/include/rs_vand @GCOV_FLAGS@

# Version format  (C - A).(A).(R) for C:R:A input
liberasurecode_rsvand_la_LDFLAGS = @GCOV_LDFLAGS@ -rpath '$(libdir)' -version-info 1:1:0

MOSTLYCLEANFILES = *.gcda *.gcno *.gcov
