#!/usr/bin/env bash
set -euo pipefail

# Purpose: Merge-blocking regression gate runner for Unix-like shells.
# Invariant: overall test suite failures must not fail this gate unless
#            Neira.RegressionGate.* explicitly fails.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEST_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
LOG_PATH="$TEST_DIR/regression_gate.log"

cd "$TEST_DIR"

if [[ ! -x ./neira_tests ]]; then
  echo "[regression-gate] neira_tests binary not found; building with make"
  make neira_tests
fi

set +e
./neira_tests | tee "$LOG_PATH"
SUITE_EXIT=${PIPESTATUS[0]}
set -e

if ! grep -Eq '^  (PASS|FAIL)  Neira\.RegressionGate\.' "$LOG_PATH"; then
  echo "[regression-gate] WARNING: no executable Neira.RegressionGate.* tests found in binary"
fi

if grep -Eq '^  FAIL  Neira\.RegressionGate\.' "$LOG_PATH"; then
  echo "[regression-gate] FAIL: detected failing Neira.RegressionGate.* tests"
  exit 1
fi

if [[ "$SUITE_EXIT" -ne 0 ]]; then
  echo "[regression-gate] INFO: suite exit code was $SUITE_EXIT, ignored by gate policy"
fi

echo "[regression-gate] PASS: no Neira.RegressionGate.* failures"
