#!/bin/sh

# These directory stack functions are based upon the versions in the Korn
# Shell documentation - http://docstore.mik.ua/orelly/unix3/korn/ch04_07.htm.

exit_if_error() {
  if [ $? -ne 0 ]; then
    echo "ERROR in $0"
    exit
  fi
}

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

  WGET_PROG=`command -v wget`
  CURL_PROG=`command -v curl`

  if [ -z "${WGET_PROG}" ] && [ -z "${CURL_PROG}" ]; then
    echo "Please install wget or curl!!!"
    exit 2
  fi

  rm -f `basename ${pkgurl}`
  if [ "x" != "x${WGET_PROG}" ]; then
    ${WGET_PROG} ${pkgurl}
  else
    ${CURL_PROG} -O ${pkgurl}
  fi
  exit_if_error
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

find_includedir() {
  _searchdir="$1"
  _include="$2"
  i=`dirname $(find ${_searchdir} -type f -name "${_include}*.h" | grep include | sort -u | head -1)`
  echo $(realpath "$i")
}

find_libdir() {
  _searchdir="$1"
  _lib="$2"
  _osname="$3"
  
  if [ "x${_osname}" = "xDarwin" ]; then
    d=`dirname $(find ${_searchdir} -type f -name "lib${_lib}.*.dylib" | sort -u | head -1)`
  else
    d=`dirname $(find ${_searchdir} -type f -name "lib${_lib}.so.*" | sort -u | head -1)`
  fi
  echo $(realpath "$d")
}

# autoconf args passed down
configure_args=""
if [ "x$1" != "x" ]; then
  if [ "x$1" != "xac_none" ]; then
    configure_args="$1"
  fi
fi

# rpath argument
rpath=""
if [ "x$2" != "x" ]; then
  rpath="-Wl,-rpath,$2/usr/local/lib"
fi

# Checks
C_ECLIB_TOPDIR=${PWD}
TMP_BUILD_DIR=${C_ECLIB_TOPDIR}/tmp_build

OS_NAME=`uname`
SUPPORTED_OS=`echo "Darwin Linux" | grep ${OS_NAME}`

if [ -z "${SUPPORTED_OS}" ]; then
  echo "${OS_NAME} is not supported!!!"
  exit 2
fi

# Download sources for Jerasure and GF-complete
mkdir -p ${TMP_BUILD_DIR}
pushd ${TMP_BUILD_DIR}

gf_complete_SOURCE="http://www.kaymgee.com/Kevin_Greenan/Software_files/gf-complete.tar.gz"
Jerasure_SOURCE="http://www.kaymgee.com/Kevin_Greenan/Software_files/jerasure.tar.gz"

# Build JErasure and GF-Complete
LIB_ORDER="gf_complete Jerasure"
CPPFLAGS=""
LDFLAGS=" ${rpath} "
LIBS=""

for lib in ${LIB_ORDER}; do

  # Download and extract
  src="${lib}_SOURCE"
  url=$(eval echo \$${src})
  srcfile=`basename ${url}`

  if [ ! -f ._${lib}_downloaded ]; then
    download ${url}
    touch ._${lib}_downloaded
  fi
  srcdir=`pwd`/$(tar tf ${srcfile} | sed -e 's,/.*,,' | uniq)
  echo ${srcdir} > ._${lib}_srcdir

  # Extract and Build
  tar xf ${srcfile}
  pushd ${srcdir}
  if [ ! -f ._${lib}_configured ]; then
    chmod 0755 configure
    CPPFLAGS="${CPPFLAGS}" \
      LIBS=${LIBS} LDFLAGS=${LDFLAGS} \
      ./configure
    [ $? -ne 0 ] && popd && popd && exit 4
    touch ._${lib}_configured
  fi
  make
  [ $? -ne 0 ] && popd && popd && exit 5
  touch ._${lib}_built
  popd

  # Generate LDADD lines for c_eclib
  LIBDIR=$(find_libdir ${srcdir} ${lib} ${OS_NAME})
  LDFLAGS=" ${LDFLAGS} -L${LIBDIR} "
  LIBS=" ${LIBS} -l${lib} "

  if [ "x${OS_NAME}" = "xDarwin" ]; then
    if [ -z ${DYLD_LIBRARY_PATH} ]; then
      export DYLD_LIBRARY_PATH=${LIBDIR}
    else
      export DYLD_LIBRARY_PATH=${DYLD_LIBRARY_PATH}":"${LIBDIR}
    fi
  else
    if [ -z ${LD_LIBRARY_PATH} ]; then
      export LD_LIBRARY_PATH=${LIBDIR}
    else
      export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}":"${LIBDIR}
    fi
  fi

  # Generate INCLUDE lines for c_eclib
  INCLUDEDIR=$(find_includedir ${srcdir})
  CPPFLAGS=" ${CPPFLAGS} -I${INCLUDEDIR}"
done

popd

# Build c_eclib
srcdir=${C_ECLIB_TOPDIR}
pushd ${srcdir}
if [ ! -f ._configured ]; then
  chmod 0755 configure
  CPPFLAGS="${CPPFLAGS}" LIBS=${LIBS} LDFLAGS=${LDFLAGS} \
	  ./configure ${configure_args}
  [ $? -ne 0 ] && popd && exit 4
  touch ._configured
fi
make
[ $? -ne 0 ] && popd && exit 5

# Update CPPFLAGS/LDFLAGS/LIBS
C_ECLIB_LIBS="Xorcode alg_sig"
for lib in ${C_ECLIB_LIBS}; do
  LIBDIR=$(find_libdir ${srcdir} ${lib} ${OS_NAME})
  LDFLAGS=" ${LDFLAGS} -L${LIBDIR} "
  LIBS=" ${LIBS} -l${lib}"
done

INCLUDEDIR="${srcdir}/include"
CPPFLAGS=" ${CPPFLAGS} -I${INCLUDEDIR}"
popd

echo "LDFLAGS=${LDFLAGS}"
echo ${LDFLAGS} > ${C_ECLIB_TOPDIR}/._ldflags

echo "LIBS=${LIBS}"
echo ${LIBS} > ${C_ECLIB_TOPDIR}/._libs

echo "CPPFLAGS=${CPPFLAGS}"
echo ${CPPFLAGS} > ${C_ECLIB_TOPDIR}/._cppflags
