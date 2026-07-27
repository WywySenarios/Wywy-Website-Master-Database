#!/bin/bash
set -euo pipefail

COMPOSE="docker compose \
  -f docker/docker-compose.base.yml \
  -f docker/docker-compose.test.yml \
  -f docker/docker-compose.ci.yml"

# Ensure cleanup runs on exit, even if interrupted.
LOGS_PID=
cleanup() {
	if [ -n "$LOGS_PID" ]; then
		kill -9 "$LOGS_PID" 2>/dev/null || true
	fi
	$COMPOSE down 2>/dev/null || true
}
trap cleanup EXIT INT TERM

# Ensure bind-mount sources exist so Docker doesn't create them as
# directories (which would shadow files baked into the image).
#
# config.yml:  downloaded from the control repo so the real schema is
#              available for both the bind mount and test assertions.
# Log dir:     created with world-writable permissions so the container
#              user can write even though the host dir is owned by root.
CONFIG_SRC=config/ci/config.yml
LOG_DIR=/var/log/Wywy-Website/master-database

if [ -d "$CONFIG_SRC" ]; then
	# Docker may have created a directory at this path from a failed
	# bind mount — remove it so curl can write a file.
	rmdir "$CONFIG_SRC" 2>/dev/null || rm -f "$CONFIG_SRC"
fi
if [ ! -f "$CONFIG_SRC" ]; then
	echo "==> Downloading config.yml ..."
	mkdir -p "$(dirname "$CONFIG_SRC")"
	curl -fsSL \
		https://raw.githubusercontent.com/WywySenarios/Wywy-Website-Control/main/config/config.yml \
		-o "$CONFIG_SRC"
fi

# Ensure the log directory exists and is world-writable so the
# container's non-root user can write to it.  Use sudo when the
# parent is owned by root (e.g. /var/log).
if [ ! -d "$LOG_DIR" ]; then
	mkdir -p "$LOG_DIR" 2>/dev/null || sudo mkdir -p "$LOG_DIR"
fi
# Always ensure world-writable (directory may pre-exist from Docker).
chmod 777 "$LOG_DIR" 2>/dev/null || sudo chmod 777 "$LOG_DIR"

# Start all services and wait for health checks.
$COMPOSE up --detach --wait

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
