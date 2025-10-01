#include "stdafx.h"
#include "ArenaManager.h"
#include "NtlIniFile.h"
#include "GameServer.h"
#include "ObjectManager.h"
#include "CPlayer.h"
#include "NtlPacketGU.h"
#include "NtlPacketGT.h"
#include "NtlAdmin.h"
#include "GameMain.h"
#include "World.h"
#include "WorldTable.h"
#include "TableContainerManager.h"
#include "SystemEffectTable.h"
#include "SkillTable.h"
#include "ItemManager.h"
#include "NtlLog.h"
#include "Monster.h"
#include "Party.h"
#include "Guild.h"
#include "RankBattle.h"
#include "NtlRankBattle.h"
#include "NtlResultCode.h"
#include "RankBattleTable.h"
#include "NtlRandom.h"
#include "NtlStringHandler.h"
#include <tchar.h>
#include <set>
#include <cstdarg>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <algorithm>
#include "ArenaWorld.h"

static unsigned long ToMs(unsigned int seconds) { return seconds * 1000UL; }

// Name-based check: is the given world tblidx our Arena world (map name contains "TORNEOPODER")?
static inline bool IsArenaWorldByTblidxName(TBLIDX worldTblidx)
{
	sWORLD_TBLDAT* pWorld = (sWORLD_TBLDAT*)g_pTableContainer->GetWorldTable()->FindData(worldTblidx);
	return pWorld && ArenaWorld::IsArenaWorldByWideName(pWorld->wszName);
}

// Overload: use existing CWorld* when available (no table lookup)
static inline bool IsArenaWorld(const CWorld* pWorld)
{
	if (!pWorld) return false;
	// GetTbldat is non-const in this codebase; we only read from it here.
	sWORLD_TBLDAT* pTbldat = const_cast<CWorld*>(pWorld)->GetTbldat();
	return pTbldat && ArenaWorld::IsArenaWorldByWideName(pTbldat->wszName);
}

// Treat ARENAPODER custom maps (900043/900300) specially: keep their original rule and avoid per-round rotation
static inline bool IsArenaPoderWorldTblidx(unsigned int worldTblidx)
{
	return worldTblidx >= 900043u && worldTblidx <= 900300u;
}

// Gated verbose logging: helper formats into a buffer and logs via ERR_LOG to avoid vararg macro pitfalls
static inline void ArenaErrLog(unsigned int category, const char* fmt, ...)
{
	char buf[1024];
	buf[0] = '\0';
	va_list ap;
	va_start(ap, fmt);
#if defined(_MSC_VER)
	vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, ap);
#else
	vsnprintf(buf, sizeof(buf), fmt, ap);
#endif
	va_end(ap);
	// Write to logs using wide-friendly printer. %S prints char* as wide when UNICODE.
	NTL_PRINT(category, _T("%S"), buf);
}

#ifndef ARENA_VLOG
#define ARENA_VLOG(cfg, category, ...) do { \
	if ((cfg).verboseLogs) { \
		ArenaErrLog((category), __VA_ARGS__); \
	} \
} while (0)
#endif

// Resolve a RankBattle room tblidx for the current arena world (or a safe fallback)
static TBLIDX FindRankBattleTblidxForWorld(TBLIDX worldTblidx)
{
	CRankBattleTable* pRankBattleTable = g_pTableContainer->GetRankBattleTable();
	if (!pRankBattleTable)
		return INVALID_TBLIDX;
	TBLIDX fallback = INVALID_TBLIDX;
	CTable* pBase = reinterpret_cast<CTable*>(pRankBattleTable);
	for (CTable::TABLEIT it = pBase->Begin(); it != pBase->End(); ++it)
	{
		sRANKBATTLE_TBLDAT* rb = (sRANKBATTLE_TBLDAT*)it->second;
		if (!rb) continue;
		if (fallback == INVALID_TBLIDX)
			fallback = rb->tblidx; // remember first as fallback
		if (rb->worldTblidx == worldTblidx)
			return rb->tblidx;
	}
	return fallback; // best-effort non-zero
}

bool CArenaManager::IsPartyModeWorld(unsigned int worldTblidx) const
{
	CRankBattleTable* pRankBattleTable = g_pTableContainer->GetRankBattleTable();
	if (!pRankBattleTable) return false;
	CTable* pBase = reinterpret_cast<CTable*>(pRankBattleTable);
	for (CTable::TABLEIT it = pBase->Begin(); it != pBase->End(); ++it)
	{
		sRANKBATTLE_TBLDAT* rb = (sRANKBATTLE_TBLDAT*)it->second;
		if (!rb) continue;
		if (rb->worldTblidx == (TBLIDX)worldTblidx)
		{
			return rb->byBattleMode == RANKBATTLE_MODE_PARTY;
		}
	}
	return false;
}

bool CArenaManager::IsRankBattleWorld(unsigned int worldTblidx) const
{
	CRankBattleTable* pRankBattleTable = g_pTableContainer->GetRankBattleTable();
	if (!pRankBattleTable) return false;
	CTable* pBase = reinterpret_cast<CTable*>(pRankBattleTable);
	for (CTable::TABLEIT it = pBase->Begin(); it != pBase->End(); ++it)
	{
		sRANKBATTLE_TBLDAT* rb = (sRANKBATTLE_TBLDAT*)it->second;
		if (!rb) continue;
		if (rb->worldTblidx == (TBLIDX)worldTblidx)
			return true;
	}
	return false;
}

bool CArenaManager::ShouldOverrideRuleForWorld(unsigned int worldTblidx) const
{
	// Skip override for custom ARENAPODER maps to preserve their behavior
	if (IsArenaPoderWorldTblidx(worldTblidx))
		return false;
	return true;
}

bool CArenaManager::ShouldRotatePerRoundForWorld(unsigned int worldTblidx) const
{
	// Do not rotate per round on ARENAPODER maps; they were designed to persist across rounds
	if (IsArenaPoderWorldTblidx(worldTblidx))
		return false;
	return true;
}

void CArenaManager::RevertWorldRuleOverrides()
{
	if (m_worldsWithOverride.empty()) return;
	CGameServer* app = (CGameServer*)g_pApp;
	for (auto wid : m_worldsWithOverride)
	{
		if (CWorld* pW = app->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)wid))
		{
			pW->ClearRuleOverride();
		}
	}
	m_worldsWithOverride.clear();
}

CArenaManager::CArenaManager()
{
	m_state = State::IDLE;
	m_mode = Mode::OPEN;
	m_currentWorldTblidx = 0;
	m_currentWorldId = 0; // Initialize new worldId member
	m_worldIndex = 0;
	m_rotationRemainMs = 0;
	m_roundUiActive = false;
	m_roundRemainMs = 0;
	m_roundWorldId = 0;

	// Initialize rank battle state
	m_rankBattleState = INVALID_RANKBATTLE_BATTLESTATE;
	m_rankBattleStage = 0;
	m_rankStateTimeMs = 0;

	// Clear all participant/spectator lists
	m_participants.clear();
	m_spectators.clear();
	m_winners.clear();

	NTL_PRINT(PRINT_APP, _T("[ARENA] ArenaManager initialized - state=IDLE, enabled=false"));
}

CArenaManager::~CArenaManager() {}

bool CArenaManager::LoadConfigFromIniPath(const char* iniPath)
{
	CNtlIniFile file;
	if (file.Create(iniPath) != NTL_SUCCESS)
		return false;

	// [Arena]
	int enabled = 0;
	if (file.Read("Arena", "Enabled", enabled))
	{
		m_cfg.enabled = (enabled != 0);
	}

	int onlyOnArena = 1;
	if (file.Read("Arena", "OnlyOnArenaChannel", onlyOnArena))
	{
		m_cfg.onlyOnArenaChannel = (onlyOnArena != 0);
	}

	CNtlString worldsCsv = file.Read("Arena", "WorldTblidxList");
	ParseWorldListCsv(worldsCsv);
	// Keep a copy of config-provided worlds so we can optionally merge them later
	std::vector<unsigned int> cfgWorlds = m_cfg.worldTblidxList;

	// Optional: explicit list of worlds that should use Budokai-style packets (fallback to 30000/41000)
	m_cfg.budokaiWorldTblidxList.clear();
	CNtlString budoCsv = file.Read("Arena", "BudokaiWorldTblidxList");
	if (!std::string(budoCsv.c_str()).empty())
	{
		std::string s = budoCsv.c_str();
		size_t pos = 0;
		while (pos != std::string::npos)
		{
			size_t comma = s.find(',', pos);
			std::string tok = s.substr(pos, comma == std::string::npos ? std::string::npos : (comma - pos));
			if (!tok.empty())
			{
				unsigned int v = (unsigned int)strtoul(tok.c_str(), nullptr, 10);
				if (v != 0)
					m_cfg.budokaiWorldTblidxList.push_back(v);
			}
			if (comma == std::string::npos) break;
			pos = comma + 1;
			while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\t')) ++pos;
		}
	}

	// Log the configured worlds and validate they exist
	if (!m_cfg.worldTblidxList.empty())
	{
		NTL_PRINT(PRINT_APP, _T("[ARENA] Configured worlds: %u entries"), (unsigned)m_cfg.worldTblidxList.size());
		for (size_t i = 0; i < m_cfg.worldTblidxList.size(); ++i)
		{
			unsigned int tblidx = m_cfg.worldTblidxList[i];
			sWORLD_TBLDAT* pWorldTbldat = (sWORLD_TBLDAT*)g_pTableContainer->GetWorldTable()->FindData((TBLIDX)tblidx);
			if (pWorldTbldat)
			{
				NTL_PRINT(PRINT_APP, _T("[ARENA] World %u: tblidx=%u name='%s' - OK"), (unsigned)i, tblidx, pWorldTbldat->wszName);
			}
			else
			{
				NTL_PRINT(PRINT_APP, _T("[ARENA] World %u: tblidx=%u - ERROR: Not found in World Table!"), (unsigned)i, tblidx);
			}
		}
	}
	else
	{
		NTL_PRINT(PRINT_APP, _T("[ARENA] Warning: No worlds configured in WorldTblidxList"));
	}

	// Mobs
	int mobsAllowed = 0;
	if (file.Read("Arena", "MobsAllowed", mobsAllowed)) m_cfg.mobsAllowed = (mobsAllowed != 0);
	CNtlString mobsCsv = file.Read("Arena", "Mobs");
	ParseMobListCsv(mobsCsv);
	int cfgRandomMobs = 0, cfgPerWave = 0, cfgWaveSec = 0;
	if (file.Read("Arena", "RandomMobsSpawn", cfgRandomMobs)) m_cfg.randomMobsSpawn = (cfgRandomMobs != 0);
	if (file.Read("Arena", "RandomMobsPerWave", cfgPerWave) && cfgPerWave > 0) m_cfg.randomMobsPerWave = (unsigned)cfgPerWave;
	if (file.Read("Arena", "RandomMobsWaveSeconds", cfgWaveSec) && cfgWaveSec > 0) m_cfg.randomMobsWaveSeconds = (unsigned)cfgWaveSec;
	m_cfg.mobListFile = file.Read("Arena", "MobListFile");
	m_cfg.mobPreset = file.Read("Arena", "MobPreset");
	if (m_cfg.mobListFile.c_str() && m_cfg.mobListFile.c_str()[0] != '\0')
	{
		LoadMobListFile(m_cfg.mobListFile.c_str());
	}

	unsigned int rotSec = 0;
	if (file.Read("Arena", "RotationSeconds", rotSec)) m_cfg.rotationSeconds = rotSec;
	unsigned int roundSec = 0;
	if (file.Read("Arena", "RoundTimerSeconds", roundSec)) m_cfg.roundTimerSeconds = roundSec;
	unsigned int roundsCount = 0;
	if (file.Read("Arena", "RoundsCount", roundsCount)) m_cfg.roundsCount = roundsCount;
	unsigned int startDelaySec = 0;
	if (file.Read("Arena", "StartDelaySeconds", startDelaySec)) m_cfg.startDelaySeconds = startDelaySec;
	unsigned int maxWaitAllArriveSec = 0;
	if (file.Read("Arena", "MaxWaitAllArriveSeconds", maxWaitAllArriveSec)) m_cfg.maxWaitAllArriveSeconds = maxWaitAllArriveSec;
	unsigned int rankStepMs = 0;
	if (file.Read("Arena", "RankSequenceStepMs", rankStepMs)) m_cfg.rankSequenceStepMs = rankStepMs;
	unsigned int enterReadyDelay = 0;
	if (file.Read("Arena", "EnterReadyDelayMs", enterReadyDelay)) m_cfg.enterReadyDelayMs = enterReadyDelay;
	unsigned int postReadyDelay = 0;
	if (file.Read("Arena", "PostReadyDelayMs", postReadyDelay)) m_cfg.postReadyDelayMs = postReadyDelay;
	int randomizeMapOnStart = 0;
	if (file.Read("Arena", "RandomizeMapOnStart", randomizeMapOnStart)) m_cfg.randomizeMapOnStart = (randomizeMapOnStart != 0);
	int mapPerRound = 0;
	if (file.Read("Arena", "MapPerRound", mapPerRound)) m_cfg.mapPerRound = (mapPerRound != 0);
	int useInvite = 0;
	if (file.Read("Arena", "UseInviteFlow", useInvite)) m_cfg.useInviteFlow = (useInvite != 0);
	unsigned int inviteWait = 15;
	if (file.Read("Arena", "InviteWaitSeconds", inviteWait)) m_cfg.inviteWaitSeconds = inviteWait;
	int rankUi = 0;
	if (file.Read("Arena", "RankUiEnabled", rankUi)) m_cfg.rankUiEnabled = (rankUi != 0);
	int rankPackets = 0;
	if (file.Read("Arena", "RankPacketsEnabled", rankPackets)) m_cfg.rankPacketsEnabled = (rankPackets != 0);
	int allowBudokaiRule = 0;
	// Default: 0 (false) — exclude Budokai-rule maps from Arena rotation/creation
	if (file.Read("Arena", "AllowBudokaiRuleWorlds", allowBudokaiRule)) m_cfg.allowBudokaiRuleWorlds = (allowBudokaiRule != 0);
	int stopOnTimeout = 1;
	if (file.Read("Arena", "StopOnTimeout", stopOnTimeout)) m_cfg.stopOnTimeout = (stopOnTimeout != 0);
	int reviveOnFaint = 0;
	if (file.Read("Arena", "ReviveOnFaint", reviveOnFaint)) m_cfg.reviveOnFaint = (reviveOnFaint != 0);
	unsigned int reviveDelayMs = 0;
	if (file.Read("Arena", "ReviveDelayMs", reviveDelayMs)) m_cfg.reviveDelayMs = reviveDelayMs;
	unsigned int reviveProtectMs = 0;
	if (file.Read("Arena", "ReviveProtectMs", reviveProtectMs)) m_cfg.reviveProtectMs = reviveProtectMs;
	int faintBecomeSpectator = 1;
	if (file.Read("Arena", "FaintBecomeSpectator", faintBecomeSpectator)) m_cfg.faintBecomeSpectator = (faintBecomeSpectator != 0);
	{
		int scoreOnFaint = m_cfg.scoreOnFaint ? 1 : 0;
		if (file.Read("Arena", "ScoreOnFaint", scoreOnFaint)) m_cfg.scoreOnFaint = (scoreOnFaint != 0);
	}
	int spectatorHide = 1;
	if (file.Read("Spectator", "Hide", spectatorHide)) m_cfg.spectatorHide = (spectatorHide != 0);
	unsigned int noticeType = 3;
	if (file.Read("Arena", "NoticeType", noticeType)) m_cfg.noticeType = (unsigned char)noticeType;

	// Telecast
	int telecastEnabled = 0;
	if (file.Read("Arena", "TelecastEnabled", telecastEnabled)) m_cfg.telecastEnabled = (telecastEnabled != 0);
	unsigned int telecastType = 3;
	if (file.Read("Arena", "TelecastType", telecastType)) m_cfg.telecastType = (unsigned char)telecastType;
	unsigned int telecastSpeechTblidx = 0;
	if (file.Read("Arena", "TelecastSpeechTblidx", telecastSpeechTblidx)) m_cfg.telecastSpeechTblidx = telecastSpeechTblidx;
	unsigned int telecastDisplayMs = 5000;
	if (file.Read("Arena", "TelecastDisplayMs", telecastDisplayMs)) m_cfg.telecastDisplayMs = telecastDisplayMs;

	// Post-finish teleport
	int postFinishTp = 0;
	if (file.Read("Arena", "PostFinishTeleport", postFinishTp)) m_cfg.postFinishTeleport = (postFinishTp != 0);
	unsigned int postWorld = 0;
	if (file.Read("Arena", "PostFinishWorldTblidx", postWorld)) m_cfg.postFinishWorldTblidx = postWorld;
	float pfx = 0, pfy = 0, pfz = 0;
	if (file.Read("Arena", "PostFinishPosX", pfx)) m_cfg.postFinishPosX = pfx;
	if (file.Read("Arena", "PostFinishPosY", pfy)) m_cfg.postFinishPosY = pfy;
	if (file.Read("Arena", "PostFinishPosZ", pfz)) m_cfg.postFinishPosZ = pfz;
	unsigned int postDelayMs = 0;
	if (file.Read("Arena", "PostFinishTeleportDelayMs", postDelayMs)) m_cfg.postFinishTeleportDelayMs = postDelayMs;
	// Optional direction
	float pfdx = 0, pfdy = 0, pfdz = 0;
	if (file.Read("Arena", "PostFinishDirX", pfdx)) m_cfg.postFinishDirX = pfdx;
	if (file.Read("Arena", "PostFinishDirY", pfdy)) m_cfg.postFinishDirY = pfdy;
	if (file.Read("Arena", "PostFinishDirZ", pfdz)) m_cfg.postFinishDirZ = pfdz;
	int keepRankUiAfterFinish = 0;
	if (file.Read("Arena", "KeepRankUiAfterFinish", keepRankUiAfterFinish)) m_cfg.keepRankUiAfterFinish = (keepRankUiAfterFinish != 0);

	// Provide safe defaults if user wants a fixed return point but omitted values
	// Default destination requested: world tblidx 1, MapInfoIndex 200101011, CurLoc (4975.609863, -48.869999, 4012.609863), CurDir (0.911100, -0.412000)
	// REMOVED: Don't force teleport to invalid world 1 if PostFinishWorldTblidx = 0
	// if (m_cfg.postFinishTeleport && m_cfg.postFinishWorldTblidx == 0)
	// {
	//     m_cfg.postFinishWorldTblidx = 1; // WorldTblidx 1
	// }
	if (m_cfg.postFinishTeleport)
	{
		if (m_cfg.postFinishPosX == 0 && m_cfg.postFinishPosY == 0 && m_cfg.postFinishPosZ == 0)
		{
			m_cfg.postFinishPosX = 4975.609863f;
			m_cfg.postFinishPosY = -48.869999f;
			m_cfg.postFinishPosZ = 4012.609863f;
		}
		if (m_cfg.postFinishDirX == 0 && m_cfg.postFinishDirY == 0 && m_cfg.postFinishDirZ == 0)
		{
			m_cfg.postFinishDirX = 0.911100f;
			m_cfg.postFinishDirY = -0.412000f;
			m_cfg.postFinishDirZ = 0.0f; // forward z can be 0 if dir is normalized on X/Y plane
		}
	}

	// CC Battle Mode (RankBattle style)
	int ccBattleMode = 0;
	if (file.Read("Arena", "CCBattleMode", ccBattleMode)) m_cfg.ccBattleMode = (ccBattleMode != 0);

	// Verbose logging (diagnostics)
	int verboseLogs = 0;
	if (file.Read("Arena", "VerboseLogs", verboseLogs)) m_cfg.verboseLogs = (verboseLogs != 0);
	ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA] VerboseLogs enabled=%d", m_cfg.verboseLogs ? 1 : 0);

	// Watchdog settings (optional)
	int watchdogEnabled = 1;
	if (file.Read("Arena", "WatchdogEnabled", watchdogEnabled)) m_cfg.watchdogEnabled = (watchdogEnabled != 0);
	unsigned int wdPre = 0, wdEnroll = 0, wdRun = 0;
	if (file.Read("Arena", "WatchdogPreRoundSeconds", wdPre)) m_cfg.watchdogPreRoundSeconds = wdPre;
	if (file.Read("Arena", "WatchdogEnrollmentSeconds", wdEnroll)) m_cfg.watchdogEnrollmentSeconds = wdEnroll;
	if (file.Read("Arena", "WatchdogRunHardcapSeconds", wdRun)) m_cfg.watchdogRunHardcapSeconds = wdRun;
	ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA] Watchdog cfg: enabled=%d pre=%us enroll=%us runCap=%us",
		m_cfg.watchdogEnabled ? 1 : 0, (unsigned)m_cfg.watchdogPreRoundSeconds, (unsigned)m_cfg.watchdogEnrollmentSeconds, (unsigned)m_cfg.watchdogRunHardcapSeconds);

	// Attackability reliability tuning
	unsigned int unlockSleepMs = 0;
	if (file.Read("Arena", "UnlockPulseSleepMs", unlockSleepMs) && unlockSleepMs > 0) m_cfg.unlockPulseSleepMs = unlockSleepMs;
	unsigned int unlockAttempts = 0;
	if (file.Read("Arena", "UnlockDefaultAttempts", unlockAttempts) && unlockAttempts > 0) m_cfg.unlockDefaultAttempts = unlockAttempts;

	// Allow custom world overrides (GM commands, config WorldTblidxList)
	int allowCustomWorlds = 0;
	if (file.Read("Arena", "AllowCustomWorlds", allowCustomWorlds)) m_cfg.allowCustomWorlds = (allowCustomWorlds != 0);
	int useOnlyCustom = 0;
	if (file.Read("Arena", "UseOnlyCustomWorlds", useOnlyCustom)) m_cfg.useOnlyCustomWorlds = (useOnlyCustom != 0);

	// [AutoArena]
	int autoOn = 0;
	if (file.Read("AutoArena", "Enabled", autoOn)) m_cfg.autoEnabled = (autoOn != 0);
	m_cfg.autoChannelName = file.Read("AutoArena", "ChannelNameContains");
	unsigned int autoInterval = 0;
	if (file.Read("AutoArena", "IntervalSeconds", autoInterval) && autoInterval > 0) m_cfg.autoIntervalSeconds = autoInterval;
	unsigned int autoInitDelay = 0;
	if (file.Read("AutoArena", "InitialDelaySeconds", autoInitDelay) && autoInitDelay >= 0) m_cfg.autoInitialDelaySeconds = autoInitDelay;
	unsigned int enrollSec = 0;
	if (file.Read("AutoArena", "EnrollmentSeconds", enrollSec) && enrollSec > 0) m_cfg.autoEnrollmentSeconds = enrollSec;
	unsigned int autoWorld = 0;
	if (file.Read("AutoArena", "WorldTblidx", autoWorld) && autoWorld > 0) m_cfg.autoWorldTblidx = autoWorld;
	CNtlString autoMode = file.Read("AutoArena", "Mode"); // FFA | PARTY (default FFA)
	if (autoMode.c_str())
	{
		std::string s = autoMode.c_str();
		for (auto& c : s) c = (char)tolower(c);
		if (s == "party") m_cfg.autoMode = Mode::PARTY_VS_PARTY; else m_cfg.autoMode = Mode::FREE_FOR_ALL;
	}
	int autoElim = 1;
	if (file.Read("AutoArena", "UseElimination", autoElim)) m_cfg.autoUseElimination = (autoElim != 0);

	// AutoArena optional CSV world list: AutoWorldTblidxList = 900043, 10000, 13000
	m_cfg.autoWorldTblidxList.clear();
	CNtlString autoWorldsCsv = file.Read("AutoArena", "AutoWorldTblidxList");
	if (autoWorldsCsv.c_str())
	{
		std::string s = autoWorldsCsv.c_str();
		size_t pos = 0;
		while (pos != std::string::npos)
		{
			size_t comma = s.find(',', pos);
			std::string tok = s.substr(pos, comma == std::string::npos ? std::string::npos : (comma - pos));
			// trim
			while (!tok.empty() && (tok.front() == ' ' || tok.front() == '\t')) tok.erase(tok.begin());
			while (!tok.empty() && (tok.back() == ' ' || tok.back() == '\t')) tok.pop_back();
			if (!tok.empty())
			{
				unsigned int v = (unsigned int)strtoul(tok.c_str(), nullptr, 10);
				if (v != 0) m_cfg.autoWorldTblidxList.push_back(v);
			}
			if (comma == std::string::npos) break;
			pos = comma + 1;
		}
	}
	int autoRand = 0;
	if (file.Read("AutoArena", "RandomizeWorlds", autoRand)) m_cfg.autoRandomizeWorlds = (autoRand != 0);
	int autoMpr = 0;
	if (file.Read("AutoArena", "MapPerRound", autoMpr)) m_cfg.autoMapPerRound = (autoMpr != 0);

	// [Spectator]
	int specEnabled = 0;
	if (file.Read("Spectator", "Enabled", specEnabled)) m_cfg.spectatorsEnabled = (specEnabled != 0);
	int specSame = 1;
	if (file.Read("Spectator", "UseSameWorld", specSame)) m_cfg.spectatorsUseSameWorld = (specSame != 0);
	unsigned int specWorld = 0;
	if (file.Read("Spectator", "WorldTblidx", specWorld)) m_cfg.spectatorWorldTblidx = specWorld;
	float fx = 0, fy = 0, fz = 0;
	if (file.Read("Spectator", "PosX", fx)) m_cfg.spectatorPosX = fx;
	if (file.Read("Spectator", "PosY", fy)) m_cfg.spectatorPosY = fy;
	if (file.Read("Spectator", "PosZ", fz)) m_cfg.spectatorPosZ = fz;

	// [Rewards]
	int rewardsEnabled = 0;
	if (file.Read("Rewards", "Enabled", rewardsEnabled)) m_cfg.rewardsEnabled = (rewardsEnabled != 0);
	CNtlString winCsv = file.Read("Rewards", "Winners");
	CNtlString partCsv = file.Read("Rewards", "Participants");
	ParseRewardsCsv(winCsv, m_cfg.winnerRewards);
	ParseRewardsCsv(partCsv, m_cfg.participantRewards);
	// Optional Mudosa point rewards
	int mudosaWin = 0, mudosaPart = 0;
	if (file.Read("Rewards", "MudosaWinnerPoints", mudosaWin) && mudosaWin > 0)
		m_cfg.mudosaWinnerPoints = (unsigned)mudosaWin;
	if (file.Read("Rewards", "MudosaParticipantPoints", mudosaPart) && mudosaPart > 0)
		m_cfg.mudosaParticipantPoints = (unsigned)mudosaPart;

	// World list resolution strategy
	// 1) If UseOnlyCustomWorlds=true: use only cfgWorlds (validated)
	// 2) Else: load RankBattle worlds, and merge cfgWorlds if AllowCustomWorlds=true
	size_t originalWorldCount = m_cfg.worldTblidxList.size();
	NTL_PRINT(PRINT_APP, _T("[ARENA] World list mode: useOnlyCustom=%d allowCustom=%d cfgCount=%u"), m_cfg.useOnlyCustomWorlds ? 1 : 0, m_cfg.allowCustomWorlds ? 1 : 0, (unsigned)originalWorldCount);

	if (m_cfg.useOnlyCustomWorlds)
	{
		// Validate and de-duplicate config worlds only
		std::vector<unsigned int> validated;
		validated.reserve(cfgWorlds.size());
		std::set<unsigned int> seen;
		unsigned int invalid = 0;
		for (unsigned int tblidx : cfgWorlds)
		{
			if (tblidx == 0) continue;
			if (seen.find(tblidx) != seen.end()) continue;
			sWORLD_TBLDAT* pWorld = (sWORLD_TBLDAT*)g_pTableContainer->GetWorldTable()->FindData((TBLIDX)tblidx);
			if (!pWorld) { ++invalid; continue; }
			validated.push_back(tblidx);
			seen.insert(tblidx);
		}
		m_cfg.worldTblidxList.swap(validated);
		NTL_PRINT(PRINT_APP, _T("[ARENA] Using only custom worlds: total=%u invalid=%u"), (unsigned)m_cfg.worldTblidxList.size(), invalid);
	}
	else
	{
		// Load from table and optionally merge custom worlds
		LoadAvailableWorlds();
		// If allowed, merge custom worlds from config into the RankBattle-derived list (dedup + validate)
		if (m_cfg.allowCustomWorlds && !cfgWorlds.empty())
		{
			std::set<unsigned int> existing(m_cfg.worldTblidxList.begin(), m_cfg.worldTblidxList.end());
			unsigned int added = 0, invalid = 0;
			for (unsigned int tblidx : cfgWorlds)
			{
				if (tblidx == 0) continue;
				if (existing.find(tblidx) != existing.end()) continue; // already present
				// validate
				sWORLD_TBLDAT* pWorldTbldat = (sWORLD_TBLDAT*)g_pTableContainer->GetWorldTable()->FindData((TBLIDX)tblidx);
				if (!pWorldTbldat)
				{
					++invalid;
					continue;
				}
				m_cfg.worldTblidxList.push_back(tblidx);
				existing.insert(tblidx);
				++added;
			}
			if (added > 0 || invalid > 0)
			{
				NTL_PRINT(PRINT_APP, _T("[ARENA] Custom world merge: added=%u invalid=%u totalNow=%u"), added, invalid, (unsigned)m_cfg.worldTblidxList.size());
			}
		}
	}

	// If allowed, merge custom worlds from config into the RankBattle-derived list (dedup + validate)
	if (m_cfg.allowCustomWorlds && !cfgWorlds.empty())
	{
		std::set<unsigned int> existing(m_cfg.worldTblidxList.begin(), m_cfg.worldTblidxList.end());
		unsigned int added = 0, invalid = 0;
		for (unsigned int tblidx : cfgWorlds)
		{
			if (tblidx == 0) continue;
			if (existing.find(tblidx) != existing.end()) continue; // already present
			// validate
			sWORLD_TBLDAT* pWorldTbldat = (sWORLD_TBLDAT*)g_pTableContainer->GetWorldTable()->FindData((TBLIDX)tblidx);
			if (!pWorldTbldat)
			{
				++invalid;
				continue;
			}
			m_cfg.worldTblidxList.push_back(tblidx);
			existing.insert(tblidx);
			++added;
		}
		if (added > 0 || invalid > 0)
		{
			NTL_PRINT(PRINT_APP, _T("[ARENA] Custom world merge: added=%u invalid=%u totalNow=%u"), added, invalid, (unsigned)m_cfg.worldTblidxList.size());
		}
	}

	// By default, exclude Budokai-rule worlds from rotation to avoid packet-family mismatch unless explicitly allowed
	FilterOutBudokaiRuleWorlds();

	NTL_PRINT(PRINT_APP, _T("[ARENA] After LoadAvailableWorlds: %u worlds loaded"), (unsigned)m_cfg.worldTblidxList.size());

	// If no worlds remain after resolution and we had config worlds, fall back to raw config list (validated again)
	if (m_cfg.worldTblidxList.empty() && originalWorldCount > 0)
	{
		NTL_PRINT(PRINT_APP, _T("[ARENA] No worlds available after resolution. Falling back to config WorldTblidxList only."));
		CNtlString worldsCsv2 = file.Read("Arena", "WorldTblidxList");
		m_cfg.worldTblidxList.clear();
		ParseWorldListCsv(worldsCsv2);
		// validate again
		std::vector<unsigned int> validated;
		for (unsigned int tblidx : m_cfg.worldTblidxList)
		{
			sWORLD_TBLDAT* pWorld = (sWORLD_TBLDAT*)g_pTableContainer->GetWorldTable()->FindData((TBLIDX)tblidx);
			if (pWorld) validated.push_back(tblidx);
		}
		m_cfg.worldTblidxList.swap(validated);
		NTL_PRINT(PRINT_APP, _T("[ARENA] After config restore: %u worlds loaded"), (unsigned)m_cfg.worldTblidxList.size());
	}
	else if (m_cfg.worldTblidxList.empty())
	{
		NTL_PRINT(PRINT_APP, _T("[ARENA] No worlds available - RankBattle table empty and no config worlds provided"));
	}

	// Initialize rotation
	if (!m_cfg.worldTblidxList.empty())
	{
		m_worldIndex = 0;
		m_currentWorldTblidx = m_cfg.worldTblidxList[m_worldIndex];
		m_rotationRemainMs = ToMs(m_cfg.rotationSeconds);
		NTL_PRINT(PRINT_APP, _T("[ARENA] Final initialization: currentWorldTblidx=%u from %u total worlds"),
			m_currentWorldTblidx, (unsigned)m_cfg.worldTblidxList.size());
		// Emit a compact configuration summary to verify INI application at runtime
		NTL_PRINT(PRINT_APP, _T("[ARENA] Cfg: enabled=%d rounds=%u roundSec=%u startDelay=%u stopOnTimeout=%d inviteFlow=%d inviteWait=%u rankUi=%d rankPkts=%d noticeType=%u mobs=%d worlds=%u randMobs=%d wave=%u/%us preset='%S'"),
			(int)m_cfg.enabled, (unsigned)m_cfg.roundsCount, (unsigned)m_cfg.roundTimerSeconds, (unsigned)m_cfg.startDelaySeconds,
			(int)m_cfg.stopOnTimeout, (int)m_cfg.useInviteFlow, (unsigned)m_cfg.inviteWaitSeconds, (int)m_cfg.rankUiEnabled, (int)m_cfg.rankPacketsEnabled,
			(unsigned)m_cfg.noticeType, (int)m_cfg.mobsAllowed, (unsigned)m_cfg.worldTblidxList.size(),
			m_cfg.randomMobsSpawn ? 1 : 0, (unsigned)m_cfg.randomMobsPerWave, (unsigned)m_cfg.randomMobsWaveSeconds, m_cfg.mobPreset.c_str());
	}
	else
	{
		NTL_PRINT(PRINT_APP, _T("[ARENA] Warning: No worlds available - arena will not function properly"));
	}

	return true;
}

bool CArenaManager::GetPrevLocation(unsigned int charId, unsigned int& outWorldId, CNtlVector& outLoc, CNtlVector& outDir) const
{
	auto it = m_prevLoc.find(charId);
	if (it == m_prevLoc.end())
		return false;
	if (it->second.worldId == INVALID_WORLDID)
		return false;
	outWorldId = it->second.worldId;
	outLoc = it->second.loc;
	outDir = it->second.dir;
	return true;
}

// Count participants present in any world instance matching the given world table index
unsigned CArenaManager::CountParticipantsInWorldTblidx(unsigned int worldTblidx)
{
	unsigned count = 0;
	for (auto cid : m_participants)
	{
		if (CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid))
		{
			if (p->IsInitialized() && (unsigned int)p->GetWorldTblidx() == worldTblidx)
				++count;
		}
	}
	return count;
}

// Returns the worldId of the first participant found in any world instance matching the given world tblidx
unsigned CArenaManager::GetFirstParticipantWorldIdForTblidx(unsigned int worldTblidx)
{
	for (auto cid : m_participants)
	{
		if (CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid))
		{
			if (p->IsInitialized() && (unsigned int)p->GetWorldTblidx() == worldTblidx)
				return (unsigned int)p->GetWorldID();
		}
	}
	return 0;
}

// --- Runtime configuration setters ---
void CArenaManager::SetAllowCustomWorlds(bool on)
{
	m_cfg.allowCustomWorlds = on;
	// If turning off custom worlds and current world isn't RankBattle, migrate to first RankBattle world
	if (!on)
	{
		// Rebuild to rank-only list and clamp current selection
		RebuildWorldList_RankOnly();
	}
}

void CArenaManager::SetUseOnlyCustomWorlds(bool on)
{
	m_cfg.useOnlyCustomWorlds = on;
	if (on)
	{
		// Only-custom implies custom worlds must be allowed
		m_cfg.allowCustomWorlds = true;
		// Filter current list to custom-only (exclude RankBattle worlds)
		if (!m_cfg.worldTblidxList.empty())
		{
			std::vector<unsigned int> customOnly;
			customOnly.reserve(m_cfg.worldTblidxList.size());
			for (unsigned int tblidx : m_cfg.worldTblidxList)
			{
				if (!IsRankBattleWorld(tblidx))
					customOnly.push_back(tblidx);
			}
			if (!customOnly.empty())
			{
				m_cfg.worldTblidxList.swap(customOnly);
				m_worldIndex = 0;
				m_currentWorldTblidx = m_cfg.worldTblidxList[m_worldIndex];
				m_currentWorldId = 0; // force custom world creation
			}
		}
	}
	if (on)
	{
		// When forcing only custom, clear list and wait for SetWorldListCsv or keep current if valid
		if (m_cfg.worldTblidxList.empty())
		{
			// no-op: expect caller to set a list
		}
	}
	else
	{
		// Return to RankBattle-based list
		RebuildWorldList_RankOnly();
	}
}

void CArenaManager::SetAllowBudokaiRuleWorlds(bool on)
{
	m_cfg.allowBudokaiRuleWorlds = on;
	// Re-apply filter if turning off
	if (!on)
		FilterOutBudokaiRuleWorlds();
}

void CArenaManager::SetRandomizeMapOnStart(bool on)
{
	m_cfg.randomizeMapOnStart = on;
}

void CArenaManager::SetRotationSeconds(unsigned int seconds)
{
	m_cfg.rotationSeconds = seconds;
	m_rotationRemainMs = seconds ? (seconds * 1000UL) : 0;
}

void CArenaManager::RebuildWorldList_RankOnly()
{
	// Preserve flags but rebuild list strictly from RankBattle table
	std::vector<unsigned int> before = m_cfg.worldTblidxList;
	LoadAvailableWorlds();
	// Ensure Budokai filter if disabled
	FilterOutBudokaiRuleWorlds();
	// Clamp index and reset current world id to force recreation
	if (m_cfg.worldTblidxList.empty())
	{
		m_currentWorldTblidx = 0;
		m_currentWorldId = 0;
		return;
	}
	if (m_worldIndex >= m_cfg.worldTblidxList.size()) m_worldIndex = 0;
	m_currentWorldTblidx = m_cfg.worldTblidxList[m_worldIndex];
	m_currentWorldId = 0;
}

void CArenaManager::SetWorldListCsv(const std::string& csv)
{
	// Replace list from CSV and validate
	m_cfg.worldTblidxList.clear();
	CNtlString s(csv.c_str());
	ParseWorldListCsv(s);
	// Apply Budokai filter if disabled
	FilterOutBudokaiRuleWorlds();
	// In only-custom mode, remove RankBattle worlds from the list to avoid falling back
	if (m_cfg.useOnlyCustomWorlds && !m_cfg.worldTblidxList.empty())
	{
		std::vector<unsigned int> customOnly;
		customOnly.reserve(m_cfg.worldTblidxList.size());
		for (unsigned int tblidx : m_cfg.worldTblidxList)
		{
			if (!IsRankBattleWorld(tblidx)) customOnly.push_back(tblidx);
		}
		m_cfg.worldTblidxList.swap(customOnly);
	}
	if (m_cfg.worldTblidxList.empty())
	{
		m_currentWorldTblidx = 0;
		m_currentWorldId = 0;
		return;
	}
	m_worldIndex = 0;
	m_currentWorldTblidx = m_cfg.worldTblidxList[m_worldIndex];
	m_currentWorldId = 0;
}

void CArenaManager::ShowCfgTo(CPlayer* pWho)
{
	if (!pWho) return;
	wchar_t msg[256];
	swprintf_s(msg, _countof(msg), L"[ArenaCfg] custom=%d onlyCustom=%d budokai=%d randOnStart=%d rot=%us worlds=%u cur=%u",
		m_cfg.allowCustomWorlds ? 1 : 0,
		m_cfg.useOnlyCustomWorlds ? 1 : 0,
		m_cfg.allowBudokaiRuleWorlds ? 1 : 0,
		m_cfg.randomizeMapOnStart ? 1 : 0,
		(unsigned)(m_cfg.rotationSeconds),
		(unsigned)m_cfg.worldTblidxList.size(),
		(unsigned)m_currentWorldTblidx);
	SendSystemTo(pWho, msg, SERVER_TEXT_SYSTEM);
}

void CArenaManager::FilterOutBudokaiRuleWorlds()
{
	if (m_cfg.allowBudokaiRuleWorlds || m_cfg.worldTblidxList.empty())
		return;
	std::vector<unsigned int> filtered;
	filtered.reserve(m_cfg.worldTblidxList.size());
	for (unsigned int tblidx : m_cfg.worldTblidxList)
	{
		sWORLD_TBLDAT* pWorld = (sWORLD_TBLDAT*)g_pTableContainer->GetWorldTable()->FindData((TBLIDX)tblidx);
		if (!pWorld)
			continue;
		BYTE rule = pWorld->byWorldRuleType;
		bool isBudoRule = (rule == GAMERULE_MINORMATCH || rule == GAMERULE_MAJORMATCH || rule == GAMERULE_FINALMATCH);
		if (!isBudoRule)
			filtered.push_back(tblidx);
	}
	if (filtered.size() != m_cfg.worldTblidxList.size())
	{
		NTL_PRINT(PRINT_APP, _T("[ARENA] Filtered Budokai-rule worlds: %u -> %u"), (unsigned)m_cfg.worldTblidxList.size(), (unsigned)filtered.size());
		m_cfg.worldTblidxList.swap(filtered);
		// Reset world index if needed
		if (m_worldIndex >= m_cfg.worldTblidxList.size())
			m_worldIndex = 0;
		if (!m_cfg.worldTblidxList.empty())
			m_currentWorldTblidx = m_cfg.worldTblidxList[m_worldIndex];
	}
}

// Broadcast RANKBATTLE_BATTLE_PLAYER_STATE_NFY with ATTACKABLE for current participants in world
static void ArenaBroadcastRankPlayerAttackableToWorld(CWorld* pWorld, const std::unordered_set<unsigned int>& participants)
{
	if (!pWorld) return;
	// Validate world table data exists
	if (!pWorld->GetTbldat()) return;
	// For each participant in this world, notify only arena participants present in the same world
	std::vector<CPlayer*> receivers;
	receivers.reserve(participants.size());
	for (auto rcid : participants)
	{
		CPlayer* r = g_pObjectManager->FindByChar((CHARACTERID)rcid);
		if (r && r->IsInitialized() && r->GetWorldID() == pWorld->GetID())
			receivers.push_back(r);
	}

	for (auto cid : participants)
	{
		CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid);
		if (!p || !p->IsInitialized()) continue;
		if (p->GetWorldID() != pWorld->GetID()) continue;

		// Ensure server-side rank battle state allows attacks in RANKBATTLE-rule worlds
		sRANK_BATTLE_DATA* rd = p->GetRankBattleData();
		if (rd)
		{
			rd->eState = RANKBATTLE_MEMBER_STATE_ATTACKABLE;
		}
		else
		{
			// Defensive: skip state mutation if rank data missing; still notify client
			NTL_PRINT(PRINT_APP, _T("[ARENA][WARN] Null RankBattleData in rank ATTACKABLE broadcast for char=%u"), (unsigned)p->GetCharID());
		}

		CNtlPacket packet(sizeof(sGU_RANKBATTLE_BATTLE_PLAYER_STATE_NFY));
		sGU_RANKBATTLE_BATTLE_PLAYER_STATE_NFY* res = (sGU_RANKBATTLE_BATTLE_PLAYER_STATE_NFY*)packet.GetPacketData();
		res->wOpCode = GU_RANKBATTLE_BATTLE_PLAYER_STATE_NFY;
		res->hPc = p->GetID();
		res->byPCState = RANKBATTLE_MEMBER_STATE_ATTACKABLE;
		packet.SetPacketLen(sizeof(sGU_RANKBATTLE_BATTLE_PLAYER_STATE_NFY));
		for (CPlayer* recv : receivers)
			recv->SendPacket(&packet);
	}
}

// Broadcast GU_MATCH_*_PLAYER_STATE_NFY (Budokai) for current participants in world
static void ArenaBroadcastBudokaiPlayerStateToWorld(CWorld* pWorld, const std::unordered_set<unsigned int>& participants, BYTE byPcState)
{
	if (!pWorld)
		return;
	if (!pWorld->GetTbldat())
		return;

	BYTE rule = pWorld->GetTbldat()->byWorldRuleType;

	for (auto cid : participants)
	{
		CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid);
		if (!p || !p->IsInitialized() || p->GetWorldID() != pWorld->GetID())
			continue;

		// Set server-side Budokai player state
		p->SetBudokaiPcState(byPcState);

		// Notify clients using the appropriate Budokai packet variant
		if (rule == GAMERULE_MINORMATCH)
		{
			CNtlPacket packet(sizeof(sGU_MATCH_MINORMATCH_PLAYER_STATE_NFY));
			sGU_MATCH_MINORMATCH_PLAYER_STATE_NFY* res = (sGU_MATCH_MINORMATCH_PLAYER_STATE_NFY*)packet.GetPacketData();
			res->wOpCode = GU_MATCH_MINORMATCH_PLAYER_STATE_NFY;
			res->hPc = p->GetID();
			res->byPcState = byPcState;
			packet.SetPacketLen(sizeof(sGU_MATCH_MINORMATCH_PLAYER_STATE_NFY));
			pWorld->Broadcast(&packet);
		}
		else if (rule == GAMERULE_MAJORMATCH)
		{
			CNtlPacket packet(sizeof(sGU_MATCH_MAJORMATCH_PLAYER_STATE_NFY));
			sGU_MATCH_MAJORMATCH_PLAYER_STATE_NFY* res = (sGU_MATCH_MAJORMATCH_PLAYER_STATE_NFY*)packet.GetPacketData();
			res->wOpCode = GU_MATCH_MAJORMATCH_PLAYER_STATE_NFY;
			res->hPc = p->GetID();
			res->byPcState = byPcState;
			packet.SetPacketLen(sizeof(sGU_MATCH_MAJORMATCH_PLAYER_STATE_NFY));
			pWorld->Broadcast(&packet);
		}
		else if (rule == GAMERULE_FINALMATCH)
		{
			CNtlPacket packet(sizeof(sGU_MATCH_FINALMATCH_PLAYER_STATE_NFY));
			sGU_MATCH_FINALMATCH_PLAYER_STATE_NFY* res = (sGU_MATCH_FINALMATCH_PLAYER_STATE_NFY*)packet.GetPacketData();
			res->wOpCode = GU_MATCH_FINALMATCH_PLAYER_STATE_NFY;
			res->hPc = p->GetID();
			res->byPcState = byPcState;
			packet.SetPacketLen(sizeof(sGU_MATCH_FINALMATCH_PLAYER_STATE_NFY));
			pWorld->Broadcast(&packet);
		}
	}
}

void CArenaManager::TickProcess(unsigned long dwTickDiff)
{
	// Do nothing unless the Arena feature is enabled and actively started
	if (!m_cfg.enabled || m_state == State::IDLE)
		return;

	// Track elapsed time in the current arena state for watchdog purposes
	if (m_prevState != m_state)
	{
		m_prevState = m_state;
		m_stateElapsedMs = 0;
	}
	else
	{
		m_stateElapsedMs += dwTickDiff;
	}

	/*ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA][Tick] state=%u runSettleMs=%u roundUi=%d roundRemainMs=%u worldTblidx=%u worldId=%u participants=%u spectators=%u",
		(unsigned)m_state, (unsigned)m_runSettleMs, m_roundUiActive ? 1 : 0, (unsigned)m_roundRemainMs,
		(unsigned)m_currentWorldTblidx, (unsigned)m_currentWorldId, (unsigned)m_participants.size(), (unsigned)m_spectators.size());*/

		// Decrement run-settle window so we don't prematurely end rounds at start
	if (m_runSettleMs > 0)
	{
		if (m_runSettleMs > dwTickDiff) m_runSettleMs -= dwTickDiff; else m_runSettleMs = 0;
	}

	// Handle deferred post-finish teleport and cleanup
	if (m_postFinishTeleportRemainMs > 0)
	{
		if (m_postFinishTeleportRemainMs > dwTickDiff)
			m_postFinishTeleportRemainMs -= dwTickDiff;
		else
		{
			m_postFinishTeleportRemainMs = 0;
			// Perform the same immediate cleanup/teleport flow as in FinishMatch's no-delay branch
			DespawnArenaMobs();
			if (m_cfg.postFinishTeleport)
				PostFinishTeleportAll();
			else
				PostFinishTeleportDefault();
			BroadcastSystem(L"[Arena] Rank mode cleared. You can use normal Rank features again.");
			m_participants.clear();
			m_spectators.clear();
			m_winners.clear();
			m_killPoints.clear();
			m_inviting = false;
			m_inviteRemainMs = 0;
			m_pendingStartMs = 0;
			m_pendingStartWorldId = 0;
			m_waitAllArriveMs = 0;
			m_readyParticipants.clear();
			m_readyDelayMs.clear();
			m_pendingStartParticipants.clear();
			m_postStartDelayMs = 0;
			m_matchFinishWatchdogMs = 0;
			m_rankBattleState = INVALID_RANKBATTLE_BATTLESTATE;
			m_rankBattleStage = 0;
			m_rankStateTimeMs = 0;
			m_state = State::COMPLETE;
			// Stay COMPLETE; if Stop was used, next Start will reset to ENROLLMENT
			ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA] Deferred post-finish teleport executed and cleaned up state");
		}
	}
	// Drive a tiny unlock pulse window to re-send ATTACKABLE shortly after RUN starts
	if (m_runUnlockPulseMs > 0)
	{
		if (m_runUnlockPulseMs > dwTickDiff)
			m_runUnlockPulseMs -= dwTickDiff;
		else
		{
			m_runUnlockPulseMs = 0;
			unsigned int worldId = m_currentWorldId ? m_currentWorldId : EnsureCurrentWorldId();
			if (worldId)
			{
				CGameServer* app2 = (CGameServer*)g_pApp;
				if (CWorld* pWorld2 = app2->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)worldId))
				{
					if (!pWorld2->GetTbldat())
					{
						ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA][WARN] Unlock pulse: world has no tbldat worldId=%u", worldId);
					}
					BYTE rule2 = pWorld2->GetTbldat() ? pWorld2->GetTbldat()->byWorldRuleType : GAMERULE_NORMAL;
					bool isBudokaiRule2 = (rule2 == GAMERULE_MINORMATCH || rule2 == GAMERULE_MAJORMATCH || rule2 == GAMERULE_FINALMATCH);

					// Send multiple unlock signals based on config to ensure attackability is applied
					int unlockAttempts = (int)m_cfg.unlockDefaultAttempts;
					if (unlockAttempts <= 0) unlockAttempts = 1;

					for (int attempt = 0; attempt < unlockAttempts; attempt++)
					{
						if (isBudokaiRule2)
						{
							// Budokai-rule worlds: pulse NORMAL state again for safety
							ArenaBroadcastBudokaiPlayerStateToWorld(pWorld2, m_participants, MATCH_MEMBER_STATE_NORMAL);
							ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA] Unlock pulse %d: re-sent Budokai NORMAL after RUN", attempt + 1);
						}
						else
						{
							// Rank-rule worlds: ensure ATTACKABLE state again
							for (auto cid : m_participants)
							{
								CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid);
								if (p && p->IsInitialized() && (unsigned int)p->GetWorldID() == worldId)
								{
									sRANK_BATTLE_DATA* rd = p->GetRankBattleData();
									if (rd)
									{
										rd->eState = RANKBATTLE_MEMBER_STATE_ATTACKABLE;
										// For problematic worlds, also clear any conditions that might block combat
										p->SendCharStateStanding();
										p->GetStateManager()->RemoveConditionState(CHARCOND_CONFUSED, NULL, true);
										p->GetStateManager()->RemoveConditionState(CHARCOND_TERROR, NULL, true);
										p->GetStateManager()->RemoveConditionState(CHARCOND_ATTACK_DISALLOW, NULL, true);
										p->GetStateManager()->RemoveConditionState(CHARCOND_CANT_BE_TARGETTED, NULL, true);
									}
									else
										ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA][WARN] Unlock pulse: null RankBattleData for char=%u", (unsigned)p->GetCharID());
								}
							}
							ArenaBroadcastRankPlayerAttackableToWorld(pWorld2, m_participants);
							ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA] Unlock pulse %d: re-sent ATTACKABLE after RUN", attempt + 1);
						}
						// As extra safety, clear any combat-restricting conditions again
						ClearCombatRestrictionsForParticipants();

						if (attempt < unlockAttempts - 1)
						{
							// Brief delay between attempts per configuration
							if (m_cfg.unlockPulseSleepMs > 0)
								Sleep(m_cfg.unlockPulseSleepMs);
						}
					}
				}
			}
		}
	}

	// Global watchdog: ensure MATCH_FINISH actually completes even if state machine stalls
	if (m_matchFinishWatchdogMs > 0)
	{
		if (m_matchFinishWatchdogMs > dwTickDiff)
			m_matchFinishWatchdogMs -= dwTickDiff;
		else
		{
			m_matchFinishWatchdogMs = 0;
			if (m_state != State::COMPLETE && m_state != State::IDLE)
			{
				ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA][WATCHDOG] Fired; forcing FinishMatch");
				FinishMatch(false);
				return; // state transitioned; exit early
			}
		}
	}

	// Watchdog: detect stuck states (PRE_ROUND/STAGE_READY/IN_ROUND) and recover safely
	if (m_cfg.watchdogEnabled)
	{
		// Determine dynamic defaults if config is 0
		unsigned int preRoundLimitMs = ToMs(m_cfg.watchdogPreRoundSeconds ? m_cfg.watchdogPreRoundSeconds : (m_cfg.maxWaitAllArriveSeconds ? m_cfg.maxWaitAllArriveSeconds + m_cfg.startDelaySeconds + 10 : 45));
		unsigned int runHardcapMs = 0;
		if (m_cfg.watchdogRunHardcapSeconds)
			runHardcapMs = ToMs(m_cfg.watchdogRunHardcapSeconds);
		else
		{
			// If a round timer UI is configured, add a small buffer; otherwise use a safe cap (10 minutes)
			unsigned int base = (m_cfg.roundTimerSeconds ? (m_cfg.roundTimerSeconds + 15) : 600);
			runHardcapMs = ToMs(base);
		}

		switch (m_state)
		{
		case State::PRE_ROUND:
		case State::MATCH_READY:
		case State::STAGE_READY:
			if (m_stateElapsedMs > preRoundLimitMs)
			{
				ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA][WATCHDOG] PreRound stuck > %ums. Forcing start or reset.", preRoundLimitMs);
				// If we have at least 2 participants online, try to jump into RUN quickly; else reset to ENROLLMENT
				if (CountOnlineParticipants() >= 2)
				{
					// Attempt a minimal start: bind world and send RUN with a short timer
					unsigned int wid = m_currentWorldId ? m_currentWorldId : EnsureCurrentWorldId();
					if (wid)
					{
						m_pendingStartMs = 0;
						m_pendingStartWorldId = 0;
						m_waitAllArriveMs = 0;
						// Initialize HUD minimal state if enabled
						if (m_cfg.rankUiEnabled)
						{
							BroadcastRankJoinToWorld(wid);
							BroadcastRankStateToWorld(wid, RANKBATTLE_BATTLESTATE_STAGE_READY, m_rankBattleStage);
						}
						// Enter RUN with remaining or default timer
						unsigned long runMs = ToMs(m_cfg.roundTimerSeconds ? m_cfg.roundTimerSeconds : 120);
						UpdateRankBattleState(RANKBATTLE_BATTLESTATE_STAGE_RUN, m_rankBattleStage, runMs);
						m_state = State::IN_ROUND;
						m_stateElapsedMs = 0;
						MakeParticipantsAttackable(wid);
						ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA][WATCHDOG] Forced start into RUN with %ums.", (unsigned)(runMs / 1000));
					}
					else
					{
						// Could not ensure a world; reset to enrollment
						m_state = State::ENROLLMENT;
						m_stateElapsedMs = 0;
						ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA][WATCHDOG] Reset to ENROLLMENT due to missing worldId.");
					}
				}
				else
				{
					// Not enough participants; stop and reset cleanly
					Stop(true);
					m_state = State::IDLE;
					m_stateElapsedMs = 0;
				}
			}
			break;
		case State::IN_ROUND:
			// If we have a UI timer, our normal timeout handler will call FinishOnTimeout; otherwise enforce a hard cap
			if (!m_roundUiActive && m_stateElapsedMs > runHardcapMs)
			{
				ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA][WATCHDOG] RUN hardcap exceeded > %ums. Finishing on timeout.", (unsigned)(runHardcapMs / 1000));
				FinishOnTimeout();
			}
			break;
		default:
			break;
		}
	}

	if (m_cfg.reviveOnFaint || m_cfg.scoreOnFaint) {
		// Drive pending delayed revives (only during IN_ROUND)
		if (m_state == State::IN_ROUND && !m_pendingReviveMs.empty())
		{
			std::vector<unsigned int> toRevive;
			for (auto& kv : m_pendingReviveMs)
			{
				if (kv.second > dwTickDiff)
					kv.second -= dwTickDiff;
				else
					toRevive.push_back(kv.first);
			}
			for (unsigned int cid : toRevive)
			{
				m_pendingReviveMs.erase(cid);
				ReviveParticipantNow(cid, /*bApplyRespawnBuff*/true);
			}
		}

		// Drive post-revive protection timers
		if (!m_reviveProtectRemainMs.empty())
		{
			std::vector<unsigned int> toClear;
			for (auto& kv : m_reviveProtectRemainMs)
			{
				if (kv.second > dwTickDiff)
					kv.second -= dwTickDiff;
				else
					toClear.push_back(kv.first);
			}
			for (unsigned int cid : toClear)
			{
				m_reviveProtectRemainMs.erase(cid);
				CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid);
				if (p && p->IsInitialized())
					ClearReviveProtection(p);
			}
		}

	}

	// Auto-detect if participants are already in arena world while in ENROLLMENT state
	// This handles cases where players were teleported manually or the state got stuck
	if (m_state == State::ENROLLMENT && !m_participants.empty())
	{
		ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA][Tick] ENROLLMENT auto-detect: participants=%u", (unsigned)m_participants.size());
		unsigned int arenaWorldId = EnsureCurrentWorldId();
		if (arenaWorldId > 0)
		{
			unsigned int participantsInArena = CountParticipantsInWorld(arenaWorldId);
			ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA][Tick] ENROLLMENT worldId=%u present=%u", arenaWorldId, participantsInArena);
			if (participantsInArena >= 2) // Minimum for arena battle
			{
				// Auto-transition to PRE_ROUND to start the battle sequence
				m_state = State::PRE_ROUND;
				m_pendingStartMs = ToMs(m_cfg.startDelaySeconds);
				m_pendingStartWorldId = arenaWorldId;
				m_currentWorldId = arenaWorldId;
				m_waitAllArriveMs = ToMs(m_cfg.maxWaitAllArriveSeconds);

				// Mark participants already in arena as ready
				for (auto cid : m_participants)
				{
					CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid);
					if (p && p->IsInitialized() && (unsigned int)p->GetWorldID() == arenaWorldId)
					{
						m_readyParticipants.insert(cid);
					}
				}

				BroadcastSystem(L"[Arena] Battle starting - participants detected in arena!");
				ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA] Auto PRE_ROUND: worldId=%u readyCount=%u", arenaWorldId, (unsigned)m_readyParticipants.size());
				SendNotice(L"Arena battle sequence starting...", SERVER_TEXT_SYSNOTICE);
				NTL_PRINT(PRINT_APP, _T("[ARENA] Auto-transitioned from ENROLLMENT to PRE_ROUND: %u participants in arena world %u"), participantsInArena, arenaWorldId);
			}
		}
	}

	// Drive per-player enter-ready delay timers (only meaningful before start)
	if (m_state == State::PRE_ROUND || (m_pendingStartWorldId != 0 && m_waitAllArriveMs > 0))
	{
		if (!m_readyDelayMs.empty())
		{
			std::vector<unsigned int> toMarkReady;
			for (auto& kv : m_readyDelayMs)
			{
				if (kv.second > dwTickDiff)
					kv.second -= dwTickDiff;
				else
					toMarkReady.push_back(kv.first);
			}
			for (unsigned int cid : toMarkReady)
			{
				m_readyDelayMs.erase(cid);
				m_readyParticipants.insert(cid);
			}
		}
	}
	// Handle delayed round start to avoid post-TP freeze
	if (m_pendingStartMs > 0)
	{
		if (m_pendingStartMs > dwTickDiff)
		{
			m_pendingStartMs -= dwTickDiff;
			// Fast-path: if everyone is present and ready already, clamp the remaining delay
			unsigned int worldIdFast = m_currentWorldId ? m_currentWorldId : EnsureCurrentWorldId();
			if (worldIdFast)
			{
				unsigned present = CountParticipantsInWorld(worldIdFast);
				unsigned ready = 0;
				for (auto cid : m_participants)
				{
					if (m_readyParticipants.find(cid) != m_readyParticipants.end())
					{
						CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid);
						if (p && p->IsInitialized() && (unsigned int)p->GetWorldID() == worldIdFast)
							++ready;
					}
				}
				if (present >= 2 && ready >= present && present == m_participants.size())
				{
					if (m_pendingStartMs > 1000)
					{
						m_pendingStartMs = 1000; // clamp to 1s
						ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA] Fast-start clamp: pendingStartMs=1000 worldId=%u present=%u", worldIdFast, present);
					}
				}
			}
		}
		else
		{
			unsigned int worldId = m_pendingStartWorldId ? m_pendingStartWorldId : EnsureCurrentWorldId();
			m_pendingStartMs = 0;
			// Initialize arrival gating grace time
			m_waitAllArriveMs = ToMs(m_cfg.maxWaitAllArriveSeconds);
			// If we have a valid world, hold until everyone online is present or grace expires
			if (worldId)
			{
				m_pendingStartWorldId = worldId;
				// Ensure our current arena world id is bound to this instance so all later broadcasts target it
				m_currentWorldId = worldId;

				// If we're in an active pre-run/run state but have no participants at all, reset to enrollment
				if ((m_state == State::PRE_ROUND || m_state == State::MATCH_READY || m_state == State::STAGE_READY || m_state == State::IN_ROUND)
					&& m_participants.empty())
				{
					StopRoundTimerUI();
					m_pendingStartMs = 0;
					m_pendingStartWorldId = 0;
					m_waitAllArriveMs = 0;
					m_state = State::ENROLLMENT;
					NTL_PRINT(PRINT_APP, _T("[ARENA] Reset to ENROLLMENT: no participants"));
					return;
				}
				m_pendingStartParticipants = m_participants; // snapshot
				// Log the scheduled start
				NTL_PRINT(PRINT_APP, _T("[ARENA] Start scheduled: worldId=%u participants=%u waitAllArrive=%ums"), worldId, (unsigned)m_pendingStartParticipants.size(), (unsigned)m_waitAllArriveMs);
				ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA] Start scheduled: worldId=%u total=%u", worldId, (unsigned)m_pendingStartParticipants.size());
				// If some participants are already in the arena world, mark them ready immediately
				for (auto cid : m_pendingStartParticipants)
				{
					CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid);
					if (p && p->IsInitialized() && (unsigned int)p->GetWorldID() == worldId)
					{
						// Honor no-delay setting; otherwise mark ready immediately to avoid stall
						if (m_cfg.enterReadyDelayMs == 0)
							m_readyParticipants.insert(cid);
						else
							m_readyParticipants.insert(cid);
					}
				}
				// If there are no online participants anymore, cancel start
				if (CountOnlineParticipants() == 0)
				{
					m_pendingStartWorldId = 0;
					m_waitAllArriveMs = 0;
					m_state = State::ENROLLMENT;
					SendNotice(L"Arena canceled: no participants online.", SERVER_TEXT_SYSNOTICE);
					NTL_PRINT(PRINT_APP, _T("[ARENA] Start canceled: no participants online"));
					// Clear participant list so automation can proceed on next cycle
					m_participants.clear();
					return;
				}
			}
		}
	}

	// Fallback: if we're in PRE_ROUND and pending world id isn't set yet, try to obtain it
	if (m_state == State::PRE_ROUND && m_pendingStartMs == 0 && m_pendingStartWorldId == 0)
	{
		unsigned int worldId = EnsureCurrentWorldId();
		if (worldId)
		{
			m_pendingStartWorldId = worldId;
			NTL_PRINT(PRINT_APP, _T("[ARENA] Late world bind acquired: worldId=%u"), worldId);
			ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA] Late world bind acquired: worldId=%u", worldId);
		}
	}

	// After delay elapsed, if gating is active, check arrivals (only during PRE_ROUND)
	if (m_state == State::PRE_ROUND && m_pendingStartMs == 0 && m_pendingStartWorldId != 0 && m_waitAllArriveMs > 0)
	{
		unsigned int worldId = m_pendingStartWorldId;
		// Only require readiness from participants who actually accepted and are present in the arena world
		unsigned present = CountParticipantsInWorld(worldId);
		unsigned ready = 0;
		for (auto cid : m_pendingStartParticipants)
		{
			if (m_readyParticipants.find(cid) != m_readyParticipants.end())
			{
				CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid);
				if (p && p->IsInitialized() && (unsigned int)p->GetWorldID() == worldId)
					++ready;
			}
		}

		// Debug logging every 2 seconds during wait
		static unsigned int lastDebugMs = 0;
		if (m_waitAllArriveMs % 2000 < dwTickDiff || lastDebugMs == 0)
		{
			NTL_PRINT(PRINT_APP, _T("[ARENA] PRE_ROUND waiting: present=%u ready=%u total=%u remaining=%ums"),
				present, ready, (unsigned)m_pendingStartParticipants.size(), (unsigned)m_waitAllArriveMs);
			lastDebugMs = m_waitAllArriveMs;
		}

		if (present > 0 && ready >= present)
		{
			// Everyone present and ready within grace period — proceed immediately
			m_waitAllArriveMs = 0;
			m_pendingStartWorldId = 0;
			m_state = State::MATCH_READY;
			m_directionTimeMs = 3000; // tighter intro
			m_currentWorldId = worldId;
			// Assign teams only in explicit team modes (Party or Guild)
			if (m_mode == Mode::PARTY_VS_PARTY || m_mode == Mode::GUILD_VS_GUILD)
			{
				std::vector<CPlayer*> players;
				players.reserve(m_participants.size());
				for (auto cidAll : m_participants)
				{
					CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cidAll);
					if (p && p->IsInitialized() && (unsigned int)p->GetWorldID() == worldId)
						players.push_back(p);
				}
				auto getTeamKey = [&](CPlayer* p)->unsigned int {
					if (m_mode == Mode::PARTY_VS_PARTY)
					{
						PARTYID pid = p->GetPartyID();
						if (pid != INVALID_PARTYID)
							return (unsigned int)pid;
						// Assign a unique pseudo-party id for solo players so they form distinct teams
						return 0x80000000u | (unsigned int)p->GetCharID();
					}
					if (m_mode == Mode::GUILD_VS_GUILD) return (unsigned int)p->GetGuildID();
					return 0;
					};
				unsigned int ownerKey = 0, challengerKey = 0;
				{
					for (CPlayer* p : players)
					{
						unsigned int key = getTeamKey(p);
						if (key == 0) continue;
						if (ownerKey == 0) ownerKey = key; else if (key != ownerKey) { challengerKey = key; break; }
					}
				}
				for (size_t i = 0; i < players.size(); ++i)
				{
					CPlayer* p = players[i];
					BYTE team = RANKBATTLE_TEAM_OWNER;
					unsigned int key = getTeamKey(p);
					team = (key != 0 && key != ownerKey) ? RANKBATTLE_TEAM_CHALLENGER : RANKBATTLE_TEAM_OWNER;
					sRANK_BATTLE_DATA* rd = p->GetRankBattleData();
					if (rd) rd->eTeamType = (eRANKBATTLE_TEAM_TYPE)team; else ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA][WARN] Null RankBattleData when setting team type char=%u", (unsigned)p->GetCharID());
				}
			}
			// Inform clients they joined a RankBattle context before showing WAIT, so HUD initializes correctly
			if (m_cfg.rankUiEnabled) BroadcastRankJoinToWorld(worldId);
			if (m_mode == Mode::PARTY_VS_PARTY || m_mode == Mode::GUILD_VS_GUILD)
				BroadcastRankTeamInfoToWorld(worldId);
			// Compute WAIT and DIRECTION durations similar to RankBattle table (fallback to defaults)
			DWORD waitMs = 2000;
			if (CRankBattleTable* pRankBattleTable = g_pTableContainer->GetRankBattleTable())
			{
				CTable* pBase = reinterpret_cast<CTable*>(pRankBattleTable);
				for (CTable::TABLEIT it = pBase->Begin(); it != pBase->End(); ++it)
				{
					sRANKBATTLE_TBLDAT* rb = (sRANKBATTLE_TBLDAT*)it->second;
					if (rb && rb->worldTblidx == (TBLIDX)m_currentWorldTblidx)
					{
						if (rb->dwMatchReadyTime > 0)
							waitMs = rb->dwMatchReadyTime * 500; // half for visible WAIT, like invite path
						break;
					}
				}
			}
			// Start with a proper WAIT state (with timer); the state machine will advance to DIRECTION after expiry
			UpdateRankBattleState(RANKBATTLE_BATTLESTATE_WAIT, m_rankBattleStage, waitMs);
			ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA] PRE_ROUND->MATCH_READY: scheduled WAIT waitMs=%u worldId=%u", (unsigned)waitMs, worldId);
			ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA] PRE_ROUND->MATCH_READY immediate worldId=%u present=%u ready=%u stage=%u", worldId, present, ready, (unsigned)m_rankBattleStage);
			SendNotice(L"Arena match starting...", SERVER_TEXT_SYSNOTICE);
			NTL_PRINT(PRINT_APP, _T("[ARENA] PRE_ROUND -> MATCH_READY (immediate): present=%u ready=%u worldId=%u"), present, ready, worldId);
		}
		else if (m_waitAllArriveMs <= dwTickDiff)
		{
			// Grace expired: cancel if nobody present, otherwise proceed
			if (present == 0)
			{
				m_waitAllArriveMs = 0;
				m_pendingStartWorldId = 0;
				m_state = State::ENROLLMENT;
				SendNotice(L"Arena canceled: nobody arrived to the arena.", SERVER_TEXT_SYSNOTICE);
				NTL_PRINT(PRINT_APP, _T("[ARENA] Start canceled: present=0 at timeout"));
				// Avoid automation stalls: clear participants when nobody arrived
				m_participants.clear();
			}
			else
			{
				// Proceed even if not all ready
				m_waitAllArriveMs = 0;
				m_pendingStartWorldId = 0;
				m_state = State::MATCH_READY;
				m_directionTimeMs = 5000; // direction/intro
				// Bind current world id just in case it wasn't set yet
				m_currentWorldId = worldId;
				// Assign teams only for team modes; skip in FFA
				if (m_mode == Mode::PARTY_VS_PARTY || m_mode == Mode::GUILD_VS_GUILD)
				{
					std::vector<CPlayer*> players;
					players.reserve(m_participants.size());
					for (auto cidAll : m_participants)
					{
						CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cidAll);
						if (p && p->IsInitialized() && (unsigned int)p->GetWorldID() == worldId)
							players.push_back(p);
					}
					auto getTeamKey = [&](CPlayer* p)->unsigned int {
						if (m_mode == Mode::PARTY_VS_PARTY)
						{
							PARTYID pid = p->GetPartyID();
							if (pid != INVALID_PARTYID)
								return (unsigned int)pid;
							return 0x80000000u | (unsigned int)p->GetCharID();
						}
						if (m_mode == Mode::GUILD_VS_GUILD) return (unsigned int)p->GetGuildID();
						return 0;
						};
					unsigned int ownerKey = 0, challengerKey = 0;
					{
						for (CPlayer* p : players)
						{
							unsigned int key = getTeamKey(p);
							if (key == 0) continue;
							if (ownerKey == 0) ownerKey = key; else if (key != ownerKey) { challengerKey = key; break; }
						}
					}
					for (size_t i = 0; i < players.size(); ++i)
					{
						CPlayer* p = players[i];
						BYTE team = RANKBATTLE_TEAM_OWNER;
						unsigned int key = getTeamKey(p);
						team = (key != 0 && key != ownerKey) ? RANKBATTLE_TEAM_CHALLENGER : RANKBATTLE_TEAM_OWNER;
						sRANK_BATTLE_DATA* rd = p->GetRankBattleData();
						if (rd) rd->eTeamType = (eRANKBATTLE_TEAM_TYPE)team; else ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA][WARN] Null RankBattleData when setting team type char=%u", (unsigned)p->GetCharID());
					}
				}
				// RankBattle opening: JOIN -> WAIT (TeamInfo only for team modes)
				if (m_cfg.rankUiEnabled) BroadcastRankJoinToWorld(worldId);
				if (m_mode == Mode::PARTY_VS_PARTY || m_mode == Mode::GUILD_VS_GUILD)
					BroadcastRankTeamInfoToWorld(worldId);
				// Compute WAIT based on RankBattle table; DIRECTION will be scheduled by the state machine on WAIT expiry
				DWORD waitMs = 2000; // default visible WAIT time
				if (CRankBattleTable* pRankBattleTable = g_pTableContainer->GetRankBattleTable())
				{
					CTable* pBase = reinterpret_cast<CTable*>(pRankBattleTable);
					for (CTable::TABLEIT it = pBase->Begin(); it != pBase->End(); ++it)
					{
						sRANKBATTLE_TBLDAT* rb = (sRANKBATTLE_TBLDAT*)it->second;
						if (rb && rb->worldTblidx == (TBLIDX)m_currentWorldTblidx)
						{
							if (rb->dwMatchReadyTime > 0)
								waitMs = rb->dwMatchReadyTime * 500; // half of match ready for WAIT visibility
							break;
						}
					}
				}
				UpdateRankBattleState(RANKBATTLE_BATTLESTATE_WAIT, m_rankBattleStage, waitMs);
				ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA] PRE_ROUND->MATCH_READY (grace end): scheduled WAIT waitMs=%u worldId=%u", (unsigned)waitMs, worldId);
				ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA] PRE_ROUND->MATCH_READY (grace end) worldId=%u present=%u ready=%u stage=%u", worldId, present, ready, (unsigned)m_rankBattleStage);
				// Do NOT send MATCH_START here; respect RankBattle flow: WAIT->TeamInfo->DIRECTION->STAGE_PREPARE->STAGE_READY->MATCH_START->RUN
				SendNotice(L"Arena match starting...", SERVER_TEXT_SYSNOTICE);
				NTL_PRINT(PRINT_APP, _T("[ARENA] PRE_ROUND -> MATCH_READY: present=%u ready=%u worldId=%u (grace expired)"), present, ready, worldId);
			}
		}
		else
		{
			m_waitAllArriveMs -= dwTickDiff;
		}
	}
	// Announce rotation countdown at 60s, 30s, 10s, and last 5..1s
	if (m_cfg.rotationSeconds > 0 && m_cfg.worldTblidxList.size() > 1 && m_rotationRemainMs > 0)
	{
		unsigned int sec = (unsigned int)(m_rotationRemainMs / 1000);
		if (sec != m_nextRotationAnnounceSec)
		{
			if (sec == 60 || sec == 30 || sec == 10 || (sec <= 5 && sec >= 1))
			{
				AnnounceRotationTimeRemaining(sec);
			}
			m_nextRotationAnnounceSec = sec;
		}
	}

	// Handle invite countdown if active
	if (m_inviting)
	{
		ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA] Invite ticking remainMs=%u worldId=%u", (unsigned)m_inviteRemainMs, (unsigned)(m_currentWorldId ? m_currentWorldId : 0));
		// Early start if everyone already accepted
		unsigned int worldIdNow = m_currentWorldId ? m_currentWorldId : EnsureCurrentWorldId();
		unsigned acceptedNow = 0;
		if (worldIdNow)
			acceptedNow = CountParticipantsInWorld(worldIdNow);
		else if (m_currentWorldTblidx != 0)
			acceptedNow = CountParticipantsInWorldTblidx(m_currentWorldTblidx);
		if (acceptedNow == m_participants.size() && m_participants.size() > 0)
		{
			m_inviteRemainMs = 0;
		}

		if (m_inviteRemainMs > dwTickDiff)
		{
			m_inviteRemainMs -= dwTickDiff;
		}
		else
		{
			m_inviting = false;
			// After invite window, for any participant who accepted (i.e., is already in target world), proceed
			unsigned int worldId = worldIdNow;
			unsigned accepted = 0;
			if (worldId)
				accepted = CountParticipantsInWorld(worldId);
			else
				accepted = CountParticipantsInWorldTblidx(m_currentWorldTblidx);

			NTL_PRINT(PRINT_APP, _T("[ARENA] Invite timeout: worldId=%u, accepted=%u, total participants=%u"),
				worldId, accepted, (unsigned)m_participants.size());

			if (accepted == 0)
			{
				SendNotice(L"Arena invite timed out. No participants accepted.", SERVER_TEXT_SYSNOTICE);
				m_state = State::ENROLLMENT;
				// Do not cancel external proposals; Arena uses direct teleports now
				NTL_PRINT(PRINT_APP, _T("[ARENA] Invite timeout: no acceptors. Reset to ENROLLMENT"));
				// Clear participant list so AutoArena can open next cycle without @arena stop
				m_participants.clear();
			}
			else
			{
				// Require at least 2 participants to start a rank-like match
				if (accepted < 2)
				{
					SendNotice(L"Arena invite concluded: not enough participants accepted.", SERVER_TEXT_SYSNOTICE);
					m_state = State::ENROLLMENT;
					// Do not cancel external proposals; Arena uses direct teleports now
					NTL_PRINT(PRINT_APP, _T("[ARENA] Invite end: accepted=%u < 2. Reset to ENROLLMENT"), accepted);
					// Clear participant list to avoid automation stall
					m_participants.clear();
					return;
				}
				// If we don't have a shared worldId yet (per-player tblidx teleports), pick any participant's worldId for this tblidx
				if (!worldId)
					worldId = GetFirstParticipantWorldIdForTblidx(m_currentWorldTblidx);

				// Remove participants who didn't accept from the arena participant list
				std::unordered_set<unsigned int> acceptedParticipants;
				for (auto cid : m_participants)
				{
					CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid);
					if (p && p->IsInitialized() && (
						(worldId && (unsigned int)p->GetWorldID() == worldId) ||
						(!worldId && (unsigned int)p->GetWorldTblidx() == m_currentWorldTblidx)
						))
					{
						acceptedParticipants.insert(cid);
					}
				}

				// Keep a copy of all invited participants before narrowing
				auto invitedBefore = m_participants;
				// Update participant list to only include those who accepted
				m_participants = acceptedParticipants;

				// Begin RankBattle-like opening: WAIT(0) -> TeamInfo -> DIRECTION(0)
				m_state = State::MATCH_READY;
				m_currentWorldId = worldId;

				// Inform clients of RankBattle room context before WAIT to satisfy HUD expectations
				if (m_cfg.rankUiEnabled) BroadcastRankJoinToWorld(worldId);
				// Ensure HUD is initialized just before start
				BroadcastRankStateToWorld(worldId, RANKBATTLE_BATTLESTATE_WAIT, 0);
				// Assign teams only for team modes; in FFA do not assign teams
				if (m_mode == Mode::PARTY_VS_PARTY || m_mode == Mode::GUILD_VS_GUILD)
				{
					std::vector<CPlayer*> players;
					players.reserve(m_participants.size());
					for (auto cidAll : m_participants)
					{
						CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cidAll);
						if (p && p->IsInitialized() && (unsigned int)p->GetWorldID() == worldId)
							players.push_back(p);
					}
					auto getTeamKey = [&](CPlayer* p)->unsigned int {
						if (m_mode == Mode::PARTY_VS_PARTY)
						{
							PARTYID pid = p->GetPartyID();
							if (pid != INVALID_PARTYID)
								return (unsigned int)pid;
							return 0x80000000u | (unsigned int)p->GetCharID();
						}
						if (m_mode == Mode::GUILD_VS_GUILD) return (unsigned int)p->GetGuildID();
						return 0;
						};
					unsigned int ownerKey = 0;
					for (CPlayer* p : players)
					{
						unsigned int key = getTeamKey(p);
						if (key == 0) continue;
						if (ownerKey == 0) { ownerKey = key; break; }
					}
					for (size_t i = 0; i < players.size(); ++i)
					{
						CPlayer* p = players[i];
						BYTE team = RANKBATTLE_TEAM_OWNER;
						unsigned int key = getTeamKey(p);
						team = (key != 0 && key != ownerKey) ? RANKBATTLE_TEAM_CHALLENGER : RANKBATTLE_TEAM_OWNER;
						sRANK_BATTLE_DATA* rd = p->GetRankBattleData();
						if (rd) rd->eTeamType = (eRANKBATTLE_TEAM_TYPE)team; else ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA][WARN] Null RankBattleData when setting team type char=%u", (unsigned)p->GetCharID());
					}
				}
				// Send team composition only in team modes
				if (m_mode == Mode::PARTY_VS_PARTY || m_mode == Mode::GUILD_VS_GUILD)
					BroadcastRankTeamInfoToWorld(worldId);
				// Start with WAIT state for a visible duration, then transition to DIRECTION
				// Pull timings from RankBattle table if available
				DWORD waitMs = 2000; // default visible WAIT time
				DWORD dirMs = 3000; // default DIRECTION time
				if (CRankBattleTable* pRankBattleTable = g_pTableContainer->GetRankBattleTable())
				{
					CTable* pBase = reinterpret_cast<CTable*>(pRankBattleTable);
					for (CTable::TABLEIT it = pBase->Begin(); it != pBase->End(); ++it)
					{
						sRANKBATTLE_TBLDAT* rb = (sRANKBATTLE_TBLDAT*)it->second;
						if (rb && rb->worldTblidx == (TBLIDX)m_currentWorldTblidx)
						{
							dirMs = rb->dwDirectionTime * 1000;
							// Use RankBattle's match ready time as WAIT duration if available
							if (rb->dwMatchReadyTime > 0)
								waitMs = rb->dwMatchReadyTime * 500; // half of match ready time for WAIT visibility
							break;
						}
					}
				}

				// Start WAIT state with proper timer before DIRECTION; preserve current stage index
				UpdateRankBattleState(RANKBATTLE_BATTLESTATE_WAIT, m_rankBattleStage, waitMs);
				ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA] MATCH_READY: sent TeamInfo+WAIT waitMs=%u worldId=%u", (unsigned)waitMs, worldId);

				wchar_t msg[128];
				swprintf_s(msg, _countof(msg), L"Arena starting with %u participants...", accepted);
				SendNotice(msg, SERVER_TEXT_SYSNOTICE);

				NTL_PRINT(PRINT_APP, _T("[ARENA] MATCH_READY: TeamInfo sent; WAIT for %ums"), (unsigned)waitMs);

				// Cancel any remaining pending proposals for players who did not accept
				for (auto cid : invitedBefore)
				{
					if (acceptedParticipants.find(cid) != acceptedParticipants.end())
						continue; // accepted; already moved
					if (CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid))
					{
						// Only cancel Arena/Rank-related proposals; do not interfere with Dojo/Budokai
						p->CancelTeleportProposal(TELEPORT_TYPE_RANKBATTLE);
					}
				}
			}
		}
	}

	// Handle battle state transitions in CC mode via RankBattle-like machine
	if (m_cfg.ccBattleMode && m_rankStateTimeMs > 0)
	{
		if (m_rankStateTimeMs > dwTickDiff)
		{
			m_rankStateTimeMs -= dwTickDiff;
		}
		else
		{
			ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA][RB] state=%d stage=%u timerExpired, computing next", (int)m_rankBattleState, (unsigned)m_rankBattleStage);
			// Pull timings from RankBattle table
			DWORD prepMs = 3000, matchReadyMs = 3000, runMs = 60000, stageFinishMs = 3000, matchFinishMs = 4000;
			BYTE battleCount = 1;
			if (CRankBattleTable* pRankBattleTable = g_pTableContainer->GetRankBattleTable())
			{
				CTable* pBase = reinterpret_cast<CTable*>(pRankBattleTable);
				for (CTable::TABLEIT it = pBase->Begin(); it != pBase->End(); ++it)
				{
					sRANKBATTLE_TBLDAT* rb = (sRANKBATTLE_TBLDAT*)it->second;
					if (rb && rb->worldTblidx == (TBLIDX)m_currentWorldTblidx)
					{
						prepMs = rb->dwStageReadyTime * 1000;
						matchReadyMs = rb->dwMatchReadyTime * 1000;
						runMs = rb->dwStageRunTime * 1000;
						stageFinishMs = rb->dwStageFinishTime * 1000;
						matchFinishMs = rb->dwMatchFinishTime * 1000;
						battleCount = rb->byBattleCount;
						break;
					}
				}
			}
			// Apply Arena overrides if configured
			if (m_cfg.roundTimerSeconds > 0) runMs = ToMs(m_cfg.roundTimerSeconds);
			if (m_cfg.roundsCount > 0) battleCount = (BYTE)m_cfg.roundsCount;

			switch (m_rankBattleState)
			{
			case RANKBATTLE_BATTLESTATE_WAIT:
			{
				// WAIT timer expired, transition to DIRECTION
				DWORD dirMs = 3000; // default DIRECTION time
				if (CRankBattleTable* pRankBattleTable = g_pTableContainer->GetRankBattleTable())
				{
					CTable* pBase = reinterpret_cast<CTable*>(pRankBattleTable);
					for (CTable::TABLEIT it = pBase->Begin(); it != pBase->End(); ++it)
					{
						sRANKBATTLE_TBLDAT* rb = (sRANKBATTLE_TBLDAT*)it->second;
						if (rb && rb->worldTblidx == (TBLIDX)m_currentWorldTblidx)
						{
							dirMs = rb->dwDirectionTime * 1000;
							break;
						}
					}
				}
				UpdateRankBattleState(RANKBATTLE_BATTLESTATE_DIRECTION, m_rankBattleStage, dirMs);
				ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA][RB] WAIT->DIRECTION stage=%u dirMs=%u", (unsigned)m_rankBattleStage, (unsigned)dirMs);
				break;
			}
			case RANKBATTLE_BATTLESTATE_DIRECTION:
				UpdateRankBattleState(RANKBATTLE_BATTLESTATE_STAGE_PREPARE, m_rankBattleStage, prepMs);
				break;
			case RANKBATTLE_BATTLESTATE_STAGE_PREPARE:
				UpdateRankBattleState(RANKBATTLE_BATTLESTATE_STAGE_READY, m_rankBattleStage, matchReadyMs);
				break;
			case RANKBATTLE_BATTLESTATE_STAGE_READY:
			{
				unsigned int worldId = m_currentWorldId ? m_currentWorldId : EnsureCurrentWorldId();
				if (worldId)
				{
					// Make sure any leftover UI from prior round is cleared before we start a new one
					StopRoundTimerUI();
					// For rounds after the first, first revive any fainted players, then reset states like RankBattle
					if (m_rankBattleStage > 0)
					{
						ReviveParticipantsForNextRound();
						ResetParticipantsBetweenRounds();
					}
					EnsureParticipantsStanding(worldId);
					CGameServer* appWorldCtx = (CGameServer*)g_pApp;
					CWorld* pWorldCtx = appWorldCtx->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)worldId);
					if (pWorldCtx)
					{
						if (!pWorldCtx->GetTbldat())
						{
							ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA][WARN] READY: world tbldat missing worldId=%u", (unsigned)pWorldCtx->GetID());
						}
						BYTE rule = pWorldCtx->GetTbldat() ? pWorldCtx->GetTbldat()->byWorldRuleType : GAMERULE_NORMAL;
						bool isBudokaiRule = (rule == GAMERULE_MINORMATCH || rule == GAMERULE_MAJORMATCH || rule == GAMERULE_FINALMATCH);
						if (isBudokaiRule)
						{
							// Budokai worlds: use Budokai player state NORMAL and do NOT send rank packets
							ArenaBroadcastBudokaiPlayerStateToWorld(pWorldCtx, m_participants, MATCH_MEMBER_STATE_NORMAL);
						}
						else
						{
							// Non-Budokai: RankBattle NORMAL -> MATCH_START -> ATTACKABLE
							for (auto cid : m_participants)
							{
								CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid);
								if (p && p->IsInitialized() && (unsigned int)p->GetWorldID() == worldId)
								{
									sRANK_BATTLE_DATA* rdN = p->GetRankBattleData();
									if (rdN) rdN->eState = RANKBATTLE_MEMBER_STATE_NORMAL; else ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA][WARN] Null RankBattleData when setting NORMAL char=%u", (unsigned)p->GetCharID());
									// Send NORMAL state packet
									CNtlPacket pkt(sizeof(sGU_RANKBATTLE_BATTLE_PLAYER_STATE_NFY));
									sGU_RANKBATTLE_BATTLE_PLAYER_STATE_NFY* res = (sGU_RANKBATTLE_BATTLE_PLAYER_STATE_NFY*)pkt.GetPacketData();
									res->wOpCode = GU_RANKBATTLE_BATTLE_PLAYER_STATE_NFY;
									res->hPc = p->GetID();
									res->byPCState = RANKBATTLE_MEMBER_STATE_NORMAL;
									pkt.SetPacketLen(sizeof(sGU_RANKBATTLE_BATTLE_PLAYER_STATE_NFY));
									p->SendPacket(&pkt);
								}
							}
							// Send MATCH_START notify to fully unlock camera/input every round
							BroadcastRankMatchStartToWorld(worldId);
							// Do not send ATTACKABLE in READY; defer until after RUN begins
						}

					}
				}
				// Enter RUN for the current stage; preserve stage index for clients
				UpdateRankBattleState(RANKBATTLE_BATTLESTATE_STAGE_RUN, m_rankBattleStage, runMs);
				ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA][RB] READY->RUN stage=%u runMs=%u worldId=%u", (unsigned)m_rankBattleStage, (unsigned)runMs, worldId);
				// Allow a short settle period before evaluating alive-count logic and schedule an unlock pulse
				m_runSettleMs = 1500;
				m_runUnlockPulseMs = 700; // send a second ATTACKABLE ~0.7s after RUN
				// Initialize random mob wave timer at the start of RUN
				if (m_cfg.mobsAllowed && m_cfg.randomMobsSpawn && m_cfg.randomMobsWaveSeconds > 0)
					m_randomWaveRemainMs = ToMs(m_cfg.randomMobsWaveSeconds);
				// Enable combat permissions (PvP/FreeBattle) for participants while in RUN on CC maps
				SetCombatPermittedForParticipants(true);
				ClearCombatRestrictionsForParticipants();
				// Optionally mark the entire world as PvP for all players present during RUN
				if (m_cfg.worldWidePvpDuringRun)
				{
					ApplyWorldWidePvp(worldId);
				}
				m_state = State::IN_ROUND;
				// Final unlock signal: on Budokai-rule worlds, ensure Budokai NORMAL is visible; otherwise send ATTACKABLE pulse
				{
					CGameServer* app3 = (CGameServer*)g_pApp;
					if (CWorld* pWorld3 = app3->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)worldId))
					{
						if (!pWorld3->GetTbldat()) { ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA][WARN] RUN: world tbldat missing worldId=%u", (unsigned)pWorld3->GetID()); }
						BYTE rule3 = pWorld3->GetTbldat() ? pWorld3->GetTbldat()->byWorldRuleType : GAMERULE_NORMAL;
						bool isBudokaiRule3 = (rule3 == GAMERULE_MINORMATCH || rule3 == GAMERULE_MAJORMATCH || rule3 == GAMERULE_FINALMATCH);
						if (isBudokaiRule3)
							ArenaBroadcastBudokaiPlayerStateToWorld(pWorld3, m_participants, MATCH_MEMBER_STATE_NORMAL);
						else
							ArenaBroadcastRankPlayerAttackableToWorld(pWorld3, m_participants);
					}
				}
				// Announce dungeon stage after RUN so the HUD reflects the active round (stage = round number)
				BroadcastDungeonStateToWorld(worldId, m_rankBattleStage + 1);
				// Extra safety: re-broadcast unlock once more depending on world rule
				{
					CGameServer* app2 = (CGameServer*)g_pApp;
					if (CWorld* pWorld2 = app2->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)worldId))
					{
						if (!pWorld2->GetTbldat()) { ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA][WARN] Post-Run unlock: world tbldat missing worldId=%u", (unsigned)pWorld2->GetID()); }
						BYTE rule2 = pWorld2->GetTbldat() ? pWorld2->GetTbldat()->byWorldRuleType : GAMERULE_NORMAL;
						bool isBudokaiRule2 = (rule2 == GAMERULE_MINORMATCH || rule2 == GAMERULE_MAJORMATCH || rule2 == GAMERULE_FINALMATCH);
						if (isBudokaiRule2)
							ArenaBroadcastBudokaiPlayerStateToWorld(pWorld2, m_participants, MATCH_MEMBER_STATE_NORMAL);
						else
							ArenaBroadcastRankPlayerAttackableToWorld(pWorld2, m_participants);
					}
				}
				if (m_cfg.roundTimerSeconds > 0)
				{
					StartRoundTimerUI(m_cfg.roundTimerSeconds);
					NTL_PRINT(PRINT_APP, _T("[ARENA] Round timer started for stage=%u, seconds=%u, worldId=%u"), (unsigned)m_rankBattleStage, (unsigned)m_cfg.roundTimerSeconds, worldId);
				}
				// Start fallback countdown only if rank UI packets are disabled
				if (!(m_cfg.rankUiEnabled && m_cfg.rankPacketsEnabled))
				{
					BroadcastCountdownToWorld(worldId, true);
				}
				// Ensure everyone is explicitly set to ATTACKABLE one more time
				MakeParticipantsAttackable(worldId);
				if (m_cfg.mobsAllowed) SpawnArenaMobs();
				if (m_cfg.telecastEnabled) BroadcastTelecastToWorld(worldId);
				NTL_PRINT(PRINT_APP, _T("[ARENA] State: STAGE_READY -> STAGE_RUN, battle started (stage=%u, runMs=%u)"), (unsigned)m_rankBattleStage, (unsigned)runMs);
				break;
			}
			case RANKBATTLE_BATTLESTATE_STAGE_RUN:
			{
				// RUN state timer expired (or forced). End round and either advance or finish match
				bool hasNextRound = (m_rankBattleStage + 1) < battleCount;
				if (hasNextRound)
				{
					// Stop current round timer, tell clients stage finished
					StopRoundTimerUI();
					if (m_currentWorldId)
						BroadcastRankStageFinishToWorld(m_currentWorldId);
					UpdateRankBattleState(RANKBATTLE_BATTLESTATE_STAGE_FINISH, m_rankBattleStage, stageFinishMs);
					ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA][RB] RUN->STAGE_FINISH stage=%u", (unsigned)m_rankBattleStage);
				}
				else
				{
					// Final round on arena world: only stop timer and finish immediately with rewards/teleport.
					// Skip any Rank/Budokai state broadcasts to keep UI intact until teleport.
					StopRoundTimerUI();
					if (IsArenaWorldTblidx(m_currentWorldTblidx))
					{
						FinishMatch(false);
						break;
					}
					// Non-arena worlds: mirror RankBattle finish flow
					if (m_currentWorldId && !m_cfg.suppressRankFinishUi)
						BroadcastRankMatchFinishToWorld(m_currentWorldId);
					UpdateRankBattleState(RANKBATTLE_BATTLESTATE_MATCH_FINISH, m_rankBattleStage, matchFinishMs);
					// Arm watchdog slightly beyond matchFinishMs to guarantee completion
					m_matchFinishWatchdogMs = matchFinishMs + 2000; // +2s buffer
					ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA][RB] RUN->MATCH_FINISH finalStage=%u", (unsigned)m_rankBattleStage);
				}
				break;
			}
			case RANKBATTLE_BATTLESTATE_STAGE_FINISH:
			{
				// Ensure the previous round UI is fully cleared before advancing/rotating
				StopRoundTimerUI();
				if (m_currentWorldId)
					BroadcastCountdownToWorld(m_currentWorldId, false);
				ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA][RB] STAGE_FINISH: advancing to next stage (cur=%u)", (unsigned)m_rankBattleStage);
				// Advance to next round
				m_rankBattleStage = (BYTE)(m_rankBattleStage + 1);
				// Immediately broadcast the updated stage so clients show the correct Round number
				if (m_currentWorldId)
					BroadcastDungeonStateToWorld(m_currentWorldId, m_rankBattleStage + 1);
				// If configured, rotate map between rounds (skip for ARENAPODER)
				if (m_cfg.mapPerRound && !m_cfg.worldTblidxList.empty() && ShouldRotatePerRoundForWorld(m_currentWorldTblidx))
				{
					// Choose next map in list
					m_worldIndex = (m_worldIndex + 1) % m_cfg.worldTblidxList.size();
					m_currentWorldTblidx = m_cfg.worldTblidxList[m_worldIndex];
					m_currentWorldId = 0; // force new instance
					unsigned int newWorldId = EnsureCurrentWorldId();
					if (newWorldId)
					{
						// Teleport participants to new map starts, preserving rough team split
						sWORLD_TBLDAT* pWorldTbldat = (sWORLD_TBLDAT*)g_pTableContainer->GetWorldTable()->FindData((TBLIDX)m_currentWorldTblidx);
						if (pWorldTbldat)
						{
							// Build stable ordering of currently active participants
							std::vector<unsigned int> ids; ids.reserve(m_participants.size());
							for (auto cid : m_participants) ids.push_back(cid);
							std::sort(ids.begin(), ids.end());
							size_t half = (ids.size() + 1) / 2;
							for (size_t i = 0; i < ids.size(); ++i)
							{
								CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)ids[i]);
								if (!p || !p->IsInitialized()) continue;
								CNtlVector loc = (i < half) ? pWorldTbldat->vStart1Loc : pWorldTbldat->vStart2Loc;
								CNtlVector dir = (i < half) ? pWorldTbldat->vStart1Dir : pWorldTbldat->vStart2Dir;
								loc.x += RandomRangeF(0, 3.0f);
								loc.z += RandomRangeF(0, 3.0f);
								TeleportOneToWorldTblidx(p, m_currentWorldTblidx, loc.x, loc.y, loc.z);
								// Set team type only in team modes
								if (m_mode == Mode::PARTY_VS_PARTY || m_mode == Mode::GUILD_VS_GUILD)
								{
									sRANK_BATTLE_DATA* rd = p->GetRankBattleData();
									if (rd) rd->eTeamType = (i < half) ? RANKBATTLE_TEAM_OWNER : RANKBATTLE_TEAM_CHALLENGER; else ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA][WARN] Null RankBattleData when rotating map team assign char=%u", (unsigned)p->GetCharID());
								}
							}
						}
						// Bind to new world id for subsequent RB packets
						m_currentWorldId = newWorldId;
						// Reinitialize HUD/order: JOIN -> (TeamInfo for team modes) -> WAIT(stage)
						if (m_cfg.rankUiEnabled) BroadcastRankJoinToWorld(newWorldId);
						if (m_mode == Mode::PARTY_VS_PARTY || m_mode == Mode::GUILD_VS_GUILD)
							BroadcastRankTeamInfoToWorld(newWorldId);
						BroadcastRankStateToWorld(newWorldId, RANKBATTLE_BATTLESTATE_WAIT, m_rankBattleStage);
						// Ensure everyone exits any residual lock/cinematic state after TP
						EnsureParticipantsStanding(newWorldId);

						// Schedule PRE_ROUND gating for the next round to ensure packets are sent when players are present
						m_state = State::PRE_ROUND;
						m_pendingStartWorldId = newWorldId;
						m_pendingStartMs = ToMs(m_cfg.startDelaySeconds);
						m_waitAllArriveMs = ToMs(m_cfg.maxWaitAllArriveSeconds);
						m_pendingStartParticipants = m_participants;
						m_readyParticipants.clear();
						m_readyDelayMs.clear();
						// Reset rank battle state to WAIT for this stage to avoid re-entering STAGE_FINISH loop
						UpdateRankBattleState(RANKBATTLE_BATTLESTATE_WAIT, m_rankBattleStage, 0);
						ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA][RB] Map rotated to tblidx=%u worldId=%u; gating PRE_ROUND", (unsigned)m_currentWorldTblidx, (unsigned)newWorldId);
						// Do not fall-through to READY; wait for arrivals
						break;
					}
				}
				else
				{
					// Same-map next round: refresh HUD and team info and then gate start until players are present
					unsigned int worldId = m_currentWorldId ? m_currentWorldId : EnsureCurrentWorldId();
					if (worldId)
					{
						// Refresh HUD/order for next round on same map: JOIN -> (TeamInfo for team modes) -> WAIT
						if (m_cfg.rankUiEnabled) BroadcastRankJoinToWorld(worldId);
						if (m_mode == Mode::PARTY_VS_PARTY || m_mode == Mode::GUILD_VS_GUILD)
							BroadcastRankTeamInfoToWorld(worldId);
						BroadcastRankStateToWorld(worldId, RANKBATTLE_BATTLESTATE_WAIT, m_rankBattleStage);
						EnsureParticipantsStanding(worldId);
						m_state = State::PRE_ROUND;
						m_pendingStartWorldId = worldId;
						m_pendingStartMs = ToMs(m_cfg.startDelaySeconds);
						m_waitAllArriveMs = ToMs(m_cfg.maxWaitAllArriveSeconds);
						m_pendingStartParticipants = m_participants;
						m_readyParticipants.clear();
						m_readyDelayMs.clear();
						// Reset rank battle state to WAIT for this stage to avoid re-entering STAGE_FINISH loop
						UpdateRankBattleState(RANKBATTLE_BATTLESTATE_WAIT, m_rankBattleStage, 0);
						ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA][RB] Same map next round; gating PRE_ROUND worldId=%u", worldId);
						break;
					}
				}
				// No need for additional ReviveParticipantsForNextRound() here - already called above
				// Fall back to PRE_ROUND gating path (handled above) which will drive the intro for the new stage
				break;
			}
			case RANKBATTLE_BATTLESTATE_MATCH_FINISH:
			{
				// Cleanup like RankBattle end; always finalize when MATCH_FINISH elapses
				// Clear any timer/countdown HUD first
				ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA][RB] MATCH_FINISH elapsed, calling FinishMatch");
				Stop(false);
				break;
			}
			default:
				break;
			}
		}
	}

	// Update round timer UI countdown
	if (m_roundUiActive)
	{
		if (m_roundRemainMs > dwTickDiff)
		{
			m_roundRemainMs -= dwTickDiff;
			// periodic round remaining announcements: every 30s, 10s, and last 5..1s
			unsigned int sec = (unsigned int)(m_roundRemainMs / 1000);
			if (sec != m_nextRoundAnnounceSec)
			{
				if (sec % 30 == 0 || sec == 10 || (sec <= 5 && sec >= 1))
				{
					AnnounceRoundTimeRemaining(sec);
				}
				m_nextRoundAnnounceSec = sec;
			}
		}
		else
		{
			// time over
			m_roundRemainMs = 0;
			StopRoundTimerUI();
			BroadcastSystem(L"[Arena] Round time is over.");
			// Always finish on timeout in CC battle mode to avoid stuck matches
			FinishOnTimeout();
		}
	}

	// Only check faint/alive while the battle is actually running
	if (m_state == State::IN_ROUND)
	{
		// Skip alive checks during the initial settle period to avoid false finishes
		if (m_runSettleMs == 0)
		{
			CheckFaintAndAliveLogic();
		}
		// Drive random mob wave spawns during RUN
		if (m_cfg.mobsAllowed && m_cfg.randomMobsSpawn && m_cfg.randomMobsWaveSeconds > 0)
		{
			if (m_randomWaveRemainMs > dwTickDiff)
				m_randomWaveRemainMs -= dwTickDiff;
			else
			{
				m_randomWaveRemainMs = ToMs(m_cfg.randomMobsWaveSeconds);
				SpawnRandomMobWave(m_cfg.randomMobsPerWave);
			}
		}
	}
}

// Drive automation even when the arena state is IDLE (called from game loop)
void CArenaManager::AutomationTick(unsigned long dwTickDiff)
{
	if (!m_cfg.autoEnabled)
		return;

	// Track elapsed time in AutoArena phase and add an independent ensure timer
	if (m_prevAutoState != m_autoState)
	{
		m_prevAutoState = m_autoState;
		m_autoStateElapsedMs = 0;
	}
	else
	{
		m_autoStateElapsedMs += dwTickDiff;
	}
	if (m_autoEnsureRemainMs > 0)
	{
		m_autoEnsureRemainMs = (m_autoEnsureRemainMs > dwTickDiff) ? (m_autoEnsureRemainMs - dwTickDiff) : 0;
	}

	// Optional channel-name filter: only run on matching channels
	CGameServer* app = (CGameServer*)g_pApp;
	if (m_cfg.autoChannelName.c_str() && m_cfg.autoChannelName.c_str()[0] != '\0')
	{
		std::string want = m_cfg.autoChannelName.c_str();
		std::string got = app->m_config.ChannelName.c_str();
		for (auto& c : want) c = (char)tolower(c);
		for (auto& c : got) c = (char)tolower(c);
		if (got.find(want) == std::string::npos)
		{
			m_autoState = AutoState::OFF; // not our channel
			return;
		}
	}

	// Initialize automation if currently off
	if (m_autoState == AutoState::OFF)
	{
		m_autoState = AutoState::WAIT_NEXT;
		// Target the first fight to happen at InitialDelaySeconds; open enrollment earlier if needed
		unsigned int firstDelay = m_cfg.autoInitialDelaySeconds;
		unsigned int prepWaitSec = 0;
		if (firstDelay > m_cfg.autoEnrollmentSeconds)
			prepWaitSec = firstDelay - m_cfg.autoEnrollmentSeconds;
		else
			prepWaitSec = 0; // open enrollment immediately
		m_autoRemainMs = ToMs(prepWaitSec);
		// As an extra safety, arm an ensure timer to force an open if scheduler stalls
		m_autoEnsureRemainMs = ToMs((prepWaitSec > 0 ? prepWaitSec : 1) * 2);
		return;
	}

	if (m_autoRemainMs > 0)
	{
		if (m_autoRemainMs > dwTickDiff) m_autoRemainMs -= dwTickDiff; else m_autoRemainMs = 0;
	}

	if (m_autoState == AutoState::WAIT_NEXT && m_autoRemainMs == 0)
	{
		// Don't interrupt an active arena; defer until it returns to IDLE/COMPLETE.
		// However, if we're stuck in ENROLLMENT with no participants, allow automation to proceed.
		if (!(m_state == State::IDLE || m_state == State::COMPLETE ||
			(m_state == State::ENROLLMENT && m_participants.empty())))
		{
			// retry in 5 seconds
			m_autoRemainMs = 5000;
			return;
		}
		// Open a new enrollment window
		// Select world for this auto event
		unsigned int wid = 0;
		if (!m_cfg.autoWorldTblidxList.empty())
		{
			if (m_cfg.autoRandomizeWorlds)
			{
				// random pick
				unsigned int idx = (unsigned int)(rand() % m_cfg.autoWorldTblidxList.size());
				wid = m_cfg.autoWorldTblidxList[idx];
			}
			else
			{
				// round-robin
				if (m_autoWorldIndex >= m_cfg.autoWorldTblidxList.size()) m_autoWorldIndex = 0;
				wid = m_cfg.autoWorldTblidxList[m_autoWorldIndex++];
			}
			// If per-round rotation requested, set the arena world list to the AutoArena list now
			if (m_cfg.autoMapPerRound)
			{
				m_cfg.worldTblidxList = m_cfg.autoWorldTblidxList;
				// align world index with chosen wid
				for (size_t i = 0; i < m_cfg.worldTblidxList.size(); ++i) if (m_cfg.worldTblidxList[i] == wid) { m_worldIndex = (unsigned int)i; break; }
				m_cfg.mapPerRound = true;
			}
			else
			{
				m_cfg.mapPerRound = false; // honor AutoArena setting explicitly
			}
		}
		if (wid == 0) wid = (m_cfg.autoWorldTblidx ? m_cfg.autoWorldTblidx : 900043);
		ForceCurrentWorld(wid);
		// Use world_fight setup to configure elimination or score
		unsigned int sec = m_cfg.autoUseElimination ? (m_cfg.roundTimerSeconds ? m_cfg.roundTimerSeconds : 0) : (m_cfg.roundTimerSeconds ? m_cfg.roundTimerSeconds : 900);
		SetupWorldFight(!m_cfg.autoUseElimination, sec, m_cfg.autoMode);
		// Open enrollment and announce join command
		Start(m_cfg.autoMode);
		BroadcastSystem(L"[Arena] Auto event opened. Use @arenajoin within the next minutes to participate.");
		m_autoState = AutoState::ENROLLMENT_OPEN;
		m_autoRemainMs = ToMs(m_cfg.autoEnrollmentSeconds);
		// Ensure watchdog: if the arena fails to open correctly or Start() didn't change state, bump it
		if (m_state != State::ENROLLMENT)
		{
			m_state = State::ENROLLMENT;
		}
		// Arm ensure timer to force-close enrollment in case TeleportParticipants fails to reset
		m_autoEnsureRemainMs = ToMs(m_cfg.autoEnrollmentSeconds + 15);
		return;
	}

	if (m_autoState == AutoState::ENROLLMENT_OPEN)
	{
		// Announce remaining enrollment time every minute (without spamming within the same second)
		if (m_autoRemainMs > dwTickDiff)
		{
			unsigned int secLeft = (unsigned int)(m_autoRemainMs / 1000);
			static unsigned int s_lastAnnouncedSecLeft = 0;
			if (secLeft != s_lastAnnouncedSecLeft && secLeft % 60 == 0)
			{
				wchar_t msg[128];
				swprintf_s(msg, _countof(msg), L"[Arena] Enrollment closes in %u minute(s). Use @arenajoin now!", secLeft / 60);
				BroadcastSystem(msg);
				s_lastAnnouncedSecLeft = secLeft;
			}
			m_autoRemainMs -= dwTickDiff;
			return;
		}
		// Enrollment period over: teleport and start
		m_autoRemainMs = 0;
		TeleportParticipants(true);
		TeleportSpectators();
		// If teleport/start was aborted due to validation failure (e.g., not enough participants),
		// TeleportParticipants(true) leaves state as ENROLLMENT. Clear the participant list now so the
		// "no participants" branch below resets the arena to IDLE and allows the next auto cycle to open.
		if (m_state == State::ENROLLMENT)
		{
			m_participants.clear();
		}
		BroadcastSystem(L"[Arena] Enrollment closed. Teleporting participants...");
		// If nobody joined, reset arena state to IDLE so the next cycle can open properly
		if (m_participants.empty())
		{
			m_state = State::IDLE;
			m_inviting = false;
			m_inviteRemainMs = 0;
			m_pendingStartMs = 0;
			m_pendingStartWorldId = 0;
			m_waitAllArriveMs = 0;
			m_readyParticipants.clear();
			m_readyDelayMs.clear();
			m_pendingStartParticipants.clear();
		}
		// Next cycle: schedule opening so that the next fight happens after IntervalSeconds from now
		m_autoState = AutoState::WAIT_NEXT;
		unsigned int prepWaitSec = 0;
		if (m_cfg.autoIntervalSeconds > m_cfg.autoEnrollmentSeconds)
			prepWaitSec = m_cfg.autoIntervalSeconds - m_cfg.autoEnrollmentSeconds;
		else
			prepWaitSec = 0;
		m_autoRemainMs = ToMs(prepWaitSec);
		m_autoEnsureRemainMs = ToMs((prepWaitSec > 0 ? prepWaitSec : 1) * 2);
	}

	// Auto watchdog: if automation appears stalled beyond configured thresholds, nudge it
	if (m_cfg.watchdogEnabled)
	{
		// If we sit in ENROLLMENT for too long outside an open window, reset to IDLE so next cycle can open
		unsigned int enrollMaxMs = ToMs(m_cfg.watchdogEnrollmentSeconds ? m_cfg.watchdogEnrollmentSeconds : (m_cfg.autoEnrollmentSeconds ? (m_cfg.autoEnrollmentSeconds + 30) : 180));
		if (m_state == State::ENROLLMENT && m_autoState != AutoState::ENROLLMENT_OPEN && m_stateElapsedMs > enrollMaxMs)
		{
			ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA][WATCHDOG][AUTO] Enrollment stuck > %ums. Resetting to IDLE.", (unsigned)(enrollMaxMs / 1000));
			m_participants.clear();
			m_state = State::IDLE;
			m_stateElapsedMs = 0;
		}
		// If WAIT_NEXT takes too long (scheduler didn't fire), force-open a new enrollment
		if (m_autoState == AutoState::WAIT_NEXT && m_autoEnsureRemainMs == 0)
		{
			ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA][WATCHDOG][AUTO] Forcing enrollment open (ensure timer).");
			m_autoRemainMs = 0; // fall into the open branch on next tick
		}
	}
}

// [Removed duplicate definitions of Start/Stop here]

void CArenaManager::RotateMapNow()
{
	if (m_cfg.worldTblidxList.empty()) return;
	// Do not rotate map mid-match for ARENAPODER maps to preserve original behavior
	if (!ShouldRotatePerRoundForWorld(m_currentWorldTblidx))
	{
		ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA] RotateMapNow skipped for ARENAPODER world %u", (unsigned)m_currentWorldTblidx);
		return;
	}
	StopRoundTimerUI();
	m_worldIndex = (m_worldIndex + 1) % m_cfg.worldTblidxList.size();
	m_currentWorldTblidx = m_cfg.worldTblidxList[m_worldIndex];
	// If Budokai-rule worlds are disallowed, skip them during rotation
	if (!m_cfg.allowBudokaiRuleWorlds)
	{
		for (size_t i = 0; i < m_cfg.worldTblidxList.size(); ++i)
		{
			sWORLD_TBLDAT* pWorldData = (sWORLD_TBLDAT*)g_pTableContainer->GetWorldTable()->FindData((TBLIDX)m_currentWorldTblidx);
			if (!pWorldData) break;
			BYTE rule = pWorldData->byWorldRuleType;
			bool isBudoRule = (rule == GAMERULE_MINORMATCH || rule == GAMERULE_MAJORMATCH || rule == GAMERULE_FINALMATCH);
			if (!isBudoRule) break;
			m_worldIndex = (m_worldIndex + 1) % m_cfg.worldTblidxList.size();
			m_currentWorldTblidx = m_cfg.worldTblidxList[m_worldIndex];
		}
	}
	m_rotationRemainMs = ToMs(m_cfg.rotationSeconds);
	m_nextRotationAnnounceSec = (unsigned int)(m_rotationRemainMs / 1000);
	// Force fresh world creation on next EnsureCurrentWorldId()
	m_currentWorldId = 0;
	BroadcastSystem(L"[Arena] Map rotated.");
	NTL_PRINT(PRINT_APP, _T("[ARENA] Rotated to world tblidx %u"), m_currentWorldTblidx);
	// Move active participants to new map immediately
	if (!m_participants.empty())
		TeleportParticipants();
}
bool CArenaManager::SetCurrentWorld(unsigned int worldTblidx)
{
	if (worldTblidx == 0) return false;

	// Check if custom worlds are allowed
	if (!m_cfg.allowCustomWorlds)
	{
		NTL_PRINT(PRINT_APP, _T("[ARENA] SetCurrentWorld: Custom worlds disabled, ignoring request for tblidx %u"), worldTblidx);
		BroadcastSystem(L"[Arena] Custom worlds are disabled. Using RankBattle table worlds only.");
		return false;
	}

	// validate exists in table
	sWORLD_TBLDAT* pWorldTbldat = (sWORLD_TBLDAT*)g_pTableContainer->GetWorldTable()->FindData((TBLIDX)worldTblidx);
	if (!pWorldTbldat) return false;

	StopRoundTimerUI();
	m_currentWorldTblidx = worldTblidx;
	// Force fresh world creation on next EnsureCurrentWorldId()
	m_currentWorldId = 0;
	// update index if present in list
	for (size_t i = 0; i < m_cfg.worldTblidxList.size(); ++i)
		if (m_cfg.worldTblidxList[i] == worldTblidx) { m_worldIndex = i; break; }
	m_rotationRemainMs = ToMs(m_cfg.rotationSeconds);
	m_nextRotationAnnounceSec = (unsigned int)(m_rotationRemainMs / 1000);
	BroadcastSystem(L"[Arena] Map set.");
	NTL_PRINT(PRINT_APP, _T("[ARENA] Set world to tblidx %u"), worldTblidx);
	return true;
}

bool CArenaManager::ForceCurrentWorld(unsigned int worldTblidx)
{
	if (worldTblidx == 0) return false;
	// validate exists in table
	sWORLD_TBLDAT* pWorldTbldat = (sWORLD_TBLDAT*)g_pTableContainer->GetWorldTable()->FindData((TBLIDX)worldTblidx);
	if (!pWorldTbldat) return false;
	StopRoundTimerUI();
	m_currentWorldTblidx = worldTblidx;
	m_currentWorldId = 0; // force fresh instance on next ensure
	// Refresh rotation timer but do not change index list
	m_rotationRemainMs = ToMs(m_cfg.rotationSeconds);
	m_nextRotationAnnounceSec = (unsigned int)(m_rotationRemainMs / 1000);
	NTL_PRINT(PRINT_APP, _T("[ARENA] Force world to tblidx %u"), worldTblidx);
	return true;
}

unsigned int CArenaManager::GetOrCreateCurrentWorldId()
{
	return m_currentWorldId ? m_currentWorldId : EnsureCurrentWorldId();
}

bool CArenaManager::IsArenaWorldTblidx(unsigned int worldTblidx) const
{
	// Primary: name-based Arena detection (TORNEOPODER etc.)
	if (IsArenaWorldByTblidxName((TBLIDX)worldTblidx))
		return true;

	// Configured Arena rotation list (main Arena worlds)
	for (unsigned int v : m_cfg.worldTblidxList)
		if (v == worldTblidx) return true;

	// AutoArena world(s)
	if (m_cfg.autoWorldTblidx && m_cfg.autoWorldTblidx == worldTblidx)
		return true;
	for (unsigned int v : m_cfg.autoWorldTblidxList)
		if (v == worldTblidx) return true;

	// Instance expansion heuristic: if worldTblidx matches any configured base within small range
	// Example: base 10000 implies instances 10000..10100 are considered arena. Keep small to avoid false positives.
	auto within = [&](unsigned int base, unsigned int id) -> bool {
		const unsigned int kSpan = 200; // +/- range
		return id >= base && id <= base + kSpan;
		};
	for (unsigned int base : m_cfg.autoWorldTblidxList)
		if (within(base, worldTblidx)) return true;
	for (unsigned int base : m_cfg.worldTblidxList)
		if (within(base, worldTblidx)) return true;
	if (m_cfg.autoWorldTblidx && within(m_cfg.autoWorldTblidx, worldTblidx)) return true;

	return false;
}

void CArenaManager::SetupWorldFight(bool scoreMode, unsigned int roundSeconds, Mode mode)
{
	// Minimal, targeted configuration for requested world-fight event
	m_mode = mode;
	// World-fight is an on-demand event: force-enable Arena runtime so state machine, timers,
	// and PvP toggles are active regardless of INI defaults
	m_cfg.enabled = true;
	// Use CC battle flow with rank-like HUD and strict unlocks
	m_cfg.ccBattleMode = true;
	m_cfg.rankUiEnabled = true;
	m_cfg.rankPacketsEnabled = true;
	// Allow using explicitly requested worlds (like 900043) without being overridden
	m_cfg.allowCustomWorlds = true;
	// Also allow Budokai-rule worlds if the chosen world uses those rules
	m_cfg.allowBudokaiRuleWorlds = true;
	// Strictly use the forced world, never fall back silently
	m_cfg.forceExactWorld = true;
	// Lock the event to the currently forced world only: disable randomization/rotation and
	// constrain the rotation list to a single entry so nothing overrides the GM's choice.
	m_cfg.randomizeMapOnStart = false;
	// Keep current mapPerRound setting (AutoArena may enable it)
	m_cfg.rotationSeconds = 0; // no auto-rotation during world_fight
	m_cfg.useOnlyCustomWorlds = true;
	// Always use direct teleport for world_fight to avoid client-side RankBattle routing
	m_cfg.useInviteFlow = false;
	// Make the whole arena world PvP during RUN so players can attack freely (GM event expectation)
	m_cfg.worldWidePvpDuringRun = true;
	// This is not a true RankBattle: suppress Rank finish/leave UI at the end to avoid HUD bugs
	m_cfg.suppressRankFinishUi = false;
	if (m_currentWorldTblidx != 0)
	{
		m_cfg.worldTblidxList.clear();
		m_cfg.worldTblidxList.push_back(m_currentWorldTblidx);
		m_worldIndex = 0;
	}
	// Timer and finish behavior
	m_cfg.roundTimerSeconds = roundSeconds;
	m_cfg.stopOnTimeout = true;
	// Scoring and revive model
	if (scoreMode)
	{
		m_cfg.reviveOnFaint = true;       // respawn allowed
		m_cfg.faintBecomeSpectator = false;
		m_cfg.scoreOnFaint = true;        // count points per faint
	}
	else
	{
		m_cfg.reviveOnFaint = false;      // no respawn
		m_cfg.faintBecomeSpectator = true;
		m_cfg.scoreOnFaint = false;       // winner by last alive
	}
}

void CArenaManager::StatusTo(CPlayer* pWho)
{
	if (!pWho) return;
	wchar_t buf[512];
	unsigned int roundLeft = m_roundUiActive ? (unsigned int)(m_roundRemainMs / 1000) : 0;

	// Enhanced status with timing details for debugging
	unsigned int pendingStartSec = (unsigned int)(m_pendingStartMs / 1000);
	unsigned int waitArriveSec = (unsigned int)(m_waitAllArriveMs / 1000);
	unsigned int readyCount = (unsigned int)m_readyParticipants.size();

	swprintf_s(buf, L"[Arena] State=%u Mode=%u World=%u NextRotate=%us RoundLeft=%us Enabled=%d\nParticipants=%u(ready=%u) Spectators=%u Winners=%u\nTimers: PendingStart=%us WaitArrive=%us WorldId=%u",
		(unsigned)m_state, (unsigned)m_mode, m_currentWorldTblidx, (unsigned)(m_rotationRemainMs / 1000), roundLeft, (int)m_cfg.enabled,
		(unsigned)m_participants.size(), readyCount, (unsigned)m_spectators.size(), (unsigned)m_winners.size(),
		pendingStartSec, waitArriveSec, m_pendingStartWorldId);

	CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
	res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
	res->byDisplayType = SERVER_TEXT_SYSNOTICE;
	res->wMessageLengthInUnicode = (WORD)wcslen(buf);
	wcscpy_s(res->awchMessage, NTL_MAX_LENGTH_OF_CHAT_MESSAGE + 1, buf);
	packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	pWho->SendPacket(&packet);
}

void CArenaManager::BroadcastSystem(const wchar_t* msg, unsigned char byType)
{
	CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
	res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
	res->byDisplayType = byType;
	res->wMessageLengthInUnicode = (WORD)wcslen(msg);
	wcscpy_s(res->awchMessage, NTL_MAX_LENGTH_OF_CHAT_MESSAGE + 1, msg);
	packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	g_pObjectManager->SendPacketToAll(&packet);
}

void CArenaManager::SendSystemTo(CPlayer* pPlayer, const wchar_t* msg, unsigned char byType)
{
	if (!pPlayer || !msg) return;
	CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
	res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
	res->byDisplayType = byType;
	res->wMessageLengthInUnicode = (WORD)wcslen(msg);
	wcscpy_s(res->awchMessage, NTL_MAX_LENGTH_OF_CHAT_MESSAGE + 1, msg);
	packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	pPlayer->SendPacket(&packet);
}

void CArenaManager::AwardRewards(bool winnersOnly)
{
	if (!m_cfg.rewardsEnabled)
		return;

	const auto& list = winnersOnly ? m_cfg.winnerRewards : m_cfg.participantRewards;
	if (list.empty()) return;

	auto grant = [&](CPlayer* p, const std::vector<std::pair<unsigned int, unsigned int>>& rewards)
		{
			for (const auto& pr : rewards)
			{
				unsigned int item = pr.first;
				unsigned int cnt = pr.second ? pr.second : 1;
				if (item == 0)
				{
					ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA][REWARD][WARN] Skipping invalid item tblidx=0 for char=%u", (unsigned)p->GetCharID());
					continue;
				}
				sITEM_TBLDAT* pItemTbldat = (sITEM_TBLDAT*)g_pTableContainer->GetItemTable()->FindData((TBLIDX)item);
				if (!pItemTbldat)
				{
					ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA][REWARD][WARN] Item tblidx=%u not found. Skipping for char=%u", item, (unsigned)p->GetCharID());
					continue;
				}
				// Respect item stack limits by creating multiple stacks/instances until the requested count is fulfilled
				unsigned int maxStack = (pItemTbldat->byMax_Stack > 0) ? (unsigned int)pItemTbldat->byMax_Stack : 1u;
				if (maxStack == 0) maxStack = 1; // safety
				unsigned int remaining = cnt;
				while (remaining > 0)
				{
					// Per create call, cap to both the item stack size and 255 (packet/count limit)
					unsigned int cap255 = (maxStack < 255u) ? maxStack : 255u;
					unsigned int give = (remaining < cap255) ? remaining : cap255;
					if (!g_pItemManager->CreateItem(p, (TBLIDX)item, (BYTE)give))
					{
						ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA][REWARD][ERR] CreateItem failed for char=%u, item tblidx=%u, count=%u (remaining=%u)", (unsigned)p->GetCharID(), item, give, remaining);
						break; // stop trying this item if creation fails (likely due to no space)
					}
					remaining -= give;
				}
			}
		};

	if (winnersOnly)
	{
		for (auto cid : m_winners)
		{
			CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid);
			if (p && p->IsInitialized()) grant(p, list);
		}
	}
	else
	{
		for (auto cid : m_participants)
		{
			CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid);
			if (p && p->IsInitialized()) grant(p, list);
		}
	}
}

void CArenaManager::AwardMudosaPoints()
{
	// Only act if Rewards are enabled; Mudosa awards are part of the reward phase
	if (!m_cfg.rewardsEnabled)
		return;
	const unsigned int winPts = m_cfg.mudosaWinnerPoints;
	const unsigned int partPts = m_cfg.mudosaParticipantPoints;
	if (winPts == 0 && partPts == 0)
		return;

	// Winners: grant winner points
	if (winPts > 0)
	{
		for (auto cid : m_winners)
		{
			if (CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid))
			{
				if (!p->IsInitialized()) continue;
				p->UpdateMudosaPoints(p->GetMudosaPoints() + winPts, true);
			}
		}
	}
	// Participants: grant participant points to all participants who are not in winners set
	if (partPts > 0)
	{
		for (auto cid : m_participants)
		{
			if (m_winners.find(cid) != m_winners.end())
				continue; // skip, already got winner points
			if (CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid))
			{
				if (!p->IsInitialized()) continue;
				p->UpdateMudosaPoints(p->GetMudosaPoints() + partPts, true);
			}
		}
	}
}

void CArenaManager::TryRotateByTime(unsigned long dwTickDiff)
{
	if (m_cfg.rotationSeconds == 0 || m_cfg.worldTblidxList.size() <= 1)
		return;
	// Skip timed rotation for ARENAPODER maps during a match
	if (!ShouldRotatePerRoundForWorld(m_currentWorldTblidx))
		return;
	if (m_rotationRemainMs > dwTickDiff)
		m_rotationRemainMs -= dwTickDiff;
	else
		RotateMapNow();
}

void CArenaManager::ParseWorldListCsv(const CNtlString& csv)
{
	m_cfg.worldTblidxList.clear();
	std::string s = csv.c_str();
	size_t pos = 0;
	while (pos != std::string::npos)
	{
		size_t comma = s.find(',', pos);
		std::string tok = s.substr(pos, comma == std::string::npos ? std::string::npos : (comma - pos));
		if (!tok.empty())
		{
			unsigned int v = (unsigned int)strtoul(tok.c_str(), nullptr, 10);
			if (v != 0)
				m_cfg.worldTblidxList.push_back(v);
		}
		if (comma == std::string::npos) break;
		pos = comma + 1;
		while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\t')) ++pos; // trim whitespace
	}
}

bool CArenaManager::AddParticipant(CPlayer* pPlayer)
{
	if (!pPlayer || !pPlayer->IsInitialized() || !m_cfg.enabled) return false;
	// store previous location before we move them
	SavePrevLocation(pPlayer);
	m_participants.insert(pPlayer->GetCharID());
	wchar_t buf[128];
	swprintf_s(buf, _countof(buf), L"%s joined the arena!", pPlayer->GetCharName());
	SendNotice(buf, SERVER_TEXT_SYSNOTICE);
	return true;
}

bool CArenaManager::AddSpectator(CPlayer* pPlayer)
{
	if (!pPlayer || !pPlayer->IsInitialized() || !m_cfg.enabled || !m_cfg.spectatorsEnabled) return false;
	// store previous location before we move them
	SavePrevLocation(pPlayer);
	m_spectators.insert(pPlayer->GetCharID());
	wchar_t buf[128];
	swprintf_s(buf, _countof(buf), L"%s is spectating the arena.", pPlayer->GetCharName());
	SendNotice(buf, SERVER_TEXT_SYSNOTICE);
	return true;
}

bool CArenaManager::Remove(CPlayer* pPlayer)
{
	if (!pPlayer) return false;
	m_participants.erase(pPlayer->GetCharID());
	m_spectators.erase(pPlayer->GetCharID());
	m_winners.erase(pPlayer->GetCharID());
	// Clear any saved previous location snapshot once player leaves arena context
	m_prevLoc.erase(pPlayer->GetCharID());
	return true;
}

void CArenaManager::TeleportParticipants()
{
	TeleportParticipants(false); // Use normal flow
}

void CArenaManager::TeleportParticipants(bool forceDirect)
{
	ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA] TeleportParticipants forceDirect=%d useInvite=%d ccMode=%d worldTblidx=%u", forceDirect ? 1 : 0, m_cfg.useInviteFlow ? 1 : 0, m_cfg.ccBattleMode ? 1 : 0, (unsigned)m_currentWorldTblidx);
	// Validate team composition before starting the battle
	if (!ValidateTeamComposition())
	{
		ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA] TeleportParticipants: team validation failed, staying ENROLLMENT");
		// Do not Stop(true) here (which sets COMPLETE); stay in enrollment
		m_state = State::ENROLLMENT;
		return;
	}

	if (m_currentWorldTblidx == 0)
	{
		// If only-custom mode is enabled, do NOT load Rank worlds; require a custom list
		if (m_cfg.useOnlyCustomWorlds)
		{
			if (!m_cfg.worldTblidxList.empty())
			{
				m_worldIndex = 0;
				m_currentWorldTblidx = m_cfg.worldTblidxList[m_worldIndex];
				m_currentWorldId = 0;
			}
			else
			{
				SendNotice(L"[Arena] No custom worlds configured. Use @arena cfg worlds <csv>.", SERVER_TEXT_SYSTEM);
				NTL_PRINT(PRINT_APP, _T("[ARENA] onlycustom mode but no custom worlds provided"));
				return;
			}
		}
		else
		{
			NTL_PRINT(PRINT_APP, _T("[ARENA] TeleportParticipants: m_currentWorldTblidx==0; attempting to load RankBattle worlds on-the-fly."));
			LoadAvailableWorlds();
			if (m_currentWorldTblidx == 0)
			{
				NTL_PRINT(PRINT_APP, _T("[ARENA] No RankBattle worlds available after reload. Falling back to existing participant world."));
				// No arena world configured and none could be loaded: treat current participant world as arena world
				unsigned int existingWorldId = GetAnyParticipantWorldId();
				if (existingWorldId)
				{
					m_currentWorldId = existingWorldId;
					// Fast-path: if everyone is already in the same world, proceed with short READY and RUN
					m_state = State::STAGE_READY;
					m_stageReadyTimeMs = 1000; // 1 second ready toast
					if (m_cfg.rankUiEnabled)
					{
						BroadcastRankStateToWorld(existingWorldId, RANKBATTLE_BATTLESTATE_STAGE_PREPARE, m_rankBattleStage);
						BroadcastRankStateToWorld(existingWorldId, RANKBATTLE_BATTLESTATE_STAGE_READY, m_rankBattleStage);
					}
					SendNotice(L"Get ready for battle!", SERVER_TEXT_SYSNOTICE);
					NTL_PRINT(PRINT_APP, _T("[ARENA] No configured world - using existing world, state: STAGE_READY"));
				}
				else
				{
					NTL_PRINT(PRINT_APP, _T("[ARENA] TeleportParticipants: no participants online to derive world"));
				}
				return;
			}
			// World tblidx acquired; continue with invite flow below
		}
	}
	// If invite flow enabled and not forced direct, send proposals to participants and defer start
	if (m_cfg.useInviteFlow && !forceDirect)
	{
		// Use per-player teleports by worldTblidx so each player's owning GameServer creates/uses
		// a local world instance. This avoids cross-server worldId mismatches that can leave
		// players stuck on the loading screen when a world is created only on another channel.
		sWORLD_TBLDAT* pWorldTbldat = (sWORLD_TBLDAT*)g_pTableContainer->GetWorldTable()->FindData((TBLIDX)m_currentWorldTblidx);
		if (!pWorldTbldat) return;

		if (m_cfg.ccBattleMode)
		{
			size_t teamSize = m_participants.size() / 2;
			size_t teamIndex = 0;

			for (auto cid : m_participants)
			{
				CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid);
				if (!p || !p->IsInitialized()) continue;

				CNtlVector destLoc, destDir;
				if (teamIndex < teamSize)
				{
					destLoc = pWorldTbldat->vStart1Loc;
					destDir = pWorldTbldat->vStart1Dir;
					destLoc.x += RandomRangeF(-3.0f, 3.0f);
					destLoc.z += RandomRangeF(-3.0f, 3.0f);
					if (sRANK_BATTLE_DATA* rd1 = p->GetRankBattleData()) rd1->eTeamType = RANKBATTLE_TEAM_OWNER; else ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA][WARN] Null RankBattleData on invite team assign (owner) char=%u", (unsigned)p->GetCharID());
				}
				else
				{
					destLoc = pWorldTbldat->vStart2Loc;
					destDir = pWorldTbldat->vStart2Dir;
					destLoc.x += RandomRangeF(-3.0f, 3.0f);
					destLoc.z += RandomRangeF(-3.0f, 3.0f);
					if (sRANK_BATTLE_DATA* rd2 = p->GetRankBattleData()) rd2->eTeamType = RANKBATTLE_TEAM_CHALLENGER; else ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA][WARN] Null RankBattleData on invite team assign (challenger) char=%u", (unsigned)p->GetCharID());
				}

				// Per-player teleport by worldTblidx (server will create/find its local instance)
				TeleportOneToWorldTblidxDir(p, m_currentWorldTblidx, destLoc.x, destLoc.y, destLoc.z, destDir.x, destDir.y, destDir.z);
				NTL_PRINT(PRINT_APP, _T("[ARENA] Invite TP (CC Battle): char=%u team=%u tblidx=%u"), (unsigned)cid, (teamIndex < teamSize ? 1 : 2), (unsigned)m_currentWorldTblidx);
				teamIndex++;
			}
		}
		else
		{
			CNtlVector destLoc = pWorldTbldat->vStart1Loc;
			CNtlVector destDir = pWorldTbldat->vStart1Dir;

			for (auto cid : m_participants)
			{
				CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid);
				if (!p || !p->IsInitialized()) continue;

				TeleportOneToWorldTblidxDir(p, m_currentWorldTblidx, destLoc.x, destLoc.y, destLoc.z, destDir.x, destDir.y, destDir.z);
				NTL_PRINT(PRINT_APP, _T("[ARENA] Invite TP: char=%u tblidx=%u"), (unsigned)cid, (unsigned)m_currentWorldTblidx);
			}
		}

		// Prepare pre-round window after invites
		m_inviting = true;
		m_inviteRemainMs = ToMs(m_cfg.inviteWaitSeconds);
		m_state = State::PRE_ROUND;
		SendNotice(L"Arena invites sent. Please accept to join.", SERVER_TEXT_SYSNOTICE);
		ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA] Invites sent (by tblidx): tblidx=%u waitSec=%u participants=%u", (unsigned)m_currentWorldTblidx, (unsigned)m_cfg.inviteWaitSeconds, (unsigned)m_participants.size());
		// Force the next world id lookup to create or find a fresh instance as needed
		m_currentWorldId = 0;
		return;
	}

	// Ensure our exact world instance exists before teleporting
	m_currentWorldId = EnsureCurrentWorldId();
	if (m_cfg.forceExactWorld && m_currentWorldId == 0)
	{
		SendNotice(L"[Arena] Failed to create the requested world instance.", SERVER_TEXT_SYSNOTICE);
		NTL_PRINT(PRINT_APP, _T("[ARENA] Direct teleport aborted: cannot create world for tblidx=%u (forceExactWorld)"), (unsigned)m_currentWorldTblidx);
		return;
	}

	for (auto cid : m_participants)
	{
		CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid);
		if (!p || !p->IsInitialized()) { NTL_PRINT(PRINT_APP, _T("[ARENA] Skip TP: player missing or not initialized: cid=%u"), cid); continue; }
		TeleportOneToWorldTblidx(p, m_currentWorldTblidx, 0.f, 0.f, 0.f);
	}
	// Schedule delayed round start when teleported directly
	m_state = State::PRE_ROUND;
	m_pendingStartMs = ToMs(m_cfg.startDelaySeconds);
	m_pendingStartWorldId = m_currentWorldId ? m_currentWorldId : EnsureCurrentWorldId();
	m_waitAllArriveMs = ToMs(m_cfg.maxWaitAllArriveSeconds);
	m_pendingStartParticipants = m_participants;
	m_readyParticipants.clear();
	m_readyDelayMs.clear();
	m_postStartDelayMs = m_cfg.postReadyDelayMs;

	// For direct teleportation (GM command), mark participants as immediately ready to speed up the process
	for (auto cid : m_participants)
	{
		m_readyParticipants.insert(cid);
	}

	NTL_PRINT(PRINT_APP, _T("[ARENA] Direct teleport completed: %u participants marked ready"), (unsigned)m_participants.size());
	ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA] Direct teleport completed: worldId=%u ready=%u", (unsigned)m_pendingStartWorldId, (unsigned)m_readyParticipants.size());
}

void CArenaManager::TeleportSpectators()
{
	if (!m_cfg.spectatorsEnabled) return;
	unsigned int worldTblidx = m_cfg.spectatorsUseSameWorld ? m_currentWorldTblidx : m_cfg.spectatorWorldTblidx;
	if (worldTblidx == 0) return;
	for (auto cid : m_spectators)
	{
		CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid);
		if (!p || !p->IsInitialized()) continue;
		// Spectators never use invite; just move them to spectator target
		TeleportOneToWorldTblidx(p, worldTblidx, m_cfg.spectatorPosX, m_cfg.spectatorPosY, m_cfg.spectatorPosZ);
		if (m_cfg.spectatorHide)
			ApplySpectatorHide(p, true);
	}
}

void CArenaManager::TeleportParticipantsHere(CPlayer* pGm)
{
	if (!pGm || !pGm->IsInitialized()) return;
	for (auto cid : m_participants)
	{
		if (CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid))
		{
			if (!p->IsInitialized()) continue;
			// Only teleports players connected to this GameServer instance
			p->StartTeleport(pGm->GetCurLoc(), pGm->GetCurDir(), pGm->GetWorldID(), TELEPORT_TYPE_COMMAND);
		}
	}
}

void CArenaManager::TeleportSpectatorsHere(CPlayer* pGm)
{
	if (!pGm || !pGm->IsInitialized()) return;
	for (auto cid : m_spectators)
	{
		if (CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid))
		{
			if (!p->IsInitialized()) continue;
			// Only teleports players connected to this GameServer instance
			p->StartTeleport(pGm->GetCurLoc(), pGm->GetCurDir(), pGm->GetWorldID(), TELEPORT_TYPE_COMMAND);
		}
	}
}

void CArenaManager::MarkWinner(CPlayer* pPlayer)
{
	if (!pPlayer) return;
	m_winners.insert(pPlayer->GetCharID());
}

void CArenaManager::ClearWinners()
{
	m_winners.clear();
}

void CArenaManager::ParseRewardsCsv(const CNtlString& csv, std::vector<std::pair<unsigned int, unsigned int>>& out)
{
	out.clear();
	std::string s = csv.c_str();
	size_t pos = 0;
	while (pos != std::string::npos)
	{
		size_t sep = s.find(',', pos);
		std::string tok = s.substr(pos, sep == std::string::npos ? std::string::npos : (sep - pos));
		if (!tok.empty())
		{
			size_t colon = tok.find(':');
			if (colon == std::string::npos)
			{
				unsigned int item = (unsigned int)strtoul(tok.c_str(), nullptr, 10);
				if (item) out.push_back({ item, 1 });
			}
			else
			{
				unsigned int item = (unsigned int)strtoul(tok.substr(0, colon).c_str(), nullptr, 10);
				unsigned int cnt = (unsigned int)strtoul(tok.substr(colon + 1).c_str(), nullptr, 10);
				if (item && cnt) out.push_back({ item, cnt });
			}
		}
		if (sep == std::string::npos) break;
		pos = sep + 1;
		while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\t')) ++pos;
	}
}

void CArenaManager::MoveToSpectator(CPlayer* pPlayer)
{
	if (!pPlayer || !pPlayer->IsInitialized())
		return;
	// Clear any active round timers/countdown for this player to avoid HUD freeze
	SendRoundTimerEndTo(pPlayer);
	// Ensure fallback countdown is also stopped explicitly
	SendCountdownTo(pPlayer, false);
	// Clear RankBattle HUD for this player to avoid client freezes leaving the fight context
	if (m_cfg.rankUiEnabled)
		SendRankLeaveTo(pPlayer);
	// Remove from participants if present
	m_participants.erase(pPlayer->GetCharID());
	// Ensure client is unlocked from any action states
	pPlayer->SendCharStateStanding();
	AddSpectator(pPlayer);
	// Apply spectator visibility regardless of teleport settings
	if (m_cfg.spectatorHide)
		ApplySpectatorHide(pPlayer, true);
	// If spectator teleporting is enabled/configured, move them; otherwise keep in place to watch
	if (m_cfg.spectatorsEnabled)
	{
		unsigned int worldTblidx = m_cfg.spectatorsUseSameWorld ? m_currentWorldTblidx : m_cfg.spectatorWorldTblidx;
		if (worldTblidx != 0)
			TeleportOneToWorldTblidx(pPlayer, worldTblidx, m_cfg.spectatorPosX, m_cfg.spectatorPosY, m_cfg.spectatorPosZ);
	}
}

void CArenaManager::ApplySpectatorHide(CPlayer* pPlayer, bool hide)
{
	if (!pPlayer) return;
	if (hide)
		pPlayer->GetStateManager()->AddConditionState(CHARCOND_TRANSPARENT, NULL, true);
	else
		pPlayer->GetStateManager()->RemoveConditionState(CHARCOND_TRANSPARENT, NULL, true);
}

void CArenaManager::SendNotice(const wchar_t* text, unsigned char byType)
{
	CGameServer* app = (CGameServer*)g_pApp;
	CNtlPacket packet(sizeof(sGT_SYSTEM_DISPLAY_TEXT));
	sGT_SYSTEM_DISPLAY_TEXT* res = (sGT_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
	res->wOpCode = GT_SYSTEM_DISPLAY_TEXT;
	res->serverChannelId = INVALID_SERVERCHANNELID;
	res->byDisplayType = byType;
	wcsncpy_s(res->wszMessage, NTL_MAX_LENGTH_OF_CHAT_MESSAGE + 1, text, _TRUNCATE);
	packet.SetPacketLen(sizeof(sGT_SYSTEM_DISPLAY_TEXT));
	app->SendTo(app->GetChatServerSession(), &packet);
}

void CArenaManager::CheckFaintAndAliveLogic()
{
	// Only evaluate finish transitions while the round is actually running
	if (m_cfg.ccBattleMode && m_cfg.rankUiEnabled)
	{
		if (m_rankBattleState != RANKBATTLE_BATTLESTATE_STAGE_RUN)
			return;
	}

	// Count alive participants in current world; handle faint according to config
	unsigned aliveCount = 0;
	unsigned aliveOwner = 0, aliveChallenger = 0; // track per-team alive in team modes
	unsigned onlineParticipants = 0; // Track participants still online (including fainted)
	unsigned lastAliveCharId = 0;
	std::vector<CPlayer*> toSpectate;
	ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA][Alive] worldTblidx=%u worldId=%u participants=%u", (unsigned)m_currentWorldTblidx, (unsigned)m_currentWorldId, (unsigned)m_participants.size());
	for (auto cid : m_participants)
	{
		CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid);
		if (!p || !p->IsInitialized()) continue;
		// same arena world?
		if (m_currentWorldTblidx != 0 && p->GetWorldTblidx() != (TBLIDX)m_currentWorldTblidx)
			continue;
		// skip if he became spectator
		if (m_spectators.find(cid) != m_spectators.end())
			continue;

		++onlineParticipants; // Count as online participant
		if (p->IsFainting())
		{
			// New policy for CC arena rounds: keep fainted players dead until the round ends.
			// Do NOT hide or convert to spectator during the round to avoid client condition bugs.
			// They will be revived at the start of the next round.
		}
		else
		{
			++aliveCount;
			lastAliveCharId = cid;
			if (m_mode == Mode::PARTY_VS_PARTY || m_mode == Mode::GUILD_VS_GUILD)
			{
				if (sRANK_BATTLE_DATA* rd = p->GetRankBattleData())
				{
					if (rd->eTeamType == RANKBATTLE_TEAM_OWNER) ++aliveOwner;
					else if (rd->eTeamType == RANKBATTLE_TEAM_CHALLENGER) ++aliveChallenger;
				}
			}
		}
	}
	// Do not move fainted players to spectator mid-round in CC mode; leave them as-is

	// Check if too few players remain online (disconnects/quits) to continue
	if (onlineParticipants < 2)
	{
		SendNotice(L"Arena ended: insufficient participants remaining (disconnect/quit).", SERVER_TEXT_SYSNOTICE);
		FinishMatch(false);
		return;
	}

	// SCORE mode: do NOT advance rounds or finish when only one alive.
	// In scoring mode we auto-respawn on faint and keep accumulating points until the timer/match ends.
	// We only abort when participants drop below 2 (handled above) or when stopped explicitly.
	// IMPORTANT: Gate by scoreOnFaint only (true score mode). A config with reviveOnFaint=true but scoreOnFaint=false
	// should still end the round when only one remains.
	if (m_cfg.scoreOnFaint)
	{
		return; // keep running; ignore alive/team elimination checks below
	}

	// End early if one team is fully eliminated in team modes
	bool teamEliminated = false;
	if (m_mode == Mode::PARTY_VS_PARTY || m_mode == Mode::GUILD_VS_GUILD)
	{
		if ((aliveOwner == 0 && aliveChallenger > 0) || (aliveChallenger == 0 && aliveOwner > 0))
			teamEliminated = true;
	}

	if (aliveCount <= 1 || teamEliminated)
	{
		// Stop the round timer right away when the fight is effectively over
		if (m_roundUiActive)
			StopRoundTimerUI();
		// Also ensure countdown UI is cleared for all present participants/spectators
		{
			unsigned int worldId = m_currentWorldId ? m_currentWorldId : GetAnyParticipantWorldId();
			if (worldId)
				BroadcastCountdownToWorld(worldId, false);
		}

		// In CC Rank-like mode with multiple rounds, mirror RankBattle round flow
		if (m_cfg.ccBattleMode && m_cfg.rankUiEnabled)
		{
			BYTE battleCount = 1; DWORD stageFinishMs = 3000, matchFinishMs = 4000;
			if (CRankBattleTable* pRankBattleTable = g_pTableContainer->GetRankBattleTable())
			{
				CTable* pBase = reinterpret_cast<CTable*>(pRankBattleTable);
				for (CTable::TABLEIT it = pBase->Begin(); it != pBase->End(); ++it)
				{
					sRANKBATTLE_TBLDAT* rb = (sRANKBATTLE_TBLDAT*)it->second;
					if (rb && rb->worldTblidx == (TBLIDX)m_currentWorldTblidx)
					{
						battleCount = rb->byBattleCount;
						stageFinishMs = rb->dwStageFinishTime * 1000;
						matchFinishMs = rb->dwMatchFinishTime * 1000;
						break;
					}
				}
			}

			if (m_cfg.roundsCount > 0) battleCount = (BYTE)m_cfg.roundsCount;
			bool hasNextRound = (m_rankBattleStage + 1) < battleCount;
			if (hasNextRound)
			{
				UpdateRankBattleState(RANKBATTLE_BATTLESTATE_STAGE_FINISH, m_rankBattleStage, stageFinishMs);
				return; // defer further handling to state machine
			}
			// Final round: announce winner (if any) and finish match UX then cleanup
			if (aliveCount == 1 && lastAliveCharId)
			{
				if (CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)lastAliveCharId))
				{
					wchar_t msg[256];
					ComposeWinnerText(p, msg, _countof(msg));
					SendNotice(msg, SERVER_TEXT_SYSNOTICE);
				}
			}
			// Minimal finalization on arena world: stop timer and finish immediately
			if (IsArenaWorldTblidx(m_currentWorldTblidx))
			{
				StopRoundTimerUI();
				FinishMatch(false);
				return;
			}
			// Avoid re-entering MATCH_FINISH if already there on non-arena worlds
			if (m_rankBattleState != RANKBATTLE_BATTLESTATE_MATCH_FINISH)
				UpdateRankBattleState(RANKBATTLE_BATTLESTATE_MATCH_FINISH, m_rankBattleStage, matchFinishMs);
			return;
		}

		// Non-CC mode or rank UI disabled: end immediately
		ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA][Alive] aliveCount=%u lastAlive=%u ccMode=%d rankUi=%d stage=%u state=%d", aliveCount, lastAliveCharId, m_cfg.ccBattleMode ? 1 : 0, m_cfg.rankUiEnabled ? 1 : 0, (unsigned)m_rankBattleStage, (int)m_rankBattleState);
		if (aliveCount == 1)
			FinishWithWinner(lastAliveCharId);
		else
			FinishMatch(false);
	}
}

void CArenaManager::FinishMatch(bool aborted)
{
	ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA] FinishMatch aborted=%d state=%u worldId=%u participants=%u spectators=%u", aborted ? 1 : 0, (unsigned)m_state, (unsigned)m_currentWorldId, (unsigned)m_participants.size(), (unsigned)m_spectators.size());
	// Allow finishing unless already in a terminal state; this covers cases where state changed just before finishing
	if ((m_state == State::COMPLETE || m_state == State::IDLE) && !aborted)
		return;
	// Cancel any pending respawn/protection timers to prevent revive/teleport race
	m_pendingReviveMs.clear();
	m_reviveProtectRemainMs.clear();
	// Always clear the round/countdown UI on finish (keep Rank HUD otherwise intact)
	StopRoundTimerUI();
	// Cancel any outstanding watchdog once we are finishing
	m_matchFinishWatchdogMs = 0;
	// During the fight we mirror Rank UX if enabled; at finish we ensure clients exit Rank mode.
	// On non-arena (rank-rule) worlds, mirror finish and then optionally send LEAVE.
	if (!IsArenaWorldTblidx(m_currentWorldTblidx))
	{
		// Non-arena worlds: mirror RankBattle finish UX unless suppressed
		if (m_cfg.rankUiEnabled && !m_cfg.suppressRankFinishUi)
		{
			unsigned int wid = m_currentWorldId ? m_currentWorldId : EnsureCurrentWorldId();
			if (wid)
				BroadcastRankStateToWorld(wid, RANKBATTLE_BATTLESTATE_MATCH_FINISH, 1);
			// If we are not keeping the Rank UI after finish, clear it now; otherwise, we'll send LEAVE just before teleport
			if (m_currentWorldId && !m_cfg.keepRankUiAfterFinish)
				BroadcastRankLeaveToWorld(m_currentWorldId);
		}
	}
	else
	{
		// Arena world: we used Rank style during match; now explicitly clear Rank HUD
		if (m_cfg.rankPacketsEnabled && m_currentWorldId && !m_cfg.keepRankUiAfterFinish)
		{
			BroadcastRankLeaveToWorld(m_currentWorldId);
		}
	}
	// Always clear combat permissions before any teleports/cleanup
	SetCombatPermittedForParticipants(false);
	// Also clear any leftover combat-restricting conditions to avoid next-arena issues
	ClearCombatRestrictionsForParticipants();
	// Revert any world-wide PvP toggles made during RUN so subsequent arenas start clean
	RevertWorldWidePvp();
	// Revert any temporary world rule overrides applied during the match
	RevertWorldRuleOverrides();

	// Determine and announce winner if not aborted
	if (!aborted && m_winners.size() > 0)
	{
		// On arena worlds, skip notices to keep UI calm; otherwise announce
		if (!IsArenaWorldTblidx(m_currentWorldTblidx))
		{
			for (auto cid : m_winners)
			{
				if (CPlayer* winner = g_pObjectManager->FindByChar((CHARACTERID)cid))
				{
					wchar_t msg[256];
					ComposeWinnerText(winner, msg, _countof(msg));
					SendNotice(msg, SERVER_TEXT_EMERGENCY);
					break; // Only announce first winner to avoid spam
				}
			}
		}
	}
	else if (!aborted)
	{
		// No explicit winner, but check for last alive participant
		unsigned int lastAlive = 0;
		for (auto cid : m_participants)
		{
			if (CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid))
			{
				if (p->IsInitialized() && !p->IsFainting())
				{
					lastAlive = cid;
					break;
				}
			}
		}
		if (lastAlive)
		{
			if (CPlayer* winner = g_pObjectManager->FindByChar((CHARACTERID)lastAlive))
			{
				// On arena worlds, avoid notices; still record winner for rewards
				if (!IsArenaWorldTblidx(m_currentWorldTblidx))
				{
					wchar_t msg[256];
					ComposeWinnerText(winner, msg, _countof(msg));
					SendNotice(msg, SERVER_TEXT_EMERGENCY);
				}
				m_winners.insert(lastAlive);
			}
		}
	}

	// Award rewards only on normal finish (not on abort)
	if (!aborted && m_cfg.rewardsEnabled)
	{
		AwardRewards(true);  // Winner rewards
		AwardRewards(false); // Participant rewards
		AwardMudosaPoints(); // Mudosa points per winners/participants
		// Notify players that rewards were granted
		for (auto cid : m_participants)
		{
			if (CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid))
			{
				SendSystemTo(p, L"[Arena] Rewards granted.", SERVER_TEXT_EMERGENCY);
			}
		}

		// Arena-world: announce winner to everyone and remind to check inventory, just before teleport
		if (IsArenaWorldByTblidxName(m_currentWorldTblidx))
		{
			wchar_t msg[320];
			bool announced = false;
			if (!m_winners.empty())
			{
				// Use first winner in the set for announcement text
				CHARACTERID cid = (CHARACTERID)(*m_winners.begin());
				if (CPlayer* pWin = g_pObjectManager->FindByChar(cid))
				{
					wchar_t wtxt[256];
					ComposeWinnerText(pWin, wtxt, _countof(wtxt));
					swprintf_s(msg, _countof(msg), L"%s — Check your inventory for rewards.", wtxt);
					BroadcastSystem(msg, SERVER_TEXT_SYSNOTICE);
					announced = true;
				}
			}
			if (!announced)
			{
				BroadcastSystem(L"[Arena] Finished — Check your inventory for rewards.", SERVER_TEXT_SYSNOTICE);
			}
		}
	}

	// Skip scoreboard and final notices on arena worlds to avoid UI toggles before teleport
	if (!IsArenaWorldByTblidxName(m_currentWorldTblidx))
	{
		if (m_currentWorldId)
			BroadcastScoreboardToWorld(m_currentWorldId);
		SendNotice(aborted ? L"Arena stopped." : L"Arena finished.", SERVER_TEXT_SYSNOTICE);
	}

	// If a delay is configured, announce and defer the teleport/cleanup to TickProcess
	unsigned int delayMs = m_cfg.postFinishTeleportDelayMs;
	if (delayMs > 0)
	{
		wchar_t buf[160];
		double s = (double)delayMs / 1000.0;
		swprintf_s(buf, L"[Arena] %s You will be teleported shortly (%.1fs).", aborted ? L"Stopped." : L"Finished.", s);
		BroadcastSystem(buf);
		m_postFinishTeleportRemainMs = delayMs;
		m_state = State::COMPLETE; // enter terminal state; Tick will handle the actual teleport/cleanup when timer expires
		ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA] FinishMatch deferring teleport by %u ms", delayMs);
		return;
	}

	// No delay: perform immediate post-finish teleport and cleanup
	// Despawn arena mobs
	DespawnArenaMobs();
	// Post-finish teleport everyone out regardless of aborted flag (user expectation for @arena stop)
	if (m_cfg.postFinishTeleport)
		PostFinishTeleportAll();
	else
		PostFinishTeleportDefault();

	// Let players know Rank mode was cleared explicitly
	BroadcastSystem(L"[Arena] Rank mode cleared. You can use normal Rank features again.");
	// Clear internal lists for a clean next start
	m_participants.clear();
	m_spectators.clear();
	m_winners.clear();
	m_killPoints.clear();
	// Reset rank state bookkeeping
	m_rankBattleState = INVALID_RANKBATTLE_BATTLESTATE;
	m_rankBattleStage = 0;
	m_rankStateTimeMs = 0;
	m_state = aborted ? State::IDLE : State::COMPLETE;
	ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA] FinishMatch done. New state=%u", (unsigned)m_state);
}

// GM control: start arena enrollment in specified mode
void CArenaManager::Start(Mode mode)
{
	if (!m_cfg.enabled)
	{
		ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA] Start requested but Arena is disabled in config");
		return;
	}

	// Channel gating: Only allow starting Arena on channels whose name contains "ARENA" and which are NOT the Dojo channel
	{
		CGameServer* app = (CGameServer*)g_pApp;
		bool isDojo = app && app->IsDojoChannel();
		bool nameOk = true; // default permissive, becomes strict if name available
		if (app)
		{
			std::string want = "arena";
			std::string got = app->m_config.ChannelName.c_str();
			for (auto& c : got) c = (char)tolower(c);
			nameOk = (got.find(want) != std::string::npos);
		}

		if ((m_cfg.onlyOnArenaChannel && !nameOk) || isDojo)
		{
			NTL_PRINT(PRINT_APP, _T("[ARENA][BLOCK] Start denied: channel='%S' dojo=%d (require name contains 'ARENA' and not Dojo)"), app ? app->m_config.ChannelName.c_str() : "<unknown>", (int)isDojo);
			BroadcastSystem(L"[Arena] Cannot start here. Use an ARENA channel (not Dojo).", SERVER_TEXT_SYSTEM);
			return;
		}
	}

	// Reset state to a clean enrollment phase
	StopRoundTimerUI();
	// Safety resets from any previous run to prevent stuck PvP/attackability blocking next arena
	SetCombatPermittedForParticipants(false);
	ClearCombatRestrictionsForParticipants();
	RevertWorldWidePvp();
	m_mode = mode;
	m_state = State::ENROLLMENT;
	m_inviting = false;
	m_inviteRemainMs = 0;
	m_pendingStartMs = 0;
	m_pendingStartWorldId = 0;
	m_waitAllArriveMs = 0;
	m_readyParticipants.clear();
	m_readyDelayMs.clear();
	m_pendingStartParticipants.clear();
	m_runSettleMs = 0;
	m_runUnlockPulseMs = 0;
	m_matchFinishWatchdogMs = 0;
	m_rankBattleState = INVALID_RANKBATTLE_BATTLESTATE;
	m_rankBattleStage = 0;
	m_rankStateTimeMs = 0;

	// Optional random map selection at start
	if (m_cfg.randomizeMapOnStart && !m_cfg.worldTblidxList.empty())
	{
		m_worldIndex = (size_t)(RandomRange(0, (int)m_cfg.worldTblidxList.size() - 1));
		m_currentWorldTblidx = m_cfg.worldTblidxList[m_worldIndex];
		m_currentWorldId = 0; // force fresh arena instance
	}

	// Reset rotation timer
	m_rotationRemainMs = ToMs(m_cfg.rotationSeconds);
	m_nextRotationAnnounceSec = (unsigned int)(m_rotationRemainMs / 1000);

	BroadcastSystem(L"[Arena] Enrollment opened. Use @arenajoin to participate.");
	SendNotice(L"Arena is open. Join now!", SERVER_TEXT_SYSNOTICE);
	NTL_PRINT(PRINT_APP, _T("[ARENA] Started: mode=%u worldTblidx=%u"), (unsigned)m_mode, (unsigned)m_currentWorldTblidx);
}

// GM control: stop arena (abort optional). Always teleports out and cleans up.
void CArenaManager::Stop(bool abort)
{
	// If nothing is running, just reset to IDLE and clear containers
	if (m_state == State::IDLE)
	{
		m_participants.clear();
		m_spectators.clear();
		m_winners.clear();
		m_killPoints.clear();
		m_rankBattleState = INVALID_RANKBATTLE_BATTLESTATE;
		m_rankBattleStage = 0;
		m_rankStateTimeMs = 0;
		m_matchFinishWatchdogMs = 0;
		return;
	}

	FinishMatch(abort);
}

// Enable/disable combat permissions for all participants
void CArenaManager::SetCombatPermittedForParticipants(bool enable)
{
	for (auto cid : m_participants)
	{
		if (CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid))
		{
			SetCombatPermittedFor(p, enable);
		}
	}
}

// Enable/disable combat permission on a single player (PvP zone + FreeBattle flag)
void CArenaManager::SetCombatPermittedFor(CPlayer* pPlayer, bool enable)
{
	if (!pPlayer)
		return;

	// PvP zone allows general PvP checks; FreeBattle flag bypasses forbid PC battle in static zones
	// Use UpdatePvpZone to broadcast GU_WORLD_FREE_PVP_ZONE_{ENTERED,LEFT}_NFY so clients update relations/UI
	pPlayer->UpdatePvpZone(enable);

	if (enable)
	{
		// Clear common blocking conditions to ensure damage can apply
		ClearCombatRestrictionsFor(pPlayer);

		// CRITICAL: Set RankBattle data so IsAttackable() recognizes arena participants as attackable
		sRANK_BATTLE_DATA* pRankData = pPlayer->GetRankBattleData();
		if (pRankData)
		{
			pRankData->eState = RANKBATTLE_MEMBER_STATE_ATTACKABLE;
			// Set teams based on arena mode for friendly-fire rules
			switch (m_mode)
			{
			case Mode::PARTY_VS_PARTY:
				// Use party ID hash as team identifier to create two teams
				pRankData->eTeamType = (pPlayer->GetPartyID() != INVALID_PARTYID)
					? ((pPlayer->GetPartyID() % 2 == 0) ? RANKBATTLE_TEAM_OWNER : RANKBATTLE_TEAM_CHALLENGER)
					: RANKBATTLE_TEAM_OTHER;
				break;
			case Mode::GUILD_VS_GUILD:
				// Use guild ID hash as team identifier to create two teams
				pRankData->eTeamType = (pPlayer->GetGuildID() != 0)
					? ((pPlayer->GetGuildID() % 2 == 0) ? RANKBATTLE_TEAM_OWNER : RANKBATTLE_TEAM_CHALLENGER)
					: RANKBATTLE_TEAM_OTHER;
				break;
			case Mode::FREE_FOR_ALL:
			case Mode::OPEN:
			default:
				// For FFA, assign alternating teams so everyone can attack everyone
				// (Different teams = can attack each other in IsAttackable logic)
				pRankData->eTeamType = ((pPlayer->GetCharID() % 2 == 0) ? RANKBATTLE_TEAM_OWNER : RANKBATTLE_TEAM_CHALLENGER);
				break;
			}
		}
	}
	else
	{
		// Reset RankBattle data when disabling combat
		sRANK_BATTLE_DATA* pRankData = pPlayer->GetRankBattleData();
		if (pRankData)
		{
			pRankData->eState = RANKBATTLE_MEMBER_STATE_NONE;
			pRankData->eTeamType = RANKBATTLE_TEAM_NONE;
		}
	}

	// If enabling mid-round for a late joiner, also resend ATTACKABLE to avoid client-side lock
	if (enable && m_state == State::IN_ROUND)
	{
		// If world-wide PvP is enabled, include this late joiner in PvP zone tracking too
		if (m_cfg.worldWidePvpDuringRun)
		{
			m_worldWidePvpToggled.insert((unsigned int)pPlayer->GetCharID());
		}
		// Use Budokai player-state on Budokai worlds, RankBattle ATTACKABLE elsewhere
		CGameServer* app = (CGameServer*)g_pApp;
		CWorld* pWorld = app->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)pPlayer->GetWorldID());
		if (pWorld)
		{
			BYTE r = pWorld->GetTbldat()->byWorldRuleType;
			bool isBudokaiRule = (r == GAMERULE_MINORMATCH || r == GAMERULE_MAJORMATCH || r == GAMERULE_FINALMATCH);
			if (isBudokaiRule)
			{
				std::unordered_set<unsigned int> one{ pPlayer->GetCharID() };
				ArenaBroadcastBudokaiPlayerStateToWorld(pWorld, one, MATCH_MEMBER_STATE_NORMAL);
			}
			else
			{
				CNtlPacket pkt(sizeof(sGU_RANKBATTLE_BATTLE_PLAYER_STATE_NFY));
				sGU_RANKBATTLE_BATTLE_PLAYER_STATE_NFY* res = (sGU_RANKBATTLE_BATTLE_PLAYER_STATE_NFY*)pkt.GetPacketData();
				res->wOpCode = GU_RANKBATTLE_BATTLE_PLAYER_STATE_NFY;
				res->hPc = pPlayer->GetID();
				res->byPCState = RANKBATTLE_MEMBER_STATE_ATTACKABLE;
				pkt.SetPacketLen(sizeof(sGU_RANKBATTLE_BATTLE_PLAYER_STATE_NFY));
				pPlayer->SendPacket(&pkt);
			}
		}
	}
}

void CArenaManager::ClearCombatRestrictionsFor(CPlayer* pPlayer)
{
	if (!pPlayer) return;
	// Remove common combat-restricting conditions so clients can act
	pPlayer->GetStateManager()->RemoveConditionFlags(
		MAKE_BIT_FLAG(CHARCOND_ATTACK_DISALLOW) |
		MAKE_BIT_FLAG(CHARCOND_CANT_BE_TARGETTED) |
		MAKE_BIT_FLAG(CHARCOND_SKILL_INABILITY) |
		MAKE_BIT_FLAG(CHARCOND_BATTLE_INABILITY),
		true);
	// Also ensure character returns to a neutral standing state
	pPlayer->SendCharStateStanding();
}

void CArenaManager::ClearCombatRestrictionsForParticipants()
{
	for (auto cid : m_participants)
	{
		if (CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid))
		{
			ClearCombatRestrictionsFor(p);
		}
	}
}

// Mark the entire arena world as PvP zone for all players currently in that world.
// We track which characters we toggled to revert safely on finish.
void CArenaManager::ApplyWorldWidePvp(unsigned int worldId)
{
	if (worldId == 0)
		return;
	CGameServer* app = (CGameServer*)g_pApp;
	CWorld* pWorld = app->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)worldId);
	if (!pWorld)
		return;

	// Iterate all PCs in this world using the world's object list
	int nObjCount = pWorld->GetObjectList()->GetObjCount(OBJTYPE_PC);
	CPlayer* pExistObject = (CPlayer*)pWorld->GetObjectList()->GetFirst(OBJTYPE_PC);

	for (int i = 0; i < nObjCount; i++) // use for instead of while to avoid endless loop
	{
		if (pExistObject && pExistObject->IsInitialized())
		{
			// Redundant check since we're iterating the world list, but keep for safety
			if ((unsigned int)pExistObject->GetWorldID() == worldId)
			{
				// Notify clients that characters in this world are in a PvP zone during the arena run
				pExistObject->UpdatePvpZone(true);
				m_worldWidePvpToggled.insert((unsigned int)pExistObject->GetCharID());
			}
		}

		pExistObject = (CPlayer*)pWorld->GetObjectList()->GetNext(pExistObject->GetWorldObjectLinker());
	}
}

// Revert PvP zone flag for all characters we toggled when the match ends.
void CArenaManager::RevertWorldWidePvp()
{
	if (m_worldWidePvpToggled.empty())
		return;
	for (auto cid : m_worldWidePvpToggled)
	{
		if (CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid))
		{
			// Broadcast leave so clients revert their PvP zone state
			p->UpdatePvpZone(false);
		}
	}
	m_worldWidePvpToggled.clear();
}

void CArenaManager::FinishWithWinner(unsigned int winnerCharId)
{
	// Skip only if already terminal
	if (m_state == State::COMPLETE || m_state == State::IDLE)
		return;
	// Clear combat permissions first and revert world PVP so next arena starts clean
	SetCombatPermittedForParticipants(false);
	ClearCombatRestrictionsForParticipants();
	RevertWorldWidePvp();
	// Always clear the round/countdown UI on finish (keep Rank HUD otherwise intact)
	StopRoundTimerUI();
	// Revert any temporary world rule overrides applied during the match
	RevertWorldRuleOverrides();
	if (winnerCharId)
	{
		if (CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)winnerCharId))
		{
			MarkWinner(p);
			if (!IsArenaWorldByTblidxName(m_currentWorldTblidx))
			{
				wchar_t msg[256];
				ComposeWinnerText(p, msg, _countof(msg));
				SendNotice(msg, SERVER_TEXT_SYSNOTICE);
			}
		}
	}
	if (m_cfg.rewardsEnabled)
	{
		AwardRewards(true);
		AwardRewards(false);
		AwardMudosaPoints();
		// Arena-world: announce winner + reward hint to everyone before teleport
		if (IsArenaWorldByTblidxName(m_currentWorldTblidx))
		{
			wchar_t msg[320];
			if (winnerCharId)
			{
				if (CPlayer* pWin = g_pObjectManager->FindByChar((CHARACTERID)winnerCharId))
				{
					wchar_t wtxt[256];
					ComposeWinnerText(pWin, wtxt, _countof(wtxt));
					swprintf_s(msg, _countof(msg), L"%s — Check your inventory for rewards.", wtxt);
					BroadcastSystem(msg);
				}
			}
			else
			{
				BroadcastSystem(L"[Arena] Finished — Check your inventory for rewards.");
			}
		}
	}
	// Telecast at finish as well (skip on arena world for minimal UX)
	if (m_cfg.telecastEnabled && !IsArenaWorldByTblidxName(m_currentWorldTblidx))
		BroadcastTelecastToWorld(EnsureCurrentWorldId());
	// Do not send Rank finish/leave on arena worlds to keep UI until teleport
	if (!IsArenaWorldByTblidxName(m_currentWorldTblidx))
	{
		if (m_cfg.rankUiEnabled)
			BroadcastRankStateToWorld(EnsureCurrentWorldId(), 5, 1); // RANKBATTLE_BATTLESTATE_MATCH_FINISH
		if (m_cfg.rankUiEnabled && m_currentWorldId)
			BroadcastRankLeaveToWorld(m_currentWorldId);
	}
	// Despawn arena mobs on finish
	DespawnArenaMobs();
	// Post-finish teleport if configured
	if (m_cfg.postFinishTeleportDelayMs > 0)
	{
		wchar_t buf[160];
		double s = (double)m_cfg.postFinishTeleportDelayMs / 1000.0;
		swprintf_s(buf, L"[Arena] Finished. You will be teleported shortly (%.1fs).", s);
		BroadcastSystem(buf);
		m_postFinishTeleportRemainMs = m_cfg.postFinishTeleportDelayMs;
		m_state = State::COMPLETE;
		return;
	}
	if (m_cfg.postFinishTeleport)
		PostFinishTeleportAll();
	else
		PostFinishTeleportDefault();
	// Mark complete; UI may remain visible if configured
	m_state = State::COMPLETE;
}

void CArenaManager::BroadcastRoundTimerStartToWorld(unsigned int worldId, unsigned int seconds)
{
	CGameServer* app = (CGameServer*)g_pApp;
	CWorld* pWorld = app->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)worldId);
	if (!pWorld)
		return;

	CNtlPacket packet(sizeof(sGU_BATTLE_DUNGEON_LIMIT_TIME_START_NFY));
	sGU_BATTLE_DUNGEON_LIMIT_TIME_START_NFY* res = (sGU_BATTLE_DUNGEON_LIMIT_TIME_START_NFY*)packet.GetPacketData();
	res->wOpCode = GU_BATTLE_DUNGEON_LIMIT_TIME_START_NFY;
	res->dwLimitTime = seconds; // client expects seconds
	packet.SetPacketLen(sizeof(sGU_BATTLE_DUNGEON_LIMIT_TIME_START_NFY));
	for (auto cid : m_participants)
	{
		CPlayer* pRecv = g_pObjectManager->FindByChar((CHARACTERID)cid);
		if (!pRecv || !pRecv->IsInitialized() || (unsigned int)pRecv->GetWorldID() != worldId) continue;
		pRecv->SendPacket(&packet);
	}

	// Fallback generic countdown UI if client ignores dungeon timer in some maps
	BroadcastCountdownToWorld(worldId, true);
}

void CArenaManager::BroadcastRoundTimerEndToWorld(unsigned int worldId)
{
	CGameServer* app = (CGameServer*)g_pApp;
	CWorld* pWorld = app->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)worldId);
	if (!pWorld)
		return;

	CNtlPacket packet(sizeof(sGU_BATTLE_DUNGEON_LIMIT_TIME_END_NFY));
	sGU_BATTLE_DUNGEON_LIMIT_TIME_END_NFY* res = (sGU_BATTLE_DUNGEON_LIMIT_TIME_END_NFY*)packet.GetPacketData();
	res->wOpCode = GU_BATTLE_DUNGEON_LIMIT_TIME_END_NFY;
	packet.SetPacketLen(sizeof(sGU_BATTLE_DUNGEON_LIMIT_TIME_END_NFY));
	for (auto cid : m_participants)
	{
		CPlayer* pRecv = g_pObjectManager->FindByChar((CHARACTERID)cid);
		if (!pRecv || !pRecv->IsInitialized() || (unsigned int)pRecv->GetWorldID() != worldId) continue;
		pRecv->SendPacket(&packet);
	}

	// End fallback countdown as well
	BroadcastCountdownToWorld(worldId, false);
}

// Send GU_RANKBATTLE_MATCH_START_NFY so client mirrors rank battle start UX
void CArenaManager::BroadcastRankMatchStartToWorld(unsigned int worldId)
{
	if (worldId == 0) return;
	CGameServer* app = (CGameServer*)g_pApp;
	CWorld* pWorld = app->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)worldId);
	if (!pWorld) return;
	if (!pWorld->GetTbldat()) return;
	BYTE rule = pWorld->GetTbldat()->byWorldRuleType;
	// Treat our arena world as RANKBATTLE for Arena combat UI (use world pointer, no lookup)
	if (IsArenaWorld(pWorld)) rule = GAMERULE_RANKBATTLE;
	if (rule == GAMERULE_MINORMATCH || rule == GAMERULE_MAJORMATCH || rule == GAMERULE_FINALMATCH)
		return; // Skip RankBattle packets on Budokai-rule maps
	if (rule != GAMERULE_RANKBATTLE)
		return; // Only on rank-rule worlds
	CNtlPacket packet(sizeof(sGU_RANKBATTLE_MATCH_START_NFY));
	sGU_RANKBATTLE_MATCH_START_NFY* res = (sGU_RANKBATTLE_MATCH_START_NFY*)packet.GetPacketData();
	res->wOpCode = GU_RANKBATTLE_MATCH_START_NFY;
	res->wResultCode = GAME_SUCCESS;
	packet.SetPacketLen(sizeof(sGU_RANKBATTLE_MATCH_START_NFY));
	for (auto cid : m_participants)
	{
		CPlayer* pPlayer = g_pObjectManager->FindByChar((CHARACTERID)cid);
		if (!pPlayer || !pPlayer->IsInitialized() || (unsigned int)pPlayer->GetWorldID() != worldId) continue;
		pPlayer->SendPacket(&packet);
	}
}

// Mirror RankBattle: notify stage finished, used at RUN end when another round follows
void CArenaManager::BroadcastRankStageFinishToWorld(unsigned int worldId)
{
	if (worldId == 0) return;
	CGameServer* app = (CGameServer*)g_pApp;
	CWorld* pWorld = app->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)worldId);
	if (!pWorld) return;
	if (!pWorld->GetTbldat()) return;
	BYTE rule = pWorld->GetTbldat()->byWorldRuleType;
	// Treat arena world as RANKBATTLE for Arena combat UI
	if (IsArenaWorld(pWorld)) rule = GAMERULE_RANKBATTLE;
	if (rule == GAMERULE_MINORMATCH || rule == GAMERULE_MAJORMATCH || rule == GAMERULE_FINALMATCH)
		return; // Skip RankBattle packets on Budokai-rule maps
	if (rule != GAMERULE_RANKBATTLE)
		return; // Only on rank-rule worlds

	CNtlPacket packet(sizeof(sGU_RANKBATTLE_BATTLE_STAGE_FINISH_NFY));
	sGU_RANKBATTLE_BATTLE_STAGE_FINISH_NFY* res = (sGU_RANKBATTLE_BATTLE_STAGE_FINISH_NFY*)packet.GetPacketData();
	res->wOpCode = GU_RANKBATTLE_BATTLE_STAGE_FINISH_NFY;
	packet.SetPacketLen(sizeof(sGU_RANKBATTLE_BATTLE_STAGE_FINISH_NFY));
	for (auto cid : m_participants)
	{
		CPlayer* pPlayer = g_pObjectManager->FindByChar((CHARACTERID)cid);
		if (!pPlayer || !pPlayer->IsInitialized() || (unsigned int)pPlayer->GetWorldID() != worldId) continue;
		pPlayer->SendPacket(&packet);
	}
}

// Mirror RankBattle: notify match finished (used before final cleanup)
void CArenaManager::BroadcastRankMatchFinishToWorld(unsigned int worldId)
{
	if (worldId == 0) return;
	CGameServer* app = (CGameServer*)g_pApp;
	CWorld* pWorld = app->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)worldId);
	if (!pWorld) return;
	if (!pWorld->GetTbldat()) return;
	BYTE rule = pWorld->GetTbldat()->byWorldRuleType;
	if (IsArenaWorld(pWorld)) rule = GAMERULE_RANKBATTLE;
	if (rule == GAMERULE_MINORMATCH || rule == GAMERULE_MAJORMATCH || rule == GAMERULE_FINALMATCH)
		return;
	if (rule != GAMERULE_RANKBATTLE)
		return;

	CNtlPacket packet(sizeof(sGU_RANKBATTLE_BATTLE_MATCH_FINISH_NFY));
	sGU_RANKBATTLE_BATTLE_MATCH_FINISH_NFY* res = (sGU_RANKBATTLE_BATTLE_MATCH_FINISH_NFY*)packet.GetPacketData();
	res->wOpCode = GU_RANKBATTLE_BATTLE_MATCH_FINISH_NFY;
	packet.SetPacketLen(sizeof(sGU_RANKBATTLE_BATTLE_MATCH_FINISH_NFY));
	for (auto cid : m_participants)
	{
		CPlayer* pPlayer = g_pObjectManager->FindByChar((CHARACTERID)cid);
		if (!pPlayer || !pPlayer->IsInitialized() || (unsigned int)pPlayer->GetWorldID() != worldId) continue;
		pPlayer->SendPacket(&packet);
	}
}

// Notify clients that they "joined" a rank battle room for HUD consistency
void CArenaManager::BroadcastRankJoinToWorld(unsigned int worldId)
{
	if (worldId == 0) return;
	TBLIDX roomTblidx = FindRankBattleTblidxForWorld((TBLIDX)m_currentWorldTblidx);
	if (roomTblidx == INVALID_TBLIDX) return;
	// Ensure world is rank-rule
	if (CWorld* pWorld = ((CGameServer*)g_pApp)->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)worldId))
	{
		if (!pWorld->GetTbldat()) return;
		BYTE rule = pWorld->GetTbldat()->byWorldRuleType;
		if (IsArenaWorld(pWorld)) rule = GAMERULE_RANKBATTLE;
		if (rule == GAMERULE_MINORMATCH || rule == GAMERULE_MAJORMATCH || rule == GAMERULE_FINALMATCH)
			return;
		if (rule != GAMERULE_RANKBATTLE)
			return;
	}
	else return;
	for (auto cid : m_participants)
	{
		CPlayer* pPlayer = g_pObjectManager->FindByChar((CHARACTERID)cid);
		if (!pPlayer || !pPlayer->IsInitialized() || (unsigned int)pPlayer->GetWorldID() != worldId) continue;
		CNtlPacket packet(sizeof(sGU_RANKBATTLE_JOIN_NFY));
		sGU_RANKBATTLE_JOIN_NFY* res = (sGU_RANKBATTLE_JOIN_NFY*)packet.GetPacketData();
		res->wOpCode = GU_RANKBATTLE_JOIN_NFY;
		res->rankBattleTblidx = roomTblidx;
		packet.SetPacketLen(sizeof(sGU_RANKBATTLE_JOIN_NFY));
		pPlayer->SendPacket(&packet);
	}
}

// Notify clients they left the rank battle room, used on finish/abort
void CArenaManager::BroadcastRankLeaveToWorld(unsigned int worldId)
{
	if (!m_cfg.rankPacketsEnabled) return;
	if (worldId == 0) return;
	// Ensure world is rank-rule
	if (CWorld* pWorld = ((CGameServer*)g_pApp)->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)worldId))
	{
		if (!pWorld->GetTbldat()) return;
		BYTE rule = pWorld->GetTbldat()->byWorldRuleType;
		if (IsArenaWorld(pWorld)) rule = GAMERULE_RANKBATTLE;
		if (rule == GAMERULE_MINORMATCH || rule == GAMERULE_MAJORMATCH || rule == GAMERULE_FINALMATCH)
			return;
		if (rule != GAMERULE_RANKBATTLE)
			return;
	}
	else return;
	auto sendLeave = [&](CHARACTERID cid)
		{
			CPlayer* pPlayer = g_pObjectManager->FindByChar(cid);
			if (!pPlayer || !pPlayer->IsInitialized() || (unsigned int)pPlayer->GetWorldID() != worldId) return;
			CNtlPacket packet(sizeof(sGU_RANKBATTLE_LEAVE_NFY));
			sGU_RANKBATTLE_LEAVE_NFY* res = (sGU_RANKBATTLE_LEAVE_NFY*)packet.GetPacketData();
			res->wOpCode = GU_RANKBATTLE_LEAVE_NFY;
			packet.SetPacketLen(sizeof(sGU_RANKBATTLE_LEAVE_NFY));
			pPlayer->SendPacket(&packet);
		};
	for (auto cid : m_participants)
		sendLeave((CHARACTERID)cid);
	for (auto cid : m_spectators)
		sendLeave((CHARACTERID)cid);
}

// Budokai-style: broadcast current match-state to arena participants present in the world
void CArenaManager::BroadcastBudokaiMatchStateToWorld(unsigned int worldId, BYTE byMatchType, BYTE byState, BUDOKAITIME tmNextStepTime, BUDOKAITIME tmRemainTime)
{
	if (worldId == 0) return;
	CGameServer* app = (CGameServer*)g_pApp;
	CWorld* pWorld = app->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)worldId);
	if (!pWorld) return;

	CNtlPacket packet(sizeof(sGU_BUDOKAI_UPDATE_MATCH_STATE_NFY));
	sGU_BUDOKAI_UPDATE_MATCH_STATE_NFY* res = (sGU_BUDOKAI_UPDATE_MATCH_STATE_NFY*)packet.GetPacketData();
	res->wOpCode = GU_BUDOKAI_UPDATE_MATCH_STATE_NFY;
	res->byMatchType = byMatchType;
	res->sStateInfo.byState = byState;
	res->sStateInfo.tmNextStepTime = tmNextStepTime;
	res->sStateInfo.tmRemainTime = tmRemainTime;
	packet.SetPacketLen(sizeof(sGU_BUDOKAI_UPDATE_MATCH_STATE_NFY));
	for (auto cid : m_participants)
	{
		CPlayer* pRecv = g_pObjectManager->FindByChar((CHARACTERID)cid);
		if (!pRecv || !pRecv->IsInitialized() || (unsigned int)pRecv->GetWorldID() != worldId) continue;
		pRecv->SendPacket(&packet);
	}
}

// Budokai-style: lightweight progress message (used for entering/get ready/fight messages)
void CArenaManager::BroadcastBudokaiProgressMessageToWorld(unsigned int worldId, BYTE byMsgId)
{
	if (worldId == 0) return;
	CGameServer* app = (CGameServer*)g_pApp;
	CWorld* pWorld = app->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)worldId);
	if (!pWorld) return;

	CNtlPacket packet(sizeof(sGU_BUDOKAI_PROGRESS_MESSAGE_NFY));
	sGU_BUDOKAI_PROGRESS_MESSAGE_NFY* res = (sGU_BUDOKAI_PROGRESS_MESSAGE_NFY*)packet.GetPacketData();
	res->wOpCode = GU_BUDOKAI_PROGRESS_MESSAGE_NFY;
	res->byMsgId = byMsgId;
	packet.SetPacketLen(sizeof(sGU_BUDOKAI_PROGRESS_MESSAGE_NFY));
	for (auto cid : m_participants)
	{
		CPlayer* pRecv = g_pObjectManager->FindByChar((CHARACTERID)cid);
		if (!pRecv || !pRecv->IsInitialized() || (unsigned int)pRecv->GetWorldID() != worldId) continue;
		pRecv->SendPacket(&packet);
	}
}

// Provide RankBattle HUD with team/member info to render UI bars
void CArenaManager::BroadcastRankTeamInfoToWorld(unsigned int worldId)
{
	if (worldId == 0) return;
	CGameServer* app = (CGameServer*)g_pApp;
	CWorld* pWorld = app->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)worldId);
	if (!pWorld) return;
	if (!pWorld->GetTbldat()) return;
	// Only send on rank-rule worlds (skip Budokai and others)
	BYTE rule = pWorld->GetTbldat()->byWorldRuleType;
	if (IsArenaWorldTblidx(m_currentWorldTblidx)) rule = GAMERULE_RANKBATTLE;
	if (rule == GAMERULE_MINORMATCH || rule == GAMERULE_MAJORMATCH || rule == GAMERULE_FINALMATCH)
		return;
	if (rule != GAMERULE_RANKBATTLE)
		return;

	sRANKBATTLE_MATCH_MEMBER_INFO memberInfo[NTL_MAX_MEMBER_IN_PARTY * 2];
	BYTE byCount = 0;

	// Map arena participants into teams. Some CC rank maps are Party-vs-Party only.
	// Respect per-world battle mode from RankBattle table: party split for PvP party maps.
	std::vector<CPlayer*> players;
	players.reserve(m_participants.size());
	for (auto cid : m_participants)
	{
		CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid);
		if (p && p->IsInitialized() && (unsigned int)p->GetWorldID() == worldId)
			players.push_back(p);
	}
	// Determine team key per player
	bool partyModeWorld = IsPartyModeWorld(m_currentWorldTblidx);
	auto getTeamKey = [&](CPlayer* p)->unsigned int
		{
			if (m_mode == Mode::PARTY_VS_PARTY || partyModeWorld)
				return (unsigned int)p->GetPartyID();
			if (m_mode == Mode::GUILD_VS_GUILD)
				return (unsigned int)p->GetGuildID();
			return 0; // non-team
		};

	// Build two teams
	unsigned int ownerKey = 0, challengerKey = 0;
	if (m_mode == Mode::PARTY_VS_PARTY || partyModeWorld || m_mode == Mode::GUILD_VS_GUILD)
	{
		for (CPlayer* p : players)
		{
			unsigned int key = getTeamKey(p);
			if (key == 0) continue;
			if (ownerKey == 0) ownerKey = key;
			else if (key != ownerKey) { challengerKey = key; break; }
		}
	}

	size_t half = (players.size() + 1) / 2;
	for (size_t i = 0; i < players.size() && byCount < _countof(memberInfo); ++i)
	{
		CPlayer* p = players[i];
		memberInfo[byCount].hPc = p->GetID();
		memberInfo[byCount].byState = RANKBATTLE_MEMBER_STATE_NORMAL;
		BYTE team = RANKBATTLE_TEAM_OWNER;
		if (m_mode == Mode::PARTY_VS_PARTY || partyModeWorld || m_mode == Mode::GUILD_VS_GUILD)
		{
			unsigned int key = getTeamKey(p);
			team = (key == ownerKey ? RANKBATTLE_TEAM_OWNER : RANKBATTLE_TEAM_CHALLENGER);
		}
		else
		{
			team = (i < half ? RANKBATTLE_TEAM_OWNER : RANKBATTLE_TEAM_CHALLENGER);
		}
		memberInfo[byCount].byTeam = team;
		++byCount;
	}

	if (byCount < 2)
	{
		NTL_PRINT(PRINT_APP, _T("[ARENA] Skipping TeamInfo: byCount=%u (<2)"), (unsigned)byCount);
		return;
	}

	// Validate both teams have at least one member; otherwise fallback to half split
	bool hasOwner = false, hasChallenger = false;
	for (BYTE i = 0; i < byCount; ++i)
	{
		if (memberInfo[i].byTeam == RANKBATTLE_TEAM_OWNER) hasOwner = true;
		if (memberInfo[i].byTeam == RANKBATTLE_TEAM_CHALLENGER) hasChallenger = true;
	}
	if (!hasOwner || !hasChallenger)
	{
		// Fallback split: first half owner, rest challenger
		for (BYTE i = 0; i < byCount; ++i)
		{
			memberInfo[i].byTeam = (i < (BYTE)((byCount + 1) / 2) ? RANKBATTLE_TEAM_OWNER : RANKBATTLE_TEAM_CHALLENGER);
		}
		hasOwner = hasChallenger = (byCount >= 2);
	}
	if (!hasOwner || !hasChallenger)
	{
		// Avoid sending malformed TeamInfo that can crash client
		NTL_PRINT(PRINT_APP, _T("[ARENA] Skipping TeamInfo: teams not formed (byCount=%u)"), (unsigned)byCount);
		return;
	}

	sVARIABLE_DATA sData;
	sData.Init(512);
	WORD wMemberInfo = sData.Write(byCount * sizeof(sRANKBATTLE_MATCH_MEMBER_INFO), memberInfo);
	WORD wOwnerPartyName = INVALID_WORD;
	WORD wChallengerPartyName = INVALID_WORD;

	WORD wPacketSize = sData.GetPacketSize(sizeof(sGU_RANKBATTLE_BATTLE_TEAM_INFO_NFY));

	NTL_PRINT(PRINT_APP, _T("[ARENA] TeamInfo: byCount=%u wMemberInfo=%u wOwnerPartyName=%u wChallengerPartyName=%u wPacketSize=%u reserve=%u dataSize=%u total=%u"),
		(unsigned)byCount, (unsigned)wMemberInfo, (unsigned)wOwnerPartyName, (unsigned)wChallengerPartyName, (unsigned)wPacketSize,
		(unsigned)sData.GetReserveSize(), (unsigned)sData.GetDataSize(), (unsigned)sData.GetTotalSize());

	CNtlPacket packet(wPacketSize);
	sGU_RANKBATTLE_BATTLE_TEAM_INFO_NFY* res = (sGU_RANKBATTLE_BATTLE_TEAM_INFO_NFY*)packet.GetPacketData();
	res->wOpCode = GU_RANKBATTLE_BATTLE_TEAM_INFO_NFY;
	res->wStraightWinCount = 0;
	res->wStraightKOWinCount = 0;
	res->byCount = byCount;
	res->wMemberInfo = wMemberInfo;
	res->wOwnerPartyName = wOwnerPartyName;
	res->wChallengerPartyName = wChallengerPartyName;
	sData.CopyTo(&res->sData, sData.GetDataSize());
	// Send to participants in this world only (avoid leaking to spectators)
	for (auto cid : m_participants)
	{
		CPlayer* pRecv = g_pObjectManager->FindByChar((CHARACTERID)cid);
		if (!pRecv || !pRecv->IsInitialized() || (unsigned int)pRecv->GetWorldID() != worldId) continue;
		pRecv->SendPacket(&packet);
	}
}

// Mirror UpdatePlayersAttackable for arena participants present in given world
void CArenaManager::MakeParticipantsAttackable(unsigned int worldId)
{
	if (worldId == 0) return;
	CGameServer* app = (CGameServer*)g_pApp;
	CWorld* pWorld = app->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)worldId);
	if (!pWorld) return;

	// Determine world rule to send appropriate unlock signals
	BYTE rule = pWorld->GetTbldat() ? pWorld->GetTbldat()->byWorldRuleType : GAMERULE_NORMAL;
	bool isBudokaiRule = (rule == GAMERULE_MINORMATCH || rule == GAMERULE_MAJORMATCH || rule == GAMERULE_FINALMATCH);

	for (auto cid : m_participants)
	{
		CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid);
		if (!p || !p->IsInitialized()) continue;
		if ((unsigned int)p->GetWorldID() != worldId) continue;

		// Ensure client exits any locked state
		p->SendCharStateStanding();

		// Send world-rule-appropriate unlock signal
		if (isBudokaiRule)
		{
			// Budokai-rule worlds use GU_MATCH_* packets  
			std::unordered_set<unsigned int> one{ cid };
			ArenaBroadcastBudokaiPlayerStateToWorld(pWorld, one, MATCH_MEMBER_STATE_NORMAL);
		}
		else
		{
			// RankBattle-rule worlds: set RankBattleData state and broadcast
			if (sRANK_BATTLE_DATA* rd = p->GetRankBattleData())
			{
				rd->eState = RANKBATTLE_MEMBER_STATE_ATTACKABLE;
			}
			else
			{
				ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA][WARN] MakeParticipantsAttackable: Null RankBattleData char=%u", (unsigned)p->GetCharID());
			}
			std::unordered_set<unsigned int> one{ cid };
			ArenaBroadcastRankPlayerAttackableToWorld(pWorld, one);
		}
	}
	ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA] MakeParticipantsAttackable: sent unlock signals to %u participants on world %u (rule=%u)",
		(unsigned)m_participants.size(), worldId, (unsigned)rule);
}

void CArenaManager::EnsureParticipantsStanding(unsigned int worldId)
{
	if (worldId == 0) return;
	for (auto cid : m_participants)
	{
		CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid);
		if (!p || !p->IsInitialized()) continue;
		if ((unsigned int)p->GetWorldID() != worldId) continue;
		p->SendCharStateStanding();
	}
}

void CArenaManager::StartRoundTimerUI(unsigned int seconds)
{
	unsigned int worldId = 0;
	if (m_currentWorldTblidx != 0)
		worldId = m_currentWorldId ? m_currentWorldId : EnsureCurrentWorldId();
	else
		worldId = GetAnyParticipantWorldId();
	if (!worldId)
		return;

	m_roundUiActive = true;
	m_roundRemainMs = ToMs(seconds);
	m_roundWorldId = worldId;
	m_nextRoundAnnounceSec = (unsigned int)(m_roundRemainMs / 1000);
	BroadcastRoundTimerStartToWorld(m_roundWorldId, seconds);
	{
		wchar_t msg[128];
		swprintf_s(msg, _countof(msg), L"[Arena] Round timer started: %us.", seconds);
		BroadcastSystem(msg);
	}
}

void CArenaManager::StopRoundTimerUI()
{
	if (!m_roundUiActive)
		return;
	BroadcastRoundTimerEndToWorld(m_roundWorldId);
	// Ensure fallback countdown is stopped for all present participants
	{
		for (auto cid : m_participants)
		{
			CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid);
			if (!p || !p->IsInitialized() || (unsigned int)p->GetWorldID() != m_roundWorldId) continue;
			SendCountdownTo(p, false);
		}
	}
	BroadcastSystem(L"[Arena] Round timer stopped.");
	m_roundUiActive = false;
	m_roundRemainMs = 0;
	m_roundWorldId = 0;
	m_nextRoundAnnounceSec = 0;
}

bool CArenaManager::SetRoundTimeRemaining(unsigned int seconds, bool startIfNotActive /*= true*/)
{
	if (!m_cfg.enabled)
		return false;
	if (!m_roundUiActive && !startIfNotActive)
		return false;
	if (!m_roundUiActive && startIfNotActive)
	{
		StartRoundTimerUI(seconds);
		return true;
	}
	m_roundRemainMs = ToMs(seconds);
	m_nextRoundAnnounceSec = (unsigned int)(m_roundRemainMs / 1000);
	// Re-broadcast start so UI syncs remaining time
	BroadcastRoundTimerStartToWorld(m_roundWorldId, seconds);
	wchar_t msg[128];
	swprintf_s(msg, _countof(msg), L"[Arena] Round time set: %us left.", seconds);
	BroadcastSystem(msg);
	return true;
}

bool CArenaManager::AddRoundTimeSeconds(int deltaSeconds)
{
	if (!m_cfg.enabled || !m_roundUiActive)
		return false;
	long long remaining = (long long)(m_roundRemainMs / 1000);
	long long updated = remaining + deltaSeconds;
	if (updated < 0) updated = 0;
	m_roundRemainMs = ToMs((unsigned int)updated);
	m_nextRoundAnnounceSec = (unsigned int)updated;
	BroadcastRoundTimerStartToWorld(m_roundWorldId, (unsigned int)updated);
	wchar_t msg[128];
	swprintf_s(msg, _countof(msg), L"[Arena] Round time %s %us. Left: %us.", (deltaSeconds >= 0 ? L"extended by" : L"reduced by"), (unsigned)abs(deltaSeconds), (unsigned)updated);
	BroadcastSystem(msg);
	return true;
}

void CArenaManager::SetRotationSecondsRemaining(unsigned int seconds)
{
	m_rotationRemainMs = ToMs(seconds);
	m_nextRotationAnnounceSec = seconds;
	wchar_t msg[128];
	swprintf_s(msg, _countof(msg), L"[Arena] Map rotation in %us.", seconds);
	BroadcastSystem(msg);
}

unsigned int CArenaManager::GetAnyParticipantWorldId()
{
	for (auto cid : m_participants)
	{
		if (CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid))
		{
			if (p->IsInitialized() && p->GetCurWorld())
				return (unsigned int)p->GetWorldID();
		}
	}
	return 0;
}

bool CArenaManager::IsParticipant(CPlayer* pPlayer) const
{
	if (!pPlayer) return false;
	return m_participants.find(pPlayer->GetCharID()) != m_participants.end();
}

bool CArenaManager::IsParticipantId(unsigned int charId) const
{
	return m_participants.find(charId) != m_participants.end();
}

bool CArenaManager::IsBudokaiWorld(unsigned int worldTblidx) const
{
	// Configurable list; if empty, default to well-known Budokai worlds 30000 and 41000
	if (!m_cfg.budokaiWorldTblidxList.empty())
	{
		for (unsigned int w : m_cfg.budokaiWorldTblidxList)
			if (w == worldTblidx) return true;
		return false;
	}
	return (worldTblidx == 30000u || worldTblidx == 41000u);
}

void CArenaManager::ParseMobListCsv(const CNtlString& csv)
{
	m_cfg.mobTblidxList.clear();
	std::string s = csv.c_str();
	size_t pos = 0;
	while (pos != std::string::npos)
	{
		size_t comma = s.find(',', pos);
		std::string tok = s.substr(pos, comma == std::string::npos ? std::string::npos : (comma - pos));
		if (!tok.empty())
		{
			unsigned int v = (unsigned int)strtoul(tok.c_str(), nullptr, 10);
			if (v != 0)
				m_cfg.mobTblidxList.push_back(v);
		}
		if (comma == std::string::npos) break;
		pos = comma + 1;
		while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\t')) ++pos;
	}
}

unsigned int CArenaManager::EnsureCurrentWorldId()
{
	if (m_currentWorldTblidx == 0) return 0;
	ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA] EnsureCurrentWorldId enter: tblidx=%u currentId=%u allowCustom=%d ccMode=%d", (unsigned)m_currentWorldTblidx, (unsigned)m_currentWorldId, m_cfg.allowCustomWorlds ? 1 : 0, m_cfg.ccBattleMode ? 1 : 0);

	// If Budokai-rule worlds are not allowed, ensure the current world isn't one
	if (!m_cfg.allowBudokaiRuleWorlds)
	{
		sWORLD_TBLDAT* pWorldData = (sWORLD_TBLDAT*)g_pTableContainer->GetWorldTable()->FindData((TBLIDX)m_currentWorldTblidx);
		if (pWorldData)
		{
			BYTE rule = pWorldData->byWorldRuleType;
			if (rule == GAMERULE_MINORMATCH || rule == GAMERULE_MAJORMATCH || rule == GAMERULE_FINALMATCH)
			{
				// Attempt to pick the next non-Budokai world, if any
				if (!m_cfg.worldTblidxList.empty())
				{
					for (size_t guard = 0; guard < m_cfg.worldTblidxList.size(); ++guard)
					{
						m_worldIndex = (m_worldIndex + 1) % m_cfg.worldTblidxList.size();
						unsigned int cand = m_cfg.worldTblidxList[m_worldIndex];
						sWORLD_TBLDAT* pCand = (sWORLD_TBLDAT*)g_pTableContainer->GetWorldTable()->FindData((TBLIDX)cand);
						if (pCand)
						{
							BYTE r = pCand->byWorldRuleType;
							bool isBudo = (r == GAMERULE_MINORMATCH || r == GAMERULE_MAJORMATCH || r == GAMERULE_FINALMATCH);
							if (!isBudo)
							{
								m_currentWorldTblidx = cand;
								m_currentWorldId = 0; // force creation of the non-Budokai instance
								break;
							}
						}
					}
				}
				else
				{
					NTL_PRINT(PRINT_APP, _T("[ARENA] ERROR: Budokai-rule world selected and no alternative available"));
					return 0;
				}
			}
		}
	}

	// Simple guard: if a world was just created and cached, reuse it to avoid creation loops
	if (m_currentWorldId != 0)
	{
		CGameServer* appCheck = (CGameServer*)g_pApp;
		if (CWorld* w = appCheck->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)m_currentWorldId))
			return m_currentWorldId;
	}

	// If custom worlds are disabled (and not in only-custom mode), validate that current world is from RankBattle table
	if (!m_cfg.allowCustomWorlds && !m_cfg.useOnlyCustomWorlds)
	{
		bool isValidRankWorld = false;
		for (unsigned int rankWorldTblidx : m_cfg.worldTblidxList)
		{
			if (rankWorldTblidx == m_currentWorldTblidx)
			{
				isValidRankWorld = true;
				break;
			}
		}

		if (!isValidRankWorld)
		{
			NTL_PRINT(PRINT_APP, _T("[ARENA] Current world %u not in RankBattle table, switching to first available RankBattle world"), m_currentWorldTblidx);
			ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA] World override: %u not allowed, switching to RankBattle world", (unsigned)m_currentWorldTblidx);
			if (!m_cfg.worldTblidxList.empty())
			{
				m_currentWorldTblidx = m_cfg.worldTblidxList[0];
				m_currentWorldId = 0; // Force new world creation
				NTL_PRINT(PRINT_APP, _T("[ARENA] Switched to RankBattle world: tblidx=%u"), m_currentWorldTblidx);
			}
			else
			{
				NTL_PRINT(PRINT_APP, _T("[ARENA] ERROR: No RankBattle worlds available!"));
				return 0;
			}
		}
	}

	CGameServer* app = (CGameServer*)g_pApp;

	// In CC Battle mode, create fresh instances per match (but reuse within same match)
	if (m_cfg.ccBattleMode)
	{
		// If we already have a valid world for this match, reuse it
		if (m_currentWorldId != 0)
		{
			CWorld* pExistingWorld = app->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)m_currentWorldId);
			if (pExistingWorld)
			{
				return m_currentWorldId; // Reuse existing world for this match
			}
		}

		// Create a new world instance for CC battles
		sWORLD_TBLDAT* pWorldTbldat = (sWORLD_TBLDAT*)g_pTableContainer->GetWorldTable()->FindData((TBLIDX)m_currentWorldTblidx);
		if (!pWorldTbldat)
		{
			NTL_PRINT(PRINT_APP, _T("[ARENA] ERROR: World tblidx %u not found in World Table!"), m_currentWorldTblidx);
			return 0;
		}

		// If the target world is static (non-dynamic), it is created at server startup and cannot be created via CreateWorld.
		// Reuse the existing static instance by its worldID (which equals tblidx for static worlds).
		if (!pWorldTbldat->bDynamic)
		{
			CWorld* pExistingStatic = app->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)pWorldTbldat->tblidx);
			if (pExistingStatic)
			{
				if (IsArenaWorld(pExistingStatic) && ShouldOverrideRuleForWorld(m_currentWorldTblidx))
				{
					pExistingStatic->SetRuleOverride(GAMERULE_RANKBATTLE);
					m_worldsWithOverride.insert((unsigned int)pExistingStatic->GetID());
				}
				m_currentWorldId = (unsigned int)pExistingStatic->GetID();
				NTL_PRINT(PRINT_APP, _T("[ARENA] CC Mode: Reusing static world instance ID %u for tblidx %u"), m_currentWorldId, m_currentWorldTblidx);
				ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA] Reused static world: id=%u tblidx=%u", (unsigned)m_currentWorldId, (unsigned)m_currentWorldTblidx);
				return m_currentWorldId;
			}
			else
			{
				NTL_PRINT(PRINT_APP, _T("[ARENA] CC Mode: Static world tblidx %u not found; cannot dynamically create static worlds"), m_currentWorldTblidx);
				ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA] Static world missing in manager: tblidx=%u", (unsigned)m_currentWorldTblidx);
				return 0;
			}
		}

		NTL_PRINT(PRINT_APP, _T("[ARENA] Attempting to create dynamic world: tblidx=%u name='%s'"),
			m_currentWorldTblidx, pWorldTbldat->wszName);
		ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA] CreateWorld CC mode: tblidx=%u", (unsigned)m_currentWorldTblidx);

		CWorld* pWorld = app->GetGameMain()->GetWorldManager()->CreateWorld(pWorldTbldat);
		if (pWorld)
		{
			// Treat Arena worlds as RankBattle at runtime to avoid dungeon revive and lockouts
			if (IsArenaWorld(pWorld) && ShouldOverrideRuleForWorld(m_currentWorldTblidx))
			{
				pWorld->SetRuleOverride(GAMERULE_RANKBATTLE);
				m_worldsWithOverride.insert((unsigned int)pWorld->GetID());
			}
			m_currentWorldId = (unsigned int)pWorld->GetID();
			NTL_PRINT(PRINT_APP, _T("[ARENA] CC Mode: Created fresh world instance ID %u for tblidx %u"),
				m_currentWorldId, m_currentWorldTblidx);
			ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA] Created world CC mode: id=%u tblidx=%u", (unsigned)m_currentWorldId, (unsigned)m_currentWorldTblidx);
		}
		else
		{
			m_currentWorldId = 0;
			NTL_PRINT(PRINT_APP, _T("[ARENA] CC Mode: Failed to create world for tblidx %u - CreateWorld returned NULL"), m_currentWorldTblidx);
			ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA] CreateWorld failed CC mode: tblidx=%u. Trying fallbacks...", (unsigned)m_currentWorldTblidx);
			NTL_PRINT(PRINT_APP, _T("[ARENA] World data: name='%s' mapName='%s'"),
				pWorldTbldat->wszName, pWorldTbldat->szName);

			// If strict world is requested, do not attempt fallbacks; propagate failure to caller
			if (m_cfg.forceExactWorld)
			{
				return 0;
			}

			// Otherwise, try fallback worlds
			std::vector<unsigned int> fallbackWorlds = { 1, 4, 6, 11 }; // Common DBO world IDs
			for (unsigned int fallbackTblidx : fallbackWorlds)
			{
				if (fallbackTblidx == m_currentWorldTblidx) continue; // Skip the one that already failed

				sWORLD_TBLDAT* pFallbackTbldat = (sWORLD_TBLDAT*)g_pTableContainer->GetWorldTable()->FindData((TBLIDX)fallbackTblidx);
				if (pFallbackTbldat)
				{
					CWorld* pFallbackWorld = app->GetGameMain()->GetWorldManager()->CreateWorld(pFallbackTbldat);
					if (pFallbackWorld)
					{
						if (IsArenaWorld(pFallbackWorld) && ShouldOverrideRuleForWorld(fallbackTblidx))
						{
							pFallbackWorld->SetRuleOverride(GAMERULE_RANKBATTLE);
							m_worldsWithOverride.insert((unsigned int)pFallbackWorld->GetID());
						}
						m_currentWorldId = (unsigned int)pFallbackWorld->GetID();
						m_currentWorldTblidx = fallbackTblidx; // Update the current tblidx to the working one
						NTL_PRINT(PRINT_APP, _T("[ARENA] Using fallback world: tblidx=%u name='%s' worldId=%u"),
							fallbackTblidx, pFallbackTbldat->wszName, m_currentWorldId);
						ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA] Fallback world selected: tblidx=%u worldId=%u", (unsigned)fallbackTblidx, (unsigned)m_currentWorldId);
						break;
					}
				}
			}
		}
		return m_currentWorldId;
	}

	// Normal mode: reuse existing if valid, otherwise create
	if (m_currentWorldId != 0)
	{
		CWorld* pWorldExisting = app->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)m_currentWorldId);
		if (pWorldExisting)
		{
			if (IsArenaWorld(pWorldExisting) && ShouldOverrideRuleForWorld(m_currentWorldTblidx))
			{
				pWorldExisting->SetRuleOverride(GAMERULE_RANKBATTLE);
				m_worldsWithOverride.insert((unsigned int)pWorldExisting->GetID());
			}
			return m_currentWorldId;
		}
		// fallthrough to recreate
	}

	sWORLD_TBLDAT* pWorldTbldat = (sWORLD_TBLDAT*)g_pTableContainer->GetWorldTable()->FindData((TBLIDX)m_currentWorldTblidx);
	if (!pWorldTbldat)
	{
		NTL_PRINT(PRINT_APP, _T("[ARENA] ERROR: World tblidx %u not found in World Table!"), m_currentWorldTblidx);
		return 0;
	}

	// If world is static (non-dynamic), reuse it directly rather than calling CreateWorld (which returns NULL for static worlds)
	if (!pWorldTbldat->bDynamic)
	{
		CWorld* pStatic = app->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)pWorldTbldat->tblidx);
		if (pStatic)
		{
			if (IsArenaWorld(pStatic) && ShouldOverrideRuleForWorld(m_currentWorldTblidx))
			{
				pStatic->SetRuleOverride(GAMERULE_RANKBATTLE);
				m_worldsWithOverride.insert((unsigned int)pStatic->GetID());
			}
			m_currentWorldId = (unsigned int)pStatic->GetID();
			return m_currentWorldId;
		}
		else
		{
			NTL_PRINT(PRINT_APP, _T("[ARENA] Normal Mode: Static world tblidx %u not found in manager; cannot create statics"), m_currentWorldTblidx);
			return 0;
		}
	}

	CWorld* pWorld = app->GetGameMain()->GetWorldManager()->CreateWorld(pWorldTbldat);
	if (pWorld)
	{
		if (IsArenaWorld(pWorld) && ShouldOverrideRuleForWorld(m_currentWorldTblidx))
		{
			pWorld->SetRuleOverride(GAMERULE_RANKBATTLE);
			m_worldsWithOverride.insert((unsigned int)pWorld->GetID());
		}
		m_currentWorldId = (unsigned int)pWorld->GetID();
	}
	else
	{
		// Try fallback worlds if configured world fails (unless forced exact world is required)
		m_currentWorldId = 0;
		NTL_PRINT(PRINT_APP, _T("[ARENA] Normal Mode: Failed to create world for tblidx %u"), m_currentWorldTblidx);
		if (m_cfg.forceExactWorld)
		{
			return 0;
		}

		std::vector<unsigned int> fallbackWorlds = { 1, 4, 6, 11 }; // Common DBO world IDs
		for (unsigned int fallbackTblidx : fallbackWorlds)
		{
			if (fallbackTblidx == m_currentWorldTblidx) continue; // Skip the one that already failed

			sWORLD_TBLDAT* pFallbackTbldat = (sWORLD_TBLDAT*)g_pTableContainer->GetWorldTable()->FindData((TBLIDX)fallbackTblidx);
			if (pFallbackTbldat)
			{
				CWorld* pFallbackWorld = app->GetGameMain()->GetWorldManager()->CreateWorld(pFallbackTbldat);
				if (pFallbackWorld)
				{
					if (IsArenaWorld(pFallbackWorld) && ShouldOverrideRuleForWorld(fallbackTblidx))
					{
						pFallbackWorld->SetRuleOverride(GAMERULE_RANKBATTLE);
						m_worldsWithOverride.insert((unsigned int)pFallbackWorld->GetID());
					}
					m_currentWorldId = (unsigned int)pFallbackWorld->GetID();
					m_currentWorldTblidx = fallbackTblidx; // Update the current tblidx to the working one
					NTL_PRINT(PRINT_APP, _T("[ARENA] Normal Mode: Using fallback world: tblidx=%u name='%s' worldId=%u"),
						fallbackTblidx, pFallbackTbldat->wszName, m_currentWorldId);
					break;
				}
			}
		}
	}
	return m_currentWorldId;
}

unsigned CArenaManager::CountParticipantsInWorld(unsigned int worldId)
{
	unsigned count = 0;
	for (auto cid : m_participants)
	{
		if (CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid))
		{
			if (p->IsInitialized() && (unsigned int)p->GetWorldID() == worldId)
				++count;
		}
	}
	return count;
}

unsigned CArenaManager::CountOnlineParticipants()
{
	unsigned count = 0;
	for (auto cid : m_participants)
	{
		if (CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid))
		{
			if (p->IsInitialized())
				++count;
		}
	}
	return count;
}

void CArenaManager::BroadcastCountdownToWorld(unsigned int worldId, bool bStart)
{
	CGameServer* app = (CGameServer*)g_pApp;
	CWorld* pWorld = app->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)worldId);
	if (!pWorld)
		return;

	CNtlPacket packet(sizeof(sGU_TIMEQUEST_COUNTDOWN_NFY));
	sGU_TIMEQUEST_COUNTDOWN_NFY* res = (sGU_TIMEQUEST_COUNTDOWN_NFY*)packet.GetPacketData();
	res->wOpCode = GU_TIMEQUEST_COUNTDOWN_NFY;
	res->bCountDown = bStart;
	packet.SetPacketLen(sizeof(sGU_TIMEQUEST_COUNTDOWN_NFY));
	for (auto cid : m_participants)
	{
		CPlayer* pRecv = g_pObjectManager->FindByChar((CHARACTERID)cid);
		if (!pRecv || !pRecv->IsInitialized() || (unsigned int)pRecv->GetWorldID() != worldId) continue;
		pRecv->SendPacket(&packet);
	}
}

void CArenaManager::BroadcastDungeonStateToWorld(unsigned int worldId, unsigned char byStage, unsigned int titleTblidx, unsigned int subTitleTblidx)
{
	if (!worldId)
		return;
	CGameServer* app = (CGameServer*)g_pApp;
	CWorld* pWorld = app->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)worldId);
	if (!pWorld)
		return;

	CNtlPacket packet(sizeof(sGU_BATTLE_DUNGEON_STATE_UPATE_NFY));
	sGU_BATTLE_DUNGEON_STATE_UPATE_NFY* res = (sGU_BATTLE_DUNGEON_STATE_UPATE_NFY*)packet.GetPacketData();
	res->wOpCode = GU_BATTLE_DUNGEON_STATE_UPATE_NFY;
	res->titleTblidx = titleTblidx;
	res->subTitleTblidx = subTitleTblidx;
	res->byStage = byStage;
	packet.SetPacketLen(sizeof(sGU_BATTLE_DUNGEON_STATE_UPATE_NFY));
	for (auto cid : m_participants)
	{
		CPlayer* pRecv = g_pObjectManager->FindByChar((CHARACTERID)cid);
		if (!pRecv || !pRecv->IsInitialized() || (unsigned int)pRecv->GetWorldID() != worldId) continue;
		pRecv->SendPacket(&packet);
	}
}

void CArenaManager::BroadcastRankStateToWorld(unsigned int worldId, unsigned char byState, unsigned char byStage)
{
	// Only send RankBattle packets on rank-rule worlds; skip Budokai-rule and other worlds
	if (!worldId)
		return;
	CGameServer* app = (CGameServer*)g_pApp;
	CWorld* pWorld = app->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)worldId);
	if (!pWorld)
		return;
	if (!pWorld->GetTbldat())
		return;
	BYTE rule = pWorld->GetTbldat()->byWorldRuleType;
	if (rule == GAMERULE_MINORMATCH || rule == GAMERULE_MAJORMATCH || rule == GAMERULE_FINALMATCH)
		return;
	if (rule != GAMERULE_RANKBATTLE)
		return;
	CNtlPacket packet(sizeof(sGU_RANKBATTLE_BATTLE_STATE_UPDATE_NFY));
	sGU_RANKBATTLE_BATTLE_STATE_UPDATE_NFY* res = (sGU_RANKBATTLE_BATTLE_STATE_UPDATE_NFY*)packet.GetPacketData();
	res->wOpCode = GU_RANKBATTLE_BATTLE_STATE_UPDATE_NFY;
	res->byBattleState = byState;
	res->byStage = (BYTE)byStage;
	packet.SetPacketLen(sizeof(sGU_RANKBATTLE_BATTLE_STATE_UPDATE_NFY));
	for (auto cid : m_participants)
	{
		CPlayer* pRecv = g_pObjectManager->FindByChar((CHARACTERID)cid);
		if (!pRecv || !pRecv->IsInitialized() || (unsigned int)pRecv->GetWorldID() != worldId) continue;
		pRecv->SendPacket(&packet);
	}
}

// Replicate RankBattle opening sequence to ensure client unlocks movement
void CArenaManager::BroadcastRankFullStartSequence(unsigned int worldId)
{
	if (!m_cfg.rankUiEnabled || worldId == 0) return;
	// allow only on rank-rule worlds
	if (CWorld* pWorld = ((CGameServer*)g_pApp)->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)worldId))
	{
		if (!pWorld->GetTbldat()) return;
		BYTE rule = pWorld->GetTbldat()->byWorldRuleType;
		if (rule == GAMERULE_MINORMATCH || rule == GAMERULE_MAJORMATCH || rule == GAMERULE_FINALMATCH)
			return;
		if (rule != GAMERULE_RANKBATTLE)
			return;
	}
	else return;
	// WAIT (0) with stage 0
	BroadcastRankStateToWorld(worldId, RANKBATTLE_BATTLESTATE_WAIT, m_rankBattleStage);
	// DIRECTION phase (shows VS)
	BroadcastRankStateToWorld(worldId, RANKBATTLE_BATTLESTATE_DIRECTION, m_rankBattleStage);
	// STAGE_PREPARE (gear up)
	BroadcastRankStateToWorld(worldId, RANKBATTLE_BATTLESTATE_STAGE_PREPARE, m_rankBattleStage);
	// STAGE_READY (round ready)
	BroadcastRankStateToWorld(worldId, RANKBATTLE_BATTLESTATE_STAGE_READY, m_rankBattleStage);
	// MATCH START notification
	BroadcastRankMatchStartToWorld(worldId);
	// Unlock attack/movement
	MakeParticipantsAttackable(worldId);
	// STAGE_RUN
	BroadcastRankStateToWorld(worldId, RANKBATTLE_BATTLESTATE_STAGE_RUN, m_rankBattleStage);
}

void CArenaManager::ReviveParticipantsForNextRound()
{
	// Revive all arena participants present in the arena world at next-round start
	unsigned int worldId = m_currentWorldId ? m_currentWorldId : EnsureCurrentWorldId();
	if (!worldId) return;
	for (auto cid : m_participants)
	{
		CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid);
		if (!p || !p->IsInitialized() || (unsigned int)p->GetWorldID() != worldId)
			continue;
		if (p->IsFainting())
		{
			// Revive in-place to keep flow snappy, like RankBattle between rounds
			CNtlVector loc = p->GetCurLoc();
			p->Revival(loc, p->GetWorldID(), REVIVAL_TYPE_RESCUED);
			p->UpdateCurLpEp(p->GetMaxLP(), p->GetMaxEP(), true, false);
			p->SendCharStateStanding();
			// Apply standard arena respawn buff
			ApplyRespawnBuff(p);
		}
	}
}

// Reset CC/buffs/targets/stats like RankBattle::ResetPlayers so new rounds start clean
void CArenaManager::ResetParticipantsBetweenRounds()
{
	unsigned int worldId = m_currentWorldId ? m_currentWorldId : EnsureCurrentWorldId();
	if (!worldId) return;
	for (auto cid : m_participants)
	{
		CPlayer* pPlayer = g_pObjectManager->FindByChar((CHARACTERID)cid);
		if (!pPlayer || !pPlayer->IsInitialized() || (unsigned int)pPlayer->GetWorldID() != worldId)
			continue;

		sRANK_BATTLE_DATA* pRankData = pPlayer->GetRankBattleData();

		// Stop attack and clear target
		pPlayer->ChangeAttackProgress(false);
		pPlayer->SetAttackTarget(INVALID_HOBJECT);

		// Send NORMAL first if not already attackable to mirror RankBattle reset semantics
		if (pRankData->eState != RANKBATTLE_MEMBER_STATE_ATTACKABLE)
		{
			CNtlPacket pkt(sizeof(sGU_RANKBATTLE_BATTLE_PLAYER_STATE_NFY));
			sGU_RANKBATTLE_BATTLE_PLAYER_STATE_NFY* res = (sGU_RANKBATTLE_BATTLE_PLAYER_STATE_NFY*)pkt.GetPacketData();
			res->wOpCode = GU_RANKBATTLE_BATTLE_PLAYER_STATE_NFY;
			res->hPc = pPlayer->GetID();
			res->byPCState = RANKBATTLE_MEMBER_STATE_NORMAL;
			pkt.SetPacketLen(sizeof(sGU_RANKBATTLE_BATTLE_PLAYER_STATE_NFY));
			pPlayer->SendPacket(&pkt);
		}

		// Remove buffs and cancel transformations
		pPlayer->GetBuffManager()->RemoveAllBuff();
		if (pPlayer->GetTransformationTbldat())
		{
			pPlayer->CancelTransformation();
		}

		// Clear conditions (stun, hold, etc.)
		if (pPlayer->GetStateManager()->GetConditionState() > 0)
		{
			pPlayer->GetStateManager()->RemoveConditionFlags(INVALID_QWORD, true);
		}

		// Recalculate stats and restore resources
		pPlayer->GetCharAtt()->CalculateAll();
		pPlayer->UpdateCurLP(pPlayer->GetMaxLP(), true, false);
		pPlayer->UpdateCurEP(pPlayer->GetMaxEP(), true, false);
		pPlayer->UpdateRpBall(pPlayer->GetMaxRPBall(), false, false);
		pPlayer->UpdateCurRP(pPlayer->GetCharAtt()->GetMaxRP(), false, false);

		// Clear target and CC reduction
		pPlayer->ChangeTarget(INVALID_HOBJECT);
		pPlayer->ClearCrowdControlReduction();

		// Prepare state for next round: DO NOT set NORMAL here, let the state machine handle it
		// NORMAL will be set in READY, then ATTACKABLE in RUN
		pPlayer->SendCharStateStanding();
	}
}

void CArenaManager::PostFinishTeleportDefault()
{
	// Proactively clear any HUD elements (round timer, countdown, rank HUD) before teleporting out
	// This avoids client-side leftovers like 'Please wait' or Rank button after returning
	if (m_currentWorldId)
	{
		for (auto cid : m_participants)
		{
			if (CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid))
			{
				if ((unsigned int)p->GetWorldID() == m_currentWorldId)
				{
					SendRoundTimerEndTo(p);
					// Defer LEAVE until now if UI was kept at finish
					if (m_cfg.rankPacketsEnabled && m_cfg.keepRankUiAfterFinish)
						SendRankLeaveTo(p);
					// Also ensure fallback countdown is stopped
					SendCountdownTo(p, false);
				}
			}
		}
		for (auto cid : m_spectators)
		{
			if (CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid))
			{
				if ((unsigned int)p->GetWorldID() == m_currentWorldId)
				{
					SendRoundTimerEndTo(p);
					if (m_cfg.rankPacketsEnabled && m_cfg.keepRankUiAfterFinish)
						SendRankLeaveTo(p);
					SendCountdownTo(p, false);
				}
			}
		}
	}
	// First, ensure all fainted participants in the arena world are standing to avoid FAINT carryover
	if (m_currentWorldId)
	{
		for (auto cid : m_participants)
		{
			if (CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid))
			{
				if ((unsigned int)p->GetWorldID() == m_currentWorldId && p->IsFainting())
				{
					// Restore resources and send standing so client clears FAINT UI
					p->UpdateCurLpEp(p->GetMaxLP(), p->GetMaxEP(), true, false);
					p->SendCharStateStanding();
				}
			}
		}
	}

	// send players back to their previous location if we have it; else to bind
	for (auto cid : m_participants)
	{
		if (CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid))
		{
			auto it = m_prevLoc.find(cid);
			if (it != m_prevLoc.end() && it->second.worldId != INVALID_WORLDID)
			{
				// Avoid teleporting back into the same arena world instance; go to bind instead
				if ((unsigned int)it->second.worldId == m_currentWorldId || IsArenaWorldTblidx(m_currentWorldTblidx))
				{
					TeleportToBind(p);
				}
				else
				{
					p->StartTeleport(it->second.loc, it->second.dir, (WORLDID)it->second.worldId, TELEPORT_TYPE_COMMAND);
				}
			}
			else
			{
				TeleportToBind(p);
			}
		}
		// Clear snapshot after use
		m_prevLoc.erase(cid);
	}
	for (auto cid : m_spectators)
	{
		if (CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid))
		{
			// ensure spectator hide is removed on exit
			if (m_cfg.spectatorHide)
				ApplySpectatorHide(p, false);
			auto it = m_prevLoc.find(cid);
			if (it != m_prevLoc.end() && it->second.worldId != INVALID_WORLDID)
			{
				if ((unsigned int)it->second.worldId == m_currentWorldId || IsArenaWorldTblidx(m_currentWorldTblidx))
					TeleportToBind(p);
				else
					p->StartTeleport(it->second.loc, it->second.dir, (WORLDID)it->second.worldId, TELEPORT_TYPE_COMMAND);
			}
			else
			{
				TeleportToBind(p);
			}
		}
		// Clear snapshot after use
		m_prevLoc.erase(cid);
	}
}

void CArenaManager::TeleportToBind(CPlayer* pPlayer)
{
	if (!pPlayer) return;
	CNtlVector vBindLoc(pPlayer->GetBindLoc());
	pPlayer->StartTeleport(vBindLoc, pPlayer->GetCurDir(), pPlayer->GetBindWorldID(), TELEPORT_TYPE_COMMAND);
}

void CArenaManager::SavePrevLocation(CPlayer* pPlayer)
{
	if (!pPlayer) return;
	PrevLoc s; s.worldId = (unsigned int)pPlayer->GetWorldID(); s.loc = pPlayer->GetCurLoc(); s.dir = pPlayer->GetCurDir();
	m_prevLoc[pPlayer->GetCharID()] = s;
}

void CArenaManager::FinishOnTimeout()
{
	// In CC battle mode with rank-like flow, mirror RankBattle timer behavior
	if (m_cfg.ccBattleMode && m_cfg.rankUiEnabled && m_state == State::IN_ROUND)
	{
		// Stop any round/countdown UI to prevent stuck HUDs
		StopRoundTimerUI();
		if (m_currentWorldId)
			BroadcastCountdownToWorld(m_currentWorldId, false);

		// Pull battle count and stage finish timings
		BYTE battleCount = 1; DWORD stageFinishMs = 3000, matchFinishMs = 4000, matchReadyMs = 3000;
		if (CRankBattleTable* pRankBattleTable = g_pTableContainer->GetRankBattleTable())
		{
			CTable* pBase = reinterpret_cast<CTable*>(pRankBattleTable);
			for (CTable::TABLEIT it = pBase->Begin(); it != pBase->End(); ++it)
			{
				sRANKBATTLE_TBLDAT* rb = (sRANKBATTLE_TBLDAT*)it->second;
				if (rb && rb->worldTblidx == (TBLIDX)m_currentWorldTblidx)
				{
					battleCount = rb->byBattleCount;
					stageFinishMs = rb->dwStageFinishTime * 1000;
					matchFinishMs = rb->dwMatchFinishTime * 1000;
					matchReadyMs = rb->dwMatchReadyTime * 1000;
					break;
				}
			}
		}

		if (m_cfg.roundsCount > 0) battleCount = (BYTE)m_cfg.roundsCount;
		bool hasNextRound = (m_rankBattleStage + 1) < battleCount;
		if (hasNextRound && m_rankBattleState == RANKBATTLE_BATTLESTATE_STAGE_RUN)
		{
			UpdateRankBattleState(RANKBATTLE_BATTLESTATE_STAGE_FINISH, m_rankBattleStage, stageFinishMs);
			return;
		}
		// final round: on arena world, finish immediately with minimal UX
		if (IsArenaWorldTblidx(m_currentWorldTblidx))
		{
			FinishMatch(false);
			return;
		}
		// otherwise, mirror RankBattle MATCH_FINISH
		if (m_rankBattleState != RANKBATTLE_BATTLESTATE_MATCH_FINISH)
			UpdateRankBattleState(RANKBATTLE_BATTLESTATE_MATCH_FINISH, m_rankBattleStage, matchFinishMs);
		return;
	}

	// Non-CC mode: decide winner by points if available; otherwise just finish
	FinishByPoints();
}

void CArenaManager::FinishByPoints()
{
	// Aggregate points by solo or by team depending on mode
	unsigned int bestId = 0;
	unsigned int bestScore = 0;

	if (m_mode == Mode::FREE_FOR_ALL || m_mode == Mode::OPEN)
	{
		// Clear combat permissions first
		SetCombatPermittedForParticipants(false);
		for (auto& kv : m_killPoints)
		{
			if (kv.second > bestScore) { bestScore = kv.second; bestId = kv.first; }
		}
		if (bestId != 0)
			FinishWithWinner(bestId);
		else
			FinishMatch(false);
		return;
	}

	// Team aggregation
	std::unordered_map<unsigned int, unsigned int> teamScore;
	for (auto& kv : m_killPoints)
	{
		CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)kv.first);
		if (!p) continue;
		unsigned int key = 0;
		if (m_mode == Mode::PARTY_VS_PARTY)
			key = (unsigned int)p->GetPartyID();
		else if (m_mode == Mode::GUILD_VS_GUILD)
			key = (unsigned int)p->GetGuildID();
		teamScore[key] += kv.second;
	}
	unsigned int bestTeam = 0; bestScore = 0;
	for (auto& kv : teamScore)
		if (kv.second > bestScore) { bestScore = kv.second; bestTeam = kv.first; }

	if (bestTeam != 0)
	{
		// Stop any running round timers
		StopRoundTimerUI();
		// Pick any online member of that team as representative winner for rewards message
		CPlayer* rep = nullptr;
		for (auto cid : m_participants)
		{
			CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid);
			if (!p) continue;
			if ((m_mode == Mode::PARTY_VS_PARTY && (unsigned int)p->GetPartyID() == bestTeam) ||
				(m_mode == Mode::GUILD_VS_GUILD && (unsigned int)p->GetGuildID() == bestTeam))
			{
				rep = p; break;
			}
		}
		if (rep)
		{
			// Announce team winner unless we are on arena world (minimal UX)
			if (!IsArenaWorldTblidx(m_currentWorldTblidx))
			{
				wchar_t msg[256];
				ComposeWinnerText(rep, msg, _countof(msg));
				SendNotice(msg, SERVER_TEXT_SYSNOTICE);
			}
		}
		// RankBattle-like finish UX (skip on arena worlds)
		if (m_cfg.telecastEnabled && !IsArenaWorldTblidx(m_currentWorldTblidx))
			BroadcastTelecastToWorld(EnsureCurrentWorldId());
		// Suppress RankBattle finish/leave on arena world to keep UI
		if (!IsArenaWorldTblidx(m_currentWorldTblidx))
		{
			if (m_cfg.rankUiEnabled && !m_cfg.suppressRankFinishUi)
			{
				BroadcastRankStateToWorld(EnsureCurrentWorldId(), 5, 1); // MATCH_FINISH
				if (!m_cfg.keepRankUiAfterFinish)
				{
					if (m_currentWorldId)
						BroadcastRankLeaveToWorld(m_currentWorldId);
				}
			}
		}
		if (m_cfg.rewardsEnabled)
		{
			AwardRewards(true);
			AwardRewards(false);
			// Notify players that rewards were granted
			for (auto cid : m_participants)
			{
				if (CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid))
				{
					SendSystemTo(p, L"[Arena] Rewards granted.");
				}
			}
		}
		DespawnArenaMobs();
		if (m_cfg.postFinishTeleport)
			PostFinishTeleportAll();
		m_state = State::COMPLETE;
	}
	else
	{
		FinishMatch(false);
	}
}

// Build and broadcast a simple scoreboard as system text to all participants present in the given world
void CArenaManager::BroadcastScoreboardToWorld(unsigned int worldId)
{
	if (worldId == 0) return;
	// Aggregate per-player points; in team modes also show team sums
	std::unordered_map<unsigned int, unsigned int> teamTotals;
	bool isTeamMode = (m_mode == Mode::PARTY_VS_PARTY || m_mode == Mode::GUILD_VS_GUILD);

	// Compose a compact scoreboard string
	std::wstring msg = L"[Arena] Score: ";
	bool first = true;
	for (auto& kv : m_killPoints)
	{
		CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)kv.first);
		if (!p || !p->IsInitialized() || (unsigned int)p->GetWorldID() != worldId) continue;
		if (!first) msg += L" | ";
		first = false;
		wchar_t buf[96];
		swprintf_s(buf, L"%s:%u", p->GetCharName(), kv.second);
		msg += buf;
		if (isTeamMode)
		{
			unsigned int key = (m_mode == Mode::PARTY_VS_PARTY) ? (unsigned int)p->GetPartyID() : (unsigned int)p->GetGuildID();
			teamTotals[key] += kv.second;
		}
	}
	if (isTeamMode && !teamTotals.empty())
	{
		msg += L" || Team:";
		bool firstTeam = true;
		for (auto& tk : teamTotals)
		{
			if (!firstTeam) msg += L", ";
			firstTeam = false;
			wchar_t tbuf[64];
			swprintf_s(tbuf, L"%u:%u", tk.first, tk.second);
			msg += tbuf;
		}
	}

	// Broadcast only to participants within the world
	for (auto cid : m_participants)
	{
		CPlayer* pRecv = g_pObjectManager->FindByChar((CHARACTERID)cid);
		if (!pRecv || !pRecv->IsInitialized() || (unsigned int)pRecv->GetWorldID() != worldId) continue;
		SendSystemTo(pRecv, msg.c_str());
	}
}

void CArenaManager::OnPlayerFaint(unsigned int killerCharId, unsigned int victimCharId)
{
	if (!m_cfg.enabled) return;
	if (killerCharId == 0 || killerCharId == victimCharId) return;
	if (m_participants.find(killerCharId) == m_participants.end()) return;
	bool isParticipantVictim = (m_participants.find(victimCharId) != m_participants.end());
	// Only count enemy eliminations in team modes; ignore friendly fire/self
	bool countKill = true;
	if (m_mode == Mode::PARTY_VS_PARTY || m_mode == Mode::GUILD_VS_GUILD)
	{
		CPlayer* pKiller = g_pObjectManager->FindByChar((CHARACTERID)killerCharId);
		CPlayer* pVictim = g_pObjectManager->FindByChar((CHARACTERID)victimCharId);
		if (pKiller && pVictim)
		{
			BYTE kt = RANKBATTLE_TEAM_NONE, vt = RANKBATTLE_TEAM_NONE;
			if (sRANK_BATTLE_DATA* rdK = pKiller->GetRankBattleData()) kt = rdK->eTeamType;
			if (sRANK_BATTLE_DATA* rdV = pVictim->GetRankBattleData()) vt = rdV->eTeamType;
			if (kt != RANKBATTLE_TEAM_NONE && vt != RANKBATTLE_TEAM_NONE)
				countKill = (kt != vt);
			else if (m_mode == Mode::PARTY_VS_PARTY)
				countKill = (pKiller->GetPartyID() != pVictim->GetPartyID());
			else
				countKill = (pKiller->GetGuildID() != pVictim->GetGuildID());
		}
	}
	// Count depending on configuration:
	// - scoreOnFaint: always award on faint (respawn scoring mode)
	// - otherwise: only when revive is disabled (classic elimination points)
	if (countKill && (m_cfg.scoreOnFaint || !m_cfg.reviveOnFaint))
		m_killPoints[killerCharId] += 1;

	// Optional: immediately show a lightweight scoreboard update to participants in the arena world
	if (m_state == State::IN_ROUND && m_currentWorldId)
		BroadcastScoreboardToWorld(m_currentWorldId);

	// Revive (possibly delayed) in score mode to keep action flowing
	// Only when true score mode is enabled to avoid revive/teleport races in elimination modes.
	if (isParticipantVictim && m_state == State::IN_ROUND && m_cfg.reviveOnFaint && m_cfg.scoreOnFaint)
	{
		if (m_cfg.reviveDelayMs == 0)
		{
			ReviveParticipantNow(victimCharId, /*bApplyRespawnBuff*/true);
		}
		else
		{
			m_pendingReviveMs[victimCharId] = m_cfg.reviveDelayMs;
			// Optional: notify the victim of pending respawn
			CPlayer* pVictim = g_pObjectManager->FindByChar((CHARACTERID)victimCharId);
			if (pVictim)
			{
				wchar_t buf[96];
				swprintf_s(buf, L"Respawning in %.1f seconds...", (double)m_cfg.reviveDelayMs / 1000.0);
				SendSystemTo(pVictim, buf, SERVER_TEXT_SYSNOTICE);
			}
		}
	}
}

void CArenaManager::ReviveParticipantNow(unsigned int victimCharId, bool bApplyRespawnBuff)
{
	CPlayer* pVictim = g_pObjectManager->FindByChar((CHARACTERID)victimCharId);
	if (!pVictim || !pVictim->IsInitialized()) return;
	// Only allow immediate revive during active round to prevent late revives after finish/teleport
	if (m_state != State::IN_ROUND) return;
	// Only handle victims currently in the active arena world
	unsigned int worldId = m_currentWorldId ? m_currentWorldId : EnsureCurrentWorldId();
	if (!worldId || (unsigned int)pVictim->GetWorldID() != worldId) return;
	if (!pVictim->IsFainting()) return; // already revived elsewhere

	// 1) Proactively clear harmful DOTs and all conditions to avoid instant re-faint
	//    (bleed/poison/burn/stomachache and any CC that could block actions)
	if (pVictim->GetBuffManager())
	{
		pVictim->GetBuffManager()->EndSubBuff(ACTIVE_BLEED, INVALID_SYSTEM_EFFECT_CODE);
		pVictim->GetBuffManager()->EndSubBuff(ACTIVE_POISON, INVALID_SYSTEM_EFFECT_CODE);
		pVictim->GetBuffManager()->EndSubBuff(ACTIVE_BURN, INVALID_SYSTEM_EFFECT_CODE);
		pVictim->GetBuffManager()->EndSubBuff(ACTIVE_STOMACHACHE, INVALID_SYSTEM_EFFECT_CODE);
	}
	if (pVictim->GetStateManager())
	{
		// Clear any existing conditions (including FAINT visuals) before reviving
		pVictim->GetStateManager()->RemoveConditionFlags(INVALID_QWORD, true);
	}

	// 2) Pick a safe respawn location: team start spots for CC mode; otherwise world start1
	CNtlVector loc;
	CNtlVector dir;
	if (sWORLD_TBLDAT* pWorldTbldat = (sWORLD_TBLDAT*)g_pTableContainer->GetWorldTable()->FindData((TBLIDX)m_currentWorldTblidx))
	{
		bool useTeam2 = false;
		if (m_cfg.ccBattleMode)
		{
			if (sRANK_BATTLE_DATA* rd = pVictim->GetRankBattleData())
				useTeam2 = (rd->eTeamType == RANKBATTLE_TEAM_CHALLENGER);
		}
		if (useTeam2)
		{
			loc = pWorldTbldat->vStart2Loc; dir = pWorldTbldat->vStart2Dir;
		}
		else
		{
			loc = pWorldTbldat->vStart1Loc; dir = pWorldTbldat->vStart1Dir;
		}
		// Slight randomization to avoid stacking
		loc.x += RandomRangeF(-2.5f, 2.5f);
		loc.z += RandomRangeF(-2.5f, 2.5f);
	}
	else
	{
		// Fallback: current position
		loc = pVictim->GetCurLoc(); dir = pVictim->GetCurDir();
	}

	// 3) Revive cleanly (RESCUED semantics match round reset behavior) and restore resources
	pVictim->Revival(loc, pVictim->GetWorldID(), REVIVAL_TYPE_RESCUED);
	pVictim->UpdateCurLpEp(pVictim->GetMaxLP(), pVictim->GetMaxEP(), true, false);
	// Broadcast standing to nearby players
	pVictim->SendCharStateStanding();
	// Some clients keep the FAINT UI until they themselves receive a direct STANDING update.
	// Send an explicit self-only state update to guarantee the FAINT overlay is cleared in score mode.
	{
		CNtlPacket pkt(sizeof(sGU_UPDATE_CHAR_STATE));
		sGU_UPDATE_CHAR_STATE* res = (sGU_UPDATE_CHAR_STATE*)pkt.GetPacketData();
		res->wOpCode = GU_UPDATE_CHAR_STATE;
		res->handle = pVictim->GetID();
		res->sCharState.sCharStateBase.byStateID = CHARSTATE_STANDING;
		pVictim->GetStateManager()->CopyAspectTo(&res->sCharState.sCharStateBase.aspectState);
		pVictim->GetCurLoc().CopyTo(res->sCharState.sCharStateBase.vCurLoc);
		pVictim->GetCurDir().CopyTo(res->sCharState.sCharStateBase.vCurDir);
		res->sCharState.sCharStateBase.eAirState = pVictim->GetAirState();
		res->sCharState.sCharStateBase.bFightMode = pVictim->GetFightMode();
		res->sCharState.sCharStateBase.dwConditionFlag = pVictim->GetConditionState();
		res->sCharState.sCharStateBase.dwStateTime = 0;
		pkt.SetPacketLen(sizeof(sGU_UPDATE_CHAR_STATE));
		((CGameServer*)g_pApp)->Send(pVictim->GetClientSessionID(), &pkt);
	}

	// 4) Clear environmental hazard ticks (lava) and ensure PvP zone + Rank/Budokai states are set
	pVictim->LeaveLava();
	// Ensure PvP zone + ATTACKABLE are active immediately after revive
	pVictim->UpdatePvpZone(true);
	if (CWorld* pWorld = ((CGameServer*)g_pApp)->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)pVictim->GetWorldID()))
	{
		BYTE r = pWorld->GetTbldat()->byWorldRuleType;
		bool isBudokaiRule = (r == GAMERULE_MINORMATCH || r == GAMERULE_MAJORMATCH || r == GAMERULE_FINALMATCH);
		if (isBudokaiRule)
		{
			std::unordered_set<unsigned int> one{ pVictim->GetCharID() };
			ArenaBroadcastBudokaiPlayerStateToWorld(pWorld, one, MATCH_MEMBER_STATE_NORMAL);
		}
		else
		{
			// Mirror RankBattle revive sequence: transition FAINT -> NORMAL -> ATTACKABLE
			{
				if (sRANK_BATTLE_DATA* rdN = pVictim->GetRankBattleData()) rdN->eState = RANKBATTLE_MEMBER_STATE_NORMAL;
				CNtlPacket pktN(sizeof(sGU_RANKBATTLE_BATTLE_PLAYER_STATE_NFY));
				sGU_RANKBATTLE_BATTLE_PLAYER_STATE_NFY* resN = (sGU_RANKBATTLE_BATTLE_PLAYER_STATE_NFY*)pktN.GetPacketData();
				resN->wOpCode = GU_RANKBATTLE_BATTLE_PLAYER_STATE_NFY;
				resN->hPc = pVictim->GetID();
				resN->byPCState = RANKBATTLE_MEMBER_STATE_NORMAL;
				pktN.SetPacketLen(sizeof(sGU_RANKBATTLE_BATTLE_PLAYER_STATE_NFY));
				pWorld->Broadcast(&pktN);
			}

			{
				if (sRANK_BATTLE_DATA* rdA = pVictim->GetRankBattleData()) rdA->eState = RANKBATTLE_MEMBER_STATE_ATTACKABLE;
				CNtlPacket pktA(sizeof(sGU_RANKBATTLE_BATTLE_PLAYER_STATE_NFY));
				sGU_RANKBATTLE_BATTLE_PLAYER_STATE_NFY* resA = (sGU_RANKBATTLE_BATTLE_PLAYER_STATE_NFY*)pktA.GetPacketData();
				resA->wOpCode = GU_RANKBATTLE_BATTLE_PLAYER_STATE_NFY;
				resA->hPc = pVictim->GetID();
				resA->byPCState = RANKBATTLE_MEMBER_STATE_ATTACKABLE;
				pktA.SetPacketLen(sizeof(sGU_RANKBATTLE_BATTLE_PLAYER_STATE_NFY));
				pWorld->Broadcast(&pktA);
			}
		}
	}
	// Persist RankBattle ATTACKABLE state after revive (already set above, keep for safety)
	if (sRANK_BATTLE_DATA* rd = pVictim->GetRankBattleData()) rd->eState = RANKBATTLE_MEMBER_STATE_ATTACKABLE;
	// 5) Clear any lingering restrictions (can't attack/skills)
	ClearCombatRestrictionsFor(pVictim);
	// Apply short post-revive protection if configured
	if (m_cfg.reviveProtectMs > 0)
	{
		ApplyReviveProtection(pVictim);
		m_reviveProtectRemainMs[pVictim->GetCharID()] = m_cfg.reviveProtectMs;
	}
	if (bApplyRespawnBuff)
		ApplyRespawnBuff(pVictim);
}

void CArenaManager::ApplyRespawnBuff(CPlayer* pPlayer)
{
	if (!pPlayer || !pPlayer->IsInitialized()) return;
	// Apply buff by SkillTblidx 813 (as requested)
	sSKILL_TBLDAT* pSkillTbldat = (sSKILL_TBLDAT*)g_pTableContainer->GetSkillTable()->FindData((TBLIDX)813);
	if (!pSkillTbldat) return;

	DWORD dwDurationInMs = pSkillTbldat->dwKeepTimeInMilliSecs;
	eSYSTEM_EFFECT_CODE aeEffectCode[NTL_MAX_EFFECT_IN_SKILL];
	sDBO_BUFF_PARAMETER aBuffParameter[NTL_MAX_EFFECT_IN_SKILL];
	for (int i = 0; i < NTL_MAX_EFFECT_IN_SKILL; ++i)
	{
		// Resolve effect code using the container helper (consistent with gm.cpp)
		aeEffectCode[i] = g_pTableContainer->GetSystemEffectTable()->GetEffectCodeWithTblidx(pSkillTbldat->skill_Effect[i]);
		aBuffParameter[i].byBuffParameterType = DBO_BUFF_PARAMETER_TYPE_DEFAULT;
		aBuffParameter[i].buffParameter.fParameter = (float)(pSkillTbldat->aSkill_Effect_Value[i]);
		aBuffParameter[i].buffParameter.dwRemainValue = (DWORD)pSkillTbldat->aSkill_Effect_Value[i];
		if (aeEffectCode[i] == ACTIVE_HEAL_OVER_TIME || aeEffectCode[i] == ACTIVE_EP_OVER_TIME)
		{
			aBuffParameter[i].byBuffParameterType = DBO_BUFF_PARAMETER_TYPE_HOT;
			aBuffParameter[i].buffParameter.dwRemainTime = dwDurationInMs;
		}
		else if (aeEffectCode[i] == ACTIVE_BLEED || aeEffectCode[i] == ACTIVE_POISON || aeEffectCode[i] == ACTIVE_STOMACHACHE || aeEffectCode[i] == ACTIVE_BURN)
		{
			aBuffParameter[i].byBuffParameterType = DBO_BUFF_PARAMETER_TYPE_DOT;
			aBuffParameter[i].buffParameter.dwRemainTime = dwDurationInMs;
		}
	}
	// Register as a bless-type buff sourced from system
	pPlayer->GetBuffManager()->RegisterBuff(dwDurationInMs, aeEffectCode, aBuffParameter, INVALID_HOBJECT, BUFF_TYPE_BLESS, pSkillTbldat);
}

void CArenaManager::ApplyReviveProtection(CPlayer* pPlayer)
{
	if (!pPlayer || !pPlayer->IsInitialized()) return;
	// Add true invincibility and cannot be targeted briefly
	pPlayer->GetStateManager()->AddConditionState(CHARCOND_INVINCIBLE, NULL, true);
	pPlayer->GetStateManager()->AddConditionState(CHARCOND_CANT_BE_TARGETTED, NULL, true);
	// Also disallow attacking to avoid cheap shots during protection
	pPlayer->GetStateManager()->AddConditionState(CHARCOND_ATTACK_DISALLOW, NULL, true);
	// Ensure client gets a standing update
	pPlayer->SendCharStateStanding();
}

void CArenaManager::ClearReviveProtection(CPlayer* pPlayer)
{
	if (!pPlayer || !pPlayer->IsInitialized()) return;
	pPlayer->GetStateManager()->RemoveConditionState(CHARCOND_INVINCIBLE, NULL, true);
	pPlayer->GetStateManager()->RemoveConditionState(CHARCOND_CANT_BE_TARGETTED, NULL, true);
	pPlayer->GetStateManager()->RemoveConditionState(CHARCOND_ATTACK_DISALLOW, NULL, true);
}

void CArenaManager::SpawnArenaMobs()
{
	DespawnArenaMobs();
	if (!m_cfg.mobsAllowed || m_cfg.mobTblidxList.empty()) return;
	unsigned int worldId = EnsureCurrentWorldId();
	if (!worldId) return;

	// Spawn each mob near the world start position with slight offsets
	sWORLD_TBLDAT* pWorldTbldat = (sWORLD_TBLDAT*)g_pTableContainer->GetWorldTable()->FindData((TBLIDX)m_currentWorldTblidx);
	if (!pWorldTbldat) return;
	CNtlVector base = pWorldTbldat->vStart1Loc;

	int idx = 0;
	for (unsigned int tblidx : m_cfg.mobTblidxList)
	{
		sMOB_TBLDAT* pMobTbldat = (sMOB_TBLDAT*)g_pTableContainer->GetMobTable()->FindData((TBLIDX)tblidx);
		if (!pMobTbldat) { /* invalid mob id; skip */ continue; }

		sSPAWN_TBLDAT spawn;
		ZeroMemory(&spawn, sizeof(spawn));
		sVECTOR3 loc; loc.x = base.x + (float)(idx * 3); loc.y = base.y; loc.z = base.z + (float)(idx * 3);
		sVECTOR3 dir; dir.x = 0; dir.y = 0; dir.z = 1;
		spawn.vSpawn_Loc.CopyFrom(loc);
		spawn.vSpawn_Dir.CopyFrom(dir);
		spawn.dwParty_Index = INVALID_DWORD;
		spawn.byMove_Range = 10;
		spawn.bySpawn_Move_Type = SPAWN_MOVE_WANDER;
		spawn.bySpawn_Loc_Range = 5;
		spawn.byWander_Range = 10;
		spawn.path_Table_Index = INVALID_TBLIDX;
		spawn.playScript = INVALID_TBLIDX;
		spawn.playScriptScene = INVALID_TBLIDX;
		spawn.aiScript = INVALID_TBLIDX;
		spawn.aiScriptScene = INVALID_TBLIDX;
		spawn.actionPatternTblidx = 1;

		if (CMonster* pMob = (CMonster*)g_pObjectManager->CreateCharacter(OBJTYPE_MOB))
		{
			if (pMob->CreateDataAndSpawn((WORLDID)worldId, pMobTbldat, &spawn, false, 0))
			{
				pMob->SetStandAlone(true); // mark as standalone so purge doesn't kill them
				m_spawnedMobs.push_back(pMob->GetID());
			}
			else
			{
				g_pObjectManager->DestroyCharacter(pMob);
			}
		}
		++idx;
	}
	NTL_PRINT(PRINT_APP, _T("[ARENA] Spawned %u mobs"), (unsigned)m_spawnedMobs.size());
}

void CArenaManager::DespawnArenaMobs()
{
	if (m_spawnedMobs.empty()) return;
	for (HOBJECT h : m_spawnedMobs)
	{
		CCharacter* pChar = g_pObjectManager->GetChar(h);
		CMonster* pMob = dynamic_cast<CMonster*>(pChar);
		if (pMob && pMob->GetCurWorld())
		{
			pMob->GetBotController()->ChangeControlState_Despawn();
		}
	}
	m_spawnedMobs.clear();
}

bool CArenaManager::SpawnMob(unsigned int mobTblidx, const CNtlVector* pAt, const CNtlVector* pDir)
{
	unsigned int worldId = EnsureCurrentWorldId();
	if (!worldId) return false;
	sMOB_TBLDAT* pMobTbldat = (sMOB_TBLDAT*)g_pTableContainer->GetMobTable()->FindData((TBLIDX)mobTblidx);
	if (!pMobTbldat) return false;

	CNtlVector base;
	if (pAt) base = *pAt; else
	{
		sWORLD_TBLDAT* pWorldTbldat = (sWORLD_TBLDAT*)g_pTableContainer->GetWorldTable()->FindData((TBLIDX)m_currentWorldTblidx);
		if (!pWorldTbldat) return false;
		base = pWorldTbldat->vStart1Loc;
		base.x += RandomRangeF(-5.f, 5.f);
		base.z += RandomRangeF(-5.f, 5.f);
	}
	CNtlVector dir;
	if (pDir) dir = *pDir; else { dir.x = 0; dir.y = 0; dir.z = 1; }

	sSPAWN_TBLDAT spawn; ZeroMemory(&spawn, sizeof(spawn));
	sVECTOR3 loc; loc.x = base.x; loc.y = base.y; loc.z = base.z;
	sVECTOR3 sdir; sdir.x = dir.x; sdir.y = dir.y; sdir.z = dir.z;
	spawn.vSpawn_Loc.CopyFrom(loc);
	spawn.vSpawn_Dir.CopyFrom(sdir);
	spawn.byMove_Range = 10;
	spawn.bySpawn_Move_Type = SPAWN_MOVE_WANDER;
	spawn.bySpawn_Loc_Range = 5;
	spawn.byWander_Range = 10;
	spawn.actionPatternTblidx = 1;

	if (CMonster* pMob = (CMonster*)g_pObjectManager->CreateCharacter(OBJTYPE_MOB))
	{
		if (pMob->CreateDataAndSpawn((WORLDID)worldId, pMobTbldat, &spawn, false, 0))
		{
			pMob->SetStandAlone(true);
			m_spawnedMobs.push_back(pMob->GetID());
			return true;
		}
		g_pObjectManager->DestroyCharacter(pMob);
	}
	return false;
}

void CArenaManager::BroadcastTelecastToWorld(unsigned int worldId)
{
	if (!m_cfg.telecastEnabled || worldId == 0) return;
	// Avoid blank telecast banners/logs when no speech text is configured
	if (m_cfg.telecastSpeechTblidx == 0)
	{
		ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA] TelecastEnabled=1 but TelecastSpeechTblidx=0; skipping telecast to avoid blank banner");
		return;
	}
	CNtlPacket packet(sizeof(sGU_TELECAST_MESSAGE_BEG_NFY));
	sGU_TELECAST_MESSAGE_BEG_NFY* res = (sGU_TELECAST_MESSAGE_BEG_NFY*)packet.GetPacketData();
	res->wOpCode = GU_TELECAST_MESSAGE_BEG_NFY;
	res->byTelecastType = m_cfg.telecastType;
	res->dwDisplayTime = m_cfg.telecastDisplayMs;
	res->npcTblidx = INVALID_TBLIDX;
	res->speechTblidx = m_cfg.telecastSpeechTblidx;
	packet.SetPacketLen(sizeof(sGU_TELECAST_MESSAGE_BEG_NFY));

	CGameServer* app = (CGameServer*)g_pApp;
	CWorld* pWorld = app->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)worldId);
	if (!pWorld) return;
	for (auto cid : m_participants)
	{
		CPlayer* pRecv = g_pObjectManager->FindByChar((CHARACTERID)cid);
		if (!pRecv || !pRecv->IsInitialized() || (unsigned int)pRecv->GetWorldID() != worldId) continue;
		pRecv->SendPacket(&packet);
	}
}

void CArenaManager::PostFinishTeleportAll()
{
	// If misconfigured or disabled, fallback to default behavior
	if (!m_cfg.postFinishTeleport || m_cfg.postFinishWorldTblidx == 0)
	{
		PostFinishTeleportDefault();
		return;
	}

	// Proactively clear any HUD elements (round timer, countdown, rank HUD) before teleporting out
	if (m_currentWorldId)
	{
		for (auto cid : m_participants)
		{
			if (CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid))
			{
				if ((unsigned int)p->GetWorldID() == m_currentWorldId)
				{
					SendRoundTimerEndTo(p);
					if (m_cfg.rankPacketsEnabled && m_cfg.keepRankUiAfterFinish)
						SendRankLeaveTo(p);
					SendCountdownTo(p, false);
				}
			}
		}
		for (auto cid : m_spectators)
		{
			if (CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid))
			{
				if ((unsigned int)p->GetWorldID() == m_currentWorldId)
				{
					SendRoundTimerEndTo(p);
					if (m_cfg.rankPacketsEnabled && m_cfg.keepRankUiAfterFinish)
						SendRankLeaveTo(p);
					SendCountdownTo(p, false);
				}
			}
		}
	}

	// Ensure fainted participants are standing before teleporting out to avoid client re-spawn quirks
	if (m_currentWorldId)
	{
		for (auto cid : m_participants)
		{
			if (CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid))
			{
				if ((unsigned int)p->GetWorldID() == m_currentWorldId && p->IsFainting())
				{
					p->UpdateCurLpEp(p->GetMaxLP(), p->GetMaxEP(), true, false);
					p->SendCharStateStanding();
				}
			}
		}
	}
	for (auto cid : m_participants)
	{
		if (CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid))
		{
			if (m_cfg.postFinishDirX != 0 || m_cfg.postFinishDirY != 0 || m_cfg.postFinishDirZ != 0)
				TeleportOneToWorldTblidxDir(p, m_cfg.postFinishWorldTblidx, m_cfg.postFinishPosX, m_cfg.postFinishPosY, m_cfg.postFinishPosZ, m_cfg.postFinishDirX, m_cfg.postFinishDirY, m_cfg.postFinishDirZ);
			else
				TeleportOneToWorldTblidx(p, m_cfg.postFinishWorldTblidx, m_cfg.postFinishPosX, m_cfg.postFinishPosY, m_cfg.postFinishPosZ);
		}
	}
	for (auto cid : m_spectators)
	{
		if (CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid))
		{
			// ensure spectator hide is removed when leaving
			if (m_cfg.spectatorHide)
				ApplySpectatorHide(p, false);
			if (m_cfg.postFinishDirX != 0 || m_cfg.postFinishDirY != 0 || m_cfg.postFinishDirZ != 0)
				TeleportOneToWorldTblidxDir(p, m_cfg.postFinishWorldTblidx, m_cfg.postFinishPosX, m_cfg.postFinishPosY, m_cfg.postFinishPosZ, m_cfg.postFinishDirX, m_cfg.postFinishDirY, m_cfg.postFinishDirZ);
			else
				TeleportOneToWorldTblidx(p, m_cfg.postFinishWorldTblidx, m_cfg.postFinishPosX, m_cfg.postFinishPosY, m_cfg.postFinishPosZ);
		}
	}
}

void CArenaManager::ComposeWinnerText(CPlayer* pWinner, wchar_t* outBuf, size_t cchBuf)
{
	if (!pWinner || !outBuf || cchBuf == 0) return;
	// Decide winner label based on mode and affiliation
	if (m_mode == Mode::PARTY_VS_PARTY && pWinner->GetParty() && pWinner->GetPartyID() != INVALID_PARTYID)
	{
		swprintf_s(outBuf, cchBuf, L"Arena winner: Party '%s'", pWinner->GetParty()->GetPartyName());
	}
	else if (m_mode == Mode::GUILD_VS_GUILD && pWinner->GetGuildID() != 0)
	{
		swprintf_s(outBuf, cchBuf, L"Arena winner: Guild '%s'", pWinner->GetGuildName());
	}
	else
	{
		swprintf_s(outBuf, cchBuf, L"Arena winner: %s", pWinner->GetCharName());
	}
}

// Normalize helper: lowercase and trim spaces for ASCII-only names
static std::string ArenaNormalizeName(const std::string& s)
{
	std::string t;
	t.reserve(s.size());
	for (char c : s)
	{
		if (c == ' ' || c == '\t' || c == '\r' || c == '\n') continue;
		t.push_back((char)tolower((unsigned char)c));
	}
	return t;
}

static std::string ArenaNormalizeW(const std::wstring& ws)
{
	// Convert wchar string to UTF-8 (or narrow) safely, then normalize
	return ArenaNormalizeName(ws2s(ws));
}

void CArenaManager::LoadMobListFile(const char* path)
{
	m_mobNameToId.clear();
	m_cfg.mobPresets.clear();
	if (!path || !*path)
		return;
	std::ifstream in(path, std::ios::in | std::ios::binary);
	if (!in.is_open())
	{
		NTL_PRINT(PRINT_APP, _T("[ARENA] MobListFile not found: %S"), path);
		return;
	}
	std::string line;
	unsigned int lineNo = 0;
	while (std::getline(in, line))
	{
		++lineNo;
		if (line.empty()) continue;
		// Strip BOM if present on first line
		if (lineNo == 1 && line.size() >= 3 && (unsigned char)line[0] == 0xEF && (unsigned char)line[1] == 0xBB && (unsigned char)line[2] == 0xBF)
			line = line.substr(3);
		// Trim comments starting with '#'
		size_t hash = line.find('#');
		if (hash != std::string::npos) line = line.substr(0, hash);
		// Trim spaces
		auto trim = [](std::string& x) {
			size_t a = x.find_first_not_of(" \t\r\n");
			size_t b = x.find_last_not_of(" \t\r\n");
			if (a == std::string::npos) { x.clear(); return; }
			x = x.substr(a, b - a + 1);
			};
		trim(line);
		if (line.empty()) continue;

		// Preset line: preset:Name=1,2,3
		if (line.rfind("preset:", 0) == 0)
		{
			std::string rest = line.substr(7);
			size_t eq = rest.find('=');
			if (eq == std::string::npos) continue;
			std::string presetName = rest.substr(0, eq);
			std::string values = rest.substr(eq + 1);
			trim(presetName); trim(values);
			std::vector<unsigned int> ids;
			size_t pos = 0;
			while (pos != std::string::npos)
			{
				size_t comma = values.find(',', pos);
				std::string tok = values.substr(pos, comma == std::string::npos ? std::string::npos : (comma - pos));
				trim(tok);
				if (!tok.empty())
				{
					unsigned int v = (unsigned int)strtoul(tok.c_str(), nullptr, 10);
					if (v != 0) ids.push_back(v);
				}
				if (comma == std::string::npos) break;
				pos = comma + 1;
			}
			if (!presetName.empty() && !ids.empty())
			{
				std::string key = ArenaNormalizeName(presetName);
				m_cfg.mobPresets[key] = ids;
			}
			continue;
		}

		// Mapping line: name=tblidx
		size_t eq = line.find('=');
		if (eq == std::string::npos) continue;
		std::string name = line.substr(0, eq);
		std::string val = line.substr(eq + 1);
		trim(name); trim(val);
		if (name.empty() || val.empty()) continue;
		unsigned int id = (unsigned int)strtoul(val.c_str(), nullptr, 10);
		if (id == 0) continue;
		std::string key = ArenaNormalizeName(name);
		m_mobNameToId[key] = id;
	}
	NTL_PRINT(PRINT_APP, _T("[ARENA] Loaded Mobs.txt: %u names, %u presets"), (unsigned)m_mobNameToId.size(), (unsigned)m_cfg.mobPresets.size());
}

unsigned int CArenaManager::ResolveMobIdByName(const std::wstring& name) const
{
	if (name.empty()) return 0;
	std::string key = ArenaNormalizeW(name);
	auto it = m_mobNameToId.find(key);
	if (it == m_mobNameToId.end()) return 0;
	return it->second;
}

std::vector<unsigned int> CArenaManager::GetPresetPool() const
{
	// If a preset is selected and exists, return it; otherwise fallback to static mob list
	if (m_cfg.mobPreset.c_str() && m_cfg.mobPreset.c_str()[0] != '\0')
	{
		std::string key = ArenaNormalizeName(m_cfg.mobPreset.c_str());
		auto it = m_cfg.mobPresets.find(key);
		if (it != m_cfg.mobPresets.end() && !it->second.empty())
			return it->second;
	}
	return m_cfg.mobTblidxList;
}

void CArenaManager::SpawnRandomMobWave(unsigned int count)
{
	if (!m_cfg.mobsAllowed || count == 0) return;
	unsigned int worldId = EnsureCurrentWorldId();
	if (!worldId) return;
	std::vector<unsigned int> pool = GetPresetPool();
	if (pool.empty()) return;

	sWORLD_TBLDAT* pWorldTbldat = (sWORLD_TBLDAT*)g_pTableContainer->GetWorldTable()->FindData((TBLIDX)m_currentWorldTblidx);
	if (!pWorldTbldat) return;
	CNtlVector base = pWorldTbldat->vStart1Loc;

	for (unsigned int i = 0; i < count; ++i)
	{
		// Pick a random mob id from pool
		unsigned int idx = (unsigned int)RandomRange(0, (int)pool.size() - 1);
		unsigned int mobTblidx = pool[idx];
		// Randomize around base
		CNtlVector pos = base;
		pos.x += RandomRangeF(-6.f, 6.f);
		pos.z += RandomRangeF(-6.f, 6.f);
		CNtlVector dir; dir.x = 0; dir.y = 0; dir.z = 1;
		if (SpawnMob(mobTblidx, &pos, &dir))
		{
			// success tracked by SpawnMob into m_spawnedMobs
		}
	}
	NTL_PRINT(PRINT_APP, _T("[ARENA] Spawned random mob wave: %u (pool=%u, preset='%S')"), count, (unsigned)pool.size(), m_cfg.mobPreset.c_str());
}

void CArenaManager::SetRandomMobsSpawn(bool on)
{
	m_cfg.randomMobsSpawn = on;
	if (on && m_state == State::IN_ROUND && m_cfg.randomMobsWaveSeconds > 0)
		m_randomWaveRemainMs = ToMs(m_cfg.randomMobsWaveSeconds);
}

void CArenaManager::SetRandomMobsPerWave(unsigned int n)
{
	if (n == 0) n = 1;
	m_cfg.randomMobsPerWave = n;
}

void CArenaManager::SetRandomMobsWaveSeconds(unsigned int sec)
{
	if (sec == 0) sec = 1;
	m_cfg.randomMobsWaveSeconds = sec;
	if (m_state == State::IN_ROUND)
		m_randomWaveRemainMs = ToMs(m_cfg.randomMobsWaveSeconds);
}

bool CArenaManager::SetRandomMobsPreset(const std::string& name)
{
	std::string key = ArenaNormalizeName(name);
	if (key.empty()) { m_cfg.mobPreset = ""; return true; }
	auto it = m_cfg.mobPresets.find(key);
	if (it == m_cfg.mobPresets.end()) return false;
	m_cfg.mobPreset = key.c_str();
	return true;
}

void CArenaManager::FormatTime(unsigned int seconds, wchar_t* outBuf, size_t cchBuf)
{
	if (!outBuf || cchBuf == 0) return;
	unsigned int m = seconds / 60;
	unsigned int s = seconds % 60;
	if (m > 0)
		swprintf_s(outBuf, cchBuf, L"%um %us", m, s);
	else
		swprintf_s(outBuf, cchBuf, L"%us", s);
}

void CArenaManager::AnnounceRoundTimeRemaining(unsigned int secondsLeft)
{
	wchar_t timebuf[32];
	FormatTime(secondsLeft, timebuf, _countof(timebuf));
	wchar_t msg[128];
	swprintf_s(msg, _countof(msg), L"[Arena] Round ends in %s.", timebuf);
	BroadcastSystem(msg);
}

void CArenaManager::AnnounceRotationTimeRemaining(unsigned int secondsLeft)
{
	wchar_t timebuf[32];
	FormatTime(secondsLeft, timebuf, _countof(timebuf));
	wchar_t msg[128];
	swprintf_s(msg, _countof(msg), L"[Arena] Map rotation in %s.", timebuf);
	BroadcastSystem(msg);
}

void CArenaManager::ComposeWinnerTextTeam_Party(PARTYID partyId, wchar_t* outBuf, size_t cchBuf)
{
	if (!outBuf || cchBuf == 0) return;
	if (partyId == INVALID_PARTYID)
	{
		wcsncpy_s(outBuf, cchBuf, L"Arena winner: Party", _TRUNCATE);
		return;
	}
	CParty* pParty = g_pPartyManager->GetParty(partyId);
	if (pParty)
		swprintf_s(outBuf, cchBuf, L"Arena winner: Party '%s'", pParty->GetPartyName());
	else
		wcsncpy_s(outBuf, cchBuf, L"Arena winner: Party", _TRUNCATE);
}

void CArenaManager::ComposeWinnerTextTeam_Guild(GUILDID guildId, wchar_t* outBuf, size_t cchBuf)
{
	if (!outBuf || cchBuf == 0) return;
	if (guildId == 0)
	{
		wcsncpy_s(outBuf, cchBuf, L"Arena winner: Guild", _TRUNCATE);
		return;
	}
	CGuild* pGuild = g_pGuildManager->GetGuild(guildId);
	if (pGuild)
		swprintf_s(outBuf, cchBuf, L"Arena winner: Guild '%s'", pGuild->GetGuildName());
	else
		wcsncpy_s(outBuf, cchBuf, L"Arena winner: Guild", _TRUNCATE);
}

bool CArenaManager::TeleportOneToWorldTblidx(CPlayer* pPlayer, unsigned int worldTblidx, float posX, float posY, float posZ)
{
	if (!pPlayer || !pPlayer->IsInitialized()) return false;
	if (worldTblidx == 0) return false;

	CGameServer* app = (CGameServer*)g_pApp;
	sWORLD_TBLDAT* pWorldTbldat = (sWORLD_TBLDAT*)g_pTableContainer->GetWorldTable()->FindData((TBLIDX)worldTblidx);
	if (!pWorldTbldat) return false;

	CWorld* pWorld = nullptr;
	// Reuse current arena instance when applicable
	if (worldTblidx == m_currentWorldTblidx && m_currentWorldId)
		pWorld = app->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)m_currentWorldId);
	if (!pWorld)
	{
		pWorld = app->GetGameMain()->GetWorldManager()->CreateWorld(pWorldTbldat);
		if (!pWorld) return false;
		// Apply same arena rule override when we have to create the world on-demand
		if (IsArenaWorld(pWorld) && ShouldOverrideRuleForWorld(worldTblidx))
		{
			pWorld->SetRuleOverride(GAMERULE_RANKBATTLE);
			m_worldsWithOverride.insert((unsigned int)pWorld->GetID());
		}
		if (worldTblidx == m_currentWorldTblidx)
			m_currentWorldId = (unsigned int)pWorld->GetID();
	}

	CNtlVector destLoc = pWorldTbldat->vStart1Loc;
	if (!(posX == 0.f && posY == 0.f && posZ == 0.f))
	{
		destLoc.x = posX; destLoc.y = posY; destLoc.z = posZ;
	}

	// Use COMMAND teleport type to avoid Budokai/Dojo proposal side effects
	pPlayer->StartTeleport(destLoc, pPlayer->GetCurDir(), pWorld->GetID(), TELEPORT_TYPE_COMMAND);
	return true;
}

bool CArenaManager::TeleportOneToWorldTblidxDir(CPlayer* pPlayer, unsigned int worldTblidx, float posX, float posY, float posZ, float dirX, float dirY, float dirZ)
{
	if (!pPlayer) return false;
	if (!TeleportOneToWorldTblidx(pPlayer, worldTblidx, posX, posY, posZ)) return false;
	// After teleport is initiated, update player's direction to match desired orientation
	// Note: direction is typically applied on spawn in client; server stores it for consistency
	CNtlVector vDir(dirX, dirY, dirZ);
	if (vDir.IsZero() == false)
		pPlayer->SetCurDir(vDir);
	return true;
}

void CArenaManager::SendDungeonStateTo(CPlayer* pPlayer, unsigned char byStage, unsigned int titleTblidx, unsigned int subTitleTblidx)
{
	if (!pPlayer) return;
	CNtlPacket packet(sizeof(sGU_BATTLE_DUNGEON_STATE_UPATE_NFY));
	sGU_BATTLE_DUNGEON_STATE_UPATE_NFY* res = (sGU_BATTLE_DUNGEON_STATE_UPATE_NFY*)packet.GetPacketData();
	res->wOpCode = GU_BATTLE_DUNGEON_STATE_UPATE_NFY;
	res->titleTblidx = titleTblidx;
	res->subTitleTblidx = subTitleTblidx;
	res->byStage = byStage;
	packet.SetPacketLen(sizeof(sGU_BATTLE_DUNGEON_STATE_UPATE_NFY));
	pPlayer->SendPacket(&packet);
}

void CArenaManager::SendRankStateTo(CPlayer* pPlayer, unsigned char byState, unsigned char byStage)
{
	if (!pPlayer) return;
	// Guard: only send RankBattle packets on rank-rule worlds; skip Budokai-rule and others
	if (!pPlayer->GetCurWorld()) return;
	CWorld* pWorld = pPlayer->GetCurWorld();
	BYTE rule = pWorld->GetTbldat()->byWorldRuleType;
	if (rule == GAMERULE_MINORMATCH || rule == GAMERULE_MAJORMATCH || rule == GAMERULE_FINALMATCH)
		return;
	if (rule != GAMERULE_RANKBATTLE)
		return;
	CNtlPacket packet(sizeof(sGU_RANKBATTLE_BATTLE_STATE_UPDATE_NFY));
	sGU_RANKBATTLE_BATTLE_STATE_UPDATE_NFY* res = (sGU_RANKBATTLE_BATTLE_STATE_UPDATE_NFY*)packet.GetPacketData();
	res->wOpCode = GU_RANKBATTLE_BATTLE_STATE_UPDATE_NFY;
	res->byBattleState = byState;
	// Use 1-based stage for UI
	res->byStage = (BYTE)byStage;
	packet.SetPacketLen(sizeof(sGU_RANKBATTLE_BATTLE_STATE_UPDATE_NFY));
	pPlayer->SendPacket(&packet);
}

void CArenaManager::SendRankMatchStartTo(CPlayer* pPlayer)
{
	if (!pPlayer) return;
	// Guard: only send RankBattle packets on rank-rule worlds; skip Budokai-rule and others
	if (!pPlayer->GetCurWorld()) return;
	CWorld* pWorld = pPlayer->GetCurWorld();
	BYTE rule = pWorld->GetTbldat()->byWorldRuleType;
	if (rule == GAMERULE_MINORMATCH || rule == GAMERULE_MAJORMATCH || rule == GAMERULE_FINALMATCH)
		return;
	if (rule != GAMERULE_RANKBATTLE)
		return;
	CNtlPacket packet(sizeof(sGU_RANKBATTLE_MATCH_START_NFY));
	sGU_RANKBATTLE_MATCH_START_NFY* res = (sGU_RANKBATTLE_MATCH_START_NFY*)packet.GetPacketData();
	res->wOpCode = GU_RANKBATTLE_MATCH_START_NFY;
	res->wResultCode = GAME_SUCCESS;
	packet.SetPacketLen(sizeof(sGU_RANKBATTLE_MATCH_START_NFY));
	pPlayer->SendPacket(&packet);
}

void CArenaManager::SendRankFullStartSequenceTo(CPlayer* pPlayer)
{
	if (!pPlayer) return;
	SendRankStateTo(pPlayer, RANKBATTLE_BATTLESTATE_WAIT, 0); // RANKBATTLE_BATTLESTATE_WAIT
	SendRankStateTo(pPlayer, RANKBATTLE_BATTLESTATE_DIRECTION, 0); // RANKBATTLE_BATTLESTATE_DIRECTION
	SendRankStateTo(pPlayer, RANKBATTLE_BATTLESTATE_STAGE_PREPARE, 0); // RANKBATTLE_BATTLESTATE_STAGE_PREPARE
	SendRankStateTo(pPlayer, RANKBATTLE_BATTLESTATE_STAGE_READY, 0); // RANKBATTLE_BATTLESTATE_STAGE_READY
	SendRankMatchStartTo(pPlayer);
	pPlayer->SendCharStateStanding();
	SendRankStateTo(pPlayer, RANKBATTLE_BATTLESTATE_STAGE_RUN, 0);
}

void CArenaManager::SendRankTeamInfoTo(CPlayer* pPlayer)
{
	if (!pPlayer) return;
	if (m_mode == Mode::PARTY_VS_PARTY || m_mode == Mode::GUILD_VS_GUILD)
		BroadcastRankTeamInfoToWorld((unsigned int)pPlayer->GetWorldID());
}

void CArenaManager::SendRoundTimerStartTo(CPlayer* pPlayer, unsigned int seconds)
{
	if (!pPlayer) return;
	CNtlPacket packet(sizeof(sGU_BATTLE_DUNGEON_LIMIT_TIME_START_NFY));
	sGU_BATTLE_DUNGEON_LIMIT_TIME_START_NFY* res = (sGU_BATTLE_DUNGEON_LIMIT_TIME_START_NFY*)packet.GetPacketData();
	res->wOpCode = GU_BATTLE_DUNGEON_LIMIT_TIME_START_NFY;
	res->dwLimitTime = seconds;
	packet.SetPacketLen(sizeof(sGU_BATTLE_DUNGEON_LIMIT_TIME_START_NFY));
	pPlayer->SendPacket(&packet);
	// Also send fallback countdown
	SendCountdownTo(pPlayer, true);
}

void CArenaManager::SendRoundTimerEndTo(CPlayer* pPlayer)
{
	if (!pPlayer) return;
	CNtlPacket packet(sizeof(sGU_BATTLE_DUNGEON_LIMIT_TIME_END_NFY));
	sGU_BATTLE_DUNGEON_LIMIT_TIME_END_NFY* res = (sGU_BATTLE_DUNGEON_LIMIT_TIME_END_NFY*)packet.GetPacketData();
	res->wOpCode = GU_BATTLE_DUNGEON_LIMIT_TIME_END_NFY;
	packet.SetPacketLen(sizeof(sGU_BATTLE_DUNGEON_LIMIT_TIME_END_NFY));
	pPlayer->SendPacket(&packet);
	// End fallback countdown too
	SendCountdownTo(pPlayer, false);
}

// Single-recipient variant of rank-battle LEAVE notify, used when moving
// an individual participant to spectator to ensure HUD cleans up safely.
void CArenaManager::SendRankLeaveTo(CPlayer* pPlayer)
{
	if (!pPlayer) return;
	CNtlPacket packet(sizeof(sGU_RANKBATTLE_LEAVE_NFY));
	sGU_RANKBATTLE_LEAVE_NFY* res = (sGU_RANKBATTLE_LEAVE_NFY*)packet.GetPacketData();
	res->wOpCode = GU_RANKBATTLE_LEAVE_NFY;
	packet.SetPacketLen(sizeof(sGU_RANKBATTLE_LEAVE_NFY));
	pPlayer->SendPacket(&packet);
}

void CArenaManager::SendCountdownTo(CPlayer* pPlayer, bool bStart)
{
	if (!pPlayer) return;
	CNtlPacket packet(sizeof(sGU_TIMEQUEST_COUNTDOWN_NFY));
	sGU_TIMEQUEST_COUNTDOWN_NFY* res = (sGU_TIMEQUEST_COUNTDOWN_NFY*)packet.GetPacketData();
	res->wOpCode = GU_TIMEQUEST_COUNTDOWN_NFY;
	res->bCountDown = bStart;
	packet.SetPacketLen(sizeof(sGU_TIMEQUEST_COUNTDOWN_NFY));
	pPlayer->SendPacket(&packet);
}

void CArenaManager::UpdateRankBattleState(eRANKBATTLE_BATTLESTATE newState, BYTE byStage, unsigned long durationMs)
{
	// Guard: if state and stage are unchanged and a timer is already active,
	// avoid re-broadcasting and resetting timers which can cause client freezes
	if (newState == m_rankBattleState && byStage == m_rankBattleStage) // No change
	{
		// Allow re-arming and rebroadcast for MATCH_FINISH to ensure watchdog is set
		// Otherwise, if a non-terminal state is already active with a timer, skip spamming
		if (newState != RANKBATTLE_BATTLESTATE_MATCH_FINISH)
		{
			if (m_rankStateTimeMs > 0)
			{
				// Do not spam the same non-terminal state; keep current timer running
				return;
			}
			// If timer is zero and state is STAGE_FINISH, avoid re-entry as well
			if (newState == RANKBATTLE_BATTLESTATE_STAGE_FINISH)
			{
				return;
			}
		}
	}

	m_rankBattleState = newState;
	m_rankBattleStage = byStage;
	m_rankStateTimeMs = durationMs;

	// If we are entering MATCH_FINISH, guarantee completion regardless of caller path:
	// - Arm a watchdog slightly beyond the given duration (covers paths that don't set it)
	// - If duration is zero (some tables use 0), finish immediately to avoid stalls
	if (newState == RANKBATTLE_BATTLESTATE_MATCH_FINISH)
	{
		// Always arm a watchdog; if durationMs is 0, use a small fallback window
		m_matchFinishWatchdogMs = (durationMs > 0) ? (durationMs + 2000) : 2000; // +2s buffer

		if (durationMs == 0)
		{
			ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA][RB] MATCH_FINISH with 0ms duration -> finishing immediately");
			// Clear any lingering round/countdown UI before finishing
			StopRoundTimerUI();
			if (m_currentWorldId)
				BroadcastCountdownToWorld(m_currentWorldId, false);
			// Finalize now
			FinishMatch(false);
			return; // already finished
		}
	}

	NTL_PRINT(PRINT_APP, _T("[ARENA] RankBattle State: %d -> Stage: %d, Duration: %ums"),
		(int)newState, (int)byStage, durationMs);

	// Broadcast state depending on world rule: Budokai-style on MINOR/MAJOR/FINAL rule worlds, RankBattle elsewhere
	if (m_currentWorldId != 0)
	{
		CGameServer* appRule = (CGameServer*)g_pApp;
		CWorld* pWorldRule = appRule->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)m_currentWorldId);
		BYTE ruleType = GAMERULE_NORMAL;
		if (pWorldRule && pWorldRule->GetTbldat())
			ruleType = pWorldRule->GetTbldat()->byWorldRuleType;
		bool isBudokaiRule = (ruleType == GAMERULE_MINORMATCH || ruleType == GAMERULE_MAJORMATCH || ruleType == GAMERULE_FINALMATCH);
		if (isBudokaiRule)
		{
			// Map RankBattle states to Budokai equivalents
			BYTE matchType = (m_mode == Mode::PARTY_VS_PARTY || m_mode == Mode::GUILD_VS_GUILD) ? BUDOKAI_MATCH_TYPE_TEAM : BUDOKAI_MATCH_TYPE_INDIVIDIAUL;
			BYTE budokaiState = BUDOKAI_MATCHSTATE_WAIT_MINOR_MATCH;
			// Default remaining time as seconds
			BUDOKAITIME remainSec = (BUDOKAITIME)(durationMs / 1000);
			BUDOKAITIME nextStepTime = remainSec; // use same for both fields

			switch (newState)
			{
			case RANKBATTLE_BATTLESTATE_WAIT:
			case RANKBATTLE_BATTLESTATE_DIRECTION:
			case RANKBATTLE_BATTLESTATE_STAGE_PREPARE:
			case RANKBATTLE_BATTLESTATE_STAGE_READY:
				budokaiState = BUDOKAI_MATCHSTATE_WAIT_MINOR_MATCH; // "Please wait/Get ready"
				break;
			case RANKBATTLE_BATTLESTATE_STAGE_RUN:
				budokaiState = BUDOKAI_MATCHSTATE_MINOR_MATCH; // "Fight!"
				break;
			case RANKBATTLE_BATTLESTATE_STAGE_FINISH:
				// Keep as MINOR_MATCH until final finish; client HUD will handle stage end via our own notices
				budokaiState = BUDOKAI_MATCHSTATE_MINOR_MATCH;
				break;
			case RANKBATTLE_BATTLESTATE_MATCH_FINISH:
				budokaiState = BUDOKAI_MATCHSTATE_MATCH_END;
				break;
			default:
				budokaiState = BUDOKAI_MATCHSTATE_WAIT_MINOR_MATCH;
				break;
			}

			// Progress message hints: entering vs start
			if (newState == RANKBATTLE_BATTLESTATE_WAIT || newState == RANKBATTLE_BATTLESTATE_DIRECTION || newState == RANKBATTLE_BATTLESTATE_STAGE_PREPARE || newState == RANKBATTLE_BATTLESTATE_STAGE_READY)
			{
				BroadcastBudokaiProgressMessageToWorld(m_currentWorldId, BUDOKAI_PROGRESS_MESSAGE_MINORMATCH_ENTERING);
			}
			else if (newState == RANKBATTLE_BATTLESTATE_STAGE_RUN)
			{
				BroadcastBudokaiProgressMessageToWorld(m_currentWorldId, BUDOKAI_PROGRESS_MESSAGE_START);
			}

			BroadcastBudokaiMatchStateToWorld(m_currentWorldId, matchType, budokaiState, nextStepTime, remainSec);
		}
		else
		{
			CGameServer* app = (CGameServer*)g_pApp;
			CWorld* pWorld = app->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)m_currentWorldId);
			if (pWorld)
			{
				CNtlPacket packet(sizeof(sGU_RANKBATTLE_BATTLE_STATE_UPDATE_NFY));
				sGU_RANKBATTLE_BATTLE_STATE_UPDATE_NFY* res = (sGU_RANKBATTLE_BATTLE_STATE_UPDATE_NFY*)packet.GetPacketData();
				res->wOpCode = GU_RANKBATTLE_BATTLE_STATE_UPDATE_NFY;
				res->byBattleState = newState;
				// Use 1-based stage for UI display (Round = stage + 1)
				res->byStage = (BYTE)byStage;
				packet.SetPacketLen(sizeof(sGU_RANKBATTLE_BATTLE_STATE_UPDATE_NFY));

				for (auto cid : m_participants)
				{
					CPlayer* pRecv = g_pObjectManager->FindByChar((CHARACTERID)cid);
					if (!pRecv || !pRecv->IsInitialized() || (unsigned int)pRecv->GetWorldID() != (unsigned int)m_currentWorldId)
						continue;

					// Ensure client exits any locked state
					pRecv->SendCharStateStanding();

					// When the state enters RUN, explicitly mark participant attackable for RankBattle-rule worlds
					if (newState == RANKBATTLE_BATTLESTATE_STAGE_RUN)
					{
						if (sRANK_BATTLE_DATA* rd = pRecv->GetRankBattleData())
						{
							rd->eState = RANKBATTLE_MEMBER_STATE_ATTACKABLE;
						}
						else
						{
							ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA][WARN] MakeParticipantsAttackable: Null RankBattleData char=%u", (unsigned)pRecv->GetCharID());
						}
						std::unordered_set<unsigned int> one{ (unsigned int)cid };
						ArenaBroadcastRankPlayerAttackableToWorld(pWorld, one);
					}

					pRecv->SendPacket(&packet);
				}
			}
		}
	}
}

void CArenaManager::OnPlayerEnterWorld(CPlayer* pPlayer)
{
	if (!pPlayer || !m_cfg.enabled) return;

	// DEBUG: Log all arena state for troubleshooting
	NTL_PRINT(PRINT_APP, _T("[ARENA] OnPlayerEnterWorld: char=%u worldTblidx=%u state=%d enabled=%d"),
		pPlayer->GetCharID(), pPlayer->GetWorldTblidx(), (int)m_state, m_cfg.enabled ? 1 : 0);

	if (!IsParticipant(pPlayer))
	{
		NTL_PRINT(PRINT_APP, _T("[ARENA] OnPlayerEnterWorld: char=%u not a participant, ignoring"), pPlayer->GetCharID());
		ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA] OnEnterWorld ignore: not participant char=%u", (unsigned)pPlayer->GetCharID());
		return;
	}

	if (m_state != State::IN_ROUND && m_state != State::PRE_ROUND &&
		m_state != State::MATCH_READY && m_state != State::STAGE_READY)
	{
		NTL_PRINT(PRINT_APP, _T("[ARENA] OnPlayerEnterWorld: char=%u wrong arena state=%d, ignoring"),
			pPlayer->GetCharID(), (int)m_state);
		ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA] OnEnterWorld ignore: wrong state=%u for char=%u", (unsigned)m_state, (unsigned)pPlayer->GetCharID());
		return;
	}

	// Only resync if he is in the arena world when using configured world
	if (m_currentWorldTblidx != 0 && (unsigned int)pPlayer->GetWorldTblidx() != m_currentWorldTblidx)
	{
		NTL_PRINT(PRINT_APP, _T("[ARENA] OnPlayerEnterWorld: char=%u not in arena world (current=%u, arena=%u), ignoring"),
			pPlayer->GetCharID(), pPlayer->GetWorldTblidx(), m_currentWorldTblidx);
		ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA] OnEnterWorld ignore: not arena world char=%u curWorldTblidx=%u arenaTblidx=%u", (unsigned)pPlayer->GetCharID(), (unsigned)pPlayer->GetWorldTblidx(), (unsigned)m_currentWorldTblidx);
		return;
	}

	NTL_PRINT(PRINT_APP, _T("[ARENA] OnPlayerEnterWorld: Processing char=%u in arena world"), pPlayer->GetCharID());

	// Enforce no-transformation rule at arrival (Kaio-ken allowed)
	// If the player arrives already transformed (e.g., from another world), cancel it now
	if (pPlayer->GetTransformationTbldat())
	{
		BYTE aspect = pPlayer->GetAspectStateId();
		if (aspect != ASPECTSTATE_KAIOKEN)
		{
			pPlayer->CancelTransformation();
			// Inform the player once
			CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
			sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
			res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
			res->byDisplayType = SERVER_TEXT_SYSTEM;
			NTL_SAFE_WCSCPY(res->awchMessage, L"[Arena] Transformations are not allowed here. Your transformation has been removed.");
			packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
			pPlayer->SendPacket(&packet);
			ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA] OnEnter: canceled transformation for char=%u (non-Kaio-ken)", (unsigned)pPlayer->GetCharID());
		}
	}

	// Schedule readiness with optional per-player delay
	unsigned int cid = pPlayer->GetCharID();
	if (m_cfg.enterReadyDelayMs > 0)
		m_readyDelayMs[cid] = m_cfg.enterReadyDelayMs;
	else
		m_readyParticipants.insert(cid);

	if (m_state == State::PRE_ROUND)
	{
		// In PRE_ROUND: send appropriate HUD bootstrap. Use Budokai-style on Budokai worlds,
		// otherwise send RankBattle JOIN + WAIT(0) like RankBattle does on enter.
		// Use the player's current world rule to decide Budokai-style bootstrap
		if (pPlayer->GetCurWorld() && (pPlayer->GetCurWorld()->GetTbldat()->byWorldRuleType == GAMERULE_MINORMATCH ||
			pPlayer->GetCurWorld()->GetTbldat()->byWorldRuleType == GAMERULE_MAJORMATCH ||
			pPlayer->GetCurWorld()->GetTbldat()->byWorldRuleType == GAMERULE_FINALMATCH))
		{
			// Mirror WAIT state with Budokai update and progress message for the entering player
			CNtlPacket packet(sizeof(sGU_BUDOKAI_UPDATE_MATCH_STATE_NFY));
			sGU_BUDOKAI_UPDATE_MATCH_STATE_NFY* res = (sGU_BUDOKAI_UPDATE_MATCH_STATE_NFY*)packet.GetPacketData();
			res->wOpCode = GU_BUDOKAI_UPDATE_MATCH_STATE_NFY;
			res->byMatchType = (m_mode == Mode::PARTY_VS_PARTY || m_mode == Mode::GUILD_VS_GUILD) ? BUDOKAI_MATCH_TYPE_TEAM : BUDOKAI_MATCH_TYPE_INDIVIDIAUL;
			res->sStateInfo.byState = BUDOKAI_MATCHSTATE_WAIT_MINOR_MATCH;
			res->sStateInfo.tmNextStepTime = (BUDOKAITIME)0;
			res->sStateInfo.tmRemainTime = (BUDOKAITIME)0;
			packet.SetPacketLen(sizeof(sGU_BUDOKAI_UPDATE_MATCH_STATE_NFY));
			pPlayer->SendPacket(&packet);
			// Also send ENTERING message to show "Please wait"
			CNtlPacket packet2(sizeof(sGU_BUDOKAI_PROGRESS_MESSAGE_NFY));
			sGU_BUDOKAI_PROGRESS_MESSAGE_NFY* res2 = (sGU_BUDOKAI_PROGRESS_MESSAGE_NFY*)packet2.GetPacketData();
			res2->wOpCode = GU_BUDOKAI_PROGRESS_MESSAGE_NFY;
			res2->byMsgId = BUDOKAI_PROGRESS_MESSAGE_MINORMATCH_ENTERING;
			packet2.SetPacketLen(sizeof(sGU_BUDOKAI_PROGRESS_MESSAGE_NFY));
			pPlayer->SendPacket(&packet2);
		}
		else if (m_cfg.rankUiEnabled)
		{
			// Single-player JOIN for safety (in addition to world-broadcast later)
			TBLIDX roomTblidx = FindRankBattleTblidxForWorld((TBLIDX)m_currentWorldTblidx);
			if (roomTblidx != INVALID_TBLIDX)
			{
				CNtlPacket packet(sizeof(sGU_RANKBATTLE_JOIN_NFY));
				sGU_RANKBATTLE_JOIN_NFY* res = (sGU_RANKBATTLE_JOIN_NFY*)packet.GetPacketData();
				res->wOpCode = GU_RANKBATTLE_JOIN_NFY;
				res->rankBattleTblidx = roomTblidx;
				packet.SetPacketLen(sizeof(sGU_RANKBATTLE_JOIN_NFY));
				pPlayer->SendPacket(&packet);
			}
		}
		// Ensure team type is set server-side for attack rules
		if (m_cfg.ccBattleMode)
		{
			// Simple half-split or party/guild grouping
			unsigned int worldId = (unsigned int)pPlayer->GetWorldID();
			std::vector<CPlayer*> present;
			for (auto cid : m_participants)
			{
				CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid);
				if (p && p->IsInitialized() && (unsigned int)p->GetWorldID() == worldId)
					present.push_back(p);
			}
			bool useParty = (m_mode == Mode::PARTY_VS_PARTY);
			bool useGuild = (m_mode == Mode::GUILD_VS_GUILD);
			// Owner key = first non-zero, challenger = different
			unsigned int ownerKey = 0;
			auto teamKey = [&](CPlayer* x) { return useParty ? (unsigned int)x->GetPartyID() : (useGuild ? (unsigned int)x->GetGuildID() : 0); };
			for (CPlayer* x : present) { unsigned int k = teamKey(x); if (k == 0) continue; if (ownerKey == 0) ownerKey = k; else if (k != ownerKey) { /* found a different team */ break; } }
			for (size_t i = 0; i < present.size(); ++i)
			{
				CPlayer* x = present[i];
				BYTE t = RANKBATTLE_TEAM_OWNER;
				if (useParty || useGuild)
				{
					unsigned int k = teamKey(x);
					t = (k != 0 && k != ownerKey) ? RANKBATTLE_TEAM_CHALLENGER : RANKBATTLE_TEAM_OWNER;
				}
				else
				{
					t = (i < (present.size() + 1) / 2) ? RANKBATTLE_TEAM_OWNER : RANKBATTLE_TEAM_CHALLENGER;
				}
				sRANK_BATTLE_DATA* rd = x->GetRankBattleData();
				if (rd) rd->eTeamType = (eRANKBATTLE_TEAM_TYPE)t; else ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA][WARN] Null RankBattleData when setting team type (OnEnter) char=%u", (unsigned)x->GetCharID());
			}
		}
		SendRankStateTo(pPlayer, RANKBATTLE_BATTLESTATE_WAIT, 0);
		pPlayer->SendCharStateStanding();
		ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA] OnEnter PRE_ROUND: marked ready char=%u readyCount=%u", (unsigned)cid, (unsigned)m_readyParticipants.size());
		return;
	}

	// STAGE_READY/IN_ROUND resync path: ensure unlock and send UI
	pPlayer->SendCharStateStanding();
	// If player enters during STAGE_READY, mirror per-round unlock sequence for this client
	if (m_state == State::STAGE_READY)
	{
		// Do NOT send ATTACKABLE in READY; client expects ATTACKABLE after RUN
		// Do not start a new timer here; STAGE_READY -> RUN path will start the timer.
		// If a timer is already active for some reason, just sync the remaining time.
		if (m_roundUiActive && m_roundRemainMs > 0)
			SendRoundTimerStartTo(pPlayer, (unsigned int)(m_roundRemainMs / 1000));
		ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA] OnEnter STAGE_READY: char=%u timerActive=%d", (unsigned)pPlayer->GetCharID(), m_roundUiActive ? 1 : 0);
	}
	// Also re-broadcast unlock for this player to ensure client can act in RUN
	if (m_state == State::IN_ROUND)
	{
		CGameServer* app = (CGameServer*)g_pApp;
		CWorld* pWorld = app->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)pPlayer->GetWorldID());
		if (pWorld)
		{
			// Ensure combat permissions are enabled for late joiner during RUN
			SetCombatPermittedFor(pPlayer, true);
			std::unordered_set<unsigned int> one{ pPlayer->GetCharID() };
			if (!pWorld->GetTbldat())
			{
				ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA][WARN] OnEnter IN_ROUND: world tbldat missing worldId=%u", (unsigned)pWorld->GetID());
			}
			BYTE r = pWorld->GetTbldat() ? pWorld->GetTbldat()->byWorldRuleType : GAMERULE_NORMAL;
			bool isBudokaiRule = (r == GAMERULE_MINORMATCH || r == GAMERULE_MAJORMATCH || r == GAMERULE_FINALMATCH);
			if (isBudokaiRule)
				ArenaBroadcastBudokaiPlayerStateToWorld(pWorld, one, MATCH_MEMBER_STATE_NORMAL);
			else
				ArenaBroadcastRankPlayerAttackableToWorld(pWorld, one);
		}
		// Sync timer as well if running
		if (m_roundUiActive && m_roundRemainMs > 0)
			SendRoundTimerStartTo(pPlayer, (unsigned int)(m_roundRemainMs / 1000));
		ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA] OnEnter IN_ROUND: char=%u timerActive=%d", (unsigned)pPlayer->GetCharID(), m_roundUiActive ? 1 : 0);
	}
	// Send rank UI packets only if explicitly enabled and safe (skip on Budokai-rule worlds)
	{
		bool isBudokaiRule = false;
		if (pPlayer->GetCurWorld())
		{
			BYTE r = pPlayer->GetCurWorld()->GetTbldat()->byWorldRuleType;
			isBudokaiRule = (r == GAMERULE_MINORMATCH || r == GAMERULE_MAJORMATCH || r == GAMERULE_FINALMATCH);
		}
		if (m_cfg.rankUiEnabled && m_cfg.ccBattleMode && !isBudokaiRule)
		{
			NTL_PRINT(PRINT_APP, _T("[ARENA] Sending rank packets to char=%u, state=%d, stage=%d"),
				pPlayer->GetCharID(), (int)m_rankBattleState, (int)m_rankBattleStage);

			// Send current rank battle state to player who just entered
			if (m_rankBattleState != INVALID_RANKBATTLE_BATTLESTATE)
			{
				SendRankStateTo(pPlayer, m_rankBattleState, m_rankBattleStage);
			}
			else
			{
				NTL_PRINT(PRINT_APP, _T("[ARENA] Skipping rank state - state is INVALID"));
				// On Budokai-rule worlds, mirror the current state with Budokai notifications for late joiners
				if (pPlayer->GetCurWorld() && (pPlayer->GetCurWorld()->GetTbldat()->byWorldRuleType == GAMERULE_MINORMATCH ||
					pPlayer->GetCurWorld()->GetTbldat()->byWorldRuleType == GAMERULE_MAJORMATCH ||
					pPlayer->GetCurWorld()->GetTbldat()->byWorldRuleType == GAMERULE_FINALMATCH) &&
					m_rankBattleState != INVALID_RANKBATTLE_BATTLESTATE && (unsigned int)pPlayer->GetWorldID() == m_currentWorldId)
				{
					BYTE matchType = (m_mode == Mode::PARTY_VS_PARTY || m_mode == Mode::GUILD_VS_GUILD) ? BUDOKAI_MATCH_TYPE_TEAM : BUDOKAI_MATCH_TYPE_INDIVIDIAUL;
					BYTE budokaiState = BUDOKAI_MATCHSTATE_WAIT_MINOR_MATCH;
					if (m_rankBattleState == RANKBATTLE_BATTLESTATE_STAGE_RUN)
						budokaiState = BUDOKAI_MATCHSTATE_MINOR_MATCH;
					else if (m_rankBattleState == RANKBATTLE_BATTLESTATE_MATCH_FINISH)
						budokaiState = BUDOKAI_MATCHSTATE_MATCH_END;
					// Single-recipient mirror of BroadcastBudokaiMatchStateToWorld
					CNtlPacket packet(sizeof(sGU_BUDOKAI_UPDATE_MATCH_STATE_NFY));
					sGU_BUDOKAI_UPDATE_MATCH_STATE_NFY* res = (sGU_BUDOKAI_UPDATE_MATCH_STATE_NFY*)packet.GetPacketData();
					res->wOpCode = GU_BUDOKAI_UPDATE_MATCH_STATE_NFY;
					res->byMatchType = matchType;
					res->sStateInfo.byState = budokaiState;
					res->sStateInfo.tmNextStepTime = (BUDOKAITIME)(m_rankStateTimeMs / 1000);
					res->sStateInfo.tmRemainTime = (BUDOKAITIME)(m_rankStateTimeMs / 1000);
					packet.SetPacketLen(sizeof(sGU_BUDOKAI_UPDATE_MATCH_STATE_NFY));
					pPlayer->SendPacket(&packet);
				}
			}
		}
		else
		{
			NTL_PRINT(PRINT_APP, _T("[ARENA] Skipping rank packets: rankUi=%d ccBattle=%d"),
				m_cfg.rankUiEnabled ? 1 : 0, m_cfg.ccBattleMode ? 1 : 0);
			// On Budokai worlds, mirror the current state with Budokai notifications for late joiners
			if (IsBudokaiWorld(m_currentWorldTblidx) && m_rankBattleState != INVALID_RANKBATTLE_BATTLESTATE && (unsigned int)pPlayer->GetWorldID() == m_currentWorldId)
			{
				BYTE matchType = (m_mode == Mode::PARTY_VS_PARTY || m_mode == Mode::GUILD_VS_GUILD) ? BUDOKAI_MATCH_TYPE_TEAM : BUDOKAI_MATCH_TYPE_INDIVIDIAUL;
				BYTE budokaiState = BUDOKAI_MATCHSTATE_WAIT_MINOR_MATCH;
				if (m_rankBattleState == RANKBATTLE_BATTLESTATE_STAGE_RUN)
					budokaiState = BUDOKAI_MATCHSTATE_MINOR_MATCH;
				else if (m_rankBattleState == RANKBATTLE_BATTLESTATE_MATCH_FINISH)
					budokaiState = BUDOKAI_MATCHSTATE_MATCH_END;
				// Single-recipient mirror of BroadcastBudokaiMatchStateToWorld
				CNtlPacket packet(sizeof(sGU_BUDOKAI_UPDATE_MATCH_STATE_NFY));
				sGU_BUDOKAI_UPDATE_MATCH_STATE_NFY* res = (sGU_BUDOKAI_UPDATE_MATCH_STATE_NFY*)packet.GetPacketData();
				res->wOpCode = GU_BUDOKAI_UPDATE_MATCH_STATE_NFY;
				res->byMatchType = matchType;
				res->sStateInfo.tmNextStepTime = (BUDOKAITIME)(m_rankStateTimeMs / 1000);
				res->sStateInfo.tmRemainTime = (BUDOKAITIME)(m_rankStateTimeMs / 1000);
				packet.SetPacketLen(sizeof(sGU_BUDOKAI_UPDATE_MATCH_STATE_NFY));
				pPlayer->SendPacket(&packet);
			}
		}
	}

	SendDungeonStateTo(pPlayer);
	if (m_roundUiActive && m_roundRemainMs > 0)
		SendRoundTimerStartTo(pPlayer, (unsigned int)(m_roundRemainMs / 1000));
}

bool CArenaManager::ValidateTeamComposition()
{
	// For non-team modes, just check minimum participants
	if (m_mode == Mode::FREE_FOR_ALL || m_mode == Mode::OPEN)
	{
		unsigned int onlineParticipants = CountOnlineParticipants();
		if (onlineParticipants < 2)
		{
			BroadcastSystem(L"[Arena] Not enough participants online. Arena canceled.");
			SendNotice(L"Arena canceled - not enough participants.", SERVER_TEXT_SYSNOTICE);
			NTL_PRINT(PRINT_APP, _T("[ARENA] Validation failed: only %u participants online for non-team mode"), onlineParticipants);
			// If this occurred during AutoArena enrollment closing, proactively clear participants so
			// the scheduler can reset to IDLE and reopen on the next cycle without requiring @arena stop
			if (m_cfg.autoEnabled && m_autoState == AutoState::ENROLLMENT_OPEN)
			{
				m_participants.clear();
				ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA][AUTO] Cleared participants on validation failure (<2) to allow auto restart.");
			}
			return false;
		}
		return true;
	}

	// Team mode validation
	std::set<unsigned int> uniqueParties;
	std::set<unsigned int> uniqueGuilds;
	std::map<unsigned int, unsigned int> partyCount;
	std::map<unsigned int, unsigned int> guildCount;

	unsigned int validParticipants = 0;

	for (auto charId : m_participants)
	{
		CPlayer* pPlayer = g_pObjectManager->FindByChar((CHARACTERID)charId);
		if (!pPlayer || !pPlayer->IsInitialized()) continue;

		validParticipants++;

		if (m_mode == Mode::PARTY_VS_PARTY)
		{
			PARTYID partyId = pPlayer->GetPartyID();
			if (partyId != INVALID_PARTYID)
			{
				uniqueParties.insert(partyId);
				partyCount[partyId]++;
			}
			else
			{
				// Count solo players as individual "parties"
				uniqueParties.insert(UINT_MAX - charId); // unique ID for solo players
				partyCount[UINT_MAX - charId] = 1;
			}
		}
		else if (m_mode == Mode::GUILD_VS_GUILD)
		{
			GUILDID guildId = pPlayer->GetGuildID();
			if (guildId != 0)
			{
				uniqueGuilds.insert(guildId);
				guildCount[guildId]++;
			}
			else
			{
				BroadcastSystem(L"[Arena] Guild vs Guild mode requires all participants to be in guilds. Arena canceled.");
				SendNotice(L"Arena canceled - all participants must be in guilds for Guild vs Guild mode.", SERVER_TEXT_SYSNOTICE);
				NTL_PRINT(PRINT_APP, _T("[ARENA] Validation failed: Player %s (%u) not in guild for GUILD_VS_GUILD mode"),
					pPlayer->GetCharName(), charId);
				return false;
			}
		}
	}

	// Check minimum participants
	if (validParticipants < 2)
	{
		BroadcastSystem(L"[Arena] Not enough valid participants online. Arena canceled.");
		SendNotice(L"Arena canceled - not enough participants.", SERVER_TEXT_SYSNOTICE);
		NTL_PRINT(PRINT_APP, _T("[ARENA] Validation failed: only %u valid participants for team mode"), validParticipants);
		if (m_cfg.autoEnabled && m_autoState == AutoState::ENROLLMENT_OPEN)
		{
			m_participants.clear();
			ARENA_VLOG(m_cfg, LOG_GENERAL, "[ARENA][AUTO] Cleared participants on team-mode validation failure (<2) to allow auto restart.");
		}
		return false;
	}

	// Validate team composition based on mode
	if (m_mode == Mode::PARTY_VS_PARTY)
	{
		if (uniqueParties.size() < 2)
		{
			BroadcastSystem(L"[Arena] Party vs Party mode requires at least 2 different parties. Arena canceled.");
			SendNotice(L"Arena canceled - need at least 2 different parties.", SERVER_TEXT_SYSNOTICE);
			NTL_PRINT(PRINT_APP, _T("[ARENA] Validation failed: only %u parties for PARTY_VS_PARTY mode"), (unsigned)uniqueParties.size());
			return false;
		}

		NTL_PRINT(PRINT_APP, _T("[ARENA] Validation passed: %u parties with %u total participants for PARTY_VS_PARTY"),
			(unsigned)uniqueParties.size(), validParticipants);
	}
	else if (m_mode == Mode::GUILD_VS_GUILD)
	{
		if (uniqueGuilds.size() < 2)
		{
			BroadcastSystem(L"[Arena] Guild vs Guild mode requires at least 2 different guilds. Arena canceled.");
			SendNotice(L"Arena canceled - need at least 2 different guilds.", SERVER_TEXT_SYSNOTICE);
			NTL_PRINT(PRINT_APP, _T("[ARENA] Validation failed: only %u guilds for GUILD_VS_GUILD mode"), (unsigned)uniqueGuilds.size());
			return false;
		}

		NTL_PRINT(PRINT_APP, _T("[ARENA] Validation passed: %u guilds with %u total participants for GUILD_VS_GUILD"),
			(unsigned)uniqueGuilds.size(), validParticipants);
	}

	return true;
}

void CArenaManager::LoadAvailableWorlds()
{
	// Clear current world list and reload from RankBattle table
	m_cfg.worldTblidxList.clear();

	// Load all available RankBattle worlds from the table
	CRankBattleTable* pRankBattleTable = g_pTableContainer->GetRankBattleTable();
	if (!pRankBattleTable)
	{
		NTL_PRINT(PRINT_APP, _T("[ARENA] LoadAvailableWorlds: RankBattle table not found"));
		return;
	}

	// Iterate through all RankBattle entries to get available worlds
	// Use base CTable interface via reinterpret_cast to avoid incomplete-type issues
	CTable* pBaseTable = reinterpret_cast<CTable*>(pRankBattleTable);
	CTable::TABLEIT it = pBaseTable->Begin();
	CTable::TABLEIT end = pBaseTable->End();

	std::set<TBLIDX> uniqueWorlds; // Use set to avoid duplicates

	for (; it != end; ++it)
	{
		sRANKBATTLE_TBLDAT* pRankData = (sRANKBATTLE_TBLDAT*)it->second;
		if (pRankData && pRankData->worldTblidx != INVALID_TBLIDX)
		{
			// Verify the world exists in WorldTable
			sWORLD_TBLDAT* pWorldData = (sWORLD_TBLDAT*)g_pTableContainer->GetWorldTable()->FindData(pRankData->worldTblidx);
			if (pWorldData)
			{
				uniqueWorlds.insert(pRankData->worldTblidx);
			}
		}
	}

	// Convert set to vector
	for (TBLIDX worldTblidx : uniqueWorlds)
	{
		m_cfg.worldTblidxList.push_back(worldTblidx);
	}

	// Initialize rotation if we have worlds
	if (!m_cfg.worldTblidxList.empty())
	{
		m_worldIndex = 0;
		m_currentWorldTblidx = m_cfg.worldTblidxList[m_worldIndex];
		m_rotationRemainMs = ToMs(m_cfg.rotationSeconds);
		// Force recreation of world instance
		m_currentWorldId = 0;

		NTL_PRINT(PRINT_APP, _T("[ARENA] Loaded %u worlds from RankBattle table, starting with tblidx %u"),
			(unsigned)m_cfg.worldTblidxList.size(), m_currentWorldTblidx);
	}
	else
	{
		NTL_PRINT(PRINT_APP, _T("[ARENA] No valid worlds found in RankBattle table"));
	}
}
