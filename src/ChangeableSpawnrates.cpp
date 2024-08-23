/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#include "ChangeableSpawnrates.h"

void ChangeableSpawnRatesConfigurator::ConfigureSpawnrates()
{
    if (CSR_Enable)
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
                "WHEN cc.spawntimesecs = {} THEN {} "
                "WHEN cc.spawntimesecs * {} < {} THEN {} "
                "ELSE cc.spawntimesecs * {} END",
                NULL, NULL,
                CSR_RespawnMult, CSR_Minimum_Spawntime, CSR_Minimum_Spawntime,
                CSR_RespawnMult);
        }

        QueryResult xResult = WorldDatabase.Query("SELECT guid, spawntimesecs FROM custom_creature");
        if (xResult)
        {
            do
            {
                Field* xFields = xResult->Fetch();
                if (xFields)
                {
                    uint32 xCreatureGuid = xFields[NULL].Get<uint32>();
                    uint32 xCreatureNewRespawnDelay = xFields[ONE].Get<uint32>();
                    CreatureData const* xCreatureData = sObjectMgr->GetCreatureData(xCreatureGuid);
                    if (xCreatureData)
                    {
                        Map* xMap = sMapMgr->FindBaseMap(xCreatureData->mapid);
                        if (xMap)
                        {
                            int xMapInstanceID = xMap->GetInstanceId();
                            if (xMapInstanceID > NULL)
                            {
                                continue;
                            }
                            else
                            {
                                Creature* xCreature = xMap->GetCreature(ObjectGuid(HighGuid::Unit, xCreatureGuid));
                                if (xCreature)
                                {
                                    xCreature->SetRespawnDelay(xCreatureNewRespawnDelay * CSR_RespawnMult);
                                }
                            }
                        }
                        const_cast<CreatureData*>(xCreatureData)->spawntimesecs = xCreatureNewRespawnDelay * CSR_RespawnMult;
                    }
                }
            } while (xResult->NextRow());
        }
    }
}
void ChangeableSpawnRatesConfigurator::ConfigureDynamicSpawnrates(int AAddPlayer = NULL, int ASubPlayer = NULL)
{
    if (CSR_Enable && CSR_DynamicSpawnrates_Enable)
    {
        uint32 xCountOnlinePlayers = sWorld->GetPlayerCount();

        if (ASubPlayer > NULL)
        {
            xCountOnlinePlayers -= ASubPlayer;
        }

        if (xCountOnlinePlayers == ONE)
        {
            CSR_RespawnMult = DEF_RESPAWNMULT;
        }
        else if (xCountOnlinePlayers > ONE)
        {
            if ((!CSR_DynamicSpawnrates_HalfMult_Reached) or (CSR_RespawnMult > DEF_RESPAWNMULT_HALF))
            {
                CSR_RespawnMult = DEF_RESPAWNMULT - (DEF_RESPAWNMULT_SUBTRACTOR * xCountOnlinePlayers) + (ONE * DEF_RESPAWNMULT_SUBTRACTOR);

                CSR_DynamicSpawnrates_HalfMult_Reached = CSR_RespawnMult <= DEF_RESPAWNMULT_HALF;
            }
            else
            {
                if (AAddPlayer > NULL)
                {
                    CSR_RespawnMult -= (DEF_RESPAWNMULT_SUBTRACTOR * AAddPlayer) / FIVE;
                }

                if (ASubPlayer > NULL)
                {
                    if (CSR_RespawnMult <= DEF_RESPAWNMULT_HALF)
                    {
                        CSR_RespawnMult += (DEF_RESPAWNMULT_SUBTRACTOR * ASubPlayer) / FIVE;
                    };
                }
            }

            if (!CSR_DynamicSpawnrates_IgnoreMinimum && (CSR_RespawnMult < CSR_DynamicSpawnrates_MinMult))
            {
                CSR_RespawnMult = CSR_DynamicSpawnrates_MinMult;
            }
            else if (CSR_DynamicSpawnrates_IgnoreMinimum && (CSR_RespawnMult < DEF_RESPAWNMULT_MAX_MINIMUM))
            {
                CSR_RespawnMult = DEF_RESPAWNMULT_MAX_MINIMUM;
            }
        }

        CSR_RespawnMult = std::round(CSR_RespawnMult * THOUSAND) / THOUSAND;

        LOG_INFO("server.loading", "mod-ChangeableSpawnRates: PlayerCount: {}, Spawnrate: {}", xCountOnlinePlayers, CSR_RespawnMult);

        ChangeableSpawnRatesConfigurator::ConfigureSpawnrates();
    }
}
bool ChangeableSpawnRatesConfigurator::CustomCreaturesHasMissingEntrys()
{
    QueryResult xQryResult = WorldDatabase.Query(
        "SELECT COUNT(*) FROM creature c "
        "LEFT JOIN custom_creature cc ON c.guid = cc.guid "
        "WHERE cc.guid IS NULL");

    if (xQryResult)
    {
        Field* xFields = xQryResult->Fetch();
        if (xFields)
        {
            uint32_t xCount = xFields[NULL].Get<uint32_t>();
            return xCount > NULL;
        }
    }

    return false;
}
bool ChangeableSpawnRatesConfigurator::CreatureNeedsUpdate()
{
    QueryResult xQryResult = WorldDatabase.Query("SELECT COUNT(*) FROM creature c "
        "JOIN custom_creature cc ON c.guid = cc.guid "
        "WHERE c.spawntimesecs <> cc.spawntimesecs * {}", CSR_RespawnMult);

    if (xQryResult) {
        Field* xFields = xQryResult->Fetch();
        if (xFields)
        {
            uint32_t xCount = xFields[NULL].Get<uint32_t>();
            return xCount > NULL;
        }
    }

    return false;
}

void ChangeableSpawnratesConfigLoader::OnBeforeConfigLoad(bool /*reload*/)
{
    LOG_INFO("ChangeableSpawnrates", "Load Config");
    LoadConfig();
}
void ChangeableSpawnratesConfigLoader::OnAfterConfigLoad(bool /*reload*/)
{
    if (CSR_Enable && !CSR_DynamicSpawnrates_Enable)
    {
        ChangeableSpawnRatesConfigurator::ConfigureSpawnrates();
    }
}
void ChangeableSpawnratesConfigLoader::LoadConfig()
{
    CSR_Enable = sConfigMgr->GetOption<bool>("Module.Enable", true);
    CSR_Announce_Enable = sConfigMgr->GetOption<bool>("Module.Announce.Enable", true);
    CSR_DynamicSpawnrates_Enable = sConfigMgr->GetOption<bool>("Module.DynamicSpawnrates.Enable", true);
    CSR_DynamicSpawnrates_IgnoreMinimum = sConfigMgr->GetOption<bool>("Module.DynamicSpawnrates.IgnoreMinimum", true);
    CSR_DynamicSpawnrates_MinMult = sConfigMgr->GetOption<float>("Module.DynamicSpawnrates.Minimum", DEF_RESPAWNMULT_HALF);
    CSR_RespawnMult = sConfigMgr->GetOption<float>("Module.RespawnMultiplicator", DEF_RESPAWNMULT);
    CSR_Minimum_Spawntime = sConfigMgr->GetOption<float>("Module.MinimumSpawntime", DEF_MINIMUM_SPAWNTIME);
}

void ChangeableSpawnRatesPlayer::OnLogin(Player* player)
{
    if (player && CSR_Enable)
    {
        // Announce Module
        if (CSR_Announce_Enable)
        {
            WorldSession* xPlayerSession = player->GetSession();
            if (xPlayerSession)
            {
                ChatHandler(xPlayerSession).SendSysMessage("This server is running the |cff4CFF00Changeable Spawnrates |rmodule.");
            }
        }

        // Configure dynamic spawnrates
        if (CSR_DynamicSpawnrates_Enable)
        {
            ChangeableSpawnRatesConfigurator::ConfigureDynamicSpawnrates(ONE, NULL);

        }

        AnnounceSpawnrate(player);
    }
}
void ChangeableSpawnRatesPlayer::OnLogout(Player* player)
{
    // Configure dynamic spawnrates
    if (player && CSR_Enable && CSR_DynamicSpawnrates_Enable)
    {
        ChangeableSpawnRatesConfigurator::ConfigureDynamicSpawnrates(NULL, ONE);
    }
}
void ChangeableSpawnRatesPlayer::AnnounceSpawnrate(Player* player)
{
    if (player && CSR_Enable && CSR_DynamicSpawnrates_Enable)
    {
        WorldSession* xPlayerSession = player->GetSession();
        if (xPlayerSession)
        {
            ChatHandler(xPlayerSession).SendSysMessage("Respawnrate-factor: " + std::to_string(CSR_RespawnMult * HUNDRED));
        }
    }
}
