#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "$0")/env.sh"

drop_caches() {
    if [ -w /proc/sys/vm/drop_caches ]; then
        echo 3 > /proc/sys/vm/drop_caches
    else
        sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches' 2>/dev/null || true
    fi
}

printf "%-6s %10s\n" "Query" "Time(ms)"
printf "%-6s %10s\n" "-----" "--------"

for QUERY_NUM in {0..42}; do
  OUTPUT=${RESULTS}/query_${QUERY_NUM}.csv
  LOGS=${RESULTS}/query_${QUERY_NUM}.log
  drop_caches
  START=$(date +%s%3N)
  ./script/run_query.sh ${QUERY_NUM} ${COLUMNAR} ${OUTPUT} ${LOGS}
  END=$(date +%s%3N)
  printf "%-6s %10s\n" "Q${QUERY_NUM}" "$((END - START))"
  START=$(date +%s%3N)
  ./script/run_query.sh ${QUERY_NUM} ${COLUMNAR} ${OUTPUT} ${LOGS}
  END=$(date +%s%3N)
  printf "%-6s %10s\n" "Q${QUERY_NUM}" "$((END - START))"
done
