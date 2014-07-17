AC_INIT(liberasurecode,0.9.10)
AC_GNU_SOURCE

AC_PREREQ([2.61])

AM_INIT_AUTOMAKE([subdir-objects])
LT_INIT # libtool

AC_CONFIG_HEADER(include/config.h)

dnl Needed when reconfiguring with 'autoreconf -i -s'
AC_CONFIG_MACRO_DIR([m4])

AM_MAINTAINER_MODE([disable])
m4_ifndef([AM_SILENT_RULES], [m4_define([AM_SILENT_RULES],[])])
AM_SILENT_RULES([yes])

dnl Compiling with per-target flags requires AM_PROG_CC_C_O.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_LIBTOOL
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_MAKE_SET

AX_EXT()

dnl Check for C library headers
AC_HEADER_STDC
AC_CHECK_HEADERS(sys/types.h stdio.h stdlib.h stddef.h stdarg.h malloc.h memory.h \
                 string.h strings.h inttypes.h stdint.h ctype.h math.h iconv.h \
                 signal.h dlfcn.h pthread.h unistd.h limits.h errno.h)
AC_CHECK_FUNCS(malloc calloc realloc free)

# Check for jerasure/gf_complete headers
AC_CHECK_HEADERS(jerasure.h cauchy.h galois.h liberation.h reed_sol.h gf_complete.h)
    
# Enable this check when gf_complete is external
AC_CHECK_LIB([gf_complete], [gf_init_easy], 
[
  AM_CONDITIONAL([GF_COMPLETE_INSTALLED], [true])
  AC_DEFINE([HAVE_LIBGF_COMPLETE], [1], ["Defined if gf-complete is installed"])
],
[
    echo "Warning gf_complete is required for *high performance* algorithmic signature support."
    echo "gf_complete is available from https://bitbucket.org/jimplank/gf-complete.git"
    echo "On Debian/Ubuntu systems, you can use \'apt-get install gf-complete\'"
    echo "This will install with a slower Galois field backend!"
    AM_CONDITIONAL([GF_COMPLETE_INSTALLED], [false])
])


AC_ARG_ENABLE(debug,
AS_HELP_STRING([--enable-debug],
               [enable debugging, default: no]),
[case "${enableval}" in
               yes) debug=true ;;
               no)  debug=false ;;
               *)   AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
esac],
[debug=false])
AM_CONDITIONAL(DEBUG, test x"$debug" = x"true")

dnl Expand the sources and objects needed to build the library
AC_SUBST(ac_aux_dir)
AC_SUBST(OBJECTS)

AC_CONFIG_FILES([\
    src/builtin/xor_codes/Makefile \
    src/Makefile \
    test/Makefile \
    Makefile \
    erasurecode.pc \
    Xorcode.pc
])

AC_OUTPUT

