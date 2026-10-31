#!/bin/sh
# FIENDBOX: the full check: the default build (SLICE on) for the host tests, then the suite.
# The device package comes after, from ./build_device.sh (SLICE off).
AC79_SDK=~/fw-AC79-ref FELUCCA_SPARE_MIN=0 ./build.sh 2>&1 | grep -E "FAIL|package"
HOST_ONLY=1 tests/run_tests.sh 2>&1 | tee /tmp/check.log | tail -3
grep -n "FAIL\|FIND" /tmp/check.log | head
