#!/bin/bash

THIS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
pushd $THIS_DIR/sqlite-amalgamation-3420000

DEST_DIR=$THIS_DIR/../../build/win64/sqlite-amalgamation-3420000
mkdir -p $DEST_DIR/include
mkdir -p $DEST_DIR/lib

ICU_DIR=$DEST_DIR/../icu-release-78.1

cp ./sqlite3.h $DEST_DIR/include/

#echo "Building shared library libSqliteIcu.dll"
#x86_64-w64-mingw32-gcc -fPIC -shared -o $DEST_DIR/lib/libSqliteIcu.dll ext/icu/icu.c -I. -I$ICU_DIR/include -L$ICU_DIR/lib -Wl,--start-group -l:libsicuuc.a -l:libsicuin.a -l:libsicuio.a -l:libsicudt.a -Wl,--end-group -lstdc++

echo "Building static library libSqliteIcu.a"
x86_64-w64-mingw32-gcc -fPIC -c -o $DEST_DIR/lib/libSqliteIcu.a ext/icu/icu.c -I. -I$ICU_DIR/include -L$ICU_DIR/lib -lstdc++

CFLAGS="-I. -DSQLITE_ENABLE_COLUMN_METADATA -DSQLITE_ENABLE_FTS5 -DSQLITE_ENABLE_ICU -I$ICU_DIR/include -L$DEST_DIR/lib -L$ICU_DIR/lib"

#echo "Building shared library libsqlite3.dll"
#x86_64-w64-mingw32-gcc $CFLAGS -fPIC -shared -o $DEST_DIR/lib/libsqlite3.dll sqlite3.c -Wl,--start-group -l:libsicuuc.a -l:libsicuin.a -l:libsicuio.a -l:libsicudt.a -Wl,--end-group -lstdc++

echo "Building static library libsqlite3.a"
x86_64-w64-mingw32-gcc $CFLAGS -fPIC -c -o $DEST_DIR/lib/libsqlite3.a sqlite3.c -lstdc++

popd
