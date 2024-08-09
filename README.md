# mod-changeablespawnrates

This WoW-Azerothcore-Mod allows ac-server-administrators to change spawntimes based on a user-defined or dynamically calculated playerbased factor

All settings are well-described in the configuration file.

Installation
  1. Navigate to the server modules-directory
  2. Open up git-bash there
  3. Execute the command "git clone https://github.com/justin-kaufmann/mod-changeablespawnrates.git"
  4. Open up cmake and generate the project
  5. Rebuild the project via Visual Studio
  6. Execute the sql-files in modules data directory
  7. Configure the module
  8. Done!

How does this mod work?
  1. It scales the spawntimes with the spawntime-values of the duplicated creature-table (custom_creatures) multiplied by a factor which is set by the user or which is dynamically calculated.
  2. It also changes the spawntimes in servers-memory, which means that it works while the servers-runtime 

Fixed spawnfactor:
  e.g.: the factor is set to 0.5, then all spawntimes (except instance/raid-mobs) will be halfed

Dynamic spawnfactor: 
  e.g.: the factor gets calculated by the count of players. 
        This means more players -> faster respawntimes, lesser players -> slower respawntimes
          1. if the active players count is 1, the rate will be at 1.0
          2. if the active players count is greater than 1 and smaller than 50, the rate will get decreased like this (rate = 1.0 - activeplayer-count * 0.01)
          3. At rate 0.5 for every player which logs in the rate will get decreased for 1/5 of 0,1 till the userdefined minimum (default: 0.5) or the maximum minimum is reached (0.01).
          4. If the calculated spawntime of a creature in the db would be lesser than 5 secs it will get to 5 secs, to avoid instant respawning
