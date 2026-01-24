#!/bin/bash

set -xe

if [ -z "$1" ]; then
    echo "Error: Tag name required. Example: v1.0.0"
    echo "Usage: $0 <tag_name>"
    exit 1
fi

TAG_NAME=$1
COMMIT_SHA=$(git rev-list -n 1 $TAG_NAME)

gh api repos/:owner/:repo/commits/$COMMIT_SHA/comments \
-f body="@JuliaRegistrator register

Release notes:

$(cat RELEASE.md)"
