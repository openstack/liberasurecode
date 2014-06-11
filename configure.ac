# c_eclib autoconf template

# FIXME - add project url as the last argument
AC_INIT(erasurecode,0.1)

AC_PREREQ([2.61])

AM_INIT_AUTOMAKE([subdir-objects no-dependencies])
LT_INIT # libtool

AC_CONFIG_SRCDIR(src)
AC_CONFIG_HEADER(include/config.h)

dnl Needed when reconfiguring with 'autoreconf -i -s'
AC_CONFIG_MACRO_DIR([m4])

AM_MAINTAINER_MODE([disable])

dnl Compiling with per-target flags requires AM_PROG_CC_C_O.
AC_PROG_CC

# Check for jerasure/gf_complete headers
AC_CHECK_HEADERS(jerasure.h cauchy.h galois.h liberation.h reed_sol.h gf_complete.h)

AC_CHECK_LIB([Jerasure], [jerasure_autoconf_test], [],
             [
              echo "Error! You need to have libJerasure installed."
              echo "libJerasure is available from bitbucket.org/kmgreen2/jerasure-kmg.git"
              exit -1
             ])

AC_CHECK_LIB([gf_complete], [gf_init_easy], [],
             [
              echo "Error! You need to have gf_complete installed."
              echo "gf_complete is available from http://web.eecs.utk.edu/~plank/plank/papers/CS-13-703.html"
              exit -1
             ])

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
AC_OUTPUT(Makefile)

