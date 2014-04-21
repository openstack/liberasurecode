#!/bin/sh

# These directory stack functions are based upon the versions in the Korn
# Shell documentation - http://docstore.mik.ua/orelly/unix3/korn/ch04_07.htm.
dirs() {
  echo "$_DIRSTACK"
}
     
pushd() {
  dirname=$1
  cd ${dirname:?"missing directory name."} || return 1
  _DIRSTACK="$PWD $_DIRSTACK"
  echo "$_DIRSTACK"
}
		     
popd() {
  _DIRSTACK=${_DIRSTACK#* }
  top=${_DIRSTACK%% *}
  cd $top || return 1
  echo "$PWD"
}

download() {
  pkgurl="$1"

  WGET_PROG=`which wget`
  CURL_PROG=`which curl`

  if [ -z "${WGET_PROG}" ] && [ -z "${CURL_PROG}" ]; then
    echo "Please install wget or curl!!!"
    exit 2
  fi

  rm -f `basename ${pkgurl}`
  if [ -n ${WGET_PROG} ]; then
    ${WGET_PROG} ${pkgurl}
  else
    ${CURL_PROG} -O ${pkgurl}
  fi
}

realpath() {
  _dir="$1"
  case "$_dir" in
    /*)
      echo "$1"
      ;;
    *)
      echo "$PWD/${1#./}"
      ;;
  esac
}

# Determine install root
if [ "x$1" != "x" ]; then
  installroot="$1"
fi
installroot=$(realpath ${installroot})

# Checks
C_ECLIB_TOPDIR=${PWD}
TMP_BUILD_DIR=${C_ECLIB_TOPDIR}/tmp_build

OS_NAME=`uname`
SUPPORTED_OS=`echo "Darwin Linux" | grep ${OS_NAME}`

if [ -z "${SUPPORTED_OS}" ]; then
  echo "${OS_NAME} is not supported!!!"
  exit 2
fi

mkdir -p ${TMP_BUILD_DIR}
pushd ${TMP_BUILD_DIR}

# Install JErasure and GF-Complete
LIB_ORDER="gf_complete Jerasure"

for lib in ${LIB_ORDER}; do

  if [ ! -f ._${lib}_built ]; then
    (cd ${C_ECLIB_TOPDIR} && chmod 0755 build-c_eclib.sh && ./build-c_eclib.sh)
  fi

  srcdir=`cat ._${lib}_srcdir`
  # Install
  pushd ${srcdir}
  make DESTDIR=${installroot} install
  [ $? -ne 0 ] && popd && popd && exit 4
  popd
done

popd

# Build c_eclib
srcdir=${C_ECLIB_TOPDIR}
pushd ${srcdir}
make DESTDIR=${installroot} install
popd

