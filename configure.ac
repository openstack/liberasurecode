################################################################################
#                           Standard Stuff
################################################################################

AC_PREREQ([2.61])
AC_INIT(liberasurecode, [-], 
        [tusharsg AT gmail DOT com, kmgreen2 AT gmail DOT com],
        [], [https://bitbucket.org/tsg-/liberasurecode/])
AM_MAINTAINER_MODE([disable])
m4_ifndef([AM_SILENT_RULES], [m4_define([AM_SILENT_RULES],[])])
AM_SILENT_RULES([yes])

# Get release version
AC_PATH_PROG(SED, sed, "", $PATH:/bin:/usr/bin:/usr/local/bin)
if test -z "$SED"; then
  AC_MSG_WARN([sed was not found])
fi
VERINC="${srcdir}/include/erasurecode/erasurecode_version.h"
AC_MSG_CHECKING([liberasurecode API version])
LIBERASURECODE_VERSION_MAJOR=`$SED -ne 's/^#define _MAJOR *\([0-9]*\)/\1/p' ${VERINC} 2>/dev/null`
AC_MSG_RESULT($LIBERASURECODE_VERSION_MAJOR)
if test -z "$LIBERASURECODE_VERSION_MAJOR"; then
  AC_MSG_ERROR([API version number can not be retrieved from $VERINC])
fi
LIBERASURECODE_VERSION_MINOR=`$SED -ne 's/^#define _MINOR *\([0-9]*\)/\1/p' ${VERINC} 2>/dev/null`
LIBERASURECODE_VERSION_MICRO=`$SED -ne 's/^#define _REV *\([0-9]*\)/\1/p' ${VERINC} 2>/dev/null`
LIBERASURECODE_VERSION=${LIBERASURECODE_VERSION_MAJOR}.${LIBERASURECODE_VERSION_MINOR}.${LIBERASURECODE_VERSION_MICRO}
LIBERASURECODE_API_VERSION=${LIBERASURECODE_VERSION_MAJOR}

LIBERASURECODE_VERSION_INFO=$((${LIBERASURECODE_VERSION_MAJOR}+${LIBERASURECODE_VERSION_MINOR})):${LIBERASURECODE_VERSION_MICRO}:${LIBERASURECODE_VERSION_MINOR}

AC_GNU_SOURCE

AM_INIT_AUTOMAKE([subdir-objects])
LT_INIT # libtool

AC_PROG_LN_S

dnl Compiling with per-target flags requires AM_PROG_CC_C_O.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_LIBTOOL
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_MAKE_SET

PKG_PROG_PKG_CONFIG

m4_ifndef([PKG_INSTALLDIR],[
# PKG_INSTALLDIR(DIRECTORY)
# -------------------------
# Substitutes the variable pkgconfigdir as the location where a module
# should install pkg-config .pc files. By default the directory is
# $libdir/pkgconfig, but the default can be changed by passing
# DIRECTORY. The user can override through the --with-pkgconfigdir
# parameter.
AC_DEFUN([PKG_INSTALLDIR],
[m4_pushdef([pkg_default], [m4_default([$1], ['${libdir}/pkgconfig'])])
m4_pushdef([pkg_description],
    [pkg-config installation directory @<:@]pkg_default[@:>@])
AC_ARG_WITH([pkgconfigdir],
    [AS_HELP_STRING([--with-pkgconfigdir], pkg_description)],,
    [with_pkgconfigdir=]pkg_default)
AC_SUBST([pkgconfigdir], [$with_pkgconfigdir])
m4_popdef([pkg_default])
m4_popdef([pkg_description])
]) dnl PKG_INSTALLDIR
])

PKG_INSTALLDIR

AC_SUBST(LIBERASURECODE_VERSION_MAJOR)
AC_SUBST(LIBERASURECODE_API_VERSION)
AC_SUBST(LIBERASURECODE_VERSION_MINOR)
AC_SUBST(LIBERASURECODE_VERSION_MICRO)
AC_SUBST(LIBERASURECODE_VERSION)
AC_SUBST(LIBERASURECODE_VERSION_INFO)
AC_SUBST([PACKAGE_VERSION], LIBERASURECODE_VERSION)

AC_CONFIG_HEADER(include/config_liberasurecode.h)

AX_EXT()

################################################################################
#                             System Headers
################################################################################
dnl Check for C library headers
AC_HEADER_STDC
AC_CHECK_HEADERS(sys/types.h stdio.h stdlib.h stddef.h stdarg.h \
                 malloc.h memory.h string.h strings.h inttypes.h \
                 stdint.h ctype.h math.h iconv.h signal.h dlfcn.h \
                 pthread.h unistd.h limits.h errno.h syslog.h)
AC_CHECK_FUNCS(malloc calloc realloc free openlog)

#################################################################################
#                             Debug/coverage Options
#################################################################################
AC_ARG_ENABLE([werror],
[  --disable-werror    Dont treat compilation warnings as failures],
[case "${enableval}" in
    yes) werror=true ;;
    no)  werror=false ;;
    *) AC_MSG_ERROR([bad value ${enableval} for --disable-werror]) ;;
esac],[werror=true])
if test x$werror = xtrue ; then
	werror_flag="-Werror"
else
	werror_flag=""
fi

AC_ARG_ENABLE([debug],
[  --enable-debug    Turn on debugging],
[case "${enableval}" in
    yes) debug=true ;;
    no)  debug=false ;;
    *) AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
esac],[debug=false])
if test x$debug = xtrue ; then
    DEBUG=1
    CXXFLAGS=""
    CFLAGS="-O0 -ggdb -g3 ${werror_flag} -D_GNU_SOURCE=1 -Wall -pedantic -std=c99 ${CFLAGS}"
else
    DEBUG=0
    CXXFLAGS=""
    CFLAGS="-O2 -g ${werror_flag} -D_GNU_SOURCE=1 -Wall -pedantic -std=c99 ${CFLAGS}"
fi

AC_ARG_ENABLE([gcov],
[  --enable-gcov    Turn on code coverage],
[case "${enableval}" in
    yes) gcov=true ;;
    no)  gcov=false ;;
    *) AC_MSG_ERROR([bad value ${enableval} for --enable-gcov]) ;;
esac],[gcov=false])
if test x$gcov = xtrue ; then
    AC_CHECK_LIB(gcov, main,
                 [
                     GCOV_FLAGS="-fprofile-arcs -ftest-coverage"
                     GCOV_LDFLAGS="-lgcov"
                     CXXFLAGS=""
                     CFLAGS=""
                 ], [
                     AC_MSG_ERROR([failed to locate gcov library])
                 ])
fi
AC_SUBST(GCOV_FLAGS)
AC_SUBST(GCOV_LDFLAGS)

dnl Expand the sources and objects needed to build the library
AC_SUBST(ac_aux_dir)
AC_SUBST(OBJECTS)

dnl Do CPUID and associated compiler flag checks

SUPPORTED_FLAGS=""
$CC - -E -mmmx </dev/null >/dev/null 2>&1
if [[ $? == "0" ]]; then
  SUPPORTED_FLAGS="-mmmx"
  AC_MSG_RESULT([$CC supports -mmmx])
fi
$CC - -E -msse </dev/null >/dev/null 2>&1
if [[ $? == "0" ]]; then
  SUPPORTED_FLAGS="$SUPPORTED_FLAGS -msse"
  AC_MSG_RESULT([$CC supports -msse])
fi
$CC - -E -msse2 </dev/null >/dev/null 2>&1
if [[ $? == "0" ]]; then
  SUPPORTED_FLAGS="$SUPPORTED_FLAGS -msse2"
  AC_MSG_RESULT([$CC supports -msse2])
fi
$CC - -E -msse3 </dev/null >/dev/null 2>&1
if [[ $? == "0" ]]; then
  SUPPORTED_FLAGS="$SUPPORTED_FLAGS -msse3"
  AC_MSG_RESULT([$CC supports -msse3])
fi
$CC - -E -mssse3 </dev/null >/dev/null 2>&1
if [[ $? == "0" ]]; then
  SUPPORTED_FLAGS="$SUPPORTED_FLAGS -mssse3"
  AC_MSG_RESULT([$CC supports -mssse3])
fi
$CC - -E -msse4.1 </dev/null >/dev/null 2>&1
if [[ $? == "0" ]]; then
  SUPPORTED_FLAGS="$SUPPORTED_FLAGS -msse4.1"
  AC_MSG_RESULT([$CC supports -msse4.1])
fi
$CC - -E -msse4.2 </dev/null >/dev/null 2>&1
if [[ $? == "0" ]]; then
  SUPPORTED_FLAGS="$SUPPORTED_FLAGS -msse4.2"
  AC_MSG_RESULT([$CC supports -msse4.2])
fi
$CC - -E -mavx </dev/null >/dev/null 2>&1
if [[ $? == "0" ]]; then
  SUPPORTED_FLAGS="$SUPPORTED_FLAGS -mavx"
  AC_MSG_RESULT([$CC supports -mavx])
fi

# Detect the SIMD features supported by both the compiler and the CPU
SIMD_FLAGS=""
cat "$srcdir/get_flags_from_cpuid.c" \
    | sed "s/FLAGSFROMAUTOCONF/${SUPPORTED_FLAGS}/" \
    | $CC -x c -g - -o get_flags_from_cpuid
if [[ -e ./get_flags_from_cpuid ]]; then
  chmod 755 get_flags_from_cpuid; ./get_flags_from_cpuid; rm ./get_flags_from_cpuid
  if [[ -e compiler_flags ]]; then
    SIMD_FLAGS=`cat compiler_flags`
    rm -f compiler_flags
  else
    AC_MSG_WARN([Could not run the CPUID detection program])
  fi
else
  AC_MSG_WARN([Could not compile the CPUID detection program])
fi

AC_MSG_RESULT([Generating with SIMD flags: $SIMD_FLAGS])
CFLAGS="$CFLAGS $SIMD_FLAGS"

# Certain code may be dependent on 32 vs. 64-bit arch, so add a 
# flag for 64-bit
AC_CHECK_SIZEOF([long])
if test "$ac_cv_sizeof_long" -eq 8; then
  CFLAGS="$CFLAGS -DARCH_64"
  AC_MSG_RESULT([Adding -DARCH_64 to CFLAGS])
fi


#################################################################################
#                             Doxygen Documentation
#################################################################################
AC_CHECK_PROG(DOXYGEN, doxygen, true, false)
AM_CONDITIONAL(HAVE_DOXYGEN, $DOXYGEN)
AC_SUBST(HAVE_DOXYGEN)

dnl Let people disable the doxygen stuff.
AC_ARG_ENABLE(doxygen, [  --enable-doxygen         Use doxygen to build documentation (default=auto)],
    enable_doxygen="$enableval",
    enable_doxygen=auto)

if test x$enable_doxygen = xauto ; then
    if test x$DOXYGEN = xtrue ; then
        enable_doxygen=yes
    else
        enable_doxygen=no
    fi
fi

dnl NOTE: We need to use a separate automake conditional for this
dnl       to make this work with the tarballs.
AM_CONDITIONAL(ENABLE_DOXYGEN, test x$enable_doxygen = xyes)

################################################################################
#                           Output Files
################################################################################
AC_CONFIG_FILES([\
    src/builtin/null_code/Makefile \
    src/builtin/xor_codes/Makefile \
    src/builtin/rs_vand/Makefile \
    src/Makefile \
    test/Makefile \
    doc/Makefile \
    Makefile \
    erasurecode.pc \
    Xorcode.pc
])

AC_CONFIG_FILES([\
    erasurecode-${LIBERASURECODE_API_VERSION}.pc:erasurecode.pc], [],
    [LIBERASURECODE_API_VERSION='$LIBERASURECODE_API_VERSION'])

AM_CONDITIONAL([HAVE_DOXYGEN], [test -n "$DOXYGEN"])
AM_COND_IF([HAVE_DOXYGEN],
           [AC_CONFIG_FILES([doc/doxygen.cfg])])

AC_OUTPUT

AC_MSG_NOTICE([

 $PACKAGE configured:
 --------------------------

  version:                $LIBERASURECODE_VERSION
  interface revision:     $LIBERASURECODE_VERSION_INFO
  generate documentation: $DOXYGEN
  installation prefix:    $prefix
  CFLAGS:                 $CXXFLAGS $CFLAGS
  LDFLAGS:                $LIBS $LDFLAGS

 type "make" followed by "make install" as root.
 $DOXMAKE
])
