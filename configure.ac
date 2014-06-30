# c_eclib autoconf template

# FIXME - add project url as the last argument
AC_INIT(erasurecode,0.9.4)
AC_GNU_SOURCE

AC_PREREQ([2.61])

AM_INIT_AUTOMAKE([subdir-objects no-dependencies])
LT_INIT # libtool

AC_CONFIG_HEADER(include/config.h)

# Making releases:
# Edit include/erasurecode_version.h and change the version, then:
#   ERASURECODE_MICRO_VERSION += 1;
# if any functions have been added, set ERASURECODE_INTERFACE_AGE to 0.
# if backwards compatibility has been broken,
# set ERASURECODE_BINARY_AGE and ERASURECODE_INTERFACE_AGE to 0.
#
ERASURECODE_MAJOR_VERSION=0
ERASURECODE_MINOR_VERSION=9
ERASURECODE_MICRO_VERSION=4
ERASURECODE_VERSION=$ERASURECODE_MAJOR_VERSION.$ERASURECODE_MINOR_VERSION.$ERASURECODE_MICRO_VERSION

AC_SUBST(ERASURECODE_MAJOR_VERSION)
AC_SUBST(ERASURECODE_MINOR_VERSION)
AC_SUBST(ERASURECODE_MICRO_VERSION)
AC_SUBST(ERASURECODE_VERSION)

# libtool versioning
LT_RELEASE=$ERASURECODE_MAJOR_VERSION.$ERASURECODE_MINOR_VERSION
m4_pattern_allow([^LT_])

AC_SUBST(LT_RELEASE)
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)

# Xorcode version
XORCODE_MAJOR_VERSION=0
XORCODE_MINOR_VERSION=9
XORCODE_MICRO_VERSION=4
XORCODE_VERSION=$XORCODE_MAJOR_VERSION.$XORCODE_MINOR_VERSION.$XORCODE_MICRO_VERSION

AC_SUBST(XORCODE_MAJOR_VERSION)
AC_SUBST(XORCODE_MINOR_VERSION)
AC_SUBST(XORCODE_MICRO_VERSION)
AC_SUBST(XORCODE_VERSION)


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

dnl Set up compile and link options
INCLUDE="-I$srcdir/include -I$srcdir/include/erasurecode -I$srcdir/xor_codes"
BASE_CFLAGS="-D_GNU_SOURCE=1"
BASE_LDFLAGS=""
BUILD_CFLAGS="$CFLAGS $CPPFLAGS"
EXTRA_CFLAGS="$INCLUDE $BASE_CFLAGS"
BUILD_LDFLAGS="$LDFLAGS"
EXTRA_LDFLAGS="$BASE_LDFLAGS"

dnl Check for C library headers
AC_HEADER_STDC
AC_CHECK_HEADERS(sys/types.h stdio.h stdlib.h stddef.h stdarg.h \
                 malloc.h memory.h string.h strings.h inttypes.h \
                 stdint.h ctype.h math.h iconv.h signal.h dlfcn.h)
AC_CHECK_FUNCS(malloc calloc realloc free)

# Enable this check when gf_complete is external
AC_CHECK_LIB([gf_complete], [gf_init_easy], [],
[
    echo "Error! You need to have gf_complete installed."
    echo "gf_complete is available from https://bitbucket.org/jimplank/gf-complete.git"
    exit -1
])


# C sources
SOURCES="$SOURCES $srcdir/src/*.c"
SOURCES="$SOURCES $srcdir/src/backends/null/*.c"
SOURCES="$SOURCES $srcdir/src/backends/jerasure/*.c"
SOURCES="$SOURCES $srcdir/src/backends/gf_complete/*.c"
SOURCES="$SOURCES $srcdir/src/backends/isa_l/*.c"
SOURCES="$SOURCES $srcdir/src/backends/xor/*.c"
SOURCES="$SOURCES $srcdir/src/builtins/xor/*.c"
SOURCES="$SOURCES $srcdir/src/utils/chksum/*.c"
SOURCES="$SOURCES $srcdir/test/*.c"

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
AC_SUBST(INCLUDE)
AC_SUBST(SOURCES)
AC_SUBST(OBJECTS)
AC_SUBST(BUILD_CFLAGS)
AC_SUBST(EXTRA_CFLAGS)
AC_SUBST(BUILD_LDFLAGS)
AC_SUBST(EXTRA_LDFLAGS)

AC_CONFIG_FILES([Makefile \
                 src/utils/chksum/Makefile \
                 erasurecode.pc \
                 Xorcode.pc])
AC_OUTPUT

