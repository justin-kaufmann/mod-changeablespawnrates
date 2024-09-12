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
  The dynamic spawn rate calculation follows these rules:

  - If the active players count is 0 or 1, the spawn rate will be set to 1.0.
  - For each additional active player, the spawn rate will decrease by 0.01 until it reaches 0.5.
    Conversely, for every player that logs out, the spawn rate will be increased by 0.01.
  - Once the spawn rate reaches 0.5, for every player that logs in, the spawn rate will be decreased by 0.002.
    Conversely, for every player that logs out, the spawn rate will be increased by 0.002.
  - If the calculated spawn time of a creature in the database would be less than the minimum spawn time set in the configuration file (default: 5 seconds), the spawn time will be set to the minimum to avoid instant respawning.
  - The minimum spawn rate can be configured by the user, with the default value being 0.5.
    this is not configured, the lowest possible spawn rate will be 0.01.

This spawn rate calculation mechanism ensures that the spawn rate is dynamically adjusted based on the number of active players in the game, with a gradual decrease in the rate as more players join. Once the rate reaches a minimum threshold, further adjustments are made in smaller increments, both when players log in and log out, to maintain a balanced gameplay experience. Additionally, the system enforces a configurable minimum spawn time to prevent instant respawning of creatures (default 5 secs).
