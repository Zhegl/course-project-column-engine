#!/usr/bin/env bash
set -euo pipefail

QUERY_NUM="$1"
COLUMNAR="$2"
OUTPUT="$3"
LOGS="$4"

./build/bin/run_query "${QUERY_NUM}" "${COLUMNAR}" "${OUTPUT}" 2>"${LOGS}"
