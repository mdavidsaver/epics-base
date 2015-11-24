#!/bin/sh
set -e -x

die() {
  echo "$1" >&2
  exit 1
}

EPICS_HOST_ARCH=`sh startup/EpicsHostArch`

[ -e configure/os/CONFIG_SITE.Common.linux-x86 ] || die "Wrong location: $PWD"

echo "Build for $CC"
case "$CC" in
clang)
  cat << EOF >> configure/os/CONFIG_SITE.Common.$EPICS_HOST_ARCH
GNU         = NO
CMPLR_CLASS = clang
CC          = clang
CCC         = clang++
EOF
  ;;
gcc) ;;
*) die "Unsupported $CC"
esac

case "$PROF" in
*-static-*)
  cat << EOF >> configure/CONFIG_SITE
SHARED_LIBRARIES=NO
STATIC_BUILD=YES
EOF
  ;;
*) ;;
esac

make

make runtests
