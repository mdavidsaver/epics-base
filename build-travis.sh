#!/bin/sh
set -e -x

die() {
  echo "$1" >&2
  exit 1
}

ticker() {
  while true
  do
    sleep 60
    date -R
    [ -r "$1" ] && tail -n10 "$1"
  done
}

CACHEKEY=1

EPICS_HOST_ARCH=`sh startup/EpicsHostArch`

[ -e configure/os/CONFIG_SITE.Common.linux-x86 ] || die "Wrong location: $PWD"

case "$CMPLR" in
clang)
  echo "Host compiler is clang"
  cat << EOF >> configure/os/CONFIG_SITE.Common.$EPICS_HOST_ARCH
GNU         = NO
CMPLR_CLASS = clang
CC          = clang
CCC         = clang++
EOF
  ;;
*) echo "Host compiler is default";;
esac

if [ "$STATIC" = "YES" ]
then
  echo "Build static libraries/executables"
  cat << EOF >> configure/CONFIG_SITE
SHARED_LIBRARIES=NO
STATIC_BUILD=YES
EOF
fi

# requires wine and g++-mingw-w64-i686
if [ "$WINE" = "32" ]
then
  echo "Cross mingw32"
  sed -i -e '/CMPLR_PREFIX/d' configure/os/CONFIG_SITE.linux-x86.win32-x86-mingw
  cat << EOF >> configure/os/CONFIG_SITE.linux-x86.win32-x86-mingw
CMPLR_PREFIX=i686-w64-mingw32-
EOF
  cat << EOF >> configure/CONFIG_SITE
CROSS_COMPILER_TARGET_ARCHS+=win32-x86-mingw
EOF
fi

# set RTEMS to eg. "4.9" or "4.10"
# requires qemu, bison, flex, texinfo, install-info
if [ -n "$RTEMS" ]
then
  echo "Cross RTEMS${RTEMS} for pc386"
  install -d /home/travis/.cache
  curl -L "https://github.com/mdavidsaver/rsb/releases/download/travis-20160306/rtems${RTEMS}-i386-precise-20160306.tar.bz2" \
  | tar -C /home/travis/.cache -xj

  sed -i -e '/^RTEMS_VERSION/d' -e '/^RTEMS_BASE/d' configure/os/CONFIG_SITE.Common.RTEMS
  cat << EOF >> configure/os/CONFIG_SITE.Common.RTEMS
RTEMS_VERSION=$RTEMS
RTEMS_BASE=/home/travis/.cache/rtems${RTEMS}-i386
EOF
  cat << EOF >> configure/CONFIG_SITE
CROSS_COMPILER_TARGET_ARCHS+=RTEMS-pc386
EOF
fi

if ! make
then
  cd /home/travis/build/mdavidsaver/epics-base/src/ca/legacy/gdd/O.RTEMS-pc386
  /home/travis/.cache/rtems4.10-i386/bin/i386-rtems4.10-g++ -B/home/travis/.cache/rtems4.10-i386/i386-rtems4.10/pc386/lib/ -specs bsp_specs -qrtems     -mtune=i386                 -DUNIX      -O2 -g -g   -Wall     -fno-strict-aliasing       -I. -I../O.Common -I. -I. -I.. -I../../../../../include/compiler/gcc -I../../../../../include/os/RTEMS -I../../../../../include         -S ../gdd.cc
  cat gdd.s
  exit 1
fi

[ "$TEST" != "NO" ] && make -s runtests
