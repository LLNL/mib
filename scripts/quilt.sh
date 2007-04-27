#!/bin/sh
prog=quilt.sh

QUILTFLAGS="$@"

QUILT=$(which quilt)
LNDIR=$(which lndir)

TOP=$(pwd)
VANILLA_DIR="${TOP}/dbench"
PATCHED_DIR="${TOP}/dbench+chaos"
SERIES="${TOP}/patches/series"
PATCHES="${TOP}/patches"

die() {
    echo "${prog}: $1" >&2
    exit 1
}

warn() {
    echo "${prog}: $1" >&2
}

[ -n ${QUILT} ] || die "could not find \"quilt\" in your PATH"
[ -n ${LNDIR} ] || die "could not find \"lndir\" in your PATH"

mkdir ${PATCHED_DIR} || exit 1
pushd ${PATCHED_DIR} >/dev/null
    warn "creating link-copy of $(basename ${VANILLA_DIR})"
    ${LNDIR} -silent ${VANILLA_DIR}

    if [[ ! -e series && ! -e patches ]]; then
        ln -s ${SERIES} series
        ln -s ${PATCHES} patches
    fi

    warn "patching $(basename ${PATCHED_DIR})"

    # check for empty or missing patches, which should be an 
    # error here, but is not an error in quilt push.
    PATCH_LIST=$(sed -e '/^$/d' -e '/^#/d' -e 's/^[ '$'\t'']*//' \
               -e  's/[ '$'\t''].*//' series)
    for patch in $PATCH_LIST; do
	[ -s patches/$patch ] || die "patches/$patch does not exist, aborting"
    done

    quilt setup series # return value is 0|1 on success

    if [ -n "$PATCH_LIST" ]; then
        warn "running quilt push -a ${QUILTFLAGS}"
        quilt push -a ${QUILTFLAGS} || die "quilt push -a failed"
    fi
popd >/dev/null

touch .quilt
exit 0
