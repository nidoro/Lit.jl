#!/bin/bash

docker run --rm -v "$PWD":/work lit-build:20.04 bash -c "./net-layer/third/build-icu-release-78.1-linux-x86_64.sh"
docker run --rm -v "$PWD":/work lit-build:20.04 bash -c "./net-layer/third/build-sqlite-3420000-linux-x86_64.sh"
docker run --rm -v "$PWD":/work lit-build:20.04 bash -c "./net-layer/third/build-openssl-1.1.1t-linux-x86_64.sh"
docker run --rm -v "$PWD":/work lit-build:20.04 bash -c "./net-layer/third/build-libwebsockets-4.3.2-linux-x86_64.sh"
