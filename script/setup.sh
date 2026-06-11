#!/usr/bin/env bash
set -euo pipefail

apt-get update
apt-get install -y --no-install-recommends \
    cmake ninja-build \
    clang \
    ca-certificates \
    libre2-dev
rm -rf /var/lib/apt/lists/*
