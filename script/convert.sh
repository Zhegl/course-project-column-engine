#!/usr/bin/env bash
set -euo pipefail

INPUT_CSV="$1"
COLUMNAR="$2"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

./build/bin/convert \
    --input "${INPUT_CSV}" \
    --schema "${SCRIPT_DIR}/hits_schema.csv" \
    --output "${COLUMNAR}" \
    --batch 8192
