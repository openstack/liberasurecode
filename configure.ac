AC_INIT(erasurecode,0.9.4)
AC_GNU_SOURCE

AC_PREREQ([2.61])

AM_INIT_AUTOMAKE([subdir-objects no-dependencies])
LT_INIT # libtool

AC_CONFIG_HEADER(include/config.h)

dnl Needed when reconfiguring with 'autoreconf -i -s'
AC_CONFIG_MACRO_DIR([m4])

AM_MAINTAINER_MODE([disable])

dnl Compiling with per-target flags requires AM_PROG_CC_C_O.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_LIBTOOL
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_MAKE_SET

dnl Check for C library headers
AC_HEADER_STDC
AC_CHECK_HEADERS(sys/types.h stdio.h stdlib.h stddef.h stdarg.h malloc.h memory.h \
                 string.h strings.h inttypes.h stdint.h ctype.h math.h iconv.h \
                 signal.h dlfcn.h pthread.h)
AC_CHECK_FUNCS(malloc calloc realloc free)

# Enable this check when gf_complete is external
AC_CHECK_LIB([gf_complete], [gf_init_easy], [],
[
    echo "Error! gf_complete is required for algorithmic signature support."
    echo "gf_complete is available from https://bitbucket.org/jimplank/gf-complete.git"
    echo "On Debian/Ubuntu systems, you can use \'apt-get install gf-complete\'"
    exit -1
])

# Check for jerasure/gf_complete headers
AC_CHECK_HEADERS(jerasure.h cauchy.h galois.h liberation.h reed_sol.h gf_complete.h)

AX_EXT()

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

AC_CONFIG_FILES([Makefile \
                 src/builtin/xor_codes/Makefile \
                 test/Makefile \
                 erasurecode.pc \
                 Xorcode.pc])
AC_OUTPUT

