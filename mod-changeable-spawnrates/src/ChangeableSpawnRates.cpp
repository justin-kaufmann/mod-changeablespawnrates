#include "ScriptMgr.h"
#include "Player.h"
#include "Config.h"
#include "Chat.h"
#include "Log.h"
#include "Unit.h"
#include <unordered_map>
#include "Map.h"
#include "Creature.h"
#include <MapMgr.h>

// Settings
bool ModuleEnabled = 1;
bool ModuleAnnounceEnabled = 1;
bool ModuleDynamicSpawnratesEnabled = 0;
bool ModuleDynamicSpawnratesIgnoreMinimum = 0;
bool ModuleDynamicSpawnratesHalfReached = 0;
float ModuleRespawnMultiplicator = 1.0f;
float ModuleDynamicSpawnratesMinimum = 0.5f;
int ModuleDynamicSpawnratesHalfReachedPlayerCount = 0;

// Constants
const float DefaultRespawnMultiplicator = 1.0f;
const float DefaultRespawnMultiplicatorHalf = 0.5f;
const float DefaultRespawnMultiplicatorMaxMinimum = 0.01f;
const float DefaultRespawnMultiplicatorSubtractor = 0.01f;
const int ZeroActivePlayers = 0;
const int MinActivePlayers = 1;
const int FivePlayers = 5;


class ChangeableSpawnratesConfig : public WorldScript
{

public:
	ChangeableSpawnratesConfig() : WorldScript("ChangeableSpawnratesConfig") {}

    // Load Configuration Settings 1/2
    void OnBeforeConfigLoad(bool /*reload*/) override
    {
        SetInitialWorldSettings();
    }

    // Load Configuration Settings 2/2
    void SetInitialWorldSettings()
    {
        ModuleEnabled = sConfigMgr->GetOption<bool>("Module.Enabled", 1);
        ModuleAnnounceEnabled = sConfigMgr->GetOption<bool>("Module.Announce.Enabled", 1 && ModuleEnabled);
        ModuleDynamicSpawnratesEnabled = sConfigMgr->GetOption<bool>("Module.DynamicSpawnrates.Enabled", 1 && ModuleEnabled);
        ModuleDynamicSpawnratesIgnoreMinimum = sConfigMgr->GetOption<bool>("Module.DynamicSpawnrates.IgnoreMinimum", 1 && ModuleEnabled && ModuleDynamicSpawnratesEnabled);
        ModuleDynamicSpawnratesMinimum = sConfigMgr->GetOption<float>("Module.DynamicSpawnrates.Minimum", 0.5f);
        ModuleRespawnMultiplicator = sConfigMgr->GetOption<float>("Module.RespawnMultiplicator", 1.0f);
    }

    // Configure spawnrates 1/2
    void OnAfterConfigLoad(bool /*reload*/) override
    {
        if (ModuleEnabled && !ModuleDynamicSpawnratesEnabled)
        {
            ConfigureSpawnrates();
        }
    }

    // Configure spawnrates 2/2
    static void ConfigureSpawnrates()
    {
        if (ModuleEnabled)
        {
            if (CustomCreaturesHasMissingEntrys())
            {
                WorldDatabase.Execute(
                    "INSERT INTO custom_creature (guid, id1, id2, id3, map, zoneId, areaId, spawnMask, phaseMask, equipment_id, position_x, position_y, position_z,"
                    "orientation, spawntimesecs, wander_distance, currentwaypoint, curhealth, curmana, MovementType, npcflag, unit_flags, dynamicflags, ScriptName,"
                    "VerifiedBuild, CreateObject, Comment) "
                    "SELECT c.guid, c.id1, c.id2, c.id3, c.map, c.zoneId, c.areaId, c.spawnMask, c.phaseMask, c.equipment_id, c.position_x, c.position_y, c.position_z,"
                    "c.orientation, c.spawntimesecs, c.wander_distance, c.currentwaypoint, c.curhealth, c.curmana, c.MovementType, c.npcflag, c.unit_flags, c.dynamicflags,"
                    "c.ScriptName, c.VerifiedBuild, c.CreateObject, c.Comment "
                    "FROM creature c "
                    "LEFT JOIN custom_creature cc ON c.guid = cc.guid "
                    "WHERE cc.guid IS NULL");
            }

            if (CreatureNeedsUpdate())
            {
                WorldDatabase.Execute(
                    "UPDATE creature c "
                    "JOIN custom_creature cc ON c.guid = cc.guid "
                    "SET c.spawntimesecs = CASE "
                    "WHEN cc.spawntimesecs = 0 THEN 0 "
                    "WHEN cc.spawntimesecs * {} < 5 THEN 5 "
                    "ELSE cc.spawntimesecs * {} END",
                    ModuleRespawnMultiplicator, ModuleRespawnMultiplicator);
            }

            if (QueryResult result = WorldDatabase.Query("SELECT guid, spawntimesecs FROM custom_creature"))
            {
                do
                {
                    Field* XFields = result->Fetch();
                    uint32 XCreatureGuid = XFields[0].Get<uint32>();
                    uint32 XCreatureNewRespawnDelay = XFields[1].Get<uint32>();
                    
                    if (CreatureData const* creatureData = sObjectMgr->GetCreatureData(XCreatureGuid))
                    {
                        if (Map* map = sMapMgr->FindBaseMap(creatureData->mapid))
                        {
                            if (int XMapInstanceID = map->GetInstanceId() > 0)
                            {
                                continue;
                            }
                            else
                            {
                                if (Creature* creature = map->GetCreature(ObjectGuid(HighGuid::Unit, XCreatureGuid)))
                                {
                                    creature->SetRespawnDelay(XCreatureNewRespawnDelay * ModuleRespawnMultiplicator);
                                }
                            }
                        }
                        const_cast<CreatureData*>(creatureData)->spawntimesecs = XCreatureNewRespawnDelay * ModuleRespawnMultiplicator;
                    }
                } while (result->NextRow());
            }
        }
    }

    // Configure dynamic spawnrates
    static void ConfigureDynamicSpawnrates(int AAddPlayer = NULL, int ASubPlayer = NULL)
    {
        if (ModuleEnabled)
        {
            if (ModuleDynamicSpawnratesEnabled)
            {
                uint32 XCountOnlinePlayers = sWorld->GetPlayerCount();

                if (ASubPlayer > NULL)
                {
                    XCountOnlinePlayers -= ASubPlayer;
                }

                if (XCountOnlinePlayers == MinActivePlayers)
                {
                    ModuleRespawnMultiplicator = DefaultRespawnMultiplicator;
                }
                else if (XCountOnlinePlayers > MinActivePlayers)
                {
                    if ((!ModuleDynamicSpawnratesHalfReached) or (ModuleRespawnMultiplicator > DefaultRespawnMultiplicatorHalf))
                    {
                        ModuleRespawnMultiplicator = DefaultRespawnMultiplicator - (DefaultRespawnMultiplicatorSubtractor * XCountOnlinePlayers) + (MinActivePlayers * DefaultRespawnMultiplicatorSubtractor);

                        ModuleDynamicSpawnratesHalfReached = ModuleRespawnMultiplicator <= DefaultRespawnMultiplicatorHalf;
                    }
                    else
                    {
                        if (AAddPlayer > 0)
                        {
                            ModuleRespawnMultiplicator -= (DefaultRespawnMultiplicatorSubtractor * AAddPlayer) / FivePlayers;
                        }

                        if (ASubPlayer > 0)
                        {
                            if (ModuleRespawnMultiplicator <= DefaultRespawnMultiplicatorHalf)
                            {
                                ModuleRespawnMultiplicator += (DefaultRespawnMultiplicatorSubtractor * ASubPlayer) / FivePlayers;
                            };
                        }
                    }

                    if (!ModuleDynamicSpawnratesIgnoreMinimum)
                    {
                        if (ModuleRespawnMultiplicator < ModuleDynamicSpawnratesMinimum)
                        {
                            ModuleRespawnMultiplicator = ModuleDynamicSpawnratesMinimum;
                        }
                    }
                    else if (ModuleRespawnMultiplicator < DefaultRespawnMultiplicatorMaxMinimum)
                    {
                        ModuleRespawnMultiplicator = DefaultRespawnMultiplicatorMaxMinimum;
                    }
                }
                ModuleRespawnMultiplicator = std::round(ModuleRespawnMultiplicator * 1000) / 1000;

                LOG_INFO("server.loading", "mod-ChangeableSpawnRates: PlayerCount: {}, Spawnrate: {}", XCountOnlinePlayers, ModuleRespawnMultiplicator);
   
                ChangeableSpawnratesConfig::ConfigureSpawnrates();
            }
        }
    }

private:
    // Check for missing entrys in custom creatures
    static bool CustomCreaturesHasMissingEntrys()
    {
        QueryResult result = WorldDatabase.Query(
            "SELECT COUNT(*) FROM creature c "
            "LEFT JOIN custom_creature cc ON c.guid = cc.guid "
            "WHERE cc.guid IS NULL");
        
        if (result)
        {
            Field* fields = result->Fetch();
            uint32_t count = fields[0].Get<uint32_t>();
            return count > 0;
        }

        return false;
    }

    // Check if the table creature needs an update
    static bool CreatureNeedsUpdate()
    {
        QueryResult result = WorldDatabase.Query("SELECT COUNT(*) FROM creature c "
            "JOIN custom_creature cc ON c.guid = cc.guid "
            "WHERE c.spawntimesecs <> cc.spawntimesecs * {}", ModuleRespawnMultiplicator);

        if (result) {
            Field* fields = result->Fetch();
            uint32_t count = fields[0].Get<uint32_t>();
            return count > 0;
        }

        return false;
    }
};

class MyPlayer : public PlayerScript
{
public:
    MyPlayer() : PlayerScript("MyPlayer") { }

    void OnLogin(Player* player) override
    {
        if (ModuleEnabled)
        {
            // Announce Module
            if (ModuleAnnounceEnabled)
            {
                ChatHandler(player->GetSession()).SendSysMessage("This server is running the |cff4CFF00Changeable Spawnrates |rmodule.");
            }

            // Configure dynamic spawnrates
            if (ModuleDynamicSpawnratesEnabled)
            {
                ChangeableSpawnratesConfig::ConfigureDynamicSpawnrates(1, 0);
                AnnounceSpawnrate(player);
            }
        }
    }

    void OnLogout(Player* player) override
    {
        // Configure dynamic spawnrates
        if (ModuleEnabled)
        {
            if (ModuleDynamicSpawnratesEnabled)
            {
                ChangeableSpawnratesConfig::ConfigureDynamicSpawnrates(0, 1);
            }
        }
    }

    static void AnnounceSpawnrate(Player* player)
    {
        if (ModuleEnabled)
        {
            if (ModuleDynamicSpawnratesEnabled)
            {
                ChatHandler(player->GetSession()).SendSysMessage("Respawnrate-factor: " + std::to_string(ModuleRespawnMultiplicator * 100));
            }
        }
    }
};

// Add scripts
void AddChangeableSpawnRatesScripts()
{
    new ChangeableSpawnratesConfig();
    new MyPlayer();
}

