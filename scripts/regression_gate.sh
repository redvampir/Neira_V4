#!/usr/bin/env bash
set -euo pipefail

# Compatibility wrapper.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec bash "$SCRIPT_DIR/../Source/Tests/scripts/regression_gate.sh" "$@"
