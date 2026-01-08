#!/bin/bash

set -xe

export TARGET=x86_64-w64-mingw32
export CC=${TARGET}-gcc
export CXX=${TARGET}-g++
export WINDRES=${TARGET}-windres

THIS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
pushd $THIS_DIR

DEST_DIR=$THIS_DIR/../../../build/win64/openssl-OpenSSL_1_1_1t
mkdir -p $DEST_DIR

make clean

#./config --prefix=$DEST_DIR --host=$TARGET --build=x86_64-linux-gnu --enable-static LDFLAGS="-lwinpthread" CFLAGS="-fPIC" CXXFLAGS="-fPIC"

perl Configure mingw64 --prefix=$DEST_DIR

make -j8
make install_sw

popd
