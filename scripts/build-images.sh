#!/bin/bash
# Builds the five kind-test images and loads them into the KinD cluster.
set -euo pipefail

SCRIPT_DIR="$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")"
REPO_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
cd "$REPO_ROOT"

KIND_CLUSTER_NAME="${KIND_CLUSTER_NAME:-master-db-test}"

if git submodule status | grep -q '^-'; then
	echo "Submodules not checked out — run: git submodule update --init --recursive" >&2
	exit 1
fi

USER_ID=25231
LIBCYAML_VARIANT=debug

function build_and_load() {
	local image="$1"
	local dockerfile="$2"
	local target="$3"
	shift 3

	local args=()
	[[ -z "$target" ]] || args+=(--target "$target")
	while [[ $# -gt 0 ]]; do
		args+=(--build-arg "$1")
		shift
	done

	if ! docker build --file "$dockerfile" "${args[@]}" --tag "$image" .; then
		echo "Failed to build $image" >&2
		exit 1
	fi
	if ! kind load docker-image "$image" --name "$KIND_CLUSTER_NAME"; then
		echo "Failed to load $image into cluster $KIND_CLUSTER_NAME" >&2
		exit 1
	fi
	echo "Loaded $image"
}

build_and_load master-db/sql-receptionist:kind-test apps/sql-receptionist/Dockerfile dev USER_ID="$USER_ID" LIBCYAML_VARIANT="$LIBCYAML_VARIANT"
build_and_load master-db/db-migrate:kind-test apps/create_tables/Dockerfile ""
build_and_load master-db/tests:kind-test apps/tests/Dockerfile "" USER_ID="$USER_ID"
build_and_load master-db/unit-test:kind-test apps/sql-receptionist/Dockerfile unit_test USER_ID="$USER_ID" LIBCYAML_VARIANT="$LIBCYAML_VARIANT"
build_and_load master-db/postgres:kind-test apps/postgres/Dockerfile db
