#!/bin/bash

mkdir -p /tmp/Wywy-Website
cp "/run/secrets/ssh_private_key" "/tmp/Wywy-Website/id_ed25519"
chmod 400 "/tmp/Wywy-Website/id_ed25519"
chown postgres:postgres "/tmp/Wywy-Website/id_ed25519"

mkdir -p /tmp/Wywy-Website/backup
cp run/secrets/ssh_backup_private_key /tmp/Wywy-Website/backup/ssh_host_ed25519_key.pub
chmod 400 tmp/Wywy-Website/backup/ssh_host_ed25519_key.pub
chown postgres:postgres /tmp/Wywy-Website/backup/ssh_host_ed25519_key.pub

echo "$BACKUP_HOST $(cat /tmp/Wywy-Website/backup/ssh_host_ed25519_key.pub)" > /root/.ssh/known_hosts

exec gosu postgres postgres -D $PGDATA \
    -c "config_file=/etc/postgresql/postgresql.conf"