#!/bin/bash
# Run every Symta test suite and print one Summary line per suite.
cd "$(dirname "$0")/.."

for s in tokenizer reader macros runtime compiler gfx ffi uim; do
  echo -n "$s: "
  bash tests/$s/run.sh 2>&1 | grep "^Summary" || echo "(no summary)"
done

echo -n "lineno: "
bash tests/runtime/lineno-check.sh 2>&1 | grep "^Summary"

echo -n "drift: "
bash tests/bootstrap/drift.sh 3 2>&1 | tail -1
