lib_LTLIBRARIES = libnullcode.la

# libnullcode params
libnullcode_la_SOURCES = null_code.c
libnullcode_la_CPPFLAGS = -I$(top_srcdir)/include/null_code @GCOV_FLAGS@

# Version format  (C - A).(A).(R) for C:R:A input
libnullcode_la_LDFLAGS = @GCOV_LDFLAGS@ -rpath '$(libdir)' -version-info 1:1:0

MOSTLYCLEANFILES = *.gcda *.gcno *.gcov

