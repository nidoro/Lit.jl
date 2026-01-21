#!/bin/bash

if [ -z "$1" ]; then
    echo "Error: Version string required. Example: 1.0.0"
    echo "Usage: $0 <version>"
    exit 1
fi

VERSION_STRING=$1

THIS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
pushd $THIS_DIR/.. # Repo's root

# Linux x86_64
#----------------------------

# Build libmagic.so on Ubuntu 20.04 (LTS)
docker run --rm -v "$PWD":/work lit-build:20.04 bash -c "./net-layer/build-linux-x86_64.sh"

ARTIFACTS_DIR=build/linux-x86_64/artifacts-linux-x86_64

tar -czf "$ARTIFACTS_DIR.tar.gz" -C "$ARTIFACTS_DIR" .
LINUX_X86_64_TREE_SHA1=$(julia -e "using Pkg; using Pkg.GitTools; println(bytes2hex(Pkg.GitTools.tree_hash(\"$ARTIFACTS_DIR\")))")
LINUX_X86_64_SHA256=$(sha256sum "$ARTIFACTS_DIR.tar.gz" | cut -d' ' -f1)

# Windows 64-bit
#----------------------------
net-layer/build-win64.sh

ARTIFACTS_DIR=build/win64/artifacts-win64

tar -czf "$ARTIFACTS_DIR.tar.gz" -C "$ARTIFACTS_DIR" .
WIN64_TREE_SHA1=$(julia -e "using Pkg; using Pkg.GitTools; println(bytes2hex(Pkg.GitTools.tree_hash(\"$ARTIFACTS_DIR\")))")
WIN64_SHA256=$(sha256sum "$ARTIFACTS_DIR.tar.gz" | cut -d' ' -f1)

# Create Artifacts.toml
#---------------------------
echo "Creating Artifacts.toml..."
echo "
[[artifacts]]
git-tree-sha1 = \"$LINUX_X86_64_TREE_SHA1\"
os = \"linux\"
arch = \"x86_64\"

    [[artifacts.download]]
    url = \"https://github.com/nidoro/Magic.jl/releases/download/v$VERSION_STRING/artifacts-linux-x86_64.tar.gz\"
    sha256 = \"$LINUX_X86_64_SHA256\"

[[artifacts]]
git-tree-sha1 = \"$WIN64_TREE_SHA1\"
os = \"windows\"
arch = \"x86_64\"

    [[artifacts.download]]
    url = \"https://github.com/nidoro/Magic.jl/releases/download/v$VERSION_STRING/artifacts-win64.tar.gz\"
    sha256 = \"$WIN64_SHA256\"
" > Artifacts.toml

# Update Project.toml version
#------------------------------
echo "Updating Project.toml..."
julia -e "using TOML; d = TOML.parsefile(\"Project.toml\"); d[\"version\"] = \"$VERSION_STRING\"; open(\"Project.toml\", \"w\") do io; TOML.print(io, d) end"

echo "Ok!"

popd
