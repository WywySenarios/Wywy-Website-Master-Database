#!/bin/bash
# Test runner for master-database service.
# Runs integration tests (Django pytest) and unit tests (C/valgrind) via docker compose.
# Compliant with CI runner contract: parses --output-dir= and writes results.jsonl.
output_dir=""
for arg in "$@"; do
  case "$arg" in
    --output-dir=*) output_dir="${arg#*=}" ;;
  esac
done

PROJECT_DIR="/usr/local/Wywy-Website/Wywy-Website-Master-Database"
CONFIG_DIR="/etc/Wywy-Website-Control/config"
COMPOSE_DIR="$PROJECT_DIR/docker"

COMPOSE_FILES="-f $COMPOSE_DIR/docker-compose.base.yml -f $COMPOSE_DIR/docker-compose.dev.yml -f $COMPOSE_DIR/docker-compose.test.yml"
ENV_FILES="--env-file $CONFIG_DIR/.env --env-file $CONFIG_DIR/master-database/.env --env-file $CONFIG_DIR/.env.dev --env-file $CONFIG_DIR/master-database/.env.dev"

# Start test services and wait for health checks.
docker compose $COMPOSE_FILES $ENV_FILES up --detach --wait

# Stream container logs in background for real-time visibility.
docker compose $COMPOSE_FILES $ENV_FILES logs -f &
LOGS_PID=$!

# Capture container IDs for both test services.
test_cid=$(docker compose $COMPOSE_FILES $ENV_FILES ps -q test)
unit_cid=$(docker compose $COMPOSE_FILES $ENV_FILES ps -q unit_test)

# Wait for each container to finish and capture exit codes.
docker wait "$test_cid"
test_exit=$?
docker wait "$unit_cid"
unit_exit=$?

# Tear down.
docker compose $COMPOSE_FILES $ENV_FILES down
kill $LOGS_PID 2>/dev/null
wait $LOGS_PID 2>/dev/null

# Aggregate exit code: any failure → overall failure.
exit_code=0
[ $test_exit -ne 0 ] && exit_code=$test_exit
[ $unit_exit -ne 0 ] && exit_code=$unit_exit

# Write results.jsonl (CI runner contract).
if [ -n "$output_dir" ]; then
  if [ $exit_code -eq 0 ]; then
    echo '{"name":"master-database-tests","status":"passed"}' > "$output_dir/results.jsonl"
  else
    echo '{"name":"master-database-tests","status":"failed"}' > "$output_dir/results.jsonl"
  fi
fi

exit $exit_code
