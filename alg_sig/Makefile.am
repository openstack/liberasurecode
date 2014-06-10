# Algebraic signatures

AM_CPPFLAGS=-I./ -I../include -I/usr/local/include/jerasure
AM_CFLAGS = -O3 $(SIMD_FLAGS) -fPIC $(AM_CPPFLAGS) -L/usr/loca/lib

lib_LTLIBRARIES = libalg_sig.la
libalg_sig_la_SOURCES = alg_sig.c crc32.c
libalg_sig_la_LDFLAGS = -version-info 0:1:0
libalg_sig_la_ADDLIB = -lgf_complete

bin_PROGRAMS = alg_sig_test
alg_sig_test_SOURCES = alg_sig_test.c
alg_sig_test_LDFLAGS = -lalg_sig -lJerasure -lgf_complete

