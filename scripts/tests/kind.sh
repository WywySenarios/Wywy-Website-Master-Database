#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")"
REPO_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
cd "$REPO_ROOT"

KIND_CLUSTER_NAME="${KIND_CLUSTER_NAME:-master-db-test}"
NAMESPACE="master-database-test"
KIND_MIN_VERSION="0.33.0"
FAILED=""

# ---- preflight ---------------------------------------------------------------

command -v docker >/dev/null 2>&1 || {
	echo "docker not found — install Docker Engine (see install/docker.sh in dotfiles)" >&2
	exit 1
}
docker info >/dev/null 2>&1 || {
	echo "docker daemon not reachable — is it running?" >&2
	exit 1
}
command -v kind >/dev/null 2>&1 || {
	echo "kind not found — install it (see install/kind.sh in dotfiles)" >&2
	exit 1
}
command -v kubectl >/dev/null 2>&1 || {
	echo "kubectl not found — install kubectl" >&2
	exit 1
}
command -v jq >/dev/null 2>&1 || {
	echo "jq not found — install jq (required for structural job assertions)" >&2
	exit 1
}
[[ -f wywy-config/wywy.yml ]] || {
	echo "wywy-config/wywy.yml is missing" >&2
	exit 1
}

# The node image in kind-config.yaml is pinned to the default shipped by kind
# v0.33.0; older kind releases cannot load the pinned digest.
kind_version="$(kind version 2>/dev/null | awk '{print $2}' | sed 's/^v//' || true)"
if [[ -n "$kind_version" ]] && [[ "$(printf '%s\n%s\n' "$kind_version" "$KIND_MIN_VERSION" | sort -V | head -1)" != "$KIND_MIN_VERSION" ]]; then
	echo "kind $kind_version < required $KIND_MIN_VERSION — upgrade kind (see install/kind.sh in dotfiles)" >&2
	exit 1
fi

# ---- helpers -----------------------------------------------------------------

info() { echo "==> $*"; }
err() {
	echo "==> ERROR: $*" >&2
	exit 1
}

dump_diagnostics() {
	echo ""
	echo "============================================================"
	echo "==> Dumping diagnostics before cluster teardown"
	echo "============================================================"
	kubectl -n "$NAMESPACE" get pods -o wide 2>&1 || true
	for obj in deployment/postgres deployment/sql-receptionist job/db-migrate job/db-seed job/integration-test job/unit-test; do
		echo ""
		echo "--- $obj logs ---"
		kubectl -n "$NAMESPACE" logs "$obj" --tail=200 2>&1 || true
	done
}

cleanup() {
	# report the exit code of the tests instead of the log dump
	local rc=$?
	# kind delete destroys pod logs
	# dump before kind delete
	set +e
	if [[ -n "$FAILED" ]]; then
		dump_diagnostics
	fi
	kind delete cluster --name "$KIND_CLUSTER_NAME" 2>/dev/null || true
	exit "$rc"
}

# ---- cluster lifecycle --------------------------------------------------------

trap cleanup EXIT INT TERM

# A stale cluster from a crashed run blocks create; delete it first.
if kind get clusters 2>/dev/null | grep -q "^${KIND_CLUSTER_NAME}$"; then
	info "Deleting stale cluster $KIND_CLUSTER_NAME"
	kind delete cluster --name "$KIND_CLUSTER_NAME"
fi

info "Creating KinD cluster $KIND_CLUSTER_NAME"
kind create cluster --name "$KIND_CLUSTER_NAME" --config k8s/test/kind-config.yaml

info "Creating namespace $NAMESPACE"
kubectl create namespace "$NAMESPACE" --dry-run=client -o yaml | kubectl apply -f -

info "Building and loading images"
scripts/build-images.sh

info "Creating wywy-config ConfigMap from the wywy-config submodule"
kubectl create configmap wywy-config \
	--from-file=wywy.yml="wywy-config/wywy.yml" \
	-n "$NAMESPACE" \
	--dry-run=client -o yaml | kubectl apply -f -

info "Applying infra (base + postgres + secrets)"
kubectl apply -k k8s/test

# ---- wait chain ---------------------------------------------------------------
# Each step dumps logs + sets FAILED on timeout/failure. Steps 1-5 abort the
# chain on failure (later steps depend on earlier ones); the trap tears down.

info "Waiting for postgres to become Ready"
if ! kubectl -n "$NAMESPACE" rollout status deployment/postgres --timeout=300s; then
	FAILED=1
	err "postgres rollout failed"
fi

info "Applying db-migrate Job and waiting for completion"
kubectl apply -f k8s/test/db-migrate.yaml
if ! kubectl -n "$NAMESPACE" wait --for=condition=complete job/db-migrate --timeout=300s; then
	FAILED=1
	err "db-migrate did not complete"
fi

# seed values after schema migration
info "Applying db-seed Job and waiting for completion"
kubectl apply -f k8s/test/db-seed.yaml
if ! kubectl -n "$NAMESPACE" wait --for=condition=complete job/db-seed --timeout=120s; then
	FAILED=1
	err "db-seed did not complete"
fi

info "Waiting for sql-receptionist rollout (valgrind startup is slow)"
if ! kubectl -n "$NAMESPACE" rollout status deployment/sql-receptionist --timeout=600s; then
	FAILED=1
	err "sql-receptionist rollout failed"
fi

# ---- test suites --------------------------------------------------------------

function run_test_job() {
	local job="$1"
	info "Applying $job Job and waiting for completion"
	kubectl apply -f "k8s/test/${job}.yaml"
	if ! kubectl -n "$NAMESPACE" wait --for=condition=complete "job/$job" --timeout=900s; then
		FAILED=1
		echo "==> ERROR: $job did not complete — logs:" >&2
		kubectl -n "$NAMESPACE" logs "job/$job" --tail=200 2>&1 || true
		return
	fi
	kubectl -n "$NAMESPACE" logs "job/$job" 2>&1 || true
}

run_test_job integration-test
run_test_job unit-test

# ---- aggregate ---------------------------

for job in db-migrate db-seed integration-test unit-test; do
	if ! kubectl -n "$NAMESPACE" get job "$job" -o json |
		jq -e '.status.conditions[] | select(.type=="Complete") | .status == "True"' >/dev/null; then
		echo "==> ERROR: job $job is not Complete" >&2
		FAILED=1
	fi
done

exit $([ -n "$FAILED" ] && echo 1 || echo 0)
