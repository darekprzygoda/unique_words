#!/bin/bash

if [ $(which clang++) ]; then
    CXX_COMPILER=clang++
    BUILD_DIR=build
    CXX_STD=20
elif [ $(which clang++-10) ]; then
    CXX_COMPILER=clang++-10
    BUILD_DIR=build_old
    CXX_STD=20
else
    echo "Cannot find clang!"
    exit 1
fi

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
if [ ! -d Catch2 ]; then
    git clone https://github.com/catchorg/Catch2.git
fi

cmake -S Catch2 -B ${BUILD_DIR}/Catch2 -G Ninja -DCMAKE_CXX_COMPILER=${CXX_COMPILER} -DCMAKE_CXX_STANDARD=${CXX_STD} -DCMAKE_CXX_STANDARD_REQUIRED=ON
cmake --build ${BUILD_DIR}/Catch2
# cmake --install ${BUILD_DIR}/Catch2 --prefix ${SCRIPT_DIR}/${BUILD_DIR}/Catch2/deploy

CATCH2_DIR=${SCRIPT_DIR}/${BUILD_DIR}/Catch2/deploy

cmake -DCMAKE_BUILD_TYPE=DEBUG -S . -B ${BUILD_DIR}/debug -G Ninja -DCATCH2_DIR=${CATCH2_DIR} -DCMAKE_CXX_COMPILER=${CXX_COMPILER} -DCMAKE_CXX_STANDARD=${CXX_STD} -DCMAKE_CXX_STANDARD_REQUIRED=ON
cmake --build ${BUILD_DIR}//debug --verbose

cmake -DCMAKE_BUILD_TYPE=Release -S . -B ${BUILD_DIR}/release -G Ninja -DCATCH2_DIR=${CATCH2_DIR} -DCMAKE_CXX_COMPILER=${CXX_COMPILER} -DCMAKE_CXX_STANDARD=${CXX_STD} -DCMAKE_CXX_STANDARD_REQUIRED=ON
cmake --build ${BUILD_DIR}/release --verbose


