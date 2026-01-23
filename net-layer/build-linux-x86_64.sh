#!/bin/bash

set -xe

THIS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
pushd $THIS_DIR

mkdir -p ../build/linux-x86_64/artifacts-linux-x86_64

g++ \
    -g \
    -static-libstdc++ \
    -static-libgcc \
    -pthread \
    -Wno-unused-result \
    -shared \
    -fPIC \
    -o ../build/linux-x86_64/artifacts-linux-x86_64/libmagic.so \
    \
        -I../build/linux-x86_64/openssl-1.1.1t/include \
        -I../build/linux-x86_64/libwebsockets-4.3.2/include \
        -I../build/linux-x86_64/sqlite-amalgamation-3420000/include \
        -I../build/linux-x86_64/icu-release-78.1/include \
    \
        -L../build/linux-x86_64/openssl-1.1.1t/lib \
        -L../build/linux-x86_64/libwebsockets-4.3.2/lib \
        -L../build/linux-x86_64/sqlite-amalgamation-3420000/lib \
        -L../build/linux-x86_64/icu-release-78.1/lib \
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
    src/Magic.cpp

popd
