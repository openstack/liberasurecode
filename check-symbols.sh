#!/bin/bash
#/ Usage: check-symbols.sh {check,build}
#/ Commands:
#/
#/   build - Update the *.sym files at the root of this repository
#/           based on the *.so files that have been built.
#/   check - Compare the *.sym files against the symbols exposed in
#/           the *.so files that have been built.
#/
#/ By checking in the *.sym files and running `check-symbols.sh check`
#/ in the gate, we establish an auditable record of when symbols are
#/ added or removed from the various *.so files we produce.
set -e

get() {
    nm --dynamic --defined-only $1 | cut -c18- | LC_COLLATE=C sort
}

check() {
    if [ ! -e $2 ]; then
        touch $2
    fi
    diff -u $2 <( get $1 )
}

build() {
    get $1 > $2
}

case $1 in
    check | build)
        func=$1
        ;;
    *)
        grep '^#/' "$0" |cut -c4-
        exit 1
esac

rc=0
for lib in $(find . -name '*.so') ; do
    echo "${func}ing $( basename $lib )"
    if ! $func $lib $( basename ${lib/%.so/.sym} ) ; then
        rc=1
        echo
    fi
done

if [ $rc == 1 ]; then
    echo "Symbols don't match! Try running '$0 build' before committing."
fi
exit $rc
