#!/bin/bash

# Run from repo's root

set -xe
docker run --rm -v "$PWD":/work lit-build:20.04 bash -c "./net-layer/build.sh"
