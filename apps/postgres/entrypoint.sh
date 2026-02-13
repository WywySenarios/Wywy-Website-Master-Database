#!/bin/bash

# Prepare SSH private key
mkdir -p /tmp/Wywy-Website
cp "/run/secrets/ssh_private_key" "/tmp/Wywy-Website/id_ed25519"
chmod 400 "/tmp/Wywy-Website/id_ed25519"
chown postgres:postgres "/tmp/Wywy-Website/id_ed25519"

# Prepare SSH remote host identity
mkdir -p /tmp/Wywy-Website/backup
cp /run/secrets/ssh_backup_public_key /tmp/Wywy-Website/backup/ssh_host_ed25519_key.pub
chmod 400 /tmp/Wywy-Website/backup/ssh_host_ed25519_key.pub
chown postgres:postgres /tmp/Wywy-Website/backup/ssh_host_ed25519_key.pub

# Prepare the PGDATA folder
chown postgres:postgres $PGDATA
chmod 700 $PGDATA

if [ ! -s "$PGDATA/PG_VERSION" ]; then
    echo "Database not initialized. Running initdb..."

    gosu postgres initdb -D "$PGDATA"
fi

# Repopulate configuration
cp /etc/postgresql/postgresql.conf "$PGDATA/postgresql.conf"
chown postgres:postgres "$PGDATA/postgresql.conf"
chmod 700 "$PGDATA/postgresql.conf"
cp /etc/postgresql/pg_hba.conf "$PGDATA/pg_hba.conf"
chown postgres:postgres "$PGDATA/pg_hba.conf"
chmod 700 "$PGDATA/pg_hba.conf"

echo "$BACKUP_HOST $(cat /tmp/Wywy-Website/backup/ssh_host_ed25519_key.pub)" > /tmp/Wywy-Website/backup/host

exec gosu postgres postgres -D $PGDATA \
    -c "config_file=/etc/postgresql/postgresql.conf"