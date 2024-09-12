/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#include "ChangeableSpawnrates.h"

void CSRConfigurator::ConfigureSpawnrates()
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
                "ELSE cc.spawntimesecs * {} END "
                "WHERE cc.map NOT IN (SELECT map FROM instance_template)",
                NULL, NULL,
                CSR_RespawnMult, CSR_Minimum_Spawntime, CSR_Minimum_Spawntime,
                CSR_RespawnMult);
        }

        QueryResult xResult = WorldDatabase.Query("SELECT guid, spawntimesecs FROM custom_creature c "
            "WHERE c.map NOT IN (SELECT map FROM instance_template)");
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
                            if (xMapInstanceID > NULL) continue;
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
float CSRConfigurator::CalculateFactorUpperHalf(int ACountPlayer)
{
    // Playercount: 1 <-> Rate: 1.0
    if (ACountPlayer <= ONE)
    {
        return DEF_RESPAWNMULT;
    }

    // Rate <= 0.5 <-> CalcFactorLowerHalf
    if (CSR_RespawnMult <= DEF_RESPAWNMULT_HALF)
    {
        return CalculateFactorLowerHalf(ACountPlayer);
    }

    // Rate > 0.5 <-> xFactor = 1.0 - ((CountPlayer - ONE) * 0.01)
    float xFactor = DEF_RESPAWNMULT - ((ACountPlayer - ONE) * DEF_RESPAWNMULT_SUBTRACTOR);
        
    if (CSR_DynamicSpawnrates_IgnoreMinimum)
    {
        return (xFactor < DEF_RESPAWNMULT_MAX_MINIMUM) ? DEF_RESPAWNMULT_MAX_MINIMUM : xFactor;
    }
    return (xFactor < CSR_DynamicSpawnrates_MinMult) ? CSR_DynamicSpawnrates_MinMult : xFactor;
}
float CSRConfigurator::CalculateFactorLowerHalf(int ACountPlayer)
{
    if (CSR_RespawnMult > DEF_RESPAWNMULT_HALF)
    {
        return CalculateFactorUpperHalf(ACountPlayer);
    }
   
    float xFactor = DEF_RESPAWNMULT_HALF - ((ACountPlayer - FIFTYONE) * DEF_RESPAWNMULT_SUBTRACTOR_UP50);

    if (CSR_DynamicSpawnrates_IgnoreMinimum)
    {
        return (xFactor < DEF_RESPAWNMULT_MAX_MINIMUM) ? DEF_RESPAWNMULT_MAX_MINIMUM : xFactor;
    }
    return (xFactor < CSR_DynamicSpawnrates_MinMult) ? CSR_DynamicSpawnrates_MinMult : xFactor;
    
}
void CSRConfigurator::ConfigureDynamicSpawnrates(int AAddPlayer = NULL, int ASubPlayer = NULL)
{
    if (CSR_Enable && CSR_DynamicSpawnrates_Enable)
    {
        if (sWorld->IsShuttingDown())
        {
            CSR_RespawnMult = DEF_RESPAWNMULT;
            return;
        }

        if ((CSR_DynamicSpawnrates_IgnoreMinimum && CSR_RespawnMult == DEF_RESPAWNMULT_MAX_MINIMUM) or
            (!CSR_DynamicSpawnrates_IgnoreMinimum && CSR_RespawnMult == CSR_DynamicSpawnrates_MinMult))
        {
            return;
        }

        uint32 xCountOnlinePlayers = sWorld->GetPlayerCount();

        bool xContinue = true;
        if (ASubPlayer > NULL)
        {
            xCountOnlinePlayers -= ASubPlayer;
            if (xCountOnlinePlayers == FIFTY)
            {
                CSR_RespawnMult = DEF_RESPAWNMULT_HALF + DEF_RESPAWNMULT_SUBTRACTOR;
                xContinue = false;
            }
        }

        if (xContinue)
        {
            if (xCountOnlinePlayers <= ONE)
            {
                CSR_RespawnMult = DEF_RESPAWNMULT;
            }
            else if (CSR_RespawnMult > DEF_RESPAWNMULT_HALF)
            {
                CSR_RespawnMult = CalculateFactorUpperHalf(xCountOnlinePlayers);
            }
            else if (CSR_RespawnMult <= DEF_RESPAWNMULT_HALF)
            {
                CSR_RespawnMult = CalculateFactorLowerHalf(xCountOnlinePlayers);
            }
        }
        CSR_RespawnMult = std::round(CSR_RespawnMult * THOUSAND) / THOUSAND;

        LOG_INFO("server.loading", "mod-ChangeableSpawnRates: PlayerCount: {}, Spawnrate: {}", xCountOnlinePlayers, CSR_RespawnMult);

        CSRConfigurator::ConfigureSpawnrates();
    }
}
bool CSRConfigurator::CustomCreaturesHasMissingEntrys()
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
bool CSRConfigurator::CreatureNeedsUpdate()
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

void CSRConfigLoader::OnBeforeConfigLoad(bool /*reload*/)
{
    LOG_INFO("ChangeableSpawnrates", "Load Config");
    LoadConfig();
}
void CSRConfigLoader::OnAfterConfigLoad(bool /*reload*/)
{
    if (CSR_Enable && !CSR_DynamicSpawnrates_Enable) CSRConfigurator::ConfigureSpawnrates();
}
void CSRConfigLoader::LoadConfig()
{
    CSR_Enable = sConfigMgr->GetOption<bool>("Module.Enable", true);
    CSR_Announce_Enable = sConfigMgr->GetOption<bool>("Module.Announce.Enable", true);
    CSR_DynamicSpawnrates_Enable = sConfigMgr->GetOption<bool>("Module.DynamicSpawnrates.Enable", true);
    CSR_DynamicSpawnrates_IgnoreMinimum = sConfigMgr->GetOption<bool>("Module.DynamicSpawnrates.IgnoreMinimum", true);
    CSR_DynamicSpawnrates_MinMult = sConfigMgr->GetOption<float>("Module.DynamicSpawnrates.Minimum", DEF_RESPAWNMULT_HALF);
    CSR_RespawnMult = sConfigMgr->GetOption<float>("Module.RespawnMultiplicator", DEF_RESPAWNMULT);
    CSR_Minimum_Spawntime = sConfigMgr->GetOption<float>("Module.MinimumSpawntime", DEF_MINIMUM_SPAWNTIME);
}

void CSRPlayer::OnLogin(Player* APlayer)
{
    if (APlayer && CSR_Enable)
    {
        if (CSR_Announce_Enable)
        {
            WorldSession* xPlayerSession = APlayer->GetSession();
            if (xPlayerSession) ChatHandler(xPlayerSession).SendSysMessage("This server is running the |cff4CFF00Changeable Spawnrates |rmodule.");
        }

        if (CSR_DynamicSpawnrates_Enable) CSRConfigurator::ConfigureDynamicSpawnrates(ONE, NULL);

        AnnounceSpawnrate(APlayer);
    }
}
void CSRPlayer::OnLogout(Player* APlayer)
{
    if (APlayer && CSR_Enable && CSR_DynamicSpawnrates_Enable) CSRConfigurator::ConfigureDynamicSpawnrates(NULL, ONE);
}
void CSRPlayer::AnnounceSpawnrate(Player* APlayer)
{
    if (APlayer && CSR_Enable && CSR_DynamicSpawnrates_Enable)
    {
        WorldSession* xPlayerSession = APlayer->GetSession();
        if (xPlayerSession) ChatHandler(xPlayerSession).SendSysMessage("Respawnrate-factor: " + std::to_string(CSR_RespawnMult * HUNDRED));
    }
}
