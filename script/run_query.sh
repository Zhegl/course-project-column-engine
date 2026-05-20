#!/usr/bin/env bash
set -uo pipefail

QUERY_NUM="$1"
COLUMNAR="$2"
OUTPUT="$3"
LOGS="$4"

EXIT_CODE=0

./build/bin/run_query \
    "${QUERY_NUM}" \
    "${COLUMNAR}" \
    "${OUTPUT}" \
    2>"${LOGS}" || EXIT_CODE=$?

if [ "$EXIT_CODE" -eq 137 ]; then
    echo "[OOM] query ${QUERY_NUM} killed by OOM" >> "${LOGS}"
    exit 0
fi

exit "$EXIT_CODE"

