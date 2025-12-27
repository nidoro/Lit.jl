#!/bin/bash

set -xe

THIS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
pushd $THIS_DIR

mkdir -p ../local/build/artifacts-linux-x86_64

g++ \
    -g \
    -static-libstdc++ \
    -static-libgcc \
    -pthread \
    -Wno-unused-result \
    -shared \
    -fPIC \
    -o ../local/build/artifacts-linux-x86_64/liblit.so \
    \
        -I../local/build/openssl-OpenSSL_1_1_1t/include \
        -I../local/build/libwebsockets-4.3.2/include \
        -I../local/build/sqlite-amalgamation-3420000/include \
        -I../local/build/icu-release-78.1/include \
    \
        -L../local/build/openssl-OpenSSL_1_1_1t/lib \
        -L../local/build/libwebsockets-4.3.2/lib \
        -L../local/build/sqlite-amalgamation-3420000/lib \
        -L../local/build/icu-release-78.1/lib \
    \
    -Wl,--whole-archive \
        -l:libwebsockets.a \
        -l:libssl.a \
        -l:libcrypto.a \
    -Wl,--no-whole-archive \
    \
    -Wl,--start-group \
        -l:libSqliteIcu.a \
        -l:libicui18n.a \
        -l:libicuuc.a \
        -l:libicudata.a \
        -l:libicuio.a \
        -l:libsqlite3.a \
    -Wl,--end-group \
    \
    src/Lit.cpp

popd
