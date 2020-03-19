#!/bin/sh

set -e
. .ci/travis.sh

if [ "$1" = "release-ready" ] || [ "$1" = "mingw" ] || [ "$1" = "coverity" ] || [ "$1" = "options-enabled" ] || [ "$1" = "options-disabled" ] ; then
  exit 0
fi

NUM_TRIES=5

if [ "$1" = "codecov" ] ; then
#  for tries in $(seq 1 ${NUM_TRIES}); do
#    meson test -t 120 -C build --wrapper dbus-launch && break
#    cat build/meson-logs/testlog-dbus-launch.txt
#    if [ $tries != ${NUM_TRIES} ] ; then echo "tests failed, trying again!" ; fi
#      false
#  done
  git -C /exactness-elm-data pull
  export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
  sudo ldconfig
  EINA_LOG_LEVELS_GLOB=eina_*:0,ecore*:0,efreet*:0,eldbus:0,elementary:0 exactness -j 20 -b exactness-elm-data/default-profile -p exactness-elm-data/default-profile/ci-integration-tests.txt
  curl -s https://codecov.io/bash | bash -s -
  exit 0
fi

travis_fold check "ninja test"
if [ "$DISTRO" != "" ] ; then
  for tries in $(seq 1 ${NUM_TRIES}); do
    if [ "$1" = "asan" ]; then
      (docker exec --env EINA_LOG_BACKTRACE="0" --env EIO_MONITOR_POLL=1 --env ASAN_OPTIONS=detect_leaks=0:abort_on_error=0 $(cat $HOME/cid) meson test -t 120 -C build --wrapper dbus-launch) && break
    else
      (docker exec --env EINA_LOG_BACKTRACE="0" --env EIO_MONITOR_POLL=1 $(cat $HOME/cid) meson test -t 120 -C build --wrapper dbus-launch ) && break
    fi
    docker exec --env EIO_MONITOR_POLL=1 $(cat $HOME/cid) cat build/meson-logs/testlog-dbus-launch.txt
    if [ $tries != ${NUM_TRIES} ] ; then echo "tests failed, trying again!" ; fi
      false
  done
fi
ret=$?
travis_endfold check

exit $ret
