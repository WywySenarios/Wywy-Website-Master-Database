#!/bin/bash
# test.sh — thin entrypoint router (ci.mdx). Dispatches to the test suite
# scripts; no test logic lives here.
set -euo pipefail

SCRIPT_DIR="$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")"
cd "$SCRIPT_DIR"

case "${1:-kind}" in
kind) scripts/tests/kind.sh ;;
*)
	echo "Usage: $0 [kind]" >&2
	exit 1
	;;
esac
