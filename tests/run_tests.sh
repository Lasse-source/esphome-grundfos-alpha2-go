#!/bin/sh
# Compile and run the host-side parser tests (needs g++).
set -e
cd "$(dirname "$0")"
g++ -std=c++17 -Wall -o test_parser test_parser.cpp
./test_parser sample_frames.txt
