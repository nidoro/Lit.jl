#!/bin/bash

set -xe

export TARGET=x86_64-w64-mingw32
export CC=${TARGET}-gcc
export CXX=${TARGET}-g++

THIS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
DEST_DIR=$THIS_DIR/../../../build/win64/icu-release-78.1
NATIVE_BUILD=$THIS_DIR/../../../build/linux-x86_64/icu-release-78.1

pushd $THIS_DIR

make clean
mkdir -p $DEST_DIR

./icu4c/source/configure --with-cross-build="$NATIVE_BUILD" --host=$TARGET --build=x86_64-linux-gnu --prefix=$DEST_DIR --enable-static LDFLAGS="-lwinpthread" CFLAGS="-fPIC" CXXFLAGS="-fPIC"
make -j8
make install

popd
