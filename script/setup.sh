#!/usr/bin/env bash
set -euo pipefail

apt-get update
apt-get install -y --no-install-recommends \
    cmake ninja-build \
    clang \
    ca-certificates
rm -rf /var/lib/apt/lists/*
