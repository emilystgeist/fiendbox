#!/bin/sh
# FIENDBOX: the package for the FM-1. SLICE (retired, hidden) is left out: 41 KB back.
# The tested SDK is AC79NN_SDK_V1.2.1_2023-12-13: AC79_SDK=~/fw-AC79-ref ./build_device.sh
export FELUCCA_SLICE=0
exec "$(dirname "$0")/build.sh" "$@"
