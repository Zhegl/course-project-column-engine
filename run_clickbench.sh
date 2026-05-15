#!/usr/bin/env bash
set -euo pipefail

BIN="$(dirname "$0")/build/bin/test_clickbench"
QUERIES=(0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 19 24 25)

drop_caches() {
    if [ -w /proc/sys/vm/drop_caches ]; then
        echo 3 > /proc/sys/vm/drop_caches
    elif command -v sudo &>/dev/null; then
        sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches' 2>/dev/null || true
    fi
}

printf "%-6s %8s  %s\n" "Query" "Time(ms)" "Result"
printf "%-6s %8s  %s\n" "-----" "--------" "------"

for i in "${QUERIES[@]}"; do
    drop_caches
    line=$("$BIN" --gtest_filter="ClickBench.Q$i" 2>/dev/null \
        | grep -E "\[ *(OK|FAILED) \]")
    ms=$(echo "$line" | grep -oP '\(\K[0-9]+(?= ms\))')
    status=$(echo "$line" | grep -oP '(OK|FAILED)')
    printf "%-6s %8s  %s\n" "Q$i" "${ms:-?}" "${status:-ERROR}"
done
