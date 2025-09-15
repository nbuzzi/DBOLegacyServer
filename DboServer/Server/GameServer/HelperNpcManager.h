#ifndef __INC_DBOG_HELPER_NPC_MANAGER_H__
#define __INC_DBOG_HELPER_NPC_MANAGER_H__

#include "NtlSharedDef.h"
#include "NtlString.h"
#include <set>
#include <unordered_map>
#include <vector>

class CNtlIniFile;
class CWorld;
class CPlayer;

struct sHELPER_NPC_CONFIG
{
    bool   bEnabled = true;
    bool   bAllowUltimate = true;
    bool   bAllowBattleDungeon = true;
    BYTE   byMinPartySizeToAvoidSpawn = 5; // spawn if party size < this
    TBLIDX primaryNpcTblidx = 4754102;     // bibra
    TBLIDX fallbackNpcTblidx = 4754101;    // bibra 2
    // Optional: use a MOB as helper (e.g., healer mob) instead of NPC
    bool   bUseMobAsHelper = false;        // when true, spawn MOB below instead of NPC
    TBLIDX helperMobTblidx = 25734101;     // default healer MOB tblidx
    float  fSpawnOffset = 2.0f;
    bool   bFollowLeader = true;
    bool   bAssistLeaderTarget = true;     // if true, helper will attack leader's target
    WORD   wHealLpThresholdOverride = 0;   // optional percent (0=use skill default); when linked to PC
    float  fDamageMultiplier = 1.0f;       // multiply helper's offense (physical/energy)
    float  fMoveSpeedMultiplier = 1.0f;    // multiply helper's run/air speeds
    WORD   wAttackSpeedPercent = 0;        // add attack speed percent (e.g., 20 = +20%)
    WORD   wEpRegenPercent = 0;            // add EP regen percent (e.g., 100 = +100%)
    bool   bInvincibleHelper = false;      // when true, mark helper as invincible

    // Optional: list of buff skills to add at spawn (comma-separated in INI)
    std::vector<TBLIDX> vBuffSkills;       // e.g., 420141,420142
    BYTE   buffBasis = 5;                  // default TIME basis
    WORD   buffLP = 0;                     // ignored for TIME basis; used for LP/Give
    WORD   buffTime = 10;                  // seconds for TIME basis

    // Optional: force-add a specific skill to the helper at spawn, useful if the mob table lacks heals
    TBLIDX forcedSkillTblidx = INVALID_TBLIDX; // e.g., a heal/buff skill id
    BYTE   forcedSkillBasis = 4;                // default to Give (4). 3=LP,4=Give,5=Time,6=Ring,7=OnlyLP
    WORD   forcedSkillLP = 70;                  // LP threshold for LP/Give conditions (percent)
    WORD   forcedSkillTime = 5;                 // seconds for time-based condition
};

class CHelperNpcManager
{
public:
    static CHelperNpcManager* Instance();

    // Load [HELPER_NPC] section from the given INI file (optional, uses defaults if missing)
    bool LoadConfig(CNtlIniFile& file);

    const sHELPER_NPC_CONFIG& GetConfig() const { return m_config; }

    // Spawn helper on dungeon creation if needed. Call right after party is moved to the new world
    bool SpawnHelperIfNeededForDungeon(CPlayer* pLeader, CWorld* pWorld, bool bIsUltimateDungeon);

    // Cleanup hook: call when the dungeon/world is being destroyed
    void OnWorldDestroyed(CWorld* pWorld);

    // When the leader starts or updates an attack target, notify helper to assist
    void OnLeaderAttackTarget(CPlayer* pLeader, HOBJECT hTarget);
    // When the leader ends attack, ensure helper resumes following if configured
    void OnLeaderAttackEnd(CPlayer* pLeader);

    // Returns configured damage multiplier if this NPC is a helper; otherwise 1.0f
    float GetDamageMultiplierForHelper(class CNpc* pNpc);

private:
    CHelperNpcManager() = default;

private:
    sHELPER_NPC_CONFIG m_config;
    std::set<WORLDID> m_worldsWithHelper; // prevent duplicate spawns per world instance
    // Map leader handle -> helper npc handle
    std::unordered_map<HOBJECT, HOBJECT> m_mapLeaderToHelper;
};

#define GetHelperNpcManager() CHelperNpcManager::Instance()

#endif // __INC_DBOG_HELPER_NPC_MANAGER_H__
