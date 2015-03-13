################################################################################
#                           Standard Stuff
################################################################################
AC_INIT(liberasurecode,1.0.5)
AC_GNU_SOURCE

AC_PREREQ([2.61])

AM_INIT_AUTOMAKE([subdir-objects])
LT_INIT # libtool

AC_CONFIG_HEADER(include/config_liberasurecode.h)

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
    CFLAGS="-O0 -ggdb -g3 ${werror_flag} -D_GNU_SOURCE=1 -Wall -pedantic -std=c99"
else
    DEBUG=0
    CXXFLAGS=""
    CFLAGS="-O2 -g ${werror_flag} -D_GNU_SOURCE=1 -Wall -pedantic -std=c99"
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
    src/Makefile \
    test/Makefile \
    doc/Makefile \
    Makefile \
    erasurecode.pc \
    Xorcode.pc
])

AM_CONDITIONAL([HAVE_DOXYGEN], [test -n "$DOXYGEN"])
AM_COND_IF([HAVE_DOXYGEN],
           [AC_CONFIG_FILES([doc/doxygen.cfg])])

AC_OUTPUT

