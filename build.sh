#!/bin/bash

cmake -DCMAKE_BUILD_TYPE=DEBUG -S . -B build/debug -G Ninja
cmake --build build/debug --verbose

cmake -DCMAKE_BUILD_TYPE=Release -S . -B build/release -G Ninja
cmake --build build/release --verbose

