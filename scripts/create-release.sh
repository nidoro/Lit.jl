#!/bin/bash

set -xe

if [ -z "$1" ]; then
    echo "Error: Tag name required. Example: v1.0.0"
    echo "Usage: $0 <tag_name>"
    exit 1
fi

TAG_NAME=$1
EXTRA_ARGS=

if [ $# -gt 1 ]; then
    EXTRA_ARGS="${@:2}"
fi

gh release create $TAG_NAME \
    $EXTRA_ARGS \
    --notes-file RELEASE.md \
    ./build/linux-x86_64/artifacts-linux-x86_64.tar.gz \
    ./build/win64/artifacts-win64.tar.gz
