#!/bin/bash

CFLAGS="-g -static-libstdc++ -static-libgcc -Wno-unused-result -fPIC -shared -o ../local/build/lit-net-layer.so"
LDFLAGS="-pthread"
LIBS="-l:libwebsockets.a -l:libssl.a -l:libcrypto.a -l:libSqliteIcu.a -l:libicui18n.a -l:libicuuc.a -l:libicudata.a -l:libicuio.a -l:libsqlite3.a"

g++ \
    $CFLAGS \
    $LDFLAGS \
    -I../local/build/openssl-OpenSSL_1_1_1t/include \
    -I../local/build/libwebsockets-4.3.2/include \
    -I../local/build/sqlite-amalgamation-3420000/include \
    -I../local/build/icu-release-78.1/include \
    -L../local/build/openssl-OpenSSL_1_1_1t/lib \
    -L../local/build/libwebsockets-4.3.2/lib \
    -L../local/build/sqlite-amalgamation-3420000/lib \
    -L../local/build/icu-release-78.1/lib \
    src/Lit.cpp \
    -Wl,--start-group \
    $LIBS \
    -Wl,--end-group


