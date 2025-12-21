#!/bin/bash

THIS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

DEST_DIR=$THIS_DIR/../../../local/build/sqlite-amalgamation-3420000
mkdir -p $DEST_DIR/include
mkdir -p $DEST_DIR/lib

ICU_DIR=$DEST_DIR/../icu-release-78.1

cp ./sqlite3.h $DEST_DIR/include/

echo "Building shared library libSqliteIcu.so"
gcc -fPIC -shared -o $DEST_DIR/lib/libSqliteIcu.so ext/icu/icu.c -I. -I$ICU_DIR/include -L$ICU_DIR/lib

echo "Building static library libSqliteIcu.a"
gcc -fPIC -c -o $DEST_DIR/lib/libSqliteIcu.a ext/icu/icu.c -I. -I$ICU_DIR/include -L$ICU_DIR/lib

CFLAGS="-I. -DSQLITE_ENABLE_COLUMN_METADATA -DSQLITE_ENABLE_FTS5 -DSQLITE_ENABLE_ICU -I$ICU_DIR/include -L$DEST_DIR/lib -L$ICU_DIR/lib"
LIBS="-l:libSqliteIcu.a -l:libicui18n.a -l:libicuuc.a -l:libicudata.a"

echo "Building shared library libsqlite3.so"
gcc $CFLAGS -fPIC -shared -o $DEST_DIR/lib/libsqlite3.so sqlite3.c $LIBS

echo "Building static library libsqlite3.a"
gcc $CFLAGS -fPIC -c -o $DEST_DIR/lib/libsqlite3.a sqlite3.c $LIBS
