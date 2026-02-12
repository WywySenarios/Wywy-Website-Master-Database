# Purpose

Store and serve data via network.

# Installation

## Dependencies

- npm
- python3
- docker

## Docker

1. Install Docker.

- To allow your user to run docker: sudo chuser add _YOUR_CURRENT_USER_ docker

2. Create & configure your cloudflare tokens
   Astro token:

- User API Token
  - Account, Workers KV Storage, Edit (IDK if this is necessary)
  - Account, Workers Scripts, Edit
  - User, Memberships, Read
  - User, User Details, Read (IDK if this is necessary)
  - Zone, Zone, Read
  - Zone, Worker Routes, Edit
  - Zone, DNS, Edit
    Sql-receptionist Token:
- Tunnel Token
  - You do not need to follow the instructions to create a tunnel. Just throw in the token into the correct secret location.

3. Insert cloudflare tokens

- Create secrets folder using `mkdir secrets`
- cd to secrets folder
- Insert Astro token `nano astro-cloudflare-token.txt`
- Insert Sql-receptionist token `nano cloudflared-cloudflare-token.txt`

4. Insert domain names

- References to the main website & sql-receptionist in config.yml
- The final URL of the main website inside apps/astro-app/wrangler.jsonc
  - under "routes"

5. Change the database password in docker/.env

6. cd to docker folder

```
cd docker
```

7. Run your servers.

```
./run.sh prod
```

## Backup

SSH keys are required for backing up information.

- Backup server's public SSH key.
- A public & private key to use for authentication.
