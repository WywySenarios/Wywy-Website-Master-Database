Developed on a Windows 11 computer
Made in WSL -> Ubuntu 24.04.2 LTS

# WSL Config
This is only necessary if you're using WSL.
To open the WSL config, cd to ~ and open .wslconfig in notepad:
 * cd ~
 * notepad .wslconfig

The content inside .wslconfig is:
[wsl2]
networkingMode=mirrored

# Required Libraries
* libcyaml
* libpq-dev
* jansson

# Docker Watch
Docker watch only appears to work when the user running the entire docker (e.g. root if you run `sudo docker compose up --watch`) also owns the associated folders and files here. (e.g. sql-receptionist folder & all the children files)

If the files are not owned by the correct user, Docker Watch will **invisibly**/**softly** fail to sync files.

There is now a own.sh script that handles this. @TODO restrict chmod perms so it gives only the people who need it read & write & execute perms.