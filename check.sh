#!/bin/sh
# FIENDBOX: the full check. The device build (SLICE off) is what ships: its size checks and its disassembly (the
# target-cost test) come from it. The host tests then need the generated data with SLICE on (--gen-only).
AC79_SDK=~/fw-AC79-ref ./build_device.sh 2>&1 | grep -E "FAIL|package"
python3 tools/build.py --gen-only > /dev/null 2>&1
HOST_ONLY=1 tests/run_tests.sh 2>&1 | tee /tmp/check.log | tail -3
grep -n "FAIL\|FIND" /tmp/check.log | head
