#!/bin/bash
# Bootstrap roles, databases, and extensions for the master-database test flow.
#
# Runs ONLY under the STOCK postgres entrypoint (docker-entrypoint.sh), which
# executes /docker-entrypoint-initdb.d/* on first initdb (test/CI use
# `command: ["postgres"]`). The custom apps/postgres/entrypoint.sh (dev/prod)
# does NOT run this script — on DB VMs the equivalent superuser init lives in
# the control repo (scripts/proxmox/vm/db/init-rbac.sh).
#
# Idempotent: roles/DBs are created only when missing (WHERE NOT EXISTS +
# \gexec); extensions use IF NOT EXISTS. Safe to run repeatedly.
#
# Why these objects exist:
#   - master_db_migrator owns both databases. In PG15+ the public schema is
#     owned by pg_database_owner, so the DB owner is the only non-superuser who
#     can CREATE TABLE there. Liquibase connects as master_db_migrator.
#   - master_db_app must EXIST before the grants changesets run, or
#     `GRANT ... TO master_db_app` fails. It is the DML role (seed, tests).
#   - postgis must exist BEFORE liquibase update in the data database: the
#     baseline has geography(POINT, 4326) columns there. info (auth tables
#     only) and postgres_fdw (no FDW consumer) need neither.
#
# Required environment variables:
#   DATABASE_MIGRATOR_USERNAME / DATABASE_MIGRATOR_PASSWORD
#   DATABASE_PASSWORD
set -euo pipefail

psql -v ON_ERROR_STOP=1 \
	-v master_db_migrator_username="$DATABASE_MIGRATOR_USERNAME" \
	-v master_db_migrator_password="$DATABASE_MIGRATOR_PASSWORD" \
	-v master_db_app_password="$DATABASE_PASSWORD" \
	--username "$POSTGRES_USER" --dbname postgres <<'EOSQL'
-- # roles
SELECT format('CREATE ROLE %I LOGIN PASSWORD %L', :'master_db_migrator_username', :'master_db_migrator_password')
  WHERE NOT EXISTS (SELECT FROM pg_roles WHERE rolname = :'master_db_migrator_username') \gexec
SELECT format('CREATE ROLE %I LOGIN PASSWORD %L', 'master_db_app', :'master_db_app_password')
  WHERE NOT EXISTS (SELECT FROM pg_roles WHERE rolname = 'master_db_app') \gexec

-- # databases
SELECT format('CREATE DATABASE wywywebsite OWNER %I', :'master_db_migrator_username')
  WHERE NOT EXISTS (SELECT FROM pg_database WHERE datname = 'wywywebsite') \gexec
SELECT format('CREATE DATABASE info OWNER %I', :'master_db_migrator_username')
  WHERE NOT EXISTS (SELECT FROM pg_database WHERE datname = 'info') \gexec
EOSQL

psql -v ON_ERROR_STOP=1 --username "$POSTGRES_USER" --dbname wywywebsite \
	-c "CREATE EXTENSION IF NOT EXISTS postgis;"
