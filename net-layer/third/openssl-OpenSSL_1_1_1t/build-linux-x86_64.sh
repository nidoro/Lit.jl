#!/bin/bash

THIS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

DEST_DIR=$THIS_DIR/../../../build/linux-x86_64/openssl-OpenSSL_1_1_1t
mkdir -p $DEST_DIR

./config --prefix=$DEST_DIR
make -j8
make install_sw
