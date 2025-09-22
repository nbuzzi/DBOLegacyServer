#ifndef __INC_DBOG_HELPER_NPC_MANAGER_H__
#define __INC_DBOG_HELPER_NPC_MANAGER_H__

#include "NtlSharedDef.h"
#include "NtlString.h"
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class CNtlIniFile;
class CWorld;
class CPlayer;

struct sHELPER_NPC_CONFIG
{
    bool   bEnabled = true;
    bool   bAllowUltimate = true;
    bool   bAllowBattleDungeon = true;
    bool   bAllowTimeQuest = true;
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
    float  fHealPowerMultiplier = 1.0f;    // multiply helper's direct/over-time healing power
    WORD   wHealPriorityMinMissingPercent = 8; // if HealLpThresholdOverride==0, require this % missing to prioritize heals (deadband)

    // Attribute modifiers (helpers only)
    WORD   wMaxLPPercent = 0;              // +% to Max LP
    WORD   wMaxEPPercent = 0;              // +% to Max EP
    WORD   wPhysicalOffensePercent = 0;    // +% to Physical Offense
    WORD   wEnergyOffensePercent = 0;      // +% to Energy Offense
    WORD   wPhysicalDefensePercent = 0;    // +% to Physical Defense
    WORD   wEnergyDefensePercent = 0;      // +% to Energy Defense
    WORD   wAttackRangePercent = 0;        // +% to attack range
    float  fAttackRangeBonusMeters = 0.0f; // +meters to attack range
    WORD   wSkillAnimSpeedPercent = 0;     // +% to skill animation speed

    // Proactive combat behavior
    bool   bProactiveAutoAttack = false;   // when true, helper scans and engages nearby enemies when idle
    WORD   wAttackScanRange = 45;          // meters; default large range to proactively find nearby mobs
    DWORD  dwAttackScanCooldownMs = 1500;  // scan interval while idle (ms)

    // Logging control
    bool   bVerboseLogs = false;           // reduce noisy logs unless debugging

    // Healing reach tuning
    float  fHealUseRangeBonusMeters = 0.0f;     // add to skill use range for non-enemy skills
    float  fHealApplyAreaBonusMeters = 0.0f;    // add to apply area sizes for party/alliance targeting

    // Buff reach tuning (helpers can buff entire party even when spread out)
    bool   bBuffPartyWide = true;               // when true, helper's non-enemy buffs target the full party
    float  fBuffApplyAreaMeters = 60.0f;        // override apply area for buffs to cover typical party spread

    // Optional: list of buff skills to add at spawn (comma-separated in INI)
    std::vector<TBLIDX> vBuffSkills;       // e.g., 420141,420142
    BYTE   buffBasis = 5;                  // default TIME basis
    WORD   buffLP = 0;                     // ignored for TIME basis; used for LP/Give
    WORD   buffTime = 10;                  // seconds for TIME basis
    // Optional alias map to support alternate "buff index" tokens resolving to SkillTable IDs
    // Format example in INI: BuffIndexMap=1:420141, 2=420142
    std::unordered_map<DWORD, TBLIDX> buffIndexAlias; // key: external buff index; value: SkillTable tblidx

    // Optional: force-add a specific skill to the helper at spawn, useful if the mob table lacks heals
    TBLIDX forcedSkillTblidx = INVALID_TBLIDX; // e.g., a heal/buff skill id
    std::vector<TBLIDX> vForcedSkills;         // optional list (comma-separated in INI)
    BYTE   forcedSkillBasis = 4;                // default to Give (4). 3=LP,4=Give,5=Time,6=Ring,7=OnlyLP
    WORD   forcedSkillLP = 70;                  // LP threshold for LP/Give conditions (percent)
    WORD   forcedSkillTime = 5;                 // seconds for time-based condition

    // Optional resurrection skill to revive fainted party members
    TBLIDX resurrectSkillTblidx = INVALID_TBLIDX; // e.g., 1520065

    // Rebuff controller: periodically re-check and reapply buffs
    DWORD  dwRebuffCooldownMs = 0;          // 0 = disabled
    DWORD  dwRebuffMinRemainingMs = 3000;   // reapply if remaining below this

    // Healer responsiveness (cadence tuning)
    DWORD  dwHealScanCooldownMs = 150;      // how often to evaluate party HP for heals (ms)
    DWORD  dwResurrectScanCooldownMs = 150; // how often to scan for fainted members (ms)
    DWORD  dwSkillTryCooldownMs = 100;      // minimum delay between queued skill attempts (ms)

    // AI preference: if true, attempt forced skills first when choosing an offensive ability
    bool   bPrioritizeForcedSkills = false;

    // Tank aggro enforcement (default off; enable explicitly for [TANK] role to reduce unintended threat churn)
    bool   bEnforceTankAggro = false;      // default off: only pulse aggro if role config enables it
    DWORD  dwTankAggroPulseMs = 500;      // how often to pulse aggro (ms)
    DWORD  dwTankAggroBonus = 800;        // flat bonus threat added per pulse when not top

    // Resurrection retry/backoff tuning (exposed via INI)
    // Attempt 1 occurs immediately when a faint target is detected; these control subsequent retries.
    DWORD  dwResurrectRetryDelay1Ms = 1200; // delay before second attempt
    DWORD  dwResurrectRetryDelay2Ms = 2500; // delay before third attempt
    BYTE   byResurrectMaxAttempts = 3;      // total attempts including first; 0=disable retry logic

    // Buff audit burst: number of buffs allowed to queue in a single audit cycle (default 1 for fairness)
    BYTE   byMaxBuffsPerAudit = 1;          // increase to accelerate full restoration after wipe
    BYTE   byMaxBuffsPerTargetPerAudit = 1; // cap per single target within one audit

    // Metrics (runtime counters; not configurable; zeroed at spawn copy)
    mutable DWORD dwMetricResurrectAttempts = 0;    // total resurrect casts queued
    mutable DWORD dwMetricResurrectRetries = 0;      // attempts beyond the first
    mutable DWORD dwMetricResurrectSuccess = 0;      // successful revivals observed (clears pending)
    mutable DWORD dwMetricBuffsQueuedMissing = 0;    // buffs queued because missing
    mutable DWORD dwMetricBuffsQueuedRefresh = 0;    // buffs queued because expiring

    // Administrative / duplication controls
    bool   bAllowGMHelpers = false;            // when false, GM characters never spawn helpers on entering worlds
    bool   bAllowMultipleHelpersPerWorld = true; // when false, limit to at most one helper entity per world instance (first creator wins)
    bool   bDisallowDuplicateHelperKindPerWorld = true; // when true, prevent spawning another helper with identical tblidx (NPC or MOB) in same world
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

    // Spawn helper in Time Machine Quest instances if allowed by config
    bool SpawnHelperIfNeededForTmq(CPlayer* pLeader, CWorld* pWorld);

    // Cleanup hook: call when the dungeon/world is being destroyed
    void OnWorldDestroyed(CWorld* pWorld);

    // When the leader starts or updates an attack target, notify helper to assist
    void OnLeaderAttackTarget(CPlayer* pLeader, HOBJECT hTarget);
    // When the leader ends attack, ensure helper resumes following if configured
    void OnLeaderAttackEnd(CPlayer* pLeader);

    // Returns configured damage multiplier if this NPC is a helper; otherwise 1.0f
    float GetDamageMultiplierForHelper(class CNpc* pNpc);
    // Returns configured heal power multiplier if this NPC is a helper; otherwise 1.0f
    float GetHealMultiplierForHelper(class CNpc* pNpc);

    // True if this NPC is a registered helper we spawned and track
    bool IsRegisteredHelper(class CNpc* pNpc) const;

    // True if registered helper and currently linked to a valid PC handle (active association)
    bool IsActiveLinkedHelper(class CNpc* pNpc) const;

    // Returns per-helper config snapshot if this NPC is a registered helper; otherwise nullptr
    const sHELPER_NPC_CONFIG* GetConfigForHelper(class CNpc* pNpc) const;

    // Periodic watchdog to ensure helpers are present after stage/floor transitions
    void TickWatchdog(DWORD dwNow);

    // Immediate repair/spawn on teleport: ensure helper exists for leader in current world
    void EnsureHelperForLeaderNow(class CPlayer* pLeader);

    // Despawn helpers when a leader leaves a world (e.g., exiting a dungeon)
    void OnLeaderLeaveWorld(class CPlayer* pLeader, class CWorld* pWorld);

    // Party leader changed: move any helpers from oldLeader to newLeader and re-link/follow
    void OnPartyLeaderChanged(HOBJECT oldLeader, HOBJECT newLeader);

    // Evaluate party composition and spawn role helpers if missing (HEALER/TANK/BUFFER)
    void EvaluateAndSpawnRoleHelpers(class CPlayer* pLeader, class CWorld* pWorld);

    // Called when a new member joins a party to remove conflicting role helpers
    void OnPartyMemberJoined(class CParty* pParty, class CPlayer* pNewMember);

    // Metrics utilities
    void ResetMetrics();
    void DumpMetrics(); // logs aggregated and per-helper metrics

    // Refresh existing helpers: despawn all current helpers and respawn according to current config (roles + base)
    void RefreshAllHelpers(bool bRespawn);

private:
    CHelperNpcManager() = default;
    // Common spawn path after mode-specific allow checks pass
    bool SpawnIfAllowed(CPlayer* pLeader, CWorld* pWorld, const sHELPER_NPC_CONFIG& cfg);

    // Load a config section by name and merge into destination; returns number of keys found
    int LoadConfigSection(CNtlIniFile& file, const char* sectionName, sHELPER_NPC_CONFIG& out);

private:
    // Base/default configuration from [HELPER_NPC]
    sHELPER_NPC_CONFIG m_config;
    // Optional per-dungeon overrides; if not present, fall back to m_config
    sHELPER_NPC_CONFIG m_cfgUD;   // [HELPER_NPC_UD]
    sHELPER_NPC_CONFIG m_cfgBD;   // [HELPER_NPC_BD]
    sHELPER_NPC_CONFIG m_cfgTMQ;  // [HELPER_NPC_TMQ]
    bool m_hasUDOverride = false;
    bool m_hasBDOverride = false;
    bool m_hasTMQOverride = false;
    std::set<WORLDID> m_worldsWithHelper; // prevent duplicate spawns per world instance
    // Map leader handle -> helper npc handle
    std::unordered_map<HOBJECT, HOBJECT> m_mapLeaderToHelper;
    // Support multiple helpers: leader -> list of helper handles
    std::unordered_map<HOBJECT, std::vector<HOBJECT>> m_leaderToHelpers;
    // Map helper handle -> config snapshot used at spawn
    std::unordered_map<HOBJECT, sHELPER_NPC_CONFIG> m_helperConfigByHelper;
    // Map helper handle -> base tblidx (NPC or MOB) to dedupe by ID
    std::unordered_map<HOBJECT, TBLIDX> m_helperKindByHelper;

    // Watchdog ticker
    DWORD m_dwLastWatchdogTick = 0;

    // Extra allowlists: treat these WorldIDs as UD/BD/TMQ respectively
    std::set<WORLDID> m_extraUDWorldIDs;
    std::set<WORLDID> m_extraBDWorldIDs;
    std::set<WORLDID> m_extraTMQWorldIDs;

    // Spawn-in-progress guard to avoid duplicating the same helper (same tblidx) for the same leader in the same world
    struct SpawnKey {
        HOBJECT leader;
        WORLDID world;
        TBLIDX  tblidx;
        bool operator==(const SpawnKey& o) const { return leader == o.leader && world == o.world && tblidx == o.tblidx; }
    };
    struct SpawnKeyHash {
        size_t operator()(const SpawnKey& k) const {
            // simple mix; good enough for small sets
            size_t h1 = (size_t)k.leader;
            size_t h2 = (size_t)k.world * 1315423911u;
            size_t h3 = (size_t)k.tblidx * 2654435761u;
            return h1 ^ h2 ^ h3;
        }
    };
    std::unordered_set<SpawnKey, SpawnKeyHash> m_pendingSpawns;

    // Role-based helper definitions
    struct sROLE_DEF {
        bool enabled = false;
        BYTE maxCount = 1;
        std::set<int> coveredClasses; // class IDs that satisfy this role
        sHELPER_NPC_CONFIG cfg;       // spawn behavior for this role
    };
    sROLE_DEF m_roleHealer;
    sROLE_DEF m_roleTank;
    sROLE_DEF m_roleBuffer;
    sROLE_DEF m_roleSpeed;

    // Helpers to load role sections
    bool LoadRoleSection(class CNtlIniFile& file, const char* sectionName, sROLE_DEF& outRole);

    // Remove helpers for specific roles for the party leader in their current world
    void RemoveRoleHelpersForLeader(class CPlayer* pLeader, class CWorld* pWorld, bool removeHealer, bool removeTank, bool removeBuffer, bool removeSpeed);

    // Despawn all helpers for a given leader in the specified world
    void DespawnAllHelpersForLeaderInWorld(class CPlayer* pLeader, class CWorld* pWorld);
};

#define GetHelperNpcManager() CHelperNpcManager::Instance()

#endif // __INC_DBOG_HELPER_NPC_MANAGER_H__
