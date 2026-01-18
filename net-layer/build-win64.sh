#!/bin/bash

set -xe

THIS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
pushd $THIS_DIR

mkdir -p ../build/win64/artifacts-win64

x86_64-w64-mingw32-gcc \
    -g \
    -static-libstdc++ \
    -static-libgcc \
    -Wno-unused-result \
    -shared \
    -fPIC \
    -o ../build/win64/artifacts-win64/libmagic.dll \
    \
        -I../build/win64/openssl-OpenSSL_1_1_1t/include \
        -I../build/win64/libwebsockets-4.3.2/include \
        -I../build/win64/sqlite-amalgamation-3420000/include \
        -I../build/win64/icu-release-78.1/include \
    \
        -L../build/win64/openssl-OpenSSL_1_1_1t/lib \
        -L../build/win64/libwebsockets-4.3.2/lib \
        -L../build/win64/sqlite-amalgamation-3420000/lib \
        -L../build/win64/icu-release-78.1/lib \
    \
    -Wl,--whole-archive \
        -l:libwebsockets_static.a \
        -l:libssl.a \
        -l:libcrypto.a \
    -Wl,--no-whole-archive \
    \
    -Wl,--start-group \
        -l:libSqliteIcu.a \
        -l:libsicuin.a \
        -l:libsicuuc.a \
        -l:libsicuio.a \
        -l:libsicudt.a \
        -l:libsqlite3.a \
    -Wl,--end-group \
    \
    src/Magic.cpp \
    -Wl,-Bstatic -lstdc++ -lwinpthread \
    -Wl,-Bdynamic -lcrypt32 -lws2_32

popd
