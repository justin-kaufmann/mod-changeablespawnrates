# mod-changeablespawnrates

This WoW-Azerothcore-Mod allows ac-server-administrators to change spawntimes based on a user-defined or dynamically calculated playerbased factor

## Installation
  1. Navigate to the server modules-directory
  2. Open up git-bash there
  3. Execute the command `git clone https://github.com/justin-kaufmann/mod-changeablespawnrates.git`
  4. Open up cmake and generate the project
  5. Rebuild the project via Visual Studio
  6. Execute the sql-files in modules data directory
  7. Configure the module
  8. Done!

## How does this mod work?
  -  It scales the spawntimes with the spawntime-values of the duplicated creature-table (custom_creatures) multiplied by a factor which is set by the user or which is dynamically calculated.
  -  It also changes the spawntimes in servers-memory, which means that it works while the servers-runtime 

### Fixed spawnfactor:
  e.g.: the factor is set to 0.5, then all spawntimes (except instance/raid-mobs) will be halfed

### Dynamic spawnfactor:   
  the factor gets calculated by the count of players.
  
  This means more players -> faster respawntimes, lesser players -> slower respawntimes
  
-  if the active players count is 1, the rate will be at 1.0
-  if the active players count is greater than 1 and smaller than 50, the rate will get decreased like this $(rate = 1.0 - activeplayer-count * 0.01)$
-  At rate 0.5 for every player which logs in the rate will get decreased for 1/5 of 0,01 till the userdefined minimum (default: 0.5) or the maximum minimum is reached (0.01).
-  If the calculated spawntime of a creature in the db would be lesser than the minimum spawntime set in the config file (default: 5 secs) it will get to the minimum spawntime (e.g. 5 secs), to avoid instant respawning.
