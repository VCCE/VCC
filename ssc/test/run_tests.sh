#!/usr/bin/env bash
# Builds and runs the standalone unit tests for the portable chip cores
# (no VCC or Windows needed -- these are plain C++17). Run from anywhere;
# the script cd's to its own directory first.
set -euo pipefail
cd "$(dirname "$0")"

CXX=${CXX:-g++}
FLAGS="-std=c++17 -Wall -Wextra -O2"

echo "== TMS7000 core =="
$CXX $FLAGS -o /tmp/ssc_test_cpu smoke_cpu.cpp ../tms7000.cpp
/tmp/ssc_test_cpu

echo
echo "== SP0256 speech core =="
$CXX $FLAGS -o /tmp/ssc_test_sp0256 smoke_sp0256.cpp ../sp0256.cpp
/tmp/ssc_test_sp0256

echo
echo "== AY-3-8913 PSG core =="
$CXX $FLAGS -o /tmp/ssc_test_ay8913 smoke_ay8913.cpp ../ay8913.cpp
/tmp/ssc_test_ay8913

echo
echo "== Full SSC glue (ports, timing, mixing) =="
$CXX $FLAGS -o /tmp/ssc_test_core smoke_ssc_core.cpp ../ssc_core.cpp ../tms7000.cpp ../sp0256.cpp ../ay8913.cpp
/tmp/ssc_test_core

echo
echo "All tests passed."
echo
echo "Optional: to also validate against the REAL S/SC ROM dumps (not"
echo "included here -- see README.md), build and run:"
echo "  g++ -std=c++17 -O2 -o /tmp/real_rom_test real_rom_test.cpp ../ssc_core.cpp ../tms7000.cpp ../sp0256.cpp ../ay8913.cpp"
echo "  /tmp/real_rom_test <path-to-pic-7040-510.bin> <path-to-sp0256-al2.bin>"
echo "  g++ -std=c++17 -O2 -o /tmp/real_sp0256_speech_test real_sp0256_speech_test.cpp ../sp0256.cpp"
echo "  /tmp/real_sp0256_speech_test <path-to-sp0256-al2.bin>"
echo "real_rom_capture.wav and real_sp0256_speech.wav in this folder are"
echo "captured output from prior runs of those two, kept as reference."
