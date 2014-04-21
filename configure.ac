# c_eclib autoconf template

# FIXME - add project url as the last argument
AC_INIT(c_eclib,0.9)

AC_PREREQ([2.61])

AM_INIT_AUTOMAKE(no-dependencies)
LT_INIT # libtool

AC_CONFIG_SRCDIR(xor_codes)
AC_CONFIG_SRCDIR(alg_sig)
AC_CONFIG_HEADER(include/config.h)

dnl Needed when reconfiguring with 'autoreconf -i -s'
AC_CONFIG_MACRO_DIR([m4])

AM_MAINTAINER_MODE([disable])

dnl Compiling with per-target flags requires AM_PROG_CC_C_O.
AC_PROG_CC

# Check for jerasure/gf_complete headers
AC_CHECK_HEADERS(jerasure.h cauchy.h galois.h liberation.h reed_sol.h gf_complete.h)

#AC_CHECK_LIB([Jerasure], [jerasure_autoconf_test], [],
#             [
#              echo "Error! You need to have libJerasure installed."
#              echo "libJerasure is available from bitbucket.org/kmgreen2/jerasure-kmg.git"
#              exit -1
#             ])
#
#AC_CHECK_LIB([gf_complete], [gf_init_easy], [],
#             [
#              echo "Error! You need to have gf_complete installed."
#              echo "gf_complete is available from http://web.eecs.utk.edu/~plank/plank/papers/CS-13-703.html"
#              exit -1
#             ])

AX_EXT()

AC_OUTPUT(Makefile xor_codes/Makefile alg_sig/Makefile)
