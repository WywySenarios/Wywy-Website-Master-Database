#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")"
cd "$SCRIPT_DIR"

export SECRETS_DIR="${SCRIPT_DIR}/config/ci"
export UNIVERSAL_CONFIG_DIR="${SCRIPT_DIR}/config/ci"

COMPOSE="docker compose \
  -f docker/docker-compose.base.yml \
  -f docker/docker-compose.test.yml"

# Ensure cleanup runs on exit, even if interrupted.
LOGS_PID=
cleanup() {
	if [ -n "$LOGS_PID" ]; then
		kill -9 "$LOGS_PID" 2>/dev/null || true
	fi
	$COMPOSE down 2>/dev/null || true
}
trap cleanup EXIT INT TERM

# Config file must be pre-staged at config/ci/config.yml before running
# this script — see internal/conventions/tech-stack/ci.mdx.
#
# Log dir is created with world-writable permissions so the container
# user can write even though the host dir is owned by root.
LOG_DIR=/var/log/Wywy-Website/master-database

# Ensure the log directory exists and is world-writable so the
# container's non-root user can write to it.  Use sudo when the
# parent is owned by root (e.g. /var/log).
if [ ! -d "$LOG_DIR" ]; then
	mkdir -p "$LOG_DIR" 2>/dev/null || sudo mkdir -p "$LOG_DIR"
fi
# Always ensure world-writable (directory may pre-exist from Docker).
chmod 777 "$LOG_DIR" 2>/dev/null || sudo chmod 777 "$LOG_DIR"

# Start all services and wait for health checks.
$COMPOSE up --detach --wait 2>&1 || {
	rc=$?
	echo ""
	echo "============================================================"
	$COMPOSE logs --no-color 2>&1 || true
	echo "============================================================"
	exit $rc
}

# Stream container logs in background for real-time CI visibility.
$COMPOSE logs -f &
LOGS_PID=$!

# Capture container IDs for both test services.
test_cid=$($COMPOSE ps -q test || true)
unit_cid=$($COMPOSE ps -q unit_test || true)

# Wait for each container to finish and collect exit codes in parallel.
test_exit=0
unit_exit=0
[ -n "$test_cid" ] && docker wait "$test_cid" && test_exit=$? || test_exit=1
[ -n "$unit_cid" ] && docker wait "$unit_cid" && unit_exit=$? || unit_exit=1

# Aggregate exit code: any non-zero → overall failure.
exit_code=0
[ $test_exit -ne 0 ] && exit_code=$test_exit
[ $unit_exit -ne 0 ] && exit_code=$unit_exit

# Stop background log stream before teardown.  The process can
# get stuck in a blocking Docker API read that SIGTERM doesn't
# interrupt — escalate to SIGKILL after a short grace period.
kill $LOGS_PID 2>/dev/null || true
for _ in 1 2 3; do
	kill -0 $LOGS_PID 2>/dev/null || break
	sleep 1
done
kill -9 $LOGS_PID 2>/dev/null || true
wait $LOGS_PID 2>/dev/null || true

# Tear down.
$COMPOSE down

exit $exit_code
