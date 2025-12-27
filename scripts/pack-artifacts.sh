#!/bin/bash

# Run from repo's root

ARTIFACTS_DIR=local/build/artifacts-linux-x86_64

tar -czf "$ARTIFACTS_DIR.tar.gz" -C "$ARTIFACTS_DIR" .

echo "------------------------------------------------------"
echo "Checksum of $ARTIFACTS_DIR.tar.gz"
sha256sum "$ARTIFACTS_DIR.tar.gz"

echo "------------------------------------------------------"
echo "Git Tree hash"
julia -e "using Pkg; using Pkg.GitTools; println(bytes2hex(Pkg.GitTools.tree_hash(\"$ARTIFACTS_DIR\")))"
echo "------------------------------------------------------"

