# XOR-based erasure codes (including HD Combination Codes)

INCLUDES=-I./ -I../include
AM_CFLAGS = -O3 $(SIMD_FLAGS) -fPIC $(INCLUDES)

lib_LTLIBRARIES = libXorcode.la
libXorcode_la_SOURCES = xor_code.c xor_hd_code.c
libXorcode_la_LDFLAGS = -version-info 0:1:0

bin_PROGRAMS = test_xor_hd_code
test_xor_hd_code_SOURCES = test_xor_hd_code.c test_xor_hd_code.h
test_xor_hd_code_LDFLAGS = -lXorcode

EXTRA_DIST = tools
