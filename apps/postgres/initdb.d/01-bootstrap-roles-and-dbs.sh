#!/bin/bash
# Bootstrap roles and databases for the master-database test flow.
#
# Runs only under the STOCK postgres entrypoint (docker-entrypoint.sh), which
# executes /docker-entrypoint-initdb.d/* on first initdb (test/CI use
# `command: ["postgres"]`). The custom apps/postgres/entrypoint.sh (dev/prod)
# does NOT run these scripts.
#
# Required environment variables:
#   DATABASE_MIGRATOR_USERNAME / DATABASE_MIGRATOR_PASSWORD
#   DATABASE_PASSWORD
#
# Why these objects exist:
#   - wywy_migrator owns both databases. In PG15+ the public schema is owned
#     by pg_database_owner, so the DB owner is the only non-superuser who can
#     CREATE TABLE there. Liquibase connects as wywy_migrator.
#   - wywy_app must EXIST before the grants changesets run, or
#     `GRANT ... TO wywy_app` fails. It is the DML role (seed, tests).
#   - postgis must exist BEFORE liquibase update: the baseline has
#     geography(POINT, 4326) columns. postgres_fdw is created per parent plan
#     Phase 2 (no FDW objects exist yet — old create_tables.py created none).
set -euo pipefail

psql -v ON_ERROR_STOP=1 --username "$POSTGRES_USER" --dbname postgres <<-EOSQL
	CREATE ROLE ${DATABASE_MIGRATOR_USERNAME} LOGIN PASSWORD '${DATABASE_MIGRATOR_PASSWORD}';
	CREATE ROLE wywy_app LOGIN PASSWORD '${DATABASE_PASSWORD}';
	CREATE DATABASE wywywebsite OWNER ${DATABASE_MIGRATOR_USERNAME};
	CREATE DATABASE info OWNER ${DATABASE_MIGRATOR_USERNAME};
EOSQL

psql -v ON_ERROR_STOP=1 --username "$POSTGRES_USER" --dbname wywywebsite -c "CREATE EXTENSION IF NOT EXISTS postgis;"
psql -v ON_ERROR_STOP=1 --username "$POSTGRES_USER" --dbname wywywebsite -c "CREATE EXTENSION IF NOT EXISTS postgres_fdw;"
psql -v ON_ERROR_STOP=1 --username "$POSTGRES_USER" --dbname info -c "CREATE EXTENSION IF NOT EXISTS postgres_fdw;"
