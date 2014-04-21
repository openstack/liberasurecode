#!/bin/sh

C_ECLIB_TOPDIR=${PWD}
TMP_BUILD_DIR=${C_ECLIB_TOPDIR}/tmp_build

# clean deps
rm -rf ${TMP_BUILD_DIR}
rm ._configured ._cppflags ._ldflags ._libs

# clean c_eclib
make distclean
