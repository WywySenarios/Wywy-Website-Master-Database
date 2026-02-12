#!/bin/bash
set -euo pipefail

# Check for arguments
if [[ -z "$1" ]]; then
  echo "Error: WAL_PATH not provided." >&2
  exit 1
elif [[ -z "$2" ]]; then
  echo "Error: WAL_FILE not provided." >&2
fi

WAL_PATH="$1"
WAL_FILE="$2"

# cp "$WAL_PATH" /var/lib/Wywy-Website/website/postgres_WALs

scp -i "/tmp/Wywy-Website/id_ed25519" \
  -o BatchMode=yes \
  -o StrictHostKeyChecking=accept-new \
  "$WAL_PATH" \
  "$BACKUP_USER@$BACKUP_HOST:/var/lib/Wywy-Website/backup/postgres_WALs/$WAL_FILE"

echo "Backup succeeded: $WAL_FILE"