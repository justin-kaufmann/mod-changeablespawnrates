#include "ScriptMgr.h"
#include "Player.h"
#include "Config.h"
#include "Chat.h"
#include <MapMgr.h>

/* Module-Settings */

bool CSR_Enable = 1;
bool CSR_Announce_Enable = 1;
bool CSR_DynamicSpawnrates_Enable = 0;
bool CSR_DynamicSpawnrates_IgnoreMinimum = 0;
float CSR_DynamicSpawnrates_MinMult = 0.5f;
float CSR_RespawnMult = 1.0f;
float CSR_Minimum_Spawntime = 5.0f;

/* Constants */

const float DEF_RESPAWNMULT_MAX_MINIMUM = 0.01f;
const float DEF_RESPAWNMULT_SUBTRACTOR = 0.01f;
const float DEF_RESPAWNMULT_HALF = 0.5f;
const float DEF_RESPAWNMULT = 1.0f;
const float DEF_MINIMUM_SPAWNTIME = 5.0f;

const int ONE = 1;
const int FIVE = 5;
const int HUNDRED = 100;
const int THOUSAND = 1000;

// Sets the rates
class ChangeableSpawnRatesConfigurator
{
public:
    static bool CSR_DynamicSpawnrates_HalfMult_Reached;
    static void ConfigureSpawnrates();
    static void ConfigureDynamicSpawnrates(int /*AAddPlayer = NULL*/, int /*ASubPlayer = NULL*/);
private:
    // Check for missing entrys in custom creatures
    static bool CustomCreaturesHasMissingEntrys();
    // Check if the table creature needs an update
    static bool CreatureNeedsUpdate();
};
bool ChangeableSpawnRatesConfigurator::CSR_DynamicSpawnrates_HalfMult_Reached(false);

// Loads the configuration 
class ChangeableSpawnratesConfigLoader : public WorldScript
{
public:
    ChangeableSpawnratesConfigLoader() : WorldScript("ChangeableSpawnratesConfig") {}
    void OnBeforeConfigLoad(bool /*reload*/) override;
    void OnAfterConfigLoad(bool /*reload*/) override;
private:
    void LoadConfig();
};

// Handles default player events
class ChangeableSpawnRatesPlayer : public PlayerScript
{
public:
    ChangeableSpawnRatesPlayer() : PlayerScript("ChangeableSpawnratesPlayer") { }
    void OnLogin(Player* /*player*/) override;
    void OnLogout(Player* /*player*/) override;
private:
    void AnnounceSpawnrate(Player* /*player*/);
};

// Add scripts
void AddChangeableSpawnRatesScripts()
{
    new ChangeableSpawnRatesConfigurator();
    new ChangeableSpawnratesConfigLoader();
    new ChangeableSpawnRatesPlayer();
}
