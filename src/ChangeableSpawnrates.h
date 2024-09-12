#include "ScriptMgr.h"
#include "Player.h"
#include "Config.h"
#include "Chat.h"
#include <MapMgr.h>

bool CSR_Enable = 1;
bool CSR_Announce_Enable = 1;
bool CSR_DynamicSpawnrates_Enable = 0;
bool CSR_DynamicSpawnrates_IgnoreMinimum = 0;
float CSR_DynamicSpawnrates_MinMult = 0.5f;
float CSR_RespawnMult = 1.0f;
float CSR_Minimum_Spawntime = 5.0f;

const float DEF_RESPAWNMULT_MAX_MINIMUM = 0.01f;
const float DEF_RESPAWNMULT_SUBTRACTOR = 0.01f;
const float DEF_RESPAWNMULT_SUBTRACTOR_UP50 = 0.002f;
const float DEF_RESPAWNMULT_HALF = 0.5f;
const float DEF_RESPAWNMULT = 1.0f;
const float DEF_MINIMUM_SPAWNTIME = 5.0f;

const int ONE = 1;
const int FIVE = 5;
const int FIFTY = 50;
const int FIFTYONE = 51;
const int HUNDRED = 100;
const int THREEHUNDRED = 300;
const int THOUSAND = 1000;

class CSRConfigurator
{
public:
    static bool CSR_DynamicSpawnrates_HalfMult_Reached;
    static void ConfigureSpawnrates();
    static void ConfigureDynamicSpawnrates(int /*AAddPlayer = NULL*/, int /*ASubPlayer = NULL*/);
private:
    static bool CustomCreaturesHasMissingEntrys();
    static bool CreatureNeedsUpdate();
    static float CalculateFactorLowerHalf(int ACountPlayer);
    static float CalculateFactorUpperHalf(int ACountPlayer);
};
bool CSRConfigurator::CSR_DynamicSpawnrates_HalfMult_Reached(false);

class CSRConfigLoader : public WorldScript
{
public:
    CSRConfigLoader() : WorldScript("ChangeableSpawnratesConfig") {}
    void OnBeforeConfigLoad(bool /*reload*/) override;
    void OnAfterConfigLoad(bool /*reload*/) override;
private:
    void LoadConfig();
};

class CSRPlayer : public PlayerScript
{
public:
    CSRPlayer() : PlayerScript("ChangeableSpawnratesPlayer") { }
    void OnLogin(Player* APlayer) override;
    void OnLogout(Player* APlayer) override;
private:
    void AnnounceSpawnrate(Player* /*player*/);
};

void AddChangeableSpawnRatesScripts()
{
    new CSRConfigurator();
    new CSRConfigLoader();
    new CSRPlayer();
}
