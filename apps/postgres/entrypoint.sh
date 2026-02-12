#!/bin/bash

mkdir -p /tmp/Wywy-Website
cp "/run/secrets/ssh_private_key" "/tmp/Wywy-Website/id_ed25519"
chmod 400 "/tmp/Wywy-Website/id_ed25519"
chown postgres:postgres "/tmp/Wywy-Website/id_ed25519"

mkdir -p /tmp/Wywy-Website/backup
cp run/secrets/ssh_backup_private_key /tmp/Wywy-Website/backup/id_ed25519.pub
chmod 400 tmp/Wywy-Website/backup/id_ed25519.pub
chown postgres:postgres /tmp/Wywy-Website/backup/id_ed25519.pub

echo "$BACKUP_HOST $(cat /tmp/Wywy-Website/backup/id_ed25519.pub)" > /root/.ssh/known_hosts

exec su postgres -c "postgres -D /var/lib/postgresql/data -p $DATABASE_PORT -c config_file=/etc/postgresql/postgresql.conf"