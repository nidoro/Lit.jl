#!/bin/bash

THIS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

DEST_DIR=$THIS_DIR/../../../local/build/libwebsockets-4.3.2
mkdir -p $DEST_DIR

cd $DEST_DIR
rm -rf build
rm $THIS_DIR/CMakeCache.txt
mkdir build
cd build

cmake $THIS_DIR -DLWS_OPENSSL_INCLUDE_DIRS="../../openssl-OpenSSL_1_1_1t/include" \
  -DLWS_OPENSSL_LIBRARIES="../../openssl-OpenSSL_1_1_1t/lib/libssl.so;../../openssl-OpenSSL_1_1_1t/lib/libcrypto.so" \
  -DCMAKE_C_FLAGS="-fPIC" \
  -DCMAKE_CXX_FLAGS="-fPIC" \
  -DCMAKE_BUILD_TYPE=RELEASE \
  -DCMAKE_INSTALL_PREFIX=..
make -j8
make install
cd ..
