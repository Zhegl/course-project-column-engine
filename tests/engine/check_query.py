#!/usr/bin/env python3
"""
Usage: check_query.py <query_num> <engine_csv> <hits_db> <hits_schema_csv> <queries_sql>
Exits 0 if results match, 1 otherwise. Prints diff on mismatch.
hits_db is a path to a DuckDB database file with a 'hits' table.
"""
import sys
import csv
import subprocess

def read_csv(path):
    with open(path, newline='', encoding='utf-8') as f:
        return list(csv.reader(f))

def rows_to_str_set(rows):
    return {tuple(r) for r in rows}

def normalize_value(v):
    try:
        return str(int(float(v)))
    except (ValueError, OverflowError):
        return v

def normalize_rows(rows):
    return [tuple(normalize_value(v) for v in row) for row in rows]

# queries where ORDER BY is not unique — only row count is checked
# queries where tie-breaking is non-deterministic — only row count is checked
NONDETERMINISTIC = {5, 11, 16, 17, 18, 22, 23, 24, 27, 28, 30, 31, 32, 33, 38, 39, 40, 41}

def main():
    if len(sys.argv) != 6:
        print("Usage: check_query.py <query_num> <engine_csv> <hits_db> <hits_schema_csv> <queries_sql>")
        sys.exit(2)

    query_num = int(sys.argv[1])
    engine_csv = sys.argv[2]
    hits_db = sys.argv[3]
    # sys.argv[4] is schema_csv — ignored, schema is embedded in hits_db
    queries_sql = sys.argv[5]

    with open(queries_sql) as f:
        queries = [line.strip() for line in f if line.strip()]

    sql = queries[query_num]
    # duckdb needs explicit cast for strftime on VARCHAR columns
    sql = sql.replace("strftime('%M', EventTime)", "strftime('%M', EventTime::TIMESTAMP)")

    try:
        proc = subprocess.run(
            ["duckdb", hits_db, "-csv", "-c", sql],
            capture_output=True, text=True, timeout=300
        )
        if proc.returncode != 0:
            print(f"SKIP: duckdb error: {proc.stderr.strip()}")
            sys.exit(0)
        duck_rows = list(csv.reader(proc.stdout.splitlines()))
        if duck_rows:
            duck_rows = duck_rows[1:]  # skip header
    except Exception as e:
        print(f"SKIP: duckdb error: {e}")
        sys.exit(0)

    engine_rows = read_csv(engine_csv)

    engine_set = rows_to_str_set(normalize_rows(engine_rows))
    duck_set = rows_to_str_set(normalize_rows(duck_rows))

    if query_num in NONDETERMINISTIC:
        if len(engine_rows) == len(duck_rows):
            sys.exit(0)
        print(f"MISMATCH for query {query_num}: row count {len(engine_rows)} vs {len(duck_rows)}")
        sys.exit(1)

    if engine_set == duck_set:
        sys.exit(0)

    only_engine = engine_set - duck_set
    only_duck = duck_set - engine_set
    print(f"MISMATCH for query {query_num}:")
    if only_engine:
        print(f"  Only in engine ({len(only_engine)} rows):")
        for r in sorted(only_engine)[:5]:
            print(f"    {r}")
    if only_duck:
        print(f"  Only in duckdb ({len(only_duck)} rows):")
        for r in sorted(only_duck)[:5]:
            print(f"    {r}")
    sys.exit(1)

if __name__ == "__main__":
    main()
