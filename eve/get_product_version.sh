#!/bin/bash

set -ue

READLINK=$(which greadlink 2>/dev/null >/dev/null && echo greadlink || echo readlink)

CURDIR=$(dirname $($READLINK -f $0))

VFILE=${CURDIR}/../include/erasurecode/erasurecode_version.h

VERSION=$(cat ${VFILE} |grep "define _MAJOR" | sed 's/.*_MAJOR //').$(cat ${VFILE} |grep "define _MINOR" | sed 's/.*_MINOR //').$(cat ${VFILE} |grep "define _REV" | sed 's/.*_REV //')

echo $VERSION
