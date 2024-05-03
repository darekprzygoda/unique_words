#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
CATCH2_DIR=${SCRIPT_DIR}/../../Catch2/deploy

cmake -DCMAKE_BUILD_TYPE=DEBUG -S . -B build/debug -G Ninja -DCATCH2_DIR=${CATCH2_DIR}
cmake --build build/debug --verbose

cmake -DCMAKE_BUILD_TYPE=Release -S . -B build/release -G Ninja -DCATCH2_DIR=${CATCH2_DIR}
cmake --build build/release --verbose

