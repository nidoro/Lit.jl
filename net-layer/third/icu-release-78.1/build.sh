#!/bin/bash

THIS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

DEST_DIR=$THIS_DIR/../../../local/build/icu-release-78.1
mkdir -p $DEST_DIR

./icu4c/source/configure --prefix=$DEST_DIR --enable-static CFLAGS="-fPIC" CXXFLAGS="-fPIC"
make -j8
make install
