#!/bin/bash

THIS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
DEST_DIR=$THIS_DIR/../../build/linux-x86_64/icu-release-78.1

pushd $THIS_DIR/icu-release-78.1

make clean
mkdir -p $DEST_DIR

./icu4c/source/configure \
    --prefix=$DEST_DIR \
    --enable-static \
    --disable-shared \
    --disable-tests \
    CFLAGS="-fPIC" \
    CXXFLAGS="-fPIC"

make -j8
make install

# These are needed for cross-compilation on windows
#----------------------------------------------------
cp -r ./config $DEST_DIR/
cp ./bin/gensprep $DEST_DIR/bin/
cp ./bin/icupkg $DEST_DIR/bin/

popd
