FROM ubuntu:22.04

ARG DEBIAN_FRONTEND=noninteractive

# Install base dependencies
RUN apt-get update && apt-get install -y \
    apt-utils \
    binutils \
    build-essential \
    cmake \
    curl \
    wget \
    unzip \
    make \
    git \
    python3 \
    ninja-build \
    gcc \
    g++ \
    libc6-dev \
    gcc-arm-linux-gnueabihf \
    g++-arm-linux-gnueabihf \
    gcc-aarch64-linux-gnu \
    g++-aarch64-linux-gnu \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

# Default command - build for Linux
CMD ["sh", "-c", "python3 scripts/platform_builder.py --platform=linux --arch=all"]
