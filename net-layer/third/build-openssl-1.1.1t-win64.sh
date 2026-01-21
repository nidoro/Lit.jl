#!/bin/bash

set -xe

export TARGET=x86_64-w64-mingw32
export CC=${TARGET}-gcc
export CXX=${TARGET}-g++
export WINDRES=${TARGET}-windres

THIS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
DEST_DIR=$THIS_DIR/../../build/win64/openssl-1.1.1t
pushd $THIS_DIR/openssl-1.1.1t

mkdir -p $DEST_DIR

#make clean

perl Configure mingw64 no-shared no-dso no-engine no-tests no-ssl3 no-comp --prefix=$DEST_DIR

make -j8
make install_sw

popd
