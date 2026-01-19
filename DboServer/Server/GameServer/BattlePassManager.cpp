#include "stdafx.h"
#include "BattlePassManager.h"
#include "NtlIniFile.h"
#include "CPlayer.h"
#include "ObjectManager.h"
#include "GameServer.h"
#include "NtlPacketGU.h" // for sGU_SYSTEM_DISPLAY_TEXT

// Logging helpers mirroring existing style
#define BATTLEPASS_ERR(fmt, ...) ERR_LOG(LOG_GENERAL, fmt, __VA_ARGS__)
#define BATTLEPASS_PRINT(fmt, ...) NTL_PRINT(PRINT_APP, fmt, __VA_ARGS__)

CBattlePassManager::CBattlePassManager() {}
CBattlePassManager::~CBattlePassManager() {}

bool CBattlePassManager::LoadConfigFromIniPath(const char* iniPath)
{
    CNtlIniFile file;
    int rc = file.Create(iniPath);
    if (rc != NTL_SUCCESS)
    {
        BATTLEPASS_PRINT(_T("[BATTLEPASS] Config not found or failed load %S (rc=%d) - using defaults/disabled"), iniPath, rc);
        return false;
    }

    int v = 0;
    // INI reader does not provide direct wide-char buffer read; read as narrow then widen
    std::string tmpl;
    if (file.Read("BATTLEPASS", "Enabled", v)) m_cfg.enabled = (v != 0);
    if (file.Read("BATTLEPASS", "MasterEnable", v)) m_cfg.masterEnable = (v != 0);
    if (file.Read("BATTLEPASS", "SeasonId", v)) m_cfg.seasonId = (unsigned int)v;
    if (file.Read("BATTLEPASS", "StartUnix", v)) m_cfg.startUnix = (unsigned int)v;
    if (file.Read("BATTLEPASS", "EndUnix", v)) m_cfg.endUnix = (unsigned int)v;
    if (file.Read("BATTLEPASS", "Verbose", v)) m_cfg.verbose = (v != 0);
    if (file.Read("BATTLEPASS", "AutosaveMinutes", v)) m_cfg.autosaveMinutes = (unsigned int)v;
    if (file.Read("BATTLEPASS", "MudosaPerLevel", v)) m_cfg.mudosaPerLevel = (unsigned int)v;
    if (file.Read("BATTLEPASS", "NotifyOnXpGain", v)) m_cfg.notifyOnXpGain = (v != 0);
    if (file.Read("BATTLEPASS", "BroadcastLevelUp", v)) m_cfg.broadcastLevelUp = (v != 0);
    if (file.Read("BATTLEPASS", "WelcomeMessageEnabled", v)) m_cfg.welcomeMessageEnabled = (v != 0);
    if (file.Read("BATTLEPASS", "WelcomeMessageCooldownSec", v)) m_cfg.welcomeMessageCooldownSec = (unsigned int)v;
    {
        CNtlString narrow = file.Read("BATTLEPASS", "WelcomeMessageTemplate");
        const char* cstr = narrow.c_str();
        if (cstr && *cstr)
        {
            std::wstring wide;
            for (const unsigned char* p = (const unsigned char*)cstr; *p; ++p)
                wide.push_back((wchar_t)*p);
            if (!wide.empty()) m_cfg.welcomeMessageTemplate = wide;
        }
    }
    if (file.Read("BATTLEPASS", "BaseXpPerLevel", v)) m_cfg.baseXpPerLevel = (unsigned int)v;
    if (file.Read("BATTLEPASS", "LevelXpGrowthPercent", v)) m_cfg.levelXpGrowthPercent = (unsigned int)v;
    if (file.Read("BATTLEPASS", "MobKillXp", v)) m_cfg.mobKillXp = (unsigned int)v;
    if (file.Read("BATTLEPASS", "DungeonStageXp", v)) m_cfg.dungeonStageXp = (unsigned int)v;
    if (file.Read("BATTLEPASS", "DungeonClearXp", v)) m_cfg.dungeonClearXp = (unsigned int)v;
    if (file.Read("BATTLEPASS", "RankBattleParticipationXp", v)) m_cfg.rankBattleParticipationXp = (unsigned int)v;
    if (file.Read("BATTLEPASS", "RankBattleWinXp", v)) m_cfg.rankBattleWinXp = (unsigned int)v;
    if (file.Read("BATTLEPASS", "BudokaiParticipationXp", v)) m_cfg.budokaiParticipationXp = (unsigned int)v;
    if (file.Read("BATTLEPASS", "BudokaiWinXp", v)) m_cfg.budokaiWinXp = (unsigned int)v;
    if (file.Read("BATTLEPASS", "DeathXp", v)) m_cfg.deathXp = (unsigned int)v;
    if (file.Read("BATTLEPASS", "DailyResetHour", v)) m_cfg.dailyResetHourUTC = (unsigned int)v;
    if (file.Read("BATTLEPASS", "UseDatabase", v)) m_cfg.useDatabase = (v != 0);
    if (file.Read("BATTLEPASS", "FlushSeconds", v)) m_cfg.flushSeconds = (unsigned int)v;
    if (file.Read("BATTLEPASS", "MinDeltaXp", v)) m_cfg.minDeltaXp = (unsigned int)v;

    BATTLEPASS_PRINT(_T("[BATTLEPASS] Config loaded enabled=%d season=%u baseXp=%u growth=%u mobKill=%u"),
        m_cfg.enabled ? 1 : 0, m_cfg.seasonId, m_cfg.baseXpPerLevel, m_cfg.levelXpGrowthPercent, m_cfg.mobKillXp);
    return true;
}

bool CBattlePassManager::LoadProgressFromFile(const char* path)
{
    FILE* f = nullptr;
#ifdef _WIN32
    fopen_s(&f, path, "r");
#else
    f = fopen(path, "r");
#endif
    if (!f) return false;
    char line[512];
    unsigned int count = 0; m_progress.clear();
    while (fgets(line, sizeof(line), f))
    {
        if (line[0] == '#' || line[0] == '\n') continue;
        PlayerProgress p{}; unsigned int charId = 0;
        // CSV: charId,season,level,xp,totalXp,mobKills,dungeonStages,dungeonClears,rankBattles,rankWins,budokaiEntries,budokaiWins,deaths,lastDaily
        int n = sscanf_s(line, "%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u",
            &charId,&p.seasonId,&p.level,&p.xp,&p.totalXp,&p.mobKills,&p.dungeonStages,&p.dungeonClears,
            &p.rankBattles,&p.rankWins,&p.budokaiEntries,&p.budokaiWins,&p.deaths,&p.lastDailyResetDay);
        if (n >= 14 && p.seasonId == m_cfg.seasonId)
        {
            m_progress[charId] = p;
            ++count;
        }
    }
    fclose(f);
    BATTLEPASS_PRINT(_T("[BATTLEPASS] Loaded %u progress rows from %S"), count, path);
    return true;
}

bool CBattlePassManager::SaveProgressToFile(const char* path)
{
    if (m_cfg.useDatabase)
        return false; // disabled when DB mode enabled
    FILE* f = nullptr;
#ifdef _WIN32
    fopen_s(&f, path, "w");
#else
    f = fopen(path, "w");
#endif
    if (!f) return false;
    fprintf(f, "# BattlePass progress dump season=%u generated=%u\n", m_cfg.seasonId, (unsigned)time(nullptr));
    for (auto& kv : m_progress)
    {
        const PlayerProgress& p = kv.second;
        if (p.seasonId != m_cfg.seasonId) continue; // only current season
        fprintf(f, "%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",
            kv.first,p.seasonId,p.level,p.xp,p.totalXp,p.mobKills,p.dungeonStages,p.dungeonClears,
            p.rankBattles,p.rankWins,p.budokaiEntries,p.budokaiWins,p.deaths,p.lastDailyResetDay);
    }
    fclose(f);
    return true;
}

unsigned int CBattlePassManager::GetXpForLevel(unsigned int level) const
{
    // Geometric-ish: base * (1 + g%)^level, but keep integer simple incremental
    // We'll accumulate required XP by iterating; here we just compute for next level
    double growth = 1.0 + (double)m_cfg.levelXpGrowthPercent / 100.0;
    double required = (double)m_cfg.baseXpPerLevel;
    for (unsigned int i = 0; i < level; ++i)
        required *= growth;
    return (unsigned int)required;
}

CBattlePassManager::PlayerProgress* CBattlePassManager::GetProgress(unsigned int charId)
{
    auto it = m_progress.find(charId);
    if (it == m_progress.end())
    {
        PlayerProgress blank;
        blank.seasonId = m_cfg.seasonId;
        blank.level = 0;
        blank.xp = 0;
        blank.totalXp = 0;
        auto inserted = m_progress.emplace(charId, blank);
        return &inserted.first->second;
    }
    // Reset season if mismatch
    if (it->second.seasonId != m_cfg.seasonId)
    {
        it->second = PlayerProgress();
        it->second.seasonId = m_cfg.seasonId;
        it->second.dirty = true; // new season row will need persistence
    }
    return &it->second;
}

bool CBattlePassManager::IsSeasonActive(unsigned long nowUnix) const
{
    if (!m_cfg.enabled) return false;
    if (m_cfg.startUnix && nowUnix < m_cfg.startUnix) return false;
    if (m_cfg.endUnix && nowUnix > m_cfg.endUnix) return false;
    return true;
}

unsigned int CBattlePassManager::CurrentDayUTC(unsigned long nowUnix) const
{
    // Convert seconds to YYYYMMDD naive (UTC) using simple breakdown (no DST)
    time_t t = (time_t)nowUnix;
    tm gm; memset(&gm, 0, sizeof(gm));
#ifdef _WIN32
    gmtime_s(&gm, &t);
#else
    gmtime_r(&t, &gm);
#endif
    return (unsigned int)((gm.tm_year + 1900) * 10000 + (gm.tm_mon + 1) * 100 + gm.tm_mday);
}

void CBattlePassManager::EnsureDailyReset(PlayerProgress& prog, unsigned long nowUnix)
{
    if (m_cfg.dailyResetHourUTC > 0)
    {
        // If reset hour is not midnight, adjust time by subtracting hours then compute day key
        nowUnix -= (m_cfg.dailyResetHourUTC * 3600);
    }
    unsigned int dayKey = CurrentDayUTC(nowUnix);
    if (prog.lastDailyResetDay != dayKey)
    {
        // (Future) daily counters would reset here
        prog.lastDailyResetDay = dayKey;
        if (m_cfg.verbose)
            BATTLEPASS_PRINT(_T("[BATTLEPASS] Daily reset for charId=%u day=%u"), (unsigned int)(&prog - &m_progress.begin()->second), dayKey);
    }
}

void CBattlePassManager::AwardXp(PlayerProgress& prog, unsigned int charId, unsigned int amount, const wchar_t* reason)
{
    if (amount == 0) return;
    prog.xp += amount;
    prog.totalXp += amount;
    // Player feedback (system message) – only when config enabled
    CPlayer* pPlayer = g_pObjectManager ? (CPlayer*)g_pObjectManager->GetPC(charId) : nullptr;
    if (pPlayer && m_cfg.notifyOnXpGain)
    {
        wchar_t msg[96];
        swprintf_s(msg, L"[BattlePass] +%u XP (%s)", amount, reason);
        CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
        sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
        res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
        res->byDisplayType = SERVER_TEXT_SYSNOTICE;
        res->wMessageLengthInUnicode = (WORD)wcslen(msg);
        wcsncpy_s(res->awchMessage, NTL_MAX_LENGTH_OF_CHAT_MESSAGE + 1, msg, _TRUNCATE);
        packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
        pPlayer->SendPacket(&packet);
    }
    if (m_cfg.verbose)
    {
        wchar_t buf[128];
        swprintf_s(buf, L"[BattlePass] +%u XP (%s) -> L%u %u/%u", amount, reason, prog.level, prog.xp, GetXpForLevel(prog.level));
        NTL_PRINT(PRINT_APP, _T("%S"), ""); // placeholder to keep macro usage consistent (wide log wrappers vary); direct wide below
        wprintf(L"%s\n", buf); // fallback console wide logging
    }
    CheckLevelUp(prog, charId);
    MarkDirty(prog);
}

void CBattlePassManager::CheckLevelUp(PlayerProgress& prog, unsigned int charId)
{
    while (true)
    {
        unsigned int need = GetXpForLevel(prog.level);
        if (prog.xp < need) break;
        prog.xp -= need;
        prog.level++;
        BATTLEPASS_PRINT(_T("[BATTLEPASS] Level up charId=%u newLevel=%u"), charId, prog.level);
        if (m_cfg.mudosaPerLevel > 0)
        {
            // Attempt to resolve player object for Mudosa reward
            CPlayer* pPlayer = g_pObjectManager ? (CPlayer*)g_pObjectManager->GetPC(charId) : nullptr;
            if (pPlayer)
            {
                pPlayer->UpdateMudosaPoints(pPlayer->GetMudosaPoints() + m_cfg.mudosaPerLevel, true);
            }
        }
        // System notification & optional broadcast
    CPlayer* pPlr = g_pObjectManager ? (CPlayer*)g_pObjectManager->GetPC(charId) : nullptr;
        if (pPlr)
        {
            wchar_t selfMsg[128];
            swprintf_s(selfMsg, L"[BattlePass] Level %u reached!", prog.level);
            CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
            sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
            res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
            res->byDisplayType = SERVER_TEXT_SYSNOTICE;
            res->wMessageLengthInUnicode = (WORD)wcslen(selfMsg);
            wcsncpy_s(res->awchMessage, NTL_MAX_LENGTH_OF_CHAT_MESSAGE + 1, selfMsg, _TRUNCATE);
            packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
            pPlr->SendPacket(&packet);

            // World broadcast disabled (CWorld player enumeration API differs in this branch).
            // Future: implement safe iteration over world players when accessor is available.
            (void)prog; // silence unused if broadcast removed
        }
        // TODO: reward items/currency on level up (configurable) – future extension
        MarkDirty(prog);
    }
}

void CBattlePassManager::AddActionXp(CPlayer* pPlayer, Action action, unsigned int customOverrideXp)
{
    if (!pPlayer) return;
    unsigned long nowUnix = (unsigned long)time(nullptr);
    if (!IsSeasonActive(nowUnix)) return;
    if (!m_cfg.masterEnable) return; // global kill switch
    if (!PlayerHasBattlePass(pPlayer)) return; // only players with active pass are persisted/awarded
    PlayerProgress* prog = GetProgress(pPlayer->GetCharID());
    EnsureDailyReset(*prog, nowUnix);

    unsigned int xp = customOverrideXp; // if >0 overrides mapping below
    if (xp == 0)
    {
        switch (action)
        {
        case Action::MOB_KILL: xp = m_cfg.mobKillXp; prog->mobKills++; break;
        case Action::DUNGEON_STAGE: xp = m_cfg.dungeonStageXp; prog->dungeonStages++; break;
        case Action::DUNGEON_CLEAR: xp = m_cfg.dungeonClearXp; prog->dungeonClears++; break;
        case Action::RANKBATTLE_PARTICIPATE: xp = m_cfg.rankBattleParticipationXp; prog->rankBattles++; break;
        case Action::RANKBATTLE_WIN: xp = m_cfg.rankBattleWinXp; prog->rankWins++; break;
        case Action::BUDOKAI_PARTICIPATE: xp = m_cfg.budokaiParticipationXp; prog->budokaiEntries++; break;
        case Action::BUDOKAI_WIN: xp = m_cfg.budokaiWinXp; prog->budokaiWins++; break;
        case Action::PLAYER_DEATH: xp = m_cfg.deathXp; prog->deaths++; break;
        }
    }
    const wchar_t* reason = L"";
    switch (action)
    {
    case Action::MOB_KILL: reason = L"MobKill"; break;
    case Action::DUNGEON_STAGE: reason = L"DungeonStage"; break;
    case Action::DUNGEON_CLEAR: reason = L"DungeonClear"; break;
    case Action::RANKBATTLE_PARTICIPATE: reason = L"RankBattle"; break;
    case Action::RANKBATTLE_WIN: reason = L"RankBattleWin"; break;
    case Action::BUDOKAI_PARTICIPATE: reason = L"Budokai"; break;
    case Action::BUDOKAI_WIN: reason = L"BudokaiWin"; break;
    case Action::PLAYER_DEATH: reason = L"Death"; break;
    }
    AwardXp(*prog, pPlayer->GetCharID(), xp, reason);
}

void CBattlePassManager::OnMobKill(CPlayer* pPlayer, unsigned int /*mobTblidx*/)
{
    AddActionXp(pPlayer, Action::MOB_KILL);
}

void CBattlePassManager::OnDungeonStageComplete(CPlayer* pPlayer, unsigned int /*dungeonId*/, unsigned int /*stage*/)
{
    AddActionXp(pPlayer, Action::DUNGEON_STAGE);
}

void CBattlePassManager::OnDungeonClear(CPlayer* pPlayer, unsigned int /*dungeonId*/)
{
    AddActionXp(pPlayer, Action::DUNGEON_CLEAR);
}

void CBattlePassManager::OnRankBattleParticipation(CPlayer* pPlayer, bool win)
{
    AddActionXp(pPlayer, win ? Action::RANKBATTLE_WIN : Action::RANKBATTLE_PARTICIPATE);
}

void CBattlePassManager::OnBudokaiParticipation(CPlayer* pPlayer, bool win)
{
    AddActionXp(pPlayer, win ? Action::BUDOKAI_WIN : Action::BUDOKAI_PARTICIPATE);
}

void CBattlePassManager::OnPlayerDeath(CPlayer* pPlayer)
{
    AddActionXp(pPlayer, Action::PLAYER_DEATH);
}

void CBattlePassManager::Tick(unsigned long nowUnix)
{
    if (!IsSeasonActive(nowUnix)) return;
    if (!m_cfg.enabled) return; // feature off
    if (!m_cfg.masterEnable) return; // global off
    if (m_cfg.useDatabase)
    {
        TryFlushDatabase(nowUnix);
    }
    else
    {
        // Flat-file autosave path (legacy) if DB disabled
        if (m_cfg.autosaveMinutes > 0)
        {
            if (m_lastAutosaveUnix == 0) m_lastAutosaveUnix = nowUnix;
            unsigned long interval = (unsigned long)m_cfg.autosaveMinutes * 60UL;
            if (nowUnix - m_lastAutosaveUnix >= interval)
            {
                SaveProgressToFile(".\\config\\BattlePassProgress.dat");
                m_lastAutosaveUnix = nowUnix;
            }
        }
    }
}

void CBattlePassManager::ForceAutosave()
{
    if (!m_cfg.useDatabase)
        SaveProgressToFile(".\\config\\BattlePassProgress.dat");
    else
        ForceFlushToDatabase();
}

void CBattlePassManager::FormatWelcomeMessage(CPlayer* pPlayer, const wchar_t* passName, wchar_t* outBuf, size_t outLen) const
{
    if (!outBuf || outLen == 0) return;
    const std::wstring& tpl = m_cfg.welcomeMessageTemplate;
    std::wstring result;
    result.reserve(tpl.size() + 32);
    for (size_t i = 0; i < tpl.size(); )
    {
        if (tpl[i] == L'{' )
        {
            size_t end = tpl.find(L'}', i+1);
            if (end != std::wstring::npos)
            {
                std::wstring token = tpl.substr(i+1, end - (i+1));
                if (token == L"NAME") result += pPlayer ? pPlayer->GetCharName() : L"Player";
                else if (token == L"PASS") result += passName ? passName : L"Standard";
                else result += L"{" + token + L"}"; // leave unknown token intact
                i = end + 1;
                continue;
            }
        }
        result.push_back(tpl[i]);
        ++i;
    }
    wcsncpy_s(outBuf, outLen, result.c_str(), _TRUNCATE);
}

// --- DB batching helpers (stubs / placeholder logic) ---

void CBattlePassManager::MarkDirty(PlayerProgress& prog)
{
    if (m_cfg.useDatabase)
        prog.dirty = true;
}

bool CBattlePassManager::PlayerHasBattlePass(const CPlayer* /*pPlayer*/) const
{
    // Placeholder: currently all players share standard pass when system enabled
    return m_cfg.enabled; // refine when premium tiers introduced
}

void CBattlePassManager::TryFlushDatabase(unsigned long nowUnix)
{
    if (!m_cfg.useDatabase) return;
    if (m_lastDbFlushUnix == 0) m_lastDbFlushUnix = nowUnix;
    if (nowUnix - m_lastDbFlushUnix < m_cfg.flushSeconds) return;
    FlushAllDirty();
    m_lastDbFlushUnix = nowUnix;
}

void CBattlePassManager::FlushAllDirty()
{
    std::vector<unsigned int> toFlush;
    toFlush.reserve(m_progress.size());
    for (auto& kv : m_progress)
    {
        PlayerProgress& p = kv.second;
        if (!p.dirty) continue;
        // Skip very small deltas to reduce churn (only if configured >0)
        unsigned int deltaXp = (p.totalXp >= p.lastSavedTotalXp) ? (p.totalXp - p.lastSavedTotalXp) : p.totalXp;
        if (m_cfg.minDeltaXp > 0 && deltaXp < m_cfg.minDeltaXp)
            continue;
        toFlush.push_back(kv.first);
    }
    if (toFlush.empty()) return;
    BuildAndSubmitBatch(toFlush);
    // Mark persisted
    for (unsigned int cid : toFlush)
    {
        PlayerProgress& p = m_progress[cid];
        p.dirty = false;
        p.lastSavedLevel = p.level;
        p.lastSavedXp = p.xp;
        p.lastSavedTotalXp = p.totalXp;
    }
}

void CBattlePassManager::BuildAndSubmitBatch(const std::vector<unsigned int>& charIds)
{
    if (charIds.empty()) return;
    // NOTE: We do not implement actual DB calls here (protected DB layer not shown).
    // For now, we log a summary. Integrate with QueryBuffer / async DB worker similarly to other systems.
    if (m_cfg.verbose)
        BATTLEPASS_PRINT(_T("[BATTLEPASS][DB] Flushing %u rows"), (unsigned)charIds.size());
    // Example (pseudocode):
    // QueryBuffer qb; qb.AddQuery("INSERT INTO battle_pass_progress ... ON DUPLICATE KEY UPDATE ...", ...);
    // g_pDatabase->Push(qb); (Adjust to your actual DB submission pattern.)
}

void CBattlePassManager::ForceFlushToDatabase()
{
    if (!m_cfg.useDatabase) return;
    FlushAllDirty();
}

void CBattlePassManager::FlushPlayer(unsigned int charId)
{
    if (!m_cfg.useDatabase) return;
    auto it = m_progress.find(charId);
    if (it == m_progress.end()) return;
    PlayerProgress& p = it->second;
    if (!p.dirty) return; // nothing to write
    unsigned int deltaXp = (p.totalXp >= p.lastSavedTotalXp) ? (p.totalXp - p.lastSavedTotalXp) : p.totalXp;
    if (m_cfg.minDeltaXp > 0 && deltaXp < m_cfg.minDeltaXp)
    {
        // Force flush on logout regardless of minDeltaXp threshold
    }
    if (m_cfg.verbose)
        BATTLEPASS_PRINT(_T("[BATTLEPASS][DB] FlushPlayer charId=%u level=%u xp=%u total=%u"), charId, p.level, p.xp, p.totalXp);
    std::vector<unsigned int> one{ charId };
    BuildAndSubmitBatch(one);
    p.dirty = false;
    p.lastSavedLevel = p.level;
    p.lastSavedXp = p.xp;
    p.lastSavedTotalXp = p.totalXp;
}
