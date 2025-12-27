FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC

RUN apt-get update && apt-get install -y \
    build-essential \
    g++ \
    cmake \
    git \
    pkg-config \
    ca-certificates \
    wget \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /work
