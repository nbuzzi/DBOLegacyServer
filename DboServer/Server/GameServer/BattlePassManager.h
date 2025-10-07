#pragma once

#include "NtlSingleton.h"
#include "NtlString.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>

class CNtlIniFile;
class CPlayer;

// Simple Battle Pass system (seasonal progression) – initial skeleton.
// Focus: award XP for core server events (mob kills, dungeon clears, PvP participation)
// Persistence: in-memory only for now (DB integration TODO – requires schema migration outside protected paths).
// Config file (optional): .\\config\\BattlePass.cfg (created manually – automation must not touch ExecutionEnv/)
// Example config section:
// [BATTLEPASS]
// Enabled=1
// SeasonId=1
// StartUnix=0
// EndUnix=0
// Verbose=0
// BaseXpPerLevel=1000
// LevelXpGrowthPercent=15
// MobKillXp=5
// DungeonClearXp=250
// DungeonStageXp=40
// RankBattleParticipationXp=60
// RankBattleWinXp=140
// BudokaiParticipationXp=300
// BudokaiWinXp=800
// DeathXp=0            ; normally 0 (but can be used for death-based challenges)
// DailyResetHour=0     ; UTC hour a new day starts for daily challenges

class CBattlePassManager : public CNtlSingleton<CBattlePassManager>
{
public:
    enum class Action : unsigned char
    {
        MOB_KILL = 0,
        DUNGEON_STAGE,
        DUNGEON_CLEAR,
        RANKBATTLE_PARTICIPATE,
        RANKBATTLE_WIN,
        BUDOKAI_PARTICIPATE,
        BUDOKAI_WIN,
        PLAYER_DEATH,
    };

    struct Config
    {
        bool enabled = false;
        bool masterEnable = true;        // Global kill switch (overrides everything if false)
        unsigned int seasonId = 0;
        unsigned int startUnix = 0;      // 0 = always active
        unsigned int endUnix = 0;        // 0 = no end
        bool verbose = false;            // Extra logging
        unsigned int autosaveMinutes = 5; // Interval to flush progress file (0=disable)
        unsigned int mudosaPerLevel = 0;  // Optional Mudosa points on each level-up
        bool notifyOnXpGain = true;       // Per-action XP gain system message to player
        bool broadcastLevelUp = true;     // Broadcast level-up to current world
    bool welcomeMessageEnabled = true; // Send welcome message on first world entry
    unsigned int welcomeMessageCooldownSec = 300; // Min seconds between welcome messages per session/player
        std::wstring welcomeMessageTemplate = L"Welcome {NAME}! Thanks for supporting the server. Your Battle Pass '{PASS}' is active. Claim rewards on the website!"; // tokens: {NAME} {PASS}
        // Database batching (when enabled): file autosave is disabled and progress is flushed in batches
        bool useDatabase = false;            // If true, use DB instead of flat file snapshot
        unsigned int flushSeconds = 30;      // Batch flush interval (seconds)
        unsigned int minDeltaXp = 50;        // Minimum total XP delta before we include a row in a flush (reduces tiny writes)
        // XP formula
        unsigned int baseXpPerLevel = 1000;
        unsigned int levelXpGrowthPercent = 15; // percent growth per level (simple compound)
        // XP sources
        unsigned int mobKillXp = 5;
        unsigned int dungeonStageXp = 40;
        unsigned int dungeonClearXp = 250;
        unsigned int rankBattleParticipationXp = 60;
        unsigned int rankBattleWinXp = 140;
        unsigned int budokaiParticipationXp = 300;
        unsigned int budokaiWinXp = 800;
        unsigned int deathXp = 0; // Usually zero; can support special seasons
        // Daily challenges (future extension)
        unsigned int dailyResetHourUTC = 0; // hour of day (0-23)
    };

    struct PlayerProgress
    {
        unsigned int seasonId = 0;
        unsigned int level = 0;
        unsigned int xp = 0;              // XP within current level
        unsigned int totalXp = 0;         // Cumulative XP for analytics
        unsigned int lastDailyResetDay = 0; // YYYYMMDD UTC snapshot for reset logic
        // Simple counters (can be expanded for web UI analytics)
        unsigned int mobKills = 0;
        unsigned int dungeonStages = 0;
        unsigned int dungeonClears = 0;
        unsigned int rankBattles = 0;
        unsigned int rankWins = 0;
        unsigned int budokaiEntries = 0;
        unsigned int budokaiWins = 0;
        unsigned int deaths = 0;
        // DB batching metadata
        bool dirty = false;                // Needs DB flush
        unsigned int lastSavedLevel = 0;
        unsigned int lastSavedXp = 0;
        unsigned int lastSavedTotalXp = 0;
    };

public:
    CBattlePassManager();
    ~CBattlePassManager();

    bool LoadConfigFromIniPath(const char* iniPath); // .\\config\\BattlePass.cfg
    bool LoadProgressFromFile(const char* path);     // Flat file persistence (temporary until DB)
    bool SaveProgressToFile(const char* path);       // Writes all in-memory entries

    const Config& GetConfig() const { return m_cfg; }
    bool IsEnabled() const { return m_cfg.enabled; }
    bool IsMasterEnabled() const { return m_cfg.masterEnable && m_cfg.enabled; }
    void SetMasterEnable(bool v); // implemented inline below or in cpp if needed
    const std::wstring& GetWelcomeTemplate() const { return m_cfg.welcomeMessageTemplate; }
    // Tier lookup placeholder – future: account-based premium status
    const wchar_t* GetPlayerPassTier(const CPlayer* /*player*/) const { return L"Standard"; }

    // Event hooks – safe to call even if disabled
    void OnMobKill(CPlayer* pPlayer, unsigned int mobTblidx);
    void OnDungeonStageComplete(CPlayer* pPlayer, unsigned int dungeonId, unsigned int stage);
    void OnDungeonClear(CPlayer* pPlayer, unsigned int dungeonId);
    void OnRankBattleParticipation(CPlayer* pPlayer, bool win);
    void OnBudokaiParticipation(CPlayer* pPlayer, bool win);
    void OnPlayerDeath(CPlayer* pPlayer);

    // Generic progression interface (can expose to GM commands)
    void AddActionXp(CPlayer* pPlayer, Action action, unsigned int customOverrideXp = 0);
    unsigned int GetXpForLevel(unsigned int level) const; // XP needed to go from (level) -> (level+1)
    PlayerProgress* GetProgress(unsigned int charId);

    // Tick (optional) – handles daily reset; call from main loop if integrated later
    void Tick(unsigned long nowUnix);
    void ForceAutosave();
    void ForceFlushToDatabase(); // manual flush (e.g. on shutdown / GM command)
    void FlushPlayer(unsigned int charId); // flush only a single player's progress if dirty (DB mode)
    // Format a welcome message using configured template (tokens: {NAME}, {PASS})
    void FormatWelcomeMessage(CPlayer* pPlayer, const wchar_t* passName, wchar_t* outBuf, size_t outLen) const;

private:
    void AwardXp(PlayerProgress& prog, unsigned int charId, unsigned int amount, const wchar_t* reason);
    void CheckLevelUp(PlayerProgress& prog, unsigned int charId);
    bool IsSeasonActive(unsigned long nowUnix) const;
    unsigned int CurrentDayUTC(unsigned long nowUnix) const;
    void EnsureDailyReset(PlayerProgress& prog, unsigned long nowUnix);
    void MarkDirty(PlayerProgress& prog); // mark a progress row dirty for DB flush
    void TryFlushDatabase(unsigned long nowUnix); // periodic flush if interval elapsed
    void FlushAllDirty(); // internal unconditional flush
    bool PlayerHasBattlePass(const CPlayer* pPlayer) const; // placeholder for per-player entitlement
    void BuildAndSubmitBatch(const std::vector<unsigned int>& charIds); // build multi-row upsert (stub)

private:
    Config m_cfg;
    // charId -> progress
    std::unordered_map<unsigned int, PlayerProgress> m_progress;
    unsigned long m_lastAutosaveUnix = 0;
    unsigned long m_lastDbFlushUnix = 0;
    // Rate limiting for welcome messages (charId -> last sent unix time)
    std::unordered_map<unsigned int, unsigned long> m_lastWelcomeUnix;
};

inline void CBattlePassManager::SetMasterEnable(bool v)
{
    m_cfg.masterEnable = v;
}

#define GetBattlePassManager() CBattlePassManager::GetInstance()
#define g_pBattlePassManager GetBattlePassManager()
