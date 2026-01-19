#include "stdafx.h"
#include "EventManager.h"
#include "NtlIniFile.h"
#include "GameServer.h"
#include "GameMain.h"
#include "ObjectManager.h"
#include "CPlayer.h"
#include "NtlPacketGU.h"
#include "NtlPacketGT.h"
#include "NtlLog.h"
#include "Monster.h"
#include "World.h"
#include "WorldTable.h"
#include "TableContainerManager.h"
#include "ItemManager.h"
#include "ItemDrop.h"
#include "CustomDropEvent.h"
#include "NtlRandom.h"
#include <algorithm>
#include <sstream>
#include <cstdarg>
#include <cstdio>
#include <random>
#include <vector>
#include <fstream>
#include <regex>
#include <cctype>
#include <limits>
#include <PortalTable.h>

// Guard against Windows GDI macro collision (GetObject) only; keep original ERR_LOG implementation from logging system.
#ifdef GetObject
#undef GetObject
#endif

// Windows headers define max/min macros; ensure we use std::max/std::min below.
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

static unsigned long ToMs(unsigned int seconds) { return seconds * 1000UL; }

static void EventVLog(const CEventManager::Config& cfg, int category, const char* fmt, ...)
{
    if (!cfg.verboseLogs || fmt == nullptr) return;
    char buf[1024];
    buf[0] = '\0';
    va_list ap; va_start(ap, fmt);
#if defined(_MSC_VER)
    vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, ap);
#else
    vsnprintf(buf, sizeof(buf), fmt, ap);
#endif
    va_end(ap);
    NTL_PRINT(category, _T("%S"), buf);
}

#define EVENT_VLOG(cfg, category, fmt, ...) do { \
    if ((cfg).verboseLogs) EventVLog((cfg), (category), (fmt), ##__VA_ARGS__); \
} while (0)

CEventManager::CEventManager()
	: m_state(State::IDLE)
	, m_currentRound(0)
	, m_eventWorldId(0)
	, m_tickCount(0)
	, m_enrollmentRemainMs(0)
	, m_startDelayRemainMs(0)
	, m_roundRemainMs(0)
	, m_roundTimerActive(false)
	, m_postEventTeleportRemainMs(0)
	, m_roundStartTime(0)
	, m_teamRedDamage(0)
	, m_teamBlueDamage(0)
	, m_teamRedKills(0)
	, m_teamBlueKills(0)
{
}

CEventManager::~CEventManager()
{
}

// Centralized helper: get a world instance for a tblidx.
// - If the world is static (bDynamic==false), reuse the existing instance by tblidx as WORLDID.
// - If dynamic, reuse m_eventWorldId if valid and matches, otherwise create a new world and record m_eventWorldId.
CWorld* CEventManager::GetOrCreateWorld(unsigned int worldTblidx)
{
	CGameServer* app = (CGameServer*)g_pApp;
	if (!app) return nullptr;
	sWORLD_TBLDAT* pWorldTbldat = (sWORLD_TBLDAT*)g_pTableContainer->GetWorldTable()->FindData((TBLIDX)worldTblidx);
	if (!pWorldTbldat) return nullptr;

	// If dynamic, prefer reusing current event instance when compatible
	if (pWorldTbldat->bDynamic)
	{
		if (m_eventWorldId)
		{
			CWorld* reuse = app->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)m_eventWorldId);
			if (reuse)
			{
				sWORLD_TBLDAT* curTbldat = reuse->GetTbldat();
				if (curTbldat && curTbldat->tblidx == (TBLIDX)worldTblidx)
					return reuse;
			}
		}
		CWorld* created = app->GetGameMain()->GetWorldManager()->CreateWorld(pWorldTbldat);
		if (!created) return nullptr;
		m_eventWorldId = (unsigned int)created->GetID();
		return created;
	}

	// Static world: it should already exist, find by tblidx
	CWorld* pStatic = app->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)pWorldTbldat->tblidx);
	if (pStatic)
	{
		m_eventWorldId = (unsigned int)pStatic->GetID();
		return pStatic;
	}
	return nullptr;
}

// Compute a destination location for a world: use world default loc (like @world command) then apply override if provided (non-zero vector)
bool CEventManager::ComputeDestForWorld(unsigned int worldTblidx, float overrideX, float overrideY, float overrideZ, CNtlVector& outDest)
{
	sWORLD_TBLDAT* pWorldTbldat = (sWORLD_TBLDAT*)g_pTableContainer->GetWorldTable()->FindData((TBLIDX)worldTblidx);
	if (!pWorldTbldat) return false;

	// For dynamic worlds (dungeons), use world table's default position unless we have a specific override
	if (pWorldTbldat->bDynamic)
	{
		NTL_PRINT(PRINT_APP, _T("[EVENT] World %u is dynamic, using position lookup"), worldTblidx);

		// First check if config override is provided (TeleportPosX/Y/Z in Events.cfg)
		if (!(overrideX == 0.f && overrideY == 0.f && overrideZ == 0.f))
		{
			outDest.x = overrideX;
			outDest.y = overrideY;
			outDest.z = overrideZ;
			NTL_PRINT(PRINT_APP, _T("[EVENT] Using config override position for world %u: (%.2f,%.2f,%.2f)"),
				worldTblidx, outDest.x, outDest.y, outDest.z);
			return true;
		}

		// Known problematic worlds that need hardcoded safe positions
		// Only add worlds here if their vDefaultLoc is known to be bad
		struct SafePos { float x, y, z; };
		static const std::map<unsigned int, SafePos> SAFE_POSITIONS = {
			// Add worlds here ONLY if vDefaultLoc doesn't work
			// Example: {600000, {-320.0f, 49.0f, 95.0f}},
		};

		auto it = SAFE_POSITIONS.find(worldTblidx);
		if (it != SAFE_POSITIONS.end())
		{
			outDest.x = it->second.x;
			outDest.y = it->second.y;
			outDest.z = it->second.z;
			NTL_PRINT(PRINT_APP, _T("[EVENT] Using hardcoded safe position for world %u: (%.2f,%.2f,%.2f)"),
				worldTblidx, outDest.x, outDest.y, outDest.z);
			return true;
		}

		// Default: use world table's vDefaultLoc (same as @world command)
		if (pWorldTbldat->vDefaultLoc.x != 0.0f || pWorldTbldat->vDefaultLoc.y != 0.0f || pWorldTbldat->vDefaultLoc.z != 0.0f)
		{
			outDest.x = pWorldTbldat->vDefaultLoc.x;
			outDest.y = pWorldTbldat->vDefaultLoc.y;
			outDest.z = pWorldTbldat->vDefaultLoc.z;
			NTL_PRINT(PRINT_APP, _T("[EVENT] Using vDefaultLoc for world %u: (%.2f,%.2f,%.2f)"),
				worldTblidx, outDest.x, outDest.y, outDest.z);
			return true;
		}

		// Last resort: try vStart1Loc
		if (pWorldTbldat->vStart1Loc.x != 0.0f || pWorldTbldat->vStart1Loc.y != 0.0f || pWorldTbldat->vStart1Loc.z != 0.0f)
		{
			outDest.x = pWorldTbldat->vStart1Loc.x;
			outDest.y = pWorldTbldat->vStart1Loc.y;
			outDest.z = pWorldTbldat->vStart1Loc.z;
			NTL_PRINT(PRINT_APP, _T("[EVENT] Using vStart1Loc for world %u: (%.2f,%.2f,%.2f)"),
				worldTblidx, outDest.x, outDest.y, outDest.z);
			return true;
		}

		// Absolute fallback: origin
		outDest.x = 0.0f;
		outDest.y = 0.0f;
		outDest.z = 0.0f;
		NTL_PRINT(PRINT_APP, _T("[EVENT] WARNING: No position data for world %u, using origin"), worldTblidx);
		return true;
	}

	CGameServer* app = (CGameServer*)g_pApp;
	CWorld* pWorld = nullptr;

	// Try to find existing world instance to get actual boundaries (static worlds only)
	if (app && app->GetGameMain() && app->GetGameMain()->GetWorldManager())
	{
		// For static worlds, find by tblidx
		if (!pWorldTbldat->bDynamic)
		{
			pWorld = app->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)worldTblidx);
		}
	}

	// If we have world instance with valid boundaries, calculate center position
	bool usedBoundaryCenter = false;
	if (pWorld)
	{
		CNtlVector startBoundary = pWorld->GetStartBoundary();
		CNtlVector endBoundary = pWorld->GetEndBoundary();

		// Log boundaries for debugging
		NTL_PRINT(PRINT_APP, _T("[EVENT] World %u boundaries: start(%.2f,%.2f,%.2f) end(%.2f,%.2f,%.2f)"),
			worldTblidx, startBoundary.x, startBoundary.y, startBoundary.z,
			endBoundary.x, endBoundary.y, endBoundary.z);

		// Validate boundaries are reasonable (not zero or garbage)
		auto IsValidBoundary = [](const CNtlVector& start, const CNtlVector& end) -> bool {
			const float MAX_COORD = 100000.0f;
			return std::isfinite(start.x) && std::isfinite(start.z) &&
			       std::isfinite(end.x) && std::isfinite(end.z) &&
			       fabsf(start.x) < MAX_COORD && fabsf(start.z) < MAX_COORD &&
			       fabsf(end.x) < MAX_COORD && fabsf(end.z) < MAX_COORD &&
			       (start.x != 0.0f || start.z != 0.0f || end.x != 0.0f || end.z != 0.0f);
		};

		if (IsValidBoundary(startBoundary, endBoundary))
		{
			// Calculate center of the world boundaries - ALWAYS SAFE!
			outDest.x = (startBoundary.x + endBoundary.x) / 2.0f;
			outDest.y = 0.0f; // Y will be adjusted by terrain later
			outDest.z = (startBoundary.z + endBoundary.z) / 2.0f;
			usedBoundaryCenter = true;

			NTL_PRINT(PRINT_APP, _T("[EVENT] Using world boundary center for world %u: (%.2f,%.2f,%.2f)"),
				worldTblidx, outDest.x, outDest.y, outDest.z);
		}
		else
		{
			NTL_PRINT(PRINT_APP, _T("[EVENT] World %u has invalid boundaries, using fallback"), worldTblidx);
		}
	}
	else
	{
		NTL_PRINT(PRINT_APP, _T("[EVENT] World instance not found for tblidx %u, using fallback"), worldTblidx);
	}

	// Fallback: use vDefaultLoc if boundary center not available
	if (!usedBoundaryCenter)
	{
		outDest = pWorldTbldat->vDefaultLoc;

		// Validate default position
		auto IsValidPos = [](float x, float y, float z) -> bool {
			const float MAX_COORD = 100000.0f;
			return std::isfinite(x) && std::isfinite(y) && std::isfinite(z) &&
			       fabsf(x) < MAX_COORD && fabsf(y) < MAX_COORD && fabsf(z) < MAX_COORD;
		};

		if (!IsValidPos(outDest.x, outDest.y, outDest.z))
		{
			ERR_LOG(LOG_GENERAL, _T("[EVENT] World %u has invalid vDefaultLoc (%.2f,%.2f,%.2f), using generic center"),
				worldTblidx, outDest.x, outDest.y, outDest.z);
			outDest.x = 0.0f;
			outDest.y = 0.0f;
			outDest.z = 0.0f;
		}
	}

	// Apply override if provided
	if (!(overrideX == 0.f && overrideY == 0.f && overrideZ == 0.f))
	{
		outDest.x = overrideX; outDest.y = overrideY; outDest.z = overrideZ;
	}
	return true;
}

bool CEventManager::LoadConfigFromIniPath(const char* iniPath)
{
	// Debug: Get current working directory
	char currentDir[MAX_PATH];
	GetCurrentDirectoryA(MAX_PATH, currentDir);
	printf("[EVENT] Current working directory: %s\n", currentDir);
	printf("[EVENT] Attempting to load: %s\n", iniPath);

	// Check if file exists
	DWORD fileAttr = GetFileAttributesA(iniPath);
	if (fileAttr == INVALID_FILE_ATTRIBUTES)
	{
		printf("[EVENT] File does not exist or cannot be accessed!\n");
		return false;
	}

	CNtlIniFile file;
	int createResult = file.Create(iniPath);
	if (createResult != NTL_SUCCESS)
	{
		printf("[EVENT] Failed to load config from %s (CNtlIniFile::Create returned %d)\n", iniPath, createResult);
		return false;
	}

	printf("[EVENT] CNtlIniFile::Create succeeded!\n");

	int enabled = 0;
	if (file.Read("Event", "Enabled", enabled))
		m_cfg.enabled = (enabled != 0);

	m_cfg.channelNameContains = file.Read("Event", "ChannelNameContains");
	if (m_cfg.channelNameContains.c_str()[0] == '\0')
		m_cfg.channelNameContains = "EVENTS";

	unsigned int worldTblidx = 0;
	if (file.Read("Event", "EventWorldTblidx", worldTblidx))
		m_cfg.eventWorldTblidx = worldTblidx;

	float posX = 0, posY = 0, posZ = 0;
	if (file.Read("Event", "SpawnPosX", posX)) m_cfg.spawnPosX = posX;
	if (file.Read("Event", "SpawnPosY", posY)) m_cfg.spawnPosY = posY;
	if (file.Read("Event", "SpawnPosZ", posZ)) m_cfg.spawnPosZ = posZ;

	unsigned int maxTick = 0;
	if (file.Read("Event", "MaxTickCount", maxTick))
		m_cfg.maxTickCount = maxTick;

	unsigned int tickInterval = 0;
	if (file.Read("Event", "TickIntervalMs", tickInterval))
		m_cfg.tickIntervalMs = tickInterval;

	unsigned int enrollSec = 0;
	if (file.Read("Event", "EnrollmentSeconds", enrollSec))
		m_cfg.enrollmentSeconds = enrollSec;

	int requireParticipate = 1;
	if (file.Read("Event", "RequireParticipateCommand", requireParticipate))
		m_cfg.requireParticipateCommand = (requireParticipate != 0);

	unsigned int startDelay = 0;
	if (file.Read("Event", "StartDelaySeconds", startDelay))
		m_cfg.startDelaySeconds = startDelay;

	unsigned int interm = 0;
	if (file.Read("Event", "IntermissionSeconds", interm))
		m_cfg.intermissionSeconds = interm;

	if (file.Read("Event", "TeleportPosX", posX)) m_cfg.teleportPosX = posX;
	if (file.Read("Event", "TeleportPosY", posY)) m_cfg.teleportPosY = posY;
	if (file.Read("Event", "TeleportPosZ", posZ)) m_cfg.teleportPosZ = posZ;

	int postTp = 1;
	if (file.Read("Event", "PostEventTeleport", postTp))
		m_cfg.postEventTeleport = (postTp != 0);

	unsigned int postWorld = 0;
	if (file.Read("Event", "PostEventWorldTblidx", postWorld))
		m_cfg.postEventWorldTblidx = postWorld;

	if (file.Read("Event", "PostEventPosX", posX)) m_cfg.postEventPosX = posX;
	if (file.Read("Event", "PostEventPosY", posY)) m_cfg.postEventPosY = posY;
	if (file.Read("Event", "PostEventPosZ", posZ)) m_cfg.postEventPosZ = posZ;

	unsigned int postDelay = 0;
	if (file.Read("Event", "PostEventTeleportDelayMs", postDelay))
		m_cfg.postEventTeleportDelayMs = postDelay;

	unsigned int mudosaRound = 0, mudosaComplete = 0;
	if (file.Read("Event", "MudosaPerRound", mudosaRound))
		m_cfg.mudosaPerRound = mudosaRound;
	if (file.Read("Event", "MudosaEventComplete", mudosaComplete))
		m_cfg.mudosaEventComplete = mudosaComplete;

	int autoEn = 0;
	if (file.Read("AutoEvent", "Enabled", autoEn))
		m_cfg.autoEnabled = (autoEn != 0);

	unsigned int autoInterval = 0, autoInitial = 0;
	if (file.Read("AutoEvent", "IntervalSeconds", autoInterval))
		m_cfg.autoIntervalSeconds = autoInterval;
	if (file.Read("AutoEvent", "InitialDelaySeconds", autoInitial))
		m_cfg.autoInitialDelaySeconds = autoInitial;

	int autoRestart = 0;
	if (file.Read("AutoEvent", "RestartOnComplete", autoRestart))
		m_cfg.autoRestartOnComplete = (autoRestart != 0);

	unsigned int autoRestartDelay = 0;
	if (file.Read("AutoEvent", "RestartDelaySeconds", autoRestartDelay))
		m_cfg.autoRestartDelaySeconds = autoRestartDelay;

	// World rotation
	int worldRotation = 0;
	if (file.Read("WorldRotation", "Enabled", worldRotation))
		m_cfg.worldRotationEnabled = (worldRotation != 0);

	CNtlString worldListCsv = file.Read("WorldRotation", "WorldList");
	ParseWorldListCsv(worldListCsv);

	int randomWorlds = 0;
	if (file.Read("WorldRotation", "RandomizeWorlds", randomWorlds))
		m_cfg.randomizeWorlds = (randomWorlds != 0);

	float mobRadius = 0;
	if (file.Read("Event", "MobSpawnRadius", mobRadius))
		m_cfg.mobSpawnRadius = mobRadius;

	int randomPos = 1;
	if (file.Read("Event", "RandomMobPositions", randomPos))
		m_cfg.randomMobPositions = (randomPos != 0);

	int enableWaves = 1;
	if (file.Read("Event", "EnableWaveSpawning", enableWaves))
		m_cfg.enableWaveSpawning = (enableWaves != 0);

	int waveInterval = 10;
	if (file.Read("Event", "WaveIntervalSeconds", waveInterval))
		m_cfg.waveIntervalSeconds = (unsigned int)waveInterval;

	int mobsPerWave = 5;
	if (file.Read("Event", "MobsPerWave", mobsPerWave))
		m_cfg.mobsPerWave = (unsigned int)mobsPerWave;

	int spectators = 0;
	if (file.Read("Event", "SpectatorsEnabled", spectators))
		m_cfg.spectatorsEnabled = (spectators != 0);

	int verbose = 0;
	if (file.Read("Event", "VerboseLogs", verbose))
		m_cfg.verboseLogs = (verbose != 0);

	// Engagement features
	int leaderboard = 1;
	if (file.Read("Engagement", "EnableLeaderboard", leaderboard))
		m_cfg.enableLeaderboard = (leaderboard != 0);

	unsigned int mvpBonus = 0;
	if (file.Read("Engagement", "MVPBonusMudosa", mvpBonus))
		m_cfg.mvpBonusMudosa = mvpBonus;

	int killAnnounce = 1;
	if (file.Read("Engagement", "EnableKillAnnouncements", killAnnounce))
		m_cfg.enableKillAnnouncements = (killAnnounce != 0);

	int comboBonus = 1;
	if (file.Read("Engagement", "EnableComboBonus", comboBonus))
		m_cfg.enableComboBonus = (comboBonus != 0);

	unsigned int comboTimeout = 0;
	if (file.Read("Engagement", "ComboTimeoutSeconds", comboTimeout))
		m_cfg.comboTimeoutSeconds = comboTimeout;

	float comboMult = 0;
	if (file.Read("Engagement", "ComboMultiplier", comboMult))
		m_cfg.comboMultiplier = comboMult;

	int dynDiff = 1;
	if (file.Read("Engagement", "EnableDynamicDifficulty", dynDiff))
		m_cfg.enableDynamicDifficulty = (dynDiff != 0);

	float diffPerPlayer = 0;
	if (file.Read("Engagement", "DifficultyPerPlayer", diffPerPlayer))
		m_cfg.difficultyPerPlayer = diffPerPlayer;

	int timeAttack = 1;
	if (file.Read("Engagement", "EnableTimeAttack", timeAttack))
		m_cfg.enableTimeAttack = (timeAttack != 0);

	unsigned int goldTime = 0, silverTime = 0, bronzeTime = 0, timeBonus = 0;
	if (file.Read("Engagement", "TimeAttackGoldSeconds", goldTime))
		m_cfg.timeAttackGoldSeconds = goldTime;
	if (file.Read("Engagement", "TimeAttackSilverSeconds", silverTime))
		m_cfg.timeAttackSilverSeconds = silverTime;
	if (file.Read("Engagement", "TimeAttackBronzeSeconds", bronzeTime))
		m_cfg.timeAttackBronzeSeconds = bronzeTime;
	if (file.Read("Engagement", "TimeAttackBonusMudosa", timeBonus))
		m_cfg.timeAttackBonusMudosa = timeBonus;

	int partyBonus = 1;
	if (file.Read("Engagement", "EnablePartyBonus", partyBonus))
		m_cfg.enablePartyBonus = (partyBonus != 0);

	float partyMult = 0;
	if (file.Read("Engagement", "PartyBonusMultiplier", partyMult))
		m_cfg.partyBonusMultiplier = partyMult;

	int teamComp = 0;
	if (file.Read("Engagement", "EnableTeamCompetition", teamComp))
		m_cfg.enableTeamCompetition = (teamComp != 0);

	unsigned int teamBonus = 0;
	if (file.Read("Engagement", "TeamCompetitionBonusMudosa", teamBonus))
		m_cfg.teamCompetitionBonusMudosa = teamBonus;

	// Parse rounds configuration
	CNtlString roundsCsv = file.Read("Event", "Rounds");
	ERR_LOG(LOG_GENERAL, _T("[EVENT] Rounds config length: %u characters"), (unsigned)strlen(roundsCsv.c_str()));
	ParseRoundsCsv(roundsCsv);
	ERR_LOG(LOG_GENERAL, _T("[EVENT] Parsed %u rounds from config"), (unsigned)m_cfg.rounds.size());

	// Mob pool integration
	int mobPoolEn = 0;
	if (file.Read("Event", "MobPoolEnabled", mobPoolEn))
		m_cfg.mobPoolEnabled = (mobPoolEn != 0);
	CNtlString mobPoolFile = file.Read("Event", "MobPoolFile");
	if (mobPoolFile.c_str() && mobPoolFile.c_str()[0] != '\0')
		m_cfg.mobPoolFile = mobPoolFile;
	unsigned int rndDefault = 0;
	if (file.Read("Event", "RandomMobsPerRound", rndDefault))
		m_cfg.randomMobsPerRound = rndDefault;
	if (m_cfg.mobPoolEnabled)
		LoadMobPool();

	// Auto-resurrection config
	int autoResurrectEn = m_cfg.autoResurrectEnabled ? 1 : 0;
	if (file.Read("Event", "AutoResurrectEnabled", autoResurrectEn))
		m_cfg.autoResurrectEnabled = (autoResurrectEn != 0);
	unsigned int maxDeaths = 30;
	if (file.Read("Event", "MaxDeathsBeforeElimination", maxDeaths))
		m_cfg.maxDeathsBeforeElimination = maxDeaths;
	unsigned int autoResDelay = 3000;
	if (file.Read("Event", "AutoResurrectDelayMs", autoResDelay))
		m_cfg.autoResurrectDelayMs = autoResDelay;

	// Event Helpers config
	int eventHelpersEn = m_cfg.eventHelpersEnabled ? 1 : 0;
	if (file.Read("Event", "EventHelpersEnabled", eventHelpersEn))
		m_cfg.eventHelpersEnabled = (eventHelpersEn != 0);
	unsigned int helpersPerPlayer = 1;
	if (file.Read("Event", "HelpersPerPlayer", helpersPerPlayer))
		m_cfg.helpersPerPlayer = helpersPerPlayer;
	unsigned int helperMobId = 3416101;
	if (file.Read("Event", "HelperMobId", helperMobId))
		m_cfg.helperMobId = helperMobId;
	float helperFollowDist = 3.0f;
	if (file.Read("Event", "HelperFollowDistance", helperFollowDist))
		m_cfg.helperFollowDistance = helperFollowDist;
	int helperHealing = m_cfg.helperEnableHealing ? 1 : 0;
	if (file.Read("Event", "HelperEnableHealing", helperHealing))
		m_cfg.helperEnableHealing = (helperHealing != 0);
	int helperBuffing = m_cfg.helperEnableBuffing ? 1 : 0;
	if (file.Read("Event", "HelperEnableBuffing", helperBuffing))
		m_cfg.helperEnableBuffing = (helperBuffing != 0);
	int helperAttacking = m_cfg.helperEnableAttacking ? 1 : 0;
	if (file.Read("Event", "HelperEnableAttacking", helperAttacking))
		m_cfg.helperEnableAttacking = (helperAttacking != 0);

	ParseActionRewards(file);

	EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Loaded config: enabled=%d channel='%s' rounds=%u actionRewards=%u",
		(int)m_cfg.enabled, m_cfg.channelNameContains.c_str(), (unsigned)m_cfg.rounds.size(), (unsigned)m_cfg.actionRewards.size());

	return true;
}

void CEventManager::ParseWorldListCsv(const CNtlString& csv)
{
	// Format: "world1,world2,world3"
	m_cfg.worldTblidxList.clear();

	if (csv.c_str() == nullptr || csv.c_str()[0] == '\0')
		return;

	std::string str(csv.c_str());
	std::istringstream ss(str);
	std::string worldToken;

	while (std::getline(ss, worldToken, ','))
	{
		unsigned int worldId = (unsigned int)atoi(worldToken.c_str());
		if (worldId > 0)
			m_cfg.worldTblidxList.push_back(worldId);
	}

	EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Loaded %u worlds for rotation", (unsigned)m_cfg.worldTblidxList.size());
}

void CEventManager::ParseActionRewards(CNtlIniFile& file)
{
	int enabled = m_cfg.actionRewardsEnabled ? 1 : 0;
	if (file.Read("ActionRewards", "Enabled", enabled))
		m_cfg.actionRewardsEnabled = (enabled != 0);

	int requireChannel = m_cfg.actionRewardsRequireEventChannel ? 1 : 0;
	if (file.Read("ActionRewards", "RequireEventChannel", requireChannel))
		m_cfg.actionRewardsRequireEventChannel = (requireChannel != 0);

	int announce = m_cfg.actionRewardsAnnounce ? 1 : 0;
	if (file.Read("ActionRewards", "Announce", announce))
		m_cfg.actionRewardsAnnounce = (announce != 0);

	unsigned int count = 0;
	file.Read("ActionRewards", "Count", count);

	m_cfg.actionRewards.clear();
	for (unsigned int idx = 1; idx <= count; ++idx)
	{
		ActionReward reward;
		if (ParseActionRewardEntry(file, idx, reward))
			m_cfg.actionRewards.push_back(reward);
	}

	if (m_cfg.actionRewards.empty())
	{
		m_actionRewardStates.clear();
	}
	else
	{
		ResetAllActionStates();
	}
}

bool CEventManager::ParseActionRewardEntry(CNtlIniFile& file, unsigned int index, ActionReward& outReward)
{
	char key[64];

	_snprintf_s(key, _TRUNCATE, "Reward%uId", index);
	CNtlString id = file.Read("ActionRewards", key);
	if (id.c_str() == nullptr || id.c_str()[0] == '\0')
	{
		ERR_LOG(LOG_GENERAL, _T("[EVENT] ActionReward #%u missing Id"), index);
		return false;
	}
	outReward.id = id;

	_snprintf_s(key, _TRUNCATE, "Reward%uLabel", index);
	CNtlString label = file.Read("ActionRewards", key);
	if (label.c_str() && label.c_str()[0] != '\0')
		outReward.label = label;
	else
		outReward.label = id;

	_snprintf_s(key, _TRUNCATE, "Reward%uType", index);
	CNtlString typeStr = file.Read("ActionRewards", key);
	if (typeStr.c_str() && typeStr.c_str()[0] != '\0')
		outReward.type = ResolveActionType(typeStr);
	else
		outReward.type = ActionReward::Type::PLAYTIME;

	_snprintf_s(key, _TRUNCATE, "Reward%uMode", index);
	CNtlString modeStr = file.Read("ActionRewards", key);
	if (modeStr.c_str() && modeStr.c_str()[0] != '\0')
		outReward.mode = ResolveGrantMode(modeStr);
	else
		outReward.mode = ActionReward::GrantMode::ONCE;

	unsigned int value = 0;
	_snprintf_s(key, _TRUNCATE, "Reward%uEventTblidx", index);
	if (file.Read("ActionRewards", key, value))
		outReward.eventTblidx = value;

	value = 0;
	_snprintf_s(key, _TRUNCATE, "Reward%uThresholdSeconds", index);
	if (file.Read("ActionRewards", key, value))
		outReward.thresholdSeconds = value;

	value = 0;
	_snprintf_s(key, _TRUNCATE, "Reward%uCooldownSeconds", index);
	if (file.Read("ActionRewards", key, value))
		outReward.cooldownSeconds = value;

	value = 0;
	_snprintf_s(key, _TRUNCATE, "Reward%uMinLevel", index);
	if (file.Read("ActionRewards", key, value))
		outReward.minLevel = value;

	int boolValue = 0;
	_snprintf_s(key, _TRUNCATE, "Reward%uCountWhileAfk", index);
	if (file.Read("ActionRewards", key, boolValue))
		outReward.countWhileAfk = (boolValue != 0);

	if (outReward.eventTblidx == 0 || outReward.thresholdSeconds == 0)
	{
		ERR_LOG(LOG_GENERAL, _T("[EVENT] ActionReward '%S' invalid (eventTblidx=%u threshold=%u)"), outReward.id.c_str(), outReward.eventTblidx, outReward.thresholdSeconds);
		return false;
	}

	return true;
}

void CEventManager::ParseRoundsCsv(const CNtlString& csv)
{
	// Format: "Round1:mob1,mob2,mob3:item1:count1:duration;Round2:mob4,mob5:item2:count2:duration"
	// Or with loot range: "Round1:mob1,mob2:RANGE:itemId:count:duration"
	// With world: "Round1:mob1,mob2:RANGE:itemId:count:duration:worldId"
	m_cfg.rounds.clear();

	if (csv.c_str() == nullptr || csv.c_str()[0] == '\0')
		return;

	std::string str(csv.c_str());
	std::istringstream ss(str);
	std::string roundToken;

	while (std::getline(ss, roundToken, ';'))
	{
		if (roundToken.empty()) continue;

		EventRound round;
		std::istringstream roundSs(roundToken);
		std::string part;
		int partIdx = 0;

		while (std::getline(roundSs, part, ':'))
		{
			if (part.empty()) continue;

			if (partIdx == 0)
			{
				// Mobs list or RANDOM spec: "mob1,mob2" | "RANDOM" | "RANDOM5"
				std::string first = part;
				if (first.rfind("RANDOM", 0) == 0)
				{
					// Extract optional count
					unsigned int want = m_cfg.randomMobsPerRound;
					if (first.size() > 6)
					{
						std::string num = first.substr(6);
						unsigned int v = (unsigned int)atoi(num.c_str());
						if (v > 0) want = v;
					}
					// Defer actual random selection until StartNextRound by storing 0 sentinel entries if list empty now
					// We'll detect empty mobTblidxList + a stored desired random count via negative marker technique
					// Simpler: store placeholder 0 repeated 'want' times so size known for spawn radius usage
					for (unsigned int i = 0; i < want; ++i)
						round.mobTblidxList.push_back(0); // 0 means choose later
				}
				else
				{
					std::istringstream mobSs(part);
					std::string mobToken;
					while (std::getline(mobSs, mobToken, ','))
					{
						unsigned int mobId = (unsigned int)atoi(mobToken.c_str());
						if (mobId > 0)
							round.mobTblidxList.push_back(mobId);
					}
				}
			}
			else if (partIdx == 1)
			{
				// Reward type: "RANGE" or item ID
				if (part == "RANGE")
				{
					round.useLootRange = true;
				}
				else
				{
					unsigned int itemId = (unsigned int)atoi(part.c_str());
					if (itemId > 0)
						round.fixedRewards.push_back(std::make_pair(itemId, 1));
				}
			}
			else if (partIdx == 2)
			{
				// Count or loot range item ID
				if (round.useLootRange)
					round.lootRangeItemId = (unsigned int)atoi(part.c_str());
				else if (!round.fixedRewards.empty())
					round.fixedRewards.back().second = (unsigned int)atoi(part.c_str());
			}
			else if (partIdx == 3)
			{
				// Duration or loot range count
				if (round.useLootRange)
					round.lootRangeCount = (unsigned int)atoi(part.c_str());
				else
					round.durationSeconds = (unsigned int)atoi(part.c_str());
			}
			else if (partIdx == 4)
			{
				if (round.useLootRange)
				{
					// Duration for range-based loot
					round.durationSeconds = (unsigned int)atoi(part.c_str());
				}
				else
				{
					// Optional world/portal ID for fixed rewards
					// Format: "P54" for portal, "1" for world
					if (!part.empty() && part[0] == 'P')
					{
						// Portal ID (e.g., "P54")
						round.portalTblidx = (unsigned int)atoi(part.c_str() + 1);
						round.worldTblidx = 0;
					}
					else
					{
						// World ID
						round.worldTblidx = (unsigned int)atoi(part.c_str());
						round.portalTblidx = 0;
					}
				}
			}
			else if (partIdx == 5)
			{
				// Optional world/portal ID (for range-based loot format)
				// Format: "P54" for portal, "1" for world
				if (!part.empty() && part[0] == 'P')
				{
					// Portal ID (e.g., "P54")
					round.portalTblidx = (unsigned int)atoi(part.c_str() + 1);
					round.worldTblidx = 0;
				}
				else
				{
					// World ID
					round.worldTblidx = (unsigned int)atoi(part.c_str());
					round.portalTblidx = 0;
				}
			}
			else if (partIdx == 6)
			{
				// Optional minions: "minionId1,minionId2|radius|count"
				if (!part.empty())
				{
					MinionGroup minionGroup;
					std::istringstream minionSs(part);
					std::string minionPart;
					int minionPartIdx = 0;

					while (std::getline(minionSs, minionPart, '|'))
					{
						if (minionPartIdx == 0)
						{
							// Minion IDs: "minion1,minion2,minion3"
							std::istringstream minionIdSs(minionPart);
							std::string minionIdToken;
							while (std::getline(minionIdSs, minionIdToken, ','))
							{
								unsigned int minionId = (unsigned int)atoi(minionIdToken.c_str());
								if (minionId > 0)
									minionGroup.minionTblidxList.push_back(minionId);
							}
						}
						else if (minionPartIdx == 1)
						{
							// Spawn radius
							float radius = (float)atof(minionPart.c_str());
							if (radius > 0)
								minionGroup.spawnRadius = radius;
						}
						else if (minionPartIdx == 2)
						{
							// Count
							unsigned int count = (unsigned int)atoi(minionPart.c_str());
							minionGroup.count = count;
						}
						minionPartIdx++;
					}

					if (!minionGroup.minionTblidxList.empty())
					{
						round.minionGroups.push_back(minionGroup);
					}
				}
			}
			partIdx++;
		}

		m_cfg.rounds.push_back(round);
	}
}

// Load external mob pool (Mobs.txt format) similar to Arena approach but simpler: detect lines that start with "@addmob <id>"
void CEventManager::LoadMobPool()
{
	if (!m_cfg.mobPoolEnabled) return;
	std::ifstream in(m_cfg.mobPoolFile.c_str());
	if (!in.is_open())
	{
		EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] MobPool file not found: %s", m_cfg.mobPoolFile.c_str());
		return;
	}
	m_mobPool.clear();
	std::string line; unsigned int lineNo = 0; std::regex rgx("^@addmob\\s+([0-9]+)\\b");
	while (std::getline(in, line))
	{
		++lineNo;
		if (line.empty()) continue;
		std::smatch m; if (std::regex_search(line, m, rgx))
		{
			unsigned int id = (unsigned int)strtoul(m[1].str().c_str(), nullptr, 10);
			if (id != 0)
				m_mobPool.push_back(id);
		}
	}
	EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Loaded mob pool: %u entries from %s", (unsigned)m_mobPool.size(), m_cfg.mobPoolFile.c_str());
}

std::vector<unsigned int> CEventManager::GetRandomMobs(unsigned int count)
{
	std::vector<unsigned int> out;
	if (m_mobPool.empty() || count == 0)
		return out;
	if (count >= m_mobPool.size())
	{
		out = m_mobPool; // all (could shuffle)
		std::shuffle(out.begin(), out.end(), std::mt19937{ std::random_device{}() });
		out.resize(count);
		return out;
	}
	// Reservoir sample style
	std::vector<unsigned int> pool = m_mobPool;
	std::shuffle(pool.begin(), pool.end(), std::mt19937{ std::random_device{}() });
	for (unsigned int i = 0; i < count; ++i) out.push_back(pool[i]);
	return out;
}

void CEventManager::TickProcess(unsigned long dwTickDiff)
{
	// Performance optimization: Skip event logic on non-event channels
	CGameServer* app = (CGameServer*)g_pApp;
	if (m_cfg.channelNameContains.c_str()[0] != '\0' && !app->IsEventsChannel())
		return;

	if (!m_cfg.enabled)
		return;

	switch (m_state)
	{
	case State::ENROLLMENT:
		// Announce remaining time at key intervals like Arena does for rotations
		if (m_enrollmentRemainMs > 0)
		{
			unsigned int sec = (unsigned int)(m_enrollmentRemainMs / 1000);
			if (sec != m_nextEnrollmentAnnounceSec)
			{
				if (sec == 300 || sec == 180 || sec == 120 || sec == 60 || sec == 30 || sec == 10 || (sec <= 5 && sec >= 1))
				{
					wchar_t msg[128];
					swprintf_s(msg, L"[EVENT] Enrollment ends in %u second%s. Use @participate to join.", sec, sec == 1 ? L"" : L"s");
					SendNotice(msg, SERVER_TEXT_SYSNOTICE);
				}
				m_nextEnrollmentAnnounceSec = sec;
			}
		}
		if (m_enrollmentRemainMs > dwTickDiff)
		{
			m_enrollmentRemainMs -= dwTickDiff;
		}
		else
		{
			m_enrollmentRemainMs = 0;
			// Start event if we have participants
			if (m_participants.size() > 0)
			{
				// Skip initial teleport if world rotation is enabled OR first round uses portal
				bool firstRoundHasPortal = !m_cfg.rounds.empty() && m_cfg.rounds[0].portalTblidx > 0;
				if (m_cfg.worldRotationEnabled || firstRoundHasPortal)
				{
					BroadcastSystem(L"[EVENT] Enrollment closed. Get ready!");
					SendNotice(L"[EVENT] Enrollment closed. Preparing round 1...", SERVER_TEXT_SYSNOTICE);
				}
				else
				{
					BroadcastSystem(L"[EVENT] Enrollment closed. Teleporting participants...");
					SendNotice(L"[EVENT] Enrollment closed. Teleporting participants...", SERVER_TEXT_SYSNOTICE);
					TeleportParticipants();
				}
				m_state = State::PRE_ROUND;
				m_startDelayRemainMs = ToMs(m_cfg.startDelaySeconds);
			}
			else
			{
				BroadcastSystem(L"[EVENT] No participants. Event cancelled.");
				SendNotice(L"[EVENT] No participants. Event cancelled.", SERVER_TEXT_SYSNOTICE);
				m_state = State::IDLE;
				// Schedule next automatic attempt
				ScheduleAutoAfterTermination(true);
			}
		}
		break;

	case State::PRE_ROUND:
		if (m_startDelayRemainMs > dwTickDiff)
		{
			m_startDelayRemainMs -= dwTickDiff;
			// Announce remaining time similarly to Arena
			unsigned int sec = (unsigned int)(m_startDelayRemainMs / 1000);
			if (sec != m_nextPreRoundAnnounceSec)
			{
				if (sec == 30 || sec == 10 || (sec <= 5 && sec >= 1))
				{
					wchar_t msg[128];
					swprintf_s(msg, L"[EVENT] Round starts in %u second%s.", sec, sec == 1 ? L"" : L"s");
					SendNotice(msg, SERVER_TEXT_SYSNOTICE);
				}
				m_nextPreRoundAnnounceSec = sec;
			}
		}
		else
		{
			m_startDelayRemainMs = 0;
			// End countdown UI before starting
			if (m_eventWorldId)
				BroadcastCountdownToWorld(m_eventWorldId, false);
			m_countdownActive = false;
			StartNextRound();
		}
		break;

	case State::IN_ROUND:
		// Wave spawning system
		if (m_cfg.enableWaveSpawning)
		{
			if (m_waveRemainMs <= dwTickDiff)
			{
				// Time to spawn next wave
				if (m_currentRound < m_cfg.rounds.size())
				{
					const EventRound& round = m_cfg.rounds[m_currentRound];

					// Create a mini-round with MIX of random mobs for this wave
					// Use round-robin distribution to ensure variety: cycle through all mob types
					EventRound waveRound = round;
					waveRound.mobTblidxList.clear();

					if (!round.mobTblidxList.empty())
					{
						// Create a shuffled copy of the mob list for better randomness
						std::vector<TBLIDX> shuffledMobs = round.mobTblidxList;

						// Shuffle the list once
						for (size_t i = shuffledMobs.size() - 1; i > 0; i--)
						{
							size_t j = RandomRange(0, (int)i);
							std::swap(shuffledMobs[i], shuffledMobs[j]);
						}

						// Fill the wave by cycling through the shuffled list
						// This ensures even distribution of all mob types
						for (unsigned int i = 0; i < m_cfg.mobsPerWave; i++)
						{
							unsigned int mobIdx = i % shuffledMobs.size();
							waveRound.mobTblidxList.push_back(shuffledMobs[mobIdx]);
						}
					}

					// Spawn the wave (with fallback if some mobs fail)
					if (!waveRound.mobTblidxList.empty())
					{
						SpawnRoundMobs(waveRound);
						m_waveCount++;

						wchar_t waveMsg[128];
						swprintf_s(waveMsg, L"[EVENT] Wave %u spawned! %u enemies incoming!",
							m_waveCount, (unsigned)waveRound.mobTblidxList.size());
						BroadcastSystem(waveMsg);
					}
				}

				// Reset wave timer
				m_waveRemainMs = ToMs(m_cfg.waveIntervalSeconds);
			}
			else
			{
				m_waveRemainMs -= dwTickDiff;
			}
		}

		// Update round timer
		if (m_roundTimerActive)
		{
			if (m_roundRemainMs > dwTickDiff)
			{
				m_roundRemainMs -= dwTickDiff;

				// Announce remaining time at key intervals
				unsigned int sec = (unsigned int)(m_roundRemainMs / 1000);
				if (sec != m_nextRoundAnnounceSec)
				{
					// Announce every minute (60s intervals), plus important milestones
					bool shouldAnnounce = false;

					// Every minute (60 second intervals)
					if (sec >= 60 && sec % 60 == 0)
						shouldAnnounce = true;
					// Important milestones under 1 minute
					else if (sec == 30 || sec == 10 || (sec <= 5 && sec >= 1))
						shouldAnnounce = true;

					if (shouldAnnounce)
					{
						wchar_t msg[128];
						if (sec >= 60)
						{
							unsigned int minutes = sec / 60;
							swprintf_s(msg, L"[EVENT] Round %u/%u - %u minute%s remaining!",
								m_currentRound + 1, (unsigned)m_cfg.rounds.size(), minutes, minutes == 1 ? L"" : L"s");
						}
						else
						{
							swprintf_s(msg, L"[EVENT] Round %u/%u - %u second%s remaining!",
								m_currentRound + 1, (unsigned)m_cfg.rounds.size(), sec, sec == 1 ? L"" : L"s");
						}
						BroadcastSystem(msg);
					}
					m_nextRoundAnnounceSec = sec;
				}
			}
			else
			{
				m_roundRemainMs = 0;
				// Round timeout
				BroadcastSystem(L"[EVENT] Round timed out!");
				CompleteCurrentRound();
			}
		}

		// Auto-resurrection check
		if (m_cfg.autoResurrectEnabled)
		{
			unsigned long currentTime = GetTickCount();
			for (auto it = m_playerDeathTime.begin(); it != m_playerDeathTime.end(); )
			{
				unsigned int charId = it->first;
				unsigned long deathTime = it->second;

				// Check if enough time has passed for resurrection
				if (currentTime - deathTime >= m_cfg.autoResurrectDelayMs)
				{
					CPlayer* pPlayer = g_pObjectManager->FindByChar((CHARACTERID)charId);
					if (pPlayer && pPlayer->IsInitialized() && pPlayer->IsFainting())
					{
						// Find a safe spawn position (use event spawn or near a random participant)
						CNtlVector spawnPos;
						float spawnX, spawnY, spawnZ;
						GetSpawnPosForRound(m_currentRound, spawnX, spawnY, spawnZ);
						spawnPos.x = spawnX;
						spawnPos.y = spawnY;
						spawnPos.z = spawnZ;

						// Try to spawn near a random alive player instead
						std::vector<CNtlVector> alivePlayerPositions;
						for (unsigned int pCharId : m_participants)
						{
							if (pCharId == charId) continue; // skip dead player
							CPlayer* pAlive = g_pObjectManager->FindByChar((CHARACTERID)pCharId);
							if (pAlive && pAlive->IsInitialized() && !pAlive->IsFainting() &&
								pAlive->GetWorldID() == (WORLDID)m_eventWorldId)
							{
								alivePlayerPositions.push_back(pAlive->GetCurLoc());
							}
						}
						if (!alivePlayerPositions.empty())
						{
							unsigned int randomIdx = RandomRange(0, (int)alivePlayerPositions.size() - 1);
							spawnPos = alivePlayerPositions[randomIdx];
							// Add small random offset to avoid exact overlap
							float angle = RandomRangeF(0.0f, 6.28318530718f);
							float distance = RandomRangeF(2.0f, 5.0f);
							spawnPos.x += cosf(angle) * distance;
							spawnPos.z += sinf(angle) * distance;
						}

						// Resurrect player
						pPlayer->Revival(spawnPos, pPlayer->GetWorldID(), REVIVAL_TYPE_RESCUED);

						// Force player to standing state to prevent being stuck
						pPlayer->SendCharStateStanding();

						EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Auto-resurrected player %s at (%.2f,%.2f,%.2f)",
							pPlayer->GetCharName(), spawnPos.x, spawnPos.y, spawnPos.z);

						wchar_t msg[128];
						swprintf_s(msg, L"[EVENT] You have been resurrected!");
						SendSystemTo(pPlayer, msg);
					}

					// Remove from death time tracking
					it = m_playerDeathTime.erase(it);
				}
				else
				{
					++it;
				}
			}
		}

		// Note: Wave spawning mode doesn't check for round completion based on mob kills
		// Rounds end only when timer expires
		if (!m_cfg.enableWaveSpawning)
		{
			// Check if all mobs killed (traditional mode only)
			CheckRoundCompletion();
		}
		break;

	case State::INTERMISSION:
		// Brief pause between rounds
		if (m_startDelayRemainMs > dwTickDiff)
		{
			m_startDelayRemainMs -= dwTickDiff;
		}
		else
		{
			m_startDelayRemainMs = 0;
			if (m_currentRound < m_cfg.rounds.size())
			{
				StartNextRound();
			}
			else
			{
				// All rounds complete
				m_state = State::COMPLETE;
				m_postEventTeleportRemainMs = m_cfg.postEventTeleportDelayMs;
				BroadcastSystem(L"[EVENT] Event completed! Congratulations!");
				SendNotice(L"[EVENT] Event completed! Congratulations!", SERVER_TEXT_SYSNOTICE);
			}
		}
		break;

	case State::COMPLETE:
		if (m_postEventTeleportRemainMs > dwTickDiff)
		{
			m_postEventTeleportRemainMs -= dwTickDiff;
		}
		else
		{
			m_postEventTeleportRemainMs = 0;
			// Despawn all helpers before teleporting players back
			DespawnEventHelpers();
			PostEventTeleportAll();
			m_state = State::IDLE;
			m_participants.clear();
			m_spectators.clear();
			m_spawnedMobs.clear();
			m_killedMobs.clear();
			m_prevLoc.clear();
			m_currentRound = 0;
			// Schedule next automatic event (restart or interval-based)
			ScheduleAutoAfterTermination(false);
		}
		break;

	default:
		break;
	}

	// Always tick automation system (handles auto-scheduling and restarts)
	AutomationTick(dwTickDiff);
}

void CEventManager::AutomationTick(unsigned long dwTickDiff)
{
	// Performance optimization: Skip automation on non-event channels
	CGameServer* app = (CGameServer*)g_pApp;
	if (m_cfg.channelNameContains.c_str()[0] != '\0' && !app->IsEventsChannel())
		return;

	if (!m_cfg.enabled || !m_cfg.autoEnabled)
		return;

	if (!IsChannelValid())
		return;

	// Initialize automation if currently off (like Arena does)
	if (m_autoState == AutoState::OFF)
	{
		m_autoState = AutoState::WAIT_NEXT;
		// Calculate prep time before opening enrollment
		unsigned int firstDelay = m_cfg.autoInitialDelaySeconds;
		unsigned int prepWaitSec = 0;
		if (firstDelay > m_cfg.enrollmentSeconds)
			prepWaitSec = firstDelay - m_cfg.enrollmentSeconds;
		else
			prepWaitSec = 0; // open enrollment immediately
		m_autoRemainMs = ToMs(prepWaitSec);
		EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT][AUTO] Initialized: firstDelay=%u enrollmentSeconds=%u prepWaitSec=%u",
			firstDelay, m_cfg.enrollmentSeconds, prepWaitSec);
		return;
	}

	switch (m_autoState)
	{
	case AutoState::WAIT_NEXT:
		if (m_autoRemainMs > dwTickDiff)
		{
			m_autoRemainMs -= dwTickDiff;
		}
		else
		{
			m_autoRemainMs = 0;
			// Don't interrupt an active event; defer until it returns to IDLE
			if (m_state != State::IDLE)
			{
				m_autoRemainMs = ToMs(5); // Check again in 5 seconds
				return;
			}
			// Start enrollment
			Start();
			m_autoState = AutoState::ENROLLMENT_OPEN;
			EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT][AUTO] Enrollment opened");
		}
		break;

	case AutoState::ENROLLMENT_OPEN:
		if (m_state == State::IDLE)
		{
			// Event finished
			if (m_cfg.autoRestartOnComplete)
			{
				// Auto-restart enabled, wait for restart delay
				m_autoState = AutoState::WAIT_RESTART;
				m_autoRemainMs = ToMs(m_cfg.autoRestartDelaySeconds);
				EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Auto-restart scheduled in %u seconds", m_cfg.autoRestartDelaySeconds);
			}
			else
			{
				// Schedule next event
				m_autoState = AutoState::WAIT_NEXT;
				m_autoRemainMs = ToMs(m_cfg.autoIntervalSeconds);
			}
		}
		break;

	case AutoState::WAIT_RESTART:
		if (m_autoRemainMs > dwTickDiff)
		{
			m_autoRemainMs -= dwTickDiff;
		}
		else
		{
			m_autoRemainMs = 0;
			if (m_state == State::IDLE)
			{
				// Restart event immediately
				EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Auto-restarting event");
				Start();
				m_autoState = AutoState::ENROLLMENT_OPEN;
			}
			else
			{
				// Event somehow still running, check again
				m_autoRemainMs = ToMs(60);
			}
		}
		break;
	}
}

void CEventManager::Start()
{
	if (!IsChannelValid())
	{
		EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Cannot start: invalid channel");
		return;
	}

	if (m_state != State::IDLE)
	{
		ERR_LOG(LOG_GENERAL, _T("[EVENT] Cannot start: already running (state=%d)"), (int)m_state);
		return;
	}

	m_state = State::ENROLLMENT;
	m_enrollmentRemainMs = ToMs(m_cfg.enrollmentSeconds);
	m_nextEnrollmentAnnounceSec = (unsigned int)(m_enrollmentRemainMs / 1000);
	m_currentRound = 0;
	m_participants.clear();
	m_spectators.clear();
	m_killedMobs.clear();

    EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Enrollment opened immediately on startup? initialDelay=%u autoEnabled=%d state=%d",
        m_cfg.autoInitialDelaySeconds, (int)m_cfg.autoEnabled, (int)m_autoState);

	wchar_t msg[256];
	if (m_cfg.requireParticipateCommand)
	{
		swprintf_s(msg, L"[EVENT] Event enrollment opened! Type @participate to join. Time: %u seconds", m_cfg.enrollmentSeconds);
	}
	else
	{
		swprintf_s(msg, L"[EVENT] Event enrollment opened! Time: %u seconds", m_cfg.enrollmentSeconds);
	}
	BroadcastSystem(msg);
	SendNotice(msg, SERVER_TEXT_SYSNOTICE);
}

void CEventManager::Stop(bool abort)
{
	if (m_state == State::IDLE)
		return;

	if (abort)
		BroadcastSystem(L"[EVENT] Event has been aborted by GM.");
	else
		BroadcastSystem(L"[EVENT] Event stopped.");

	// Despawn all helpers
	DespawnEventHelpers();

	// Teleport everyone back
	PostEventTeleportAll();

	m_state = State::IDLE;
	m_participants.clear();
	m_spectators.clear();
	m_spawnedMobs.clear();
	m_killedMobs.clear();
	m_prevLoc.clear();
	m_currentRound = 0;
	// Clear auto-resurrection tracking
	m_playerDeathCount.clear();
	m_playerDeathTime.clear();
	m_eliminatedPlayers.clear();

	ScheduleAutoAfterTermination(false);
}

void CEventManager::StartNextRound()
{
	if (m_currentRound >= m_cfg.rounds.size())
	{
		m_state = State::COMPLETE;
		return;
	}

	EventRound& round = m_cfg.rounds[m_currentRound];

	// Resolve any RANDOM placeholders (mob id 0) before spawning
	if (m_cfg.mobPoolEnabled)
	{
		unsigned int unresolved = 0;
		for (auto id : round.mobTblidxList) if (id == 0) ++unresolved;
		if (unresolved > 0)
		{
			std::vector<unsigned int> rnd = GetRandomMobs(unresolved);
			if (rnd.size() == unresolved)
			{
				// Replace zeros sequentially
				unsigned int idx = 0;
				for (auto& id : round.mobTblidxList) if (id == 0) id = rnd[idx++];
				EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Resolved %u RANDOM mob slots for round %u", unresolved, m_currentRound + 1);
			}
			else
			{
				ERR_LOG(LOG_GENERAL, _T("[EVENT] Could not resolve enough random mobs: needed %u pool=%u"), unresolved, (unsigned)m_mobPool.size());
			}
		}
	}
	m_killedMobs.clear();
	m_roundContribution.clear(); // Reset damage tracking for new round

	// Reset engagement tracking
	m_playerKillCount.clear();
	m_playerComboCount.clear();
	m_lastKillTime.clear();
	m_roundStartTime = GetTickCount(); // Time attack timer

	// Team competition setup
	if (m_cfg.enableTeamCompetition)
	{
		AssignTeams();
		m_teamRedDamage = 0;
		m_teamBlueDamage = 0;
		m_teamRedKills = 0;
		m_teamBlueKills = 0;
	}

	// Teleport participants to portal or world location
	if (round.portalTblidx > 0)
	{
		// Use portal location (like @teleport command)
		TeleportParticipantsToPortal(round.portalTblidx);
	}
	else if (m_cfg.worldRotationEnabled || round.worldTblidx > 0)
	{
		// Use world location (original behavior)
		unsigned int worldTblidx = GetWorldForRound(m_currentRound);
		float x, y, z;
		GetSpawnPosForRound(m_currentRound, x, y, z);
		TeleportParticipantsToWorld(worldTblidx, x, y, z);
	}

	wchar_t msg[256];
	swprintf_s(msg, L"[EVENT] Round %u/%u starting! Kill all mobs to proceed.",
		m_currentRound + 1, (unsigned)m_cfg.rounds.size());
	BroadcastSystem(msg);
	SendNotice(msg, SERVER_TEXT_SYSNOTICE);

	m_state = State::IN_ROUND;

	// Spawn helpers for first round only (they persist across rounds)
	if (m_currentRound == 0)
	{
		SpawnEventHelpers();
	}

	// Initialize wave spawning
	if (m_cfg.enableWaveSpawning)
	{
		m_waveCount = 0;
		// Delay first wave spawn by 3 seconds to allow players to finish teleporting and loading
		m_waveRemainMs = 3000; // 3 seconds delay before first wave
		EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Wave spawning enabled: %u mobs every %u seconds (first wave in 3s)",
			m_cfg.mobsPerWave, m_cfg.waveIntervalSeconds);
	}
	else
	{
		// Traditional single spawn at round start
		SpawnRoundMobs(round);
	}

	if (round.durationSeconds > 0)
	{
		StartRoundTimer(round.durationSeconds);
		// Reset announcement tracker for this round
		// Set to value higher than duration so first announcement triggers immediately
		m_nextRoundAnnounceSec = (unsigned int)round.durationSeconds + 1;
	}

	if (round.portalTblidx > 0)
	{
		EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Round %u started: %u mobs, %u seconds, portal=%u",
			m_currentRound + 1, (unsigned)round.mobTblidxList.size(), round.durationSeconds, round.portalTblidx);
	}
	else
	{
		unsigned int worldTblidx = GetWorldForRound(m_currentRound);
		EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Round %u started: %u mobs, %u seconds, world=%u",
			m_currentRound + 1, (unsigned)round.mobTblidxList.size(), round.durationSeconds, worldTblidx);
	}
}

void CEventManager::CompleteCurrentRound()
{
	if (m_currentRound >= m_cfg.rounds.size())
		return;

	const EventRound& round = m_cfg.rounds[m_currentRound];

	StopRoundTimer();

	// Check time attack bonus
	CheckTimeAttackBonus();

	// Team competition results
	if (m_cfg.enableTeamCompetition)
	{
		ShowTeamScores();
		AwardTeamBonuses();
	}

	// Show leaderboard and award MVP bonuses
	if (m_cfg.enableLeaderboard)
	{
		ShowLeaderboard();
		AwardMVPBonuses();
	}

	wchar_t msg[256];
	swprintf_s(msg, L"[EVENT] Round %u completed!", m_currentRound + 1);
	BroadcastSystem(msg);
	SendNotice(msg, SERVER_TEXT_SYSNOTICE);

	// Award rewards
	AwardRoundRewards(round);

	// Despawn all remaining event mobs before moving to next round
	DespawnAllEventMobs();

	m_currentRound++;

	if (m_currentRound < m_cfg.rounds.size())
	{
		m_state = State::INTERMISSION;
		m_startDelayRemainMs = ToMs(m_cfg.intermissionSeconds);
		m_nextPreRoundAnnounceSec = (unsigned int)(m_startDelayRemainMs / 1000);
		// Start a brief countdown UI for intermission
		if (m_eventWorldId)
		{
			BroadcastCountdownToWorld(m_eventWorldId, true);
			m_countdownActive = true;
		}
	}
	else
	{
		// All rounds complete
		m_state = State::COMPLETE;
		m_postEventTeleportRemainMs = m_cfg.postEventTeleportDelayMs;

		// Award completion bonus only to participants who fought in the last round
		if (m_cfg.mudosaEventComplete > 0)
		{
			unsigned int bonusAwarded = 0;
			for (unsigned int charId : m_participants)
			{
				// Check if they participated in the last round
				auto it = m_roundContribution.find(charId);
				if (it != m_roundContribution.end() && it->second > 0)
				{
					CPlayer* pPlayer = g_pObjectManager->FindByChar((CHARACTERID)charId);
					if (pPlayer && pPlayer->IsInitialized())
					{
						// UpdateMudosaPoints expects an absolute value; add to current total
						pPlayer->UpdateMudosaPoints(pPlayer->GetMudosaPoints() + m_cfg.mudosaEventComplete, true);
						bonusAwarded++;
					}
				}
			}
			EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Awarded completion bonus to %u/%u participants",
				bonusAwarded, (unsigned)m_participants.size());
		}

		BroadcastSystem(L"[EVENT] All rounds completed! Congratulations!");
	}
}

void CEventManager::SpawnRoundMobs(const EventRound& round)
{
	// Use m_eventWorldId directly if already set (from portal or world teleport)
	// Otherwise fall back to GetWorldForRound
	CWorld* pWorld = nullptr;
	if (m_eventWorldId != 0)
	{
		CGameServer* app = (CGameServer*)g_pApp;
		pWorld = app->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)m_eventWorldId);
	}

	if (!pWorld)
	{
		// Fallback to GetWorldForRound if m_eventWorldId not set or world not found
		unsigned int worldTblidx = GetWorldForRound(m_currentRound);
		pWorld = GetOrCreateWorld(worldTblidx);
		if (!pWorld)
		{
			ERR_LOG(LOG_GENERAL, _T("[EVENT] SpawnRoundMobs: could not resolve world (eventWorldId=%u)"), m_eventWorldId);
			return;
		}
	}

	// Reset world difficulty phase to 0 at the start of each round
	// This ensures bosses with autophase start at base phase and scale with HP
	pWorld->SetDifficultyPhase(0);

	// Get spawn position for current round (used as fallback)
	float spawnX, spawnY, spawnZ;
	GetSpawnPosForRound(m_currentRound, spawnX, spawnY, spawnZ);
	CNtlVector spawnCenter(spawnX, spawnY, spawnZ);

	// Collect all participant positions for spawning around players
	std::vector<CNtlVector> playerPositions;
	for (unsigned int charId : m_participants)
	{
		CPlayer* pPlayer = g_pObjectManager->FindByChar((CHARACTERID)charId);
		if (pPlayer && pPlayer->IsInitialized() && pPlayer->GetWorldID() == (WORLDID)pWorld->GetID())
		{
			playerPositions.push_back(pPlayer->GetCurLoc());
		}
	}

	unsigned int spawnAttempts = 0;
	unsigned int spawnSuccess = 0;
	for (unsigned int mobTblidx : round.mobTblidxList)
	{
		// Choose spawn center: random player position if available, otherwise round spawn center
		CNtlVector spawnPos = spawnCenter;
		if (!playerPositions.empty())
		{
			unsigned int randomPlayerIdx = RandomRange(0, (int)playerPositions.size() - 1);
			spawnPos = playerPositions[randomPlayerIdx];
		}

		// Randomize position around chosen center
		if (m_cfg.randomMobPositions && m_cfg.mobSpawnRadius > 0)
		{
			float angle = RandomRangeF(0.0f, 6.28318530718f); // 2*PI
			float distance = RandomRangeF(0.0f, m_cfg.mobSpawnRadius);
			spawnPos.x += cosf(angle) * distance;
			spawnPos.z += sinf(angle) * distance;
		}

		CNtlVector dir(0.0f, 0.0f, 0.0f);
		CMonster* pMob = pWorld->Add_Monster(mobTblidx, spawnPos, dir, 0xFF);
		spawnAttempts++;

		if (pMob)
		{
			m_spawnedMobs.push_back(pMob->GetID());
			spawnSuccess++;

			// Apply CustomDropEvent modifications if enabled
			if (round.useCustomDropMobs && g_pCustomDropEvent && g_pCustomDropEvent->m_bOn)
			{
				g_pCustomDropEvent->ApplyModifiers(pMob);
				g_pCustomDropEvent->ApplyBuffs(pMob);
				g_pCustomDropEvent->ApplyTitles(pMob);
				g_pCustomDropEvent->ApplyVisuals(pMob);
			}

			EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Spawned boss mob tblidx=%u handle=%u at (%.2f,%.2f,%.2f)",
				mobTblidx, pMob->GetID(), spawnPos.x, spawnPos.y, spawnPos.z);

			// Spawn minions around this boss
			for (const MinionGroup& minionGroup : round.minionGroups)
			{
				SpawnMinionsAroundBoss(spawnPos, minionGroup, m_eventWorldId);
			}
		}
		else
		{
			ERR_LOG(LOG_GENERAL, _T("[EVENT] SpawnRoundMobs: failed to spawn mob tblidx=%u in world %u"), mobTblidx, (unsigned)pWorld->GetID());
		}
	}

	// Log spawn results
	EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] SpawnRoundMobs: spawned %u/%u mobs successfully in round %u",
		spawnSuccess, spawnAttempts, m_currentRound + 1);

	// Only force round completion if we're NOT using wave spawning and ALL spawns failed
	// With wave spawning, it's OK if some mob IDs are invalid - just keep trying with valid ones
	if (spawnAttempts > 0 && spawnSuccess == 0 && !m_cfg.enableWaveSpawning)
	{
		ERR_LOG(LOG_GENERAL, _T("[EVENT] SpawnRoundMobs: all %u spawn attempts failed in round %u (world %u). Forcing round completion."), spawnAttempts, m_currentRound + 1, (unsigned)pWorld->GetID());
		CompleteCurrentRound();
		return;
	}
}

void CEventManager::SpawnMinionsAroundBoss(const CNtlVector& bossPos, const MinionGroup& minionGroup, unsigned int worldId)
{
	// Resolve world by ID via WorldManager
	CGameServer* app = (CGameServer*)g_pApp;
	CWorld* pWorld = app->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)worldId);
	if (!pWorld)
		return;

	unsigned int totalMinions = minionGroup.count;
	if (totalMinions == 0)
	{
		// Spawn one of each type
		totalMinions = (unsigned int)minionGroup.minionTblidxList.size();
	}

	unsigned int spawned = 0;
	for (unsigned int i = 0; i < totalMinions; i++)
	{
		// Pick a minion type (cycle through list or random)
		unsigned int minionIdx = i % minionGroup.minionTblidxList.size();
		unsigned int minionTblidx = minionGroup.minionTblidxList[minionIdx];

		// Calculate spawn position around boss
		float angle = RandomRangeF(0.0f, 6.28318530718f); // 2*PI
		float distance = RandomRangeF(minionGroup.spawnRadius * 0.3f, minionGroup.spawnRadius);

		CNtlVector minionPos = bossPos;
		minionPos.x += cosf(angle) * distance;
		minionPos.z += sinf(angle) * distance;

		CNtlVector dir(0.0f, 0.0f, 0.0f);
		CMonster* pMinion = pWorld->Add_Monster(minionTblidx, minionPos, dir, 0xFF);

		if (pMinion)
		{
			m_spawnedMobs.push_back(pMinion->GetID());

			// Apply CustomDropEvent modifications if enabled
			if (g_pCustomDropEvent && g_pCustomDropEvent->m_bOn)
			{
				g_pCustomDropEvent->ApplyModifiers(pMinion);
				g_pCustomDropEvent->ApplyBuffs(pMinion);
				g_pCustomDropEvent->ApplyTitles(pMinion);
				g_pCustomDropEvent->ApplyVisuals(pMinion);
			}

			spawned++;
			EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Spawned minion tblidx=%u handle=%u around boss at (%.2f,%.2f,%.2f)",
				minionTblidx, pMinion->GetID(), minionPos.x, minionPos.y, minionPos.z);
		}
	}

	if (spawned > 0)
	{
		EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Spawned %u minions around boss (radius=%.1f)", spawned, minionGroup.spawnRadius);
	}
}

void CEventManager::AwardRoundRewards(const EventRound& round)
{
	// Only award to participants who contributed damage this round (prevent AFK farming)
	std::unordered_set<unsigned int> activeParticipants;
	unsigned int minDamageRequired = 1; // Must have dealt at least 1 damage

	for (unsigned int charId : m_participants)
	{
		auto it = m_roundContribution.find(charId);
		if (it != m_roundContribution.end() && it->second >= minDamageRequired)
		{
			activeParticipants.insert(charId);
		}
		else
		{
			CPlayer* pPlayer = g_pObjectManager->GetPC(charId);
			if (pPlayer && pPlayer->IsInitialized())
			{
				SendSystemTo(pPlayer, L"[EVENT] No rewards: you did not participate in combat this round.");
			}
			EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Player charId=%u skipped for rewards: damage=%u (min=%u)",
				charId, (it != m_roundContribution.end() ? it->second : 0), minDamageRequired);
		}
	}

	if (round.useLootRange && round.lootRangeItemId > 0)
	{
		// Create loot in range (all can pick up if they participated)
		float x, y, z;
		GetSpawnPosForRound(m_currentRound, x, y, z);
		CNtlVector center(x, y, z);
		CreateLootInRange(round.lootRangeItemId, round.lootRangeCount, center, m_cfg.mobSpawnRadius);
	}
	else
	{
		// Award fixed rewards only to active participants
		unsigned int rewarded = 0;
		for (unsigned int charId : activeParticipants)
		{
			CPlayer* pPlayer = g_pObjectManager->FindByChar((CHARACTERID)charId);
			if (pPlayer && pPlayer->IsInitialized())
			{
				for (const auto& reward : round.fixedRewards)
				{
					g_pItemManager->CreateItem(pPlayer, (TBLIDX)reward.first, (BYTE)reward.second);
				}

				if (m_cfg.mudosaPerRound > 0)
				{
					// Apply party bonus if applicable
					float partyMultiplier = GetPartyBonusMultiplier(pPlayer);
					unsigned int finalMudosa = (unsigned int)(m_cfg.mudosaPerRound * partyMultiplier);
					pPlayer->UpdateMudosaPoints(pPlayer->GetMudosaPoints() + finalMudosa, true);

					if (partyMultiplier > 1.0f)
					{
						wchar_t msg[128];
						swprintf_s(msg, L"[EVENT] Party Bonus! +%.0f%% Mudosa!", (partyMultiplier - 1.0f) * 100.0f);
						SendSystemTo(pPlayer, msg);
					}
				}
				rewarded++;
			}
		}
		EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Rewarded %u/%u participants (active damage dealers only)",
			rewarded, (unsigned)m_participants.size());
	}
}

void CEventManager::CreateLootInRange(unsigned int itemId, unsigned int count, const CNtlVector& center, float radius)
{
	if (count > 100) count = 100; // Safety limit

	for (unsigned int i = 0; i < count; i++)
	{
		sVECTOR3 vec;
		vec.x = center.x + RandomRangeF(-radius, radius);
		vec.y = center.y;
		vec.z = center.z + RandomRangeF(-radius, radius);

		CItemDrop* pDrop = nullptr;
		if (g_pItemManager->IsValidSingleDropIdx(itemId))
		{
			pDrop = g_pItemManager->CreateSingleDrop(100.0f, itemId);
		}

		if (pDrop)
		{
			pDrop->AddToGround(m_eventWorldId, vec);
		}
	}

	EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Created %u loot items (id=%u) in range", count, itemId);
}

void CEventManager::CheckRoundCompletion()
{
	if (m_state != State::IN_ROUND)
		return;

	// Check if all spawned mobs are killed
	bool allKilled = true;
	for (HOBJECT mobHandle : m_spawnedMobs)
	{
		if (m_killedMobs.find(mobHandle) == m_killedMobs.end())
		{
			// Check if mob still exists
			CCharacter* pMob = (CCharacter*)g_pObjectManager->GetObjectA(mobHandle);
			if (pMob && !pMob->IsFainting())
			{
				allKilled = false;
				break;
			}
		}
	}

	// With wave spawning enabled, don't end round early - let it run for full duration
	// New waves will continue spawning until time expires
	if (allKilled && m_spawnedMobs.size() > 0 && !m_cfg.enableWaveSpawning)
	{
		CompleteCurrentRound();
	}
}

void CEventManager::OnMobKilled(unsigned int mobHandle)
{
	// Check if this is one of our event mobs
	auto it = std::find(m_spawnedMobs.begin(), m_spawnedMobs.end(), mobHandle);
	if (it != m_spawnedMobs.end())
	{
		m_killedMobs.insert(mobHandle);
		EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Mob killed: handle=%u (%u/%u)",
			mobHandle, (unsigned)m_killedMobs.size(), (unsigned)m_spawnedMobs.size());
	}
}

void CEventManager::OnPlayerDamageEventMob(unsigned int charId, unsigned int damage)
{
	// Only track if player is participant and event is active
	if (m_state != State::IN_ROUND)
		return;

	if (!IsParticipantId(charId))
		return;

	// Accumulate damage for this round
	m_roundContribution[charId] += damage;

	// Track team damage if team competition enabled
	if (m_cfg.enableTeamCompetition)
	{
		auto teamIt = m_playerTeam.find(charId);
		if (teamIt != m_playerTeam.end())
		{
			if (teamIt->second == Team::RED)
				m_teamRedDamage += damage;
			else if (teamIt->second == Team::BLUE)
				m_teamBlueDamage += damage;
		}
	}
}

bool CEventManager::AddParticipant(CPlayer* pPlayer)
{
	if (!pPlayer || !pPlayer->IsInitialized())
		return false;

	if (m_state != State::ENROLLMENT)
	{
		SendSystemTo(pPlayer, L"[EVENT] Enrollment is not open.");
		return false;
	}

	unsigned int charId = pPlayer->GetCharID();
	if (m_participants.find(charId) != m_participants.end())
	{
		SendSystemTo(pPlayer, L"[EVENT] You are already enrolled.");
		return false;
	}

	m_participants.insert(charId);

	wchar_t msg[256];
	swprintf_s(msg, L"[EVENT] %s joined the event! (%u participants)",
		pPlayer->GetCharName(), (unsigned)m_participants.size());
	BroadcastSystem(msg);
	SendNotice(msg, SERVER_TEXT_SYSNOTICE);

	return true;
}

bool CEventManager::AddSpectator(CPlayer* pPlayer)
{
	if (!pPlayer || !pPlayer->IsInitialized())
		return false;

	if (!m_cfg.spectatorsEnabled)
	{
		SendSystemTo(pPlayer, L"[EVENT] Spectators are not enabled.");
		return false;
	}

	unsigned int charId = pPlayer->GetCharID();
	m_spectators.insert(charId);
	return true;
}

bool CEventManager::RemoveParticipant(CPlayer* pPlayer)
{
	if (!pPlayer)
		return false;

	unsigned int charId = pPlayer->GetCharID();
	m_participants.erase(charId);
	m_spectators.erase(charId);
	return true;
}

bool CEventManager::IsParticipant(CPlayer* pPlayer) const
{
	if (!pPlayer)
		return false;
	return IsParticipantId(pPlayer->GetCharID());
}

bool CEventManager::IsParticipantId(unsigned int charId) const
{
	return m_participants.find(charId) != m_participants.end();
}

void CEventManager::TeleportParticipants()
{
	// Save previous locations
	EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] TeleportParticipants: count=%u to worldTblidx=%u", (unsigned)m_participants.size(), (unsigned)m_cfg.eventWorldTblidx);
	for (unsigned int charId : m_participants)
	{
		// m_participants stores CHARACTERID, so resolve via FindByChar instead of GetPC (which expects handle)
		CPlayer* pPlayer = g_pObjectManager->FindByChar((CHARACTERID)charId);
		if (pPlayer && pPlayer->IsInitialized())
		{
			PrevLoc loc;
			loc.worldId = pPlayer->GetWorldID();
			loc.loc = pPlayer->GetCurLoc();
			loc.dir = pPlayer->GetCurDir();
			m_prevLoc[charId] = loc;
		}
	}

	// Teleport to event world (coords from config are optional override; 0,0,0 means use world default)
	TeleportParticipantsToWorld(m_cfg.eventWorldTblidx, m_cfg.teleportPosX, m_cfg.teleportPosY, m_cfg.teleportPosZ);
}

void CEventManager::TeleportParticipantsToWorld(unsigned int worldTblidx, float x, float y, float z)
{
	// Get world table data first to determine spawn position
	CGameServer* app = (CGameServer*)g_pApp;
	if (!app)
	{
		ERR_LOG(LOG_GENERAL, _T("%s"), _T("[EVENT] TeleportParticipantsToWorld failed: app is null"));
		return;
	}

	sWORLD_TBLDAT* pWorldTbldat = (sWORLD_TBLDAT*)g_pTableContainer->GetWorldTable()->FindData((TBLIDX)worldTblidx);
	if (!pWorldTbldat)
	{
		ERR_LOG(LOG_GENERAL, _T("[EVENT] TeleportParticipantsToWorld: world tblidx not found %u"), worldTblidx);
		return;
	}

	// Determine destination: world start or override
	CNtlVector destLoc;
	bool haveDest = ComputeDestForWorld(worldTblidx, x, y, z, destLoc);
	bool usingConfigOverride = !(x == 0.f && y == 0.f && z == 0.f);

	// Validate destination (avoid absurd values observed in logs) and fall back if needed
	auto IsBadCoord = [](float f) -> bool { return !std::isfinite(f) || fabs(f) > 1000000.f; };
	if (IsBadCoord(destLoc.x) || IsBadCoord(destLoc.y) || IsBadCoord(destLoc.z))
	{
		ERR_LOG(LOG_GENERAL, _T("[EVENT] TeleportParticipantsToWorld: invalid destination (%.2f,%.2f,%.2f) for world %u. Using world start."), destLoc.x, destLoc.y, destLoc.z, worldTblidx);
		ComputeDestForWorld(worldTblidx, 0.f, 0.f, 0.f, destLoc);
	}

	EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] TeleportParticipantsToWorld: worldTblidx=%u worldStart=(%.2f,%.2f,%.2f) configOverride=%d finalPos=(%.2f,%.2f,%.2f) participants=%u worldIdCached=%u",
		worldTblidx, pWorldTbldat->vStart1Loc.x, pWorldTbldat->vStart1Loc.y, pWorldTbldat->vStart1Loc.z,
		usingConfigOverride, destLoc.x, destLoc.y, destLoc.z, (unsigned)m_participants.size(), m_eventWorldId);

	// Create or find world instance (handle static worlds like Arena)
	CWorld* pWorld = nullptr;
	if (m_eventWorldId)
	{
		pWorld = app->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)m_eventWorldId);
		if (pWorld)
		{
			sWORLD_TBLDAT* curTbldat = pWorld->GetTbldat();
			if (!curTbldat || curTbldat->tblidx != (TBLIDX)worldTblidx)
				pWorld = nullptr; // will resolve fresh below
		}
	}
	if (!pWorld)
	{
		if (!pWorldTbldat->bDynamic)
		{
			// Static world already exists; find by tblidx
			pWorld = app->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)pWorldTbldat->tblidx);
			if (!pWorld)
			{
				ERR_LOG(LOG_GENERAL, _T("[EVENT] TeleportParticipantsToWorld: static world instance not found (tblidx=%u)"), worldTblidx);
				return;
			}
			m_eventWorldId = (unsigned int)pWorld->GetID();
			EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Using static world: tblidx=%u worldId=%u", worldTblidx, m_eventWorldId);
		}
		else
		{
			pWorld = app->GetGameMain()->GetWorldManager()->CreateWorld(pWorldTbldat);
			if (!pWorld)
			{
				ERR_LOG(LOG_GENERAL, _T("[EVENT] TeleportParticipantsToWorld: failed to create world %u"), worldTblidx);
				return;
			}
			m_eventWorldId = (unsigned int)pWorld->GetID();
			EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Created event world: tblidx=%u worldId=%u", worldTblidx, m_eventWorldId);
		}
	}

	// Teleport all participants using COMMAND type (like Arena)
	unsigned int teleported = 0;
	unsigned int queued = 0;
	for (unsigned int charId : m_participants)
	{
		CPlayer* pPlayer = g_pObjectManager->FindByChar((CHARACTERID)charId);
		if (pPlayer)
		{
			if (!pPlayer->IsInitialized())
			{
				m_pendingTeleports.insert(charId);
				queued++;
				EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Deferred teleport: player not initialized yet (charId=%u)", charId);
				continue;
			}
			// If player already in the target world, still teleport them to the event spawn position
			// (don't skip - they need to be moved to the event spawn point)
			// Only skip if somehow they're already at a bad/invalid position
			if ((unsigned int)pPlayer->GetWorldID() == (unsigned int)pWorld->GetID())
			{
				CNtlVector cur = pPlayer->GetCurLoc();
				if (!std::isfinite(cur.x) || !std::isfinite(cur.y) || !std::isfinite(cur.z))
				{
					// Player has invalid position, force teleport
					EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Force teleport: player %s has invalid position in world %u", pPlayer->GetCharName(), pWorld->GetID());
				}
				else
				{
					// Player in correct world, just teleport to event spawn point
					EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Teleporting player %s to event spawn (already in world %u)", pPlayer->GetCharName(), pWorld->GetID());
				}
			}
			if (!std::isfinite(destLoc.x) || !std::isfinite(destLoc.y) || !std::isfinite(destLoc.z))
			{
				ComputeDestForWorld(worldTblidx, 0.f, 0.f, 0.f, destLoc);
			}

			// Clear UI elements before teleport to ensure clean entry (like Arena does)
			pPlayer->SendCharStateStanding();

			WORLDID targetWorldId = pWorld->GetID();
			ERR_LOG(LOG_GENERAL, _T("[EVENT] DEBUG: About to teleport player %s (charId=%u) to world tblidx=%u worldId=%u pos=(%.2f,%.2f,%.2f) currentWorld=%u"),
				pPlayer->GetCharName(), charId, worldTblidx, (unsigned)targetWorldId, destLoc.x, destLoc.y, destLoc.z, (unsigned)pPlayer->GetWorldID());

			pPlayer->StartTeleport(destLoc, pPlayer->GetCurDir(), targetWorldId, TELEPORT_TYPE_COMMAND);
			teleported++;
			EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Teleporting player %s (charId=%u) to (%.2f,%.2f,%.2f) worldId=%u type=COMMAND",
				pPlayer->GetCharName(), charId, destLoc.x, destLoc.y, destLoc.z, (unsigned)targetWorldId);
		}
		else // player object missing
		{
			m_pendingTeleports.insert(charId);
			queued++;
			ERR_LOG(LOG_GENERAL, _T("[EVENT] TeleportParticipantsToWorld: player object missing for charId=%u; deferred teleport queued"), charId);
		}
	}

	ERR_LOG(LOG_GENERAL, _T("[EVENT] TeleportParticipantsToWorld completed: %u/%u players teleported (%u queued) to world %u at (%.2f,%.2f,%.2f)"),
		teleported, (unsigned)m_participants.size(), queued, m_eventWorldId, destLoc.x, destLoc.y, destLoc.z);
}

void CEventManager::TeleportParticipantsToPortal(unsigned int portalTblidx)
{
	// Lookup portal table data (like @teleport command does)
	sPORTAL_TBLDAT* pPortalTblData = (sPORTAL_TBLDAT*)g_pTableContainer->GetPortalTable()->FindData(portalTblidx);
	if (!pPortalTblData)
	{
		ERR_LOG(LOG_GENERAL, _T("[EVENT] TeleportParticipantsToPortal: portal not found (portalTblidx=%u)"), portalTblidx);
		return;
	}

	EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] TeleportParticipantsToPortal: portalTblidx=%u worldId=%u pos=(%.2f,%.2f,%.2f) participants=%u",
		portalTblidx, pPortalTblData->worldId, pPortalTblData->vLoc.x, pPortalTblData->vLoc.y, pPortalTblData->vLoc.z, (unsigned)m_participants.size());

	// Update event world ID to match portal's world
	m_eventWorldId = (unsigned int)pPortalTblData->worldId;

	unsigned int teleported = 0;
	unsigned int queued = 0;

	for (unsigned int charId : m_participants)
	{
		CPlayer* pPlayer = g_pObjectManager->FindByChar((CHARACTERID)charId);
		if (pPlayer && pPlayer->IsInitialized())
		{
			// Clear UI elements before teleport
			pPlayer->SendCharStateStanding();

			// Use StartTeleport just like @teleport command does
			pPlayer->StartTeleport(pPortalTblData->vLoc, pPortalTblData->vDir, pPortalTblData->worldId, TELEPORT_TYPE_COMMAND);
			teleported++;

			EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Teleporting player %s (charId=%u) to portal %u at (%.2f,%.2f,%.2f) worldId=%u",
				pPlayer->GetCharName(), charId, portalTblidx, pPortalTblData->vLoc.x, pPortalTblData->vLoc.y, pPortalTblData->vLoc.z, pPortalTblData->worldId);
		}
		else
		{
			m_pendingTeleports.insert(charId);
			queued++;
			ERR_LOG(LOG_GENERAL, _T("[EVENT] TeleportParticipantsToPortal: player object missing for charId=%u; deferred teleport queued"), charId);
		}
	}

	ERR_LOG(LOG_GENERAL, _T("[EVENT] TeleportParticipantsToPortal completed: %u/%u players teleported (%u queued) to portal %u"),
		teleported, (unsigned)m_participants.size(), queued, portalTblidx);
}

void CEventManager::PostEventTeleportAll()
{
	if (!m_cfg.postEventTeleport)
		return;

	// CRITICAL FIX: Stop the round timer before teleporting
	// EventManager sends GU_BATTLE_DUNGEON_LIMIT_TIME_START_NFY when rounds start,
	// so we MUST send the END packet before teleporting out to prevent stuck UI timers
	StopRoundTimer();

	for (unsigned int charId : m_participants)
	{
		CPlayer* pPlayer = g_pObjectManager->FindByChar((CHARACTERID)charId);
		if (pPlayer && pPlayer->IsInitialized())
		{
			// Try to teleport to saved location first
			auto it = m_prevLoc.find(charId);
			if (it != m_prevLoc.end())
			{
				const PrevLoc& loc = it->second;
				CNtlVector destLoc = loc.loc;
				CNtlVector destDir = loc.dir;
				if (loc.worldId != 0 && loc.worldId != INVALID_WORLDID)
				{
					pPlayer->StartTeleport(destLoc, destDir, (WORLDID)loc.worldId, TELEPORT_TYPE_COMMAND);
				}
				else
				{
					ERR_LOG(LOG_GENERAL, _T("[EVENT] PostEventTeleportAll: saved worldId invalid for char=%u; using fallback"), charId);
					// Fallback to configured post-event location
					CGameServer* app = (CGameServer*)g_pApp;
					sWORLD_TBLDAT* pWorldTbldat = (sWORLD_TBLDAT*)g_pTableContainer->GetWorldTable()->FindData((TBLIDX)m_cfg.postEventWorldTblidx);
					CWorld* pWorld = nullptr;
					if (pWorldTbldat)
					{
						pWorld = app->GetGameMain()->GetWorldManager()->CreateWorld(pWorldTbldat);
					}
					else
					{
						ERR_LOG(LOG_GENERAL, _T("[EVENT] PostEventTeleportAll: fallback world tblidx not found %u"), m_cfg.postEventWorldTblidx);
					}
					CNtlVector dest = pWorldTbldat ? CNtlVector(m_cfg.postEventPosX, m_cfg.postEventPosY, m_cfg.postEventPosZ)
						: CNtlVector(0.f, 0.f, 0.f);
					WORLDID wId = (pWorld ? pWorld->GetID() : INVALID_WORLDID);
					pPlayer->StartTeleport(dest, pPlayer->GetCurDir(), wId, TELEPORT_TYPE_COMMAND);
				}
			}
			else
			{
				// Fallback to configured post-event location
				// Ensure fallback world exists
				CGameServer* app = (CGameServer*)g_pApp;
				sWORLD_TBLDAT* pWorldTbldat = (sWORLD_TBLDAT*)g_pTableContainer->GetWorldTable()->FindData((TBLIDX)m_cfg.postEventWorldTblidx);
				CWorld* pWorld = nullptr;
				if (pWorldTbldat)
				{
					pWorld = app->GetGameMain()->GetWorldManager()->CreateWorld(pWorldTbldat);
				}
				else
				{
					ERR_LOG(LOG_GENERAL, _T("[EVENT] PostEventTeleportAll: fallback world tblidx not found %u"), m_cfg.postEventWorldTblidx);
				}
				CNtlVector dest = pWorldTbldat ? CNtlVector(m_cfg.postEventPosX, m_cfg.postEventPosY, m_cfg.postEventPosZ)
					: CNtlVector(0.f, 0.f, 0.f);
				WORLDID wId = (pWorld ? pWorld->GetID() : INVALID_WORLDID);
				pPlayer->StartTeleport(dest, pPlayer->GetCurDir(), wId, TELEPORT_TYPE_COMMAND);
			}
		}
	}
}

bool CEventManager::TeleportOneToWorldTblidx(CPlayer* pPlayer, unsigned int worldTblidx, float posX, float posY, float posZ)
{
	if (!pPlayer || !pPlayer->IsInitialized()) return false;
	if (worldTblidx == 0) return false;

	CGameServer* app = (CGameServer*)g_pApp;
	sWORLD_TBLDAT* pWorldTbldat = (sWORLD_TBLDAT*)g_pTableContainer->GetWorldTable()->FindData((TBLIDX)worldTblidx);
	if (!pWorldTbldat) return false;

	CWorld* pWorld = GetOrCreateWorld(worldTblidx);
	if (!pWorld) return false;

	CNtlVector destLoc;
	if (!ComputeDestForWorld(worldTblidx, posX, posY, posZ, destLoc)) return false;

	// Use COMMAND teleport type
	pPlayer->StartTeleport(destLoc, pPlayer->GetCurDir(), pWorld->GetID(), TELEPORT_TYPE_COMMAND);
	return true;
}

bool CEventManager::TeleportOneToWorldTblidxDir(CPlayer* pPlayer, unsigned int worldTblidx, float posX, float posY, float posZ, float dirX, float dirY, float dirZ)
{
	if (!pPlayer) return false;
	if (!TeleportOneToWorldTblidx(pPlayer, worldTblidx, posX, posY, posZ)) return false;
	CNtlVector vDir(dirX, dirY, dirZ);
	if (!vDir.IsZero())
		pPlayer->SetCurDir(vDir);
	return true;
}

void CEventManager::OnPlayerEnterWorld(CPlayer* pPlayer)
{
	if (!pPlayer || !IsParticipant(pPlayer))
		return;

	// Player entered event world, resync phase UI if needed
	if (m_eventWorldId != 0 && (unsigned int)pPlayer->GetWorldID() != m_eventWorldId)
		return; // only resync inside event world

	if (m_state == State::PRE_ROUND)
	{
		// Start fallback countdown for the player if active
		SendCountdownTo(pPlayer, true);
	}
	else if (m_state == State::IN_ROUND)
	{
		if (m_roundTimerActive)
		{
			unsigned int seconds = (unsigned int)(m_roundRemainMs / 1000);
			SendRoundTimerStartTo(pPlayer, seconds);
		}
		else
		{
			// Ensure countdown UI is off for the player
			SendCountdownTo(pPlayer, false);
		}
	}
}

void CEventManager::OnPlayerEnterWorldComplete(CPlayer* pPlayer)
{
	if (!pPlayer || !m_cfg.enabled) return;

	// If enrollment is open and this channel is valid, show a lightweight notice to the logging-in player
	if (m_state == State::ENROLLMENT && IsChannelValid())
	{
		unsigned int secRemain = (unsigned int)(m_enrollmentRemainMs / 1000);
		wchar_t msg[192];
		if (secRemain > 0)
			swprintf_s(msg, _countof(msg), L"[EVENT] Enrollment is OPEN. Type @participate to join. (%us left)", secRemain);
		else
			swprintf_s(msg, _countof(msg), L"[EVENT] Enrollment is OPEN. Type @participate to join.");
		SendSystemTo(pPlayer, msg, SERVER_TEXT_SYSTEM);
	}

	// If this player is already a participant and enters the event world later, reuse the resync path
	if (IsParticipant(pPlayer))
	{
		OnPlayerEnterWorld(pPlayer);
	}

	// Handle any deferred teleport if enrollment already closed or PRE_ROUND
	if (IsParticipant(pPlayer) && m_pendingTeleports.find(pPlayer->GetCharID()) != m_pendingTeleports.end())
	{
		// Check if current round uses portal teleportation
		bool usePortal = false;
		unsigned int portalId = 0;
		if ((m_state == State::PRE_ROUND || m_state == State::IN_ROUND) && m_currentRound < m_cfg.rounds.size())
		{
			const EventRound& round = m_cfg.rounds[m_currentRound];
			if (round.portalTblidx > 0)
			{
				usePortal = true;
				portalId = round.portalTblidx;
			}
		}

		if (usePortal)
		{
			// Use portal teleportation
			sPORTAL_TBLDAT* pPortalTblData = (sPORTAL_TBLDAT*)g_pTableContainer->GetPortalTable()->FindData(portalId);
			if (pPortalTblData)
			{
				pPlayer->StartTeleport(pPortalTblData->vLoc, pPortalTblData->vDir, pPortalTblData->worldId, TELEPORT_TYPE_COMMAND);
				EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Deferred teleport to portal %u executed for %s (charId=%u)", portalId, pPlayer->GetCharName(), pPlayer->GetCharID());
			}
		}
		else
		{
			// Use world teleportation (original behavior)
			unsigned int worldTblidx = (m_state == State::PRE_ROUND || m_state == State::IN_ROUND) ? GetWorldForRound(m_currentRound) : m_cfg.eventWorldTblidx;
			CNtlVector dest;
			ComputeDestForWorld(worldTblidx, m_cfg.teleportPosX, m_cfg.teleportPosY, m_cfg.teleportPosZ, dest);
			CWorld* pWorld = GetOrCreateWorld(worldTblidx);
			if (pWorld)
			{
				if (!std::isfinite(dest.x) || !std::isfinite(dest.y) || !std::isfinite(dest.z))
					ComputeDestForWorld(worldTblidx, 0.f, 0.f, 0.f, dest);
				if ((unsigned int)pPlayer->GetWorldID() != (unsigned int)pWorld->GetID())
				{
					pPlayer->StartTeleport(dest, pPlayer->GetCurDir(), pWorld->GetID(), TELEPORT_TYPE_COMMAND);
					EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Deferred teleport to world %u executed for %s (charId=%u)", worldTblidx, pPlayer->GetCharName(), pPlayer->GetCharID());
				}
			}
		}
		m_pendingTeleports.erase(pPlayer->GetCharID());
	}
}

void CEventManager::StartRoundTimer(unsigned int seconds)
{
	m_roundTimerActive = true;
	m_roundRemainMs = ToMs(seconds);

	if (m_eventWorldId > 0)
	{
		BroadcastRoundTimerStartToWorld(m_eventWorldId, seconds);
		// Also start a generic countdown UI for redundancy (some maps ignore dungeon timer)
		BroadcastCountdownToWorld(m_eventWorldId, true);
		m_countdownActive = true;
	}
}

void CEventManager::StopRoundTimer()
{
	m_roundTimerActive = false;
	m_roundRemainMs = 0;

	if (m_eventWorldId > 0)
	{
		BroadcastRoundTimerEndToWorld(m_eventWorldId);
		// End fallback countdown as well
		BroadcastCountdownToWorld(m_eventWorldId, false);
		m_countdownActive = false;
	}
}

void CEventManager::BroadcastRoundTimerStartToWorld(unsigned int worldId, unsigned int seconds)
{
	CGameServer* app = (CGameServer*)g_pApp;
	CWorld* pWorld = app->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)worldId);
	if (!pWorld) return;
	CNtlPacket packet(sizeof(sGU_BATTLE_DUNGEON_LIMIT_TIME_START_NFY));
	sGU_BATTLE_DUNGEON_LIMIT_TIME_START_NFY* res = (sGU_BATTLE_DUNGEON_LIMIT_TIME_START_NFY*)packet.GetPacketData();
	res->wOpCode = GU_BATTLE_DUNGEON_LIMIT_TIME_START_NFY;
	res->dwLimitTime = seconds;
	packet.SetPacketLen(sizeof(sGU_BATTLE_DUNGEON_LIMIT_TIME_START_NFY));
	pWorld->Broadcast(&packet);
}

void CEventManager::BroadcastRoundTimerEndToWorld(unsigned int worldId)
{
	CGameServer* app = (CGameServer*)g_pApp;
	CWorld* pWorld = app->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)worldId);
	if (!pWorld) return;
	CNtlPacket packet(sizeof(sGU_BATTLE_DUNGEON_LIMIT_TIME_END_NFY));
	sGU_BATTLE_DUNGEON_LIMIT_TIME_END_NFY* res = (sGU_BATTLE_DUNGEON_LIMIT_TIME_END_NFY*)packet.GetPacketData();
	res->wOpCode = GU_BATTLE_DUNGEON_LIMIT_TIME_END_NFY;
	packet.SetPacketLen(sizeof(sGU_BATTLE_DUNGEON_LIMIT_TIME_END_NFY));
	pWorld->Broadcast(&packet);
}

void CEventManager::BroadcastDungeonStateToWorld(unsigned int worldId, unsigned char byStage, unsigned int titleTblidx)
{
	CGameServer* app = (CGameServer*)g_pApp;
	CWorld* pWorld = app->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)worldId);
	if (!pWorld) return;
	CNtlPacket packet(sizeof(sGU_BATTLE_DUNGEON_STATE_UPATE_NFY));
	sGU_BATTLE_DUNGEON_STATE_UPATE_NFY* res = (sGU_BATTLE_DUNGEON_STATE_UPATE_NFY*)packet.GetPacketData();
	res->wOpCode = GU_BATTLE_DUNGEON_STATE_UPATE_NFY;
	res->titleTblidx = titleTblidx;
	res->subTitleTblidx = 0;
	res->byStage = byStage;
	packet.SetPacketLen(sizeof(sGU_BATTLE_DUNGEON_STATE_UPATE_NFY));
	pWorld->Broadcast(&packet);
}

void CEventManager::BroadcastCountdownToWorld(unsigned int worldId, bool bStart)
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

void CEventManager::SendCountdownTo(CPlayer* pPlayer, bool bStart)
{
	if (!pPlayer) return;
	CNtlPacket packet(sizeof(sGU_TIMEQUEST_COUNTDOWN_NFY));
	sGU_TIMEQUEST_COUNTDOWN_NFY* res = (sGU_TIMEQUEST_COUNTDOWN_NFY*)packet.GetPacketData();
	res->wOpCode = GU_TIMEQUEST_COUNTDOWN_NFY;
	res->bCountDown = bStart;
	packet.SetPacketLen(sizeof(sGU_TIMEQUEST_COUNTDOWN_NFY));
	pPlayer->SendPacket(&packet);
}

void CEventManager::SendRoundTimerStartTo(CPlayer* pPlayer, unsigned int seconds)
{
	if (!pPlayer) return;
	CNtlPacket packet(sizeof(sGU_BATTLE_DUNGEON_LIMIT_TIME_START_NFY));
	sGU_BATTLE_DUNGEON_LIMIT_TIME_START_NFY* res = (sGU_BATTLE_DUNGEON_LIMIT_TIME_START_NFY*)packet.GetPacketData();
	res->wOpCode = GU_BATTLE_DUNGEON_LIMIT_TIME_START_NFY;
	res->dwLimitTime = seconds;
	packet.SetPacketLen(sizeof(sGU_BATTLE_DUNGEON_LIMIT_TIME_START_NFY));
	pPlayer->SendPacket(&packet);
	// Also ensure countdown is on as redundancy
	SendCountdownTo(pPlayer, true);
}

void CEventManager::SendRoundTimerEndTo(CPlayer* pPlayer)
{
	if (!pPlayer) return;
	CNtlPacket packet(sizeof(sGU_BATTLE_DUNGEON_LIMIT_TIME_END_NFY));
	sGU_BATTLE_DUNGEON_LIMIT_TIME_END_NFY* res = (sGU_BATTLE_DUNGEON_LIMIT_TIME_END_NFY*)packet.GetPacketData();
	res->wOpCode = GU_BATTLE_DUNGEON_LIMIT_TIME_END_NFY;
	packet.SetPacketLen(sizeof(sGU_BATTLE_DUNGEON_LIMIT_TIME_END_NFY));
	pPlayer->SendPacket(&packet);
	// End fallback countdown
	SendCountdownTo(pPlayer, false);
}

void CEventManager::BeginNow()
{
	if (m_state == State::ENROLLMENT)
	{
		m_enrollmentRemainMs = 0;
		if (!m_participants.empty())
		{
			// Skip initial teleport if world rotation is enabled OR first round uses portal
			bool firstRoundHasPortal = !m_cfg.rounds.empty() && m_cfg.rounds[0].portalTblidx > 0;
			if (m_cfg.worldRotationEnabled || firstRoundHasPortal)
			{
				BroadcastSystem(L"[EVENT] Enrollment closed (forced). Get ready!");
				SendNotice(L"[EVENT] Enrollment closed. Preparing round 1...", SERVER_TEXT_SYSNOTICE);
			}
			else
			{
				BroadcastSystem(L"[EVENT] Enrollment closed. Teleporting participants (forced)...");
				SendNotice(L"[EVENT] Enrollment closed. Teleporting participants (forced)...", SERVER_TEXT_SYSNOTICE);
				TeleportParticipants();
			}
			m_state = State::PRE_ROUND;
			m_startDelayRemainMs = ToMs(m_cfg.startDelaySeconds);
			m_nextPreRoundAnnounceSec = (unsigned int)(m_startDelayRemainMs / 1000);
		}
		else
		{
			BroadcastSystem(L"[EVENT] No participants. Event cancelled.");
			SendNotice(L"[EVENT] No participants. Event cancelled.", SERVER_TEXT_SYSNOTICE);
			m_state = State::IDLE;
			ScheduleAutoAfterTermination(true);
		}
	}
}
void CEventManager::ScheduleAutoAfterTermination(bool cancelledNoParticipants)
{
	if (!m_cfg.enabled || !m_cfg.autoEnabled)
		return;

	// If automation engine is OFF (e.g., manual start earlier), initialize it.
	if (m_autoState == AutoState::OFF)
	{
		m_autoState = AutoState::WAIT_NEXT;
	}

	// If event completed normally and autoRestartOnComplete is set, WAIT_RESTART logic will already handle it.
	// For cancellations with no participants or manual stop, we schedule a fresh WAIT_NEXT interval.
	if (m_autoState == AutoState::ENROLLMENT_OPEN)
	{
		// Enrollment just closed with no participants or we aborted; move to WAIT_NEXT.
		m_autoState = AutoState::WAIT_NEXT;
	}

	// Choose interval: if no participants, we can retry sooner (e.g., 1/4 of normal) but not less than 60s.
	unsigned int baseInterval = m_cfg.autoIntervalSeconds;
	unsigned int retrySeconds = baseInterval;
	if (cancelledNoParticipants && baseInterval > 0)
	{
		retrySeconds = baseInterval / 4;
		if (retrySeconds < 60) retrySeconds = (baseInterval < 60 ? baseInterval : 60);
	}

	// If autoRestartOnComplete and this was a completion path, let existing logic run.
	if (!cancelledNoParticipants && m_cfg.autoRestartOnComplete)
	{
		// Defer to WAIT_RESTART if not already set elsewhere; if we're IDLE we convert to WAIT_RESTART.
		if (m_autoState != AutoState::WAIT_RESTART)
		{
			m_autoState = AutoState::WAIT_RESTART;
			m_autoRemainMs = ToMs(m_cfg.autoRestartDelaySeconds);
		}
		return;
	}

	// Normal scheduling
	m_autoState = AutoState::WAIT_NEXT;
	m_autoRemainMs = ToMs(retrySeconds);
	EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT][AUTO] Scheduled next enrollment in %u seconds (cancelled=%d)", retrySeconds, (int)cancelledNoParticipants);
}

unsigned int CEventManager::GetWorldForRound(unsigned int roundIndex)
{
	// Priority 1: Round-specific world
	if (roundIndex < m_cfg.rounds.size())
	{
		const EventRound& round = m_cfg.rounds[roundIndex];
		if (round.worldTblidx > 0)
			return round.worldTblidx;
	}

	// Priority 2: World rotation list
	if (m_cfg.worldRotationEnabled && !m_cfg.worldTblidxList.empty())
	{
		if (m_cfg.randomizeWorlds)
		{
			// Random world from list
			unsigned int idx = RandomRange(0, (int)m_cfg.worldTblidxList.size() - 1);
			return m_cfg.worldTblidxList[idx];
		}
		else
		{
			// Sequential rotation
			unsigned int worldId = m_cfg.worldTblidxList[m_worldRotationIndex];
			m_worldRotationIndex = (m_worldRotationIndex + 1) % m_cfg.worldTblidxList.size();
			return worldId;
		}
	}

	// Priority 3: Default event world
	return m_cfg.eventWorldTblidx;
}

void CEventManager::GetSpawnPosForRound(unsigned int roundIndex, float& outX, float& outY, float& outZ)
{
	// Get world for this round to determine default spawn
	unsigned int worldTblidx = GetWorldForRound(roundIndex);

	// Use ComputeDestForWorld which intelligently calculates safe positions using world boundaries
	CNtlVector dest;
	if (ComputeDestForWorld(worldTblidx, 0.0f, 0.0f, 0.0f, dest))
	{
		outX = dest.x;
		outY = dest.y;
		outZ = dest.z;
	}
	else
	{
		// Fallback if computation failed
		outX = 0.0f;
		outY = 0.0f;
		outZ = 0.0f;
		ERR_LOG(LOG_GENERAL, _T("[EVENT] GetSpawnPosForRound: failed to compute dest for world %u"), worldTblidx);
		return;
	}

	// Override with config spawn position if provided (not 0,0,0)
	if (m_cfg.spawnPosX != 0 || m_cfg.spawnPosY != 0 || m_cfg.spawnPosZ != 0)
	{
		outX = m_cfg.spawnPosX;
		outY = m_cfg.spawnPosY;
		outZ = m_cfg.spawnPosZ;
		EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Using config spawn override: (%.2f,%.2f,%.2f)", outX, outY, outZ);
		return;
	}

	// Check if round has custom spawn position (highest priority)
	if (roundIndex < m_cfg.rounds.size())
	{
		const EventRound& round = m_cfg.rounds[roundIndex];
		if (round.spawnPosX != 0 || round.spawnPosY != 0 || round.spawnPosZ != 0)
		{
			outX = round.spawnPosX;
			outY = round.spawnPosY;
			outZ = round.spawnPosZ;
			EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Using round-specific spawn: (%.2f,%.2f,%.2f)", outX, outY, outZ);
			return;
		}
	}

	EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Using world table spawn: (%.2f,%.2f,%.2f) for world %u", outX, outY, outZ, worldTblidx);
}

bool CEventManager::IsChannelValid()
{
	CGameServer* app = (CGameServer*)g_pApp;
	if (!app)
		return false;

	if (m_cfg.channelNameContains.c_str()[0] == '\0')
		return true;

	std::string want = m_cfg.channelNameContains.c_str() ? m_cfg.channelNameContains.c_str() : "";
	std::string got = app->m_config.ChannelName.c_str() ? app->m_config.ChannelName.c_str() : "";

	std::transform(want.begin(), want.end(), want.begin(), [](unsigned char ch) { return (char)std::tolower(ch); });
	std::transform(got.begin(), got.end(), got.begin(), [](unsigned char ch) { return (char)std::tolower(ch); });

	// Allow both configured channel and TEST channel
	return got.find(want) != std::string::npos || got.find("test") != std::string::npos;
}

CEventManager::ActionReward::Type CEventManager::ResolveActionType(const CNtlString& value)
{
	std::string temp = value.c_str() ? value.c_str() : "";
	std::transform(temp.begin(), temp.end(), temp.begin(), [](unsigned char ch) { return (char)std::toupper(ch); });
	if (temp == "PLAYTIME")
		return ActionReward::Type::PLAYTIME;

	ERR_LOG(LOG_GENERAL, _T("[EVENT] Unknown ActionReward type '%S'. Defaulting to PLAYTIME."), temp.c_str());
	return ActionReward::Type::PLAYTIME;
}

CEventManager::ActionReward::GrantMode CEventManager::ResolveGrantMode(const CNtlString& value)
{
	std::string temp = value.c_str() ? value.c_str() : "";
	std::transform(temp.begin(), temp.end(), temp.begin(), [](unsigned char ch) { return (char)std::toupper(ch); });
	if (temp == "REPEATABLE")
		return ActionReward::GrantMode::REPEATABLE;
	return ActionReward::GrantMode::ONCE;
}

std::wstring CEventManager::ToWide(const CNtlString& value)
{
	std::wstring out;
	if (value.c_str() == nullptr)
		return out;
	const char* src = value.c_str();
	while (*src)
	{
		out.push_back((wchar_t)(unsigned char)*src);
		++src;
	}
	return out;
}

void CEventManager::BroadcastSystem(const wchar_t* msg, unsigned char byType)
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

void CEventManager::SendSystemTo(CPlayer* pPlayer, const wchar_t* msg, unsigned char byType)
{
	if (!pPlayer)
		return;

	CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
	res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
	res->byDisplayType = byType;
	res->wMessageLengthInUnicode = (WORD)wcslen(msg);
	wcscpy_s(res->awchMessage, NTL_MAX_LENGTH_OF_CHAT_MESSAGE + 1, msg);
	packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	pPlayer->SendPacket(&packet);
}

void CEventManager::SendNotice(const wchar_t* msg, unsigned char byType)
{
	// Mirror Arena's chat broadcast so messages go through ChatServer too
	CGameServer* app = (CGameServer*)g_pApp;
	if (!app || !app->GetChatServerSession())
	{
		// Fallback to in-world broadcast if ChatServer not available
		BroadcastSystem(msg, byType);
		return;
	}
	CNtlPacket packet(sizeof(sGT_SYSTEM_DISPLAY_TEXT));
	sGT_SYSTEM_DISPLAY_TEXT* res = (sGT_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
	res->wOpCode = GT_SYSTEM_DISPLAY_TEXT;
	res->serverChannelId = INVALID_SERVERCHANNELID;
	res->byDisplayType = byType;
	wcsncpy_s(res->wszMessage, NTL_MAX_LENGTH_OF_CHAT_MESSAGE + 1, msg, _TRUNCATE);
	packet.SetPacketLen(sizeof(sGT_SYSTEM_DISPLAY_TEXT));
	app->SendTo(app->GetChatServerSession(), &packet);
}

void CEventManager::StatusTo(CPlayer* pWho)
{
	if (!pWho)
		return;

	wchar_t msg[512];
	const wchar_t* stateStr = L"UNKNOWN";
	switch (m_state)
	{
	case State::IDLE: stateStr = L"IDLE"; break;
	case State::ENROLLMENT: stateStr = L"ENROLLMENT"; break;
	case State::PRE_ROUND: stateStr = L"PRE_ROUND"; break;
	case State::IN_ROUND: stateStr = L"IN_ROUND"; break;
	case State::INTERMISSION: stateStr = L"INTERMISSION"; break;
	case State::COMPLETE: stateStr = L"COMPLETE"; break;
	}

	swprintf_s(msg, L"[EVENT] State: %s | Round: %u/%u | Participants: %u",
		stateStr, m_currentRound + 1, (unsigned)m_cfg.rounds.size(), (unsigned)m_participants.size());
	SendSystemTo(pWho, msg);
}

bool CEventManager::ReloadConfigFromDefault()
{
	const char* eventIni = ".\\config\\Events.cfg";
	bool ok = LoadConfigFromIniPath(eventIni);
	NTL_PRINT(PRINT_APP, ok ? _T("[EVENT] Config reloaded") : _T("[EVENT] Config reload failed"));
	return ok;
}

void CEventManager::ResetAutomation(bool startIfZeroDelay)
{
	m_autoState = AutoState::OFF;
	m_autoRemainMs = 0;

	if (startIfZeroDelay && m_cfg.autoEnabled && m_cfg.autoInitialDelaySeconds == 0 && m_state == State::IDLE && IsChannelValid())
	{
		Start();
		m_autoState = AutoState::ENROLLMENT_OPEN;
	}
}

//--------------------------------------------------------------------------------------//
//  ENGAGEMENT FEATURES
//--------------------------------------------------------------------------------------//

void CEventManager::OnPlayerKilledMob(CPlayer* pKiller, const char* mobName)
{
	if (!pKiller || m_state != State::IN_ROUND)
		return;

	unsigned int charId = pKiller->GetCharID();
	if (!IsParticipantId(charId))
		return;

	// Track kill count
	m_playerKillCount[charId]++;

	// Track team kills if team competition enabled
	if (m_cfg.enableTeamCompetition)
	{
		auto teamIt = m_playerTeam.find(charId);
		if (teamIt != m_playerTeam.end())
		{
			if (teamIt->second == Team::RED)
				m_teamRedKills++;
			else if (teamIt->second == Team::BLUE)
				m_teamBlueKills++;
		}
	}

	// Process combo system
	if (m_cfg.enableComboBonus)
	{
		ProcessKillCombo(charId);
	}

	// Kill announcements
	if (m_cfg.enableKillAnnouncements)
	{
		wchar_t msg[256];
		unsigned int combo = m_playerComboCount[charId];

		// Team competition announcement
		if (m_cfg.enableTeamCompetition)
		{
			auto teamIt = m_playerTeam.find(charId);
			const wchar_t* teamColor = (teamIt != m_playerTeam.end()) ? GetTeamColor(teamIt->second) : L"";

			if (combo >= 3)
			{
				swprintf_s(msg, L"[EVENT] %s%s killed %S! (x%u COMBO!)", teamColor, pKiller->GetCharName(), mobName, combo);
			}
			else
			{
				swprintf_s(msg, L"[EVENT] %s%s killed %S!", teamColor, pKiller->GetCharName(), mobName);
			}
		}
		else
		{
			// Regular announcement
			if (combo >= 3)
			{
				swprintf_s(msg, L"[EVENT] %s killed %S! (x%u COMBO!)", pKiller->GetCharName(), mobName, combo);
			}
			else
			{
				swprintf_s(msg, L"[EVENT] %s killed %S!", pKiller->GetCharName(), mobName);
			}
		}
		BroadcastSystem(msg);
	}
}

void CEventManager::OnPlayerDeath(CPlayer* pPlayer)
{
	if (!pPlayer || m_state != State::IN_ROUND)
		return;

	unsigned int charId = pPlayer->GetCharID();
	if (!IsParticipantId(charId))
		return;

	// Check if already eliminated
	if (m_eliminatedPlayers.find(charId) != m_eliminatedPlayers.end())
		return;

	if (!m_cfg.autoResurrectEnabled)
		return;

	// Increment death count
	m_playerDeathCount[charId]++;
	m_playerDeathTime[charId] = GetTickCount();

	unsigned int deaths = m_playerDeathCount[charId];

	EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Player %s died (death #%u/%u)",
		pPlayer->GetCharName(), deaths, m_cfg.maxDeathsBeforeElimination);

	// Check if eliminated
	if (deaths >= m_cfg.maxDeathsBeforeElimination)
	{
		m_eliminatedPlayers.insert(charId);
		wchar_t msg[256];
		swprintf_s(msg, L"[EVENT] %s was eliminated due to too many deaths! (%u deaths)",
			pPlayer->GetCharName(), deaths);
		BroadcastSystem(msg);
		SendNotice(msg, SERVER_TEXT_SYSNOTICE);

		// Remove from participants to prevent rewards
		m_participants.erase(charId);

		EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Player %s eliminated after %u deaths",
			pPlayer->GetCharName(), deaths);
		return;
	}

	// Send death message with remaining lives
	unsigned int remaining = m_cfg.maxDeathsBeforeElimination - deaths;
	wchar_t msg[256];
	swprintf_s(msg, L"[EVENT] You died! Respawning in %u seconds... (%u lives remaining)",
		m_cfg.autoResurrectDelayMs / 1000, remaining);
	SendSystemTo(pPlayer, msg);
}

//--------------------------------------------------------------------------------------//
//	ACTION REWARD TRACKING
//--------------------------------------------------------------------------------------//

void CEventManager::OnPlayerTick(CPlayer* pPlayer, unsigned long dwTickDiff)
{
	if (!pPlayer || dwTickDiff == 0)
		return;

	if (!ShouldTrackPlayerForRewards(pPlayer))
		return;

	EnsureActionStateCapacity(pPlayer->GetCharID());
	auto it = m_actionRewardStates.find(pPlayer->GetCharID());
	if (it == m_actionRewardStates.end())
		return;

	std::vector<ActionRewardState>& states = it->second;
	if (states.size() != m_cfg.actionRewards.size())
		return;

	for (size_t idx = 0; idx < m_cfg.actionRewards.size(); ++idx)
	{
		const ActionReward& def = m_cfg.actionRewards[idx];
		ActionRewardState& state = states[idx];

		if (def.minLevel != 0 && pPlayer->GetLevel() < def.minLevel)
		{
			state.accumulatedMs = 0;
			continue;
		}

		if (!def.countWhileAfk && pPlayer->IsAfk())
			continue;

		unsigned long& current = state.accumulatedMs;
		if (current <= std::numeric_limits<unsigned long>::max() - dwTickDiff)
			current += dwTickDiff;
		else
			current = std::numeric_limits<unsigned long>::max();

		TryGrantActionReward(pPlayer, idx);
	}
}

void CEventManager::OnPlayerDisconnected(CPlayer* pPlayer)
{
	if (!pPlayer || !pPlayer->IsInitialized())
		return;
	m_actionRewardStates.erase(pPlayer->GetCharID());
}

bool CEventManager::ShouldTrackPlayerForRewards(CPlayer* pPlayer) const
{
	if (!pPlayer || !pPlayer->IsInitialized())
		return false;
	if (!m_cfg.actionRewardsEnabled)
		return false;
	if (m_cfg.actionRewards.empty())
		return false;
	if (m_cfg.actionRewardsRequireEventChannel && !const_cast<CEventManager*>(this)->IsChannelValid())
		return false;
	return true;
}

void CEventManager::EnsureActionStateCapacity(unsigned int charId)
{
	if (m_cfg.actionRewards.empty())
		return;

	std::vector<ActionRewardState>& states = m_actionRewardStates[charId];
	if (states.size() != m_cfg.actionRewards.size())
		states.assign(m_cfg.actionRewards.size(), ActionRewardState());
}

void CEventManager::ResetAllActionStates()
{
	if (m_cfg.actionRewards.empty())
	{
		m_actionRewardStates.clear();
		return;
	}

	for (auto& entry : m_actionRewardStates)
	{
		entry.second.assign(m_cfg.actionRewards.size(), ActionRewardState());
	}
}

bool CEventManager::TryGrantActionReward(CPlayer* pPlayer, size_t rewardIndex)
{
	if (!pPlayer || !pPlayer->IsInitialized() || rewardIndex >= m_cfg.actionRewards.size())
		return false;

	auto it = m_actionRewardStates.find(pPlayer->GetCharID());
	if (it == m_actionRewardStates.end() || rewardIndex >= it->second.size())
		return false;

	ActionRewardState& state = it->second[rewardIndex];
	const ActionReward& def = m_cfg.actionRewards[rewardIndex];

	if (def.eventTblidx == 0)
		return false;
	if (def.mode == ActionReward::GrantMode::ONCE && state.grantsCompleted > 0)
		return false;
	if (pPlayer->HasEventReward(def.eventTblidx, pPlayer->GetCharID()))
		return false;

	unsigned long requiredMs = def.thresholdSeconds * 1000UL;
	if (requiredMs == 0 || state.accumulatedMs < requiredMs)
		return false;

	unsigned long now = GetTickCount();
	if (def.cooldownSeconds > 0 && state.lastGrantMs != 0)
	{
		unsigned long elapsed = (now >= state.lastGrantMs) ? (now - state.lastGrantMs) : (0xFFFFFFFFUL - state.lastGrantMs + now + 1UL);
		if (elapsed < def.cooldownSeconds * 1000UL)
			return false;
	}

	if (!def.countWhileAfk && pPlayer->IsAfk())
		return false;

	// Reward thresholds met – record immediately (no new packets allowed)
	state.lastGrantMs = now;
	state.grantsCompleted++;
	if (def.mode == ActionReward::GrantMode::REPEATABLE && def.cooldownSeconds == 0 && state.accumulatedMs >= requiredMs)
		state.accumulatedMs -= requiredMs;
	else
		state.accumulatedMs = 0;

	RecordActionRewardGrant(pPlayer, def);
	NotifyActionRewardGranted(pPlayer, def);

	return true;
}

void CEventManager::NotifyActionRewardGranted(CPlayer* pPlayer, const ActionReward& rewardDef) const
{
	if (!m_cfg.actionRewardsAnnounce || !pPlayer || !pPlayer->IsInitialized())
		return;

	std::wstring label = ToWide(rewardDef.label);
	if (label.empty())
		label = ToWide(rewardDef.id);
	if (label.empty())
		label = L"Reward";

	wchar_t msg[256];
	swprintf_s(msg, L"[EVENT] Free reward unlocked: %ls. Visit the Event Manager NPC to claim it.", label.c_str());
	const_cast<CEventManager*>(this)->SendSystemTo(pPlayer, msg);
}

void CEventManager::RecordActionRewardGrant(CPlayer* pPlayer, const ActionReward& rewardDef) const
{
	if (!pPlayer || !pPlayer->IsInitialized())
		return;

	const char* label = rewardDef.label.c_str();
	if (!label || *label == '\0')
		label = rewardDef.id.c_str();

	EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT][ActionReward] Granted '%s' to %S (charId=%u accountId=%u)",
		label ? label : "?", pPlayer->GetCharName(), pPlayer->GetCharID(), pPlayer->GetAccountID());

	CNtlString labelStr(label ? label : "?");
	std::wstring labelWide = ToWide(labelStr);
	if (labelWide.empty())
		labelWide = L"?";

	NTL_PRINT(PRINT_APP, _T("[EVENT][ActionReward] Granted '%ls' to %s (charId=%u accountId=%u)"),
		labelWide.c_str(), pPlayer->GetCharName(), pPlayer->GetCharID(), pPlayer->GetAccountID());
}

void CEventManager::ProcessKillCombo(unsigned int charId)
{
	unsigned long currentTime = GetTickCount();
	unsigned long lastKill = m_lastKillTime[charId];
	unsigned long timeSinceLastKill = (lastKill > 0) ? (currentTime - lastKill) : UINT_MAX;

	// Check if combo still active
	if (timeSinceLastKill < ToMs(m_cfg.comboTimeoutSeconds))
	{
		// Increment combo
		m_playerComboCount[charId]++;

		unsigned int combo = m_playerComboCount[charId];
		CPlayer* pPlayer = g_pObjectManager->GetPC(charId);
		if (pPlayer && pPlayer->IsInitialized())
		{
			// Award combo bonus
			unsigned int bonusMudosa = (unsigned int)(m_cfg.mudosaPerRound * m_cfg.comboMultiplier * (combo - 1));
			if (bonusMudosa > 0 && combo >= 2)
			{
				pPlayer->UpdateMudosaPoints(pPlayer->GetMudosaPoints() + bonusMudosa, true);

				wchar_t msg[128];
				swprintf_s(msg, L"[EVENT] COMBO x%u! Bonus: +%u Mudosa!", combo, bonusMudosa);
				SendSystemTo(pPlayer, msg);
			}
		}
	}
	else
	{
		// Reset combo
		m_playerComboCount[charId] = 1;
	}

	m_lastKillTime[charId] = currentTime;
}

void CEventManager::ShowLeaderboard()
{
	if (m_roundContribution.empty())
		return;

	// Sort players by damage
	std::vector<std::pair<unsigned int, unsigned int>> rankings; // charId, damage
	for (const auto& entry : m_roundContribution)
	{
		rankings.push_back(entry);
	}
	std::sort(rankings.begin(), rankings.end(),
		[](const std::pair<unsigned int, unsigned int>& a, const std::pair<unsigned int, unsigned int>& b) { return a.second > b.second; });

	// Display top 10 (ASCII-only to avoid codepage issues)
	BroadcastSystem(L"================ LEADERBOARD ================");
	BroadcastSystem(L"            EVENT DAMAGE RANKING            ");
	BroadcastSystem(L"============================================");

	unsigned int shown = 0;
	for (size_t i = 0; i < rankings.size() && shown < 10; i++)
	{
		CPlayer* pPlayer = g_pObjectManager->GetPC(rankings[i].first);
		if (pPlayer && pPlayer->IsInitialized())
		{
			const wchar_t* medal = L"";
			if (i == 0) medal = L"[1st]";
			else if (i == 1) medal = L"[2nd]";
			else if (i == 2) medal = L"[3rd]";

			wchar_t msg[256];
			swprintf_s(msg, L"%s #%u: %s - %u damage (%u kills)",
				medal, (unsigned)(i + 1), pPlayer->GetCharName(),
				rankings[i].second, m_playerKillCount[rankings[i].first]);
			BroadcastSystem(msg);
			shown++;
		}
	}
}

void CEventManager::AwardMVPBonuses()
{
	if (m_roundContribution.empty() || m_cfg.mvpBonusMudosa == 0)
		return;

	// Sort players by damage
	std::vector<std::pair<unsigned int, unsigned int>> rankings;
	for (const auto& entry : m_roundContribution)
	{
		rankings.push_back(entry);
	}
	std::sort(rankings.begin(), rankings.end(),
		[](const std::pair<unsigned int, unsigned int>& a, const std::pair<unsigned int, unsigned int>& b) { return a.second > b.second; });

	// Award top 3
	for (size_t i = 0; i < rankings.size() && i < 3; i++)
	{
		CPlayer* pPlayer = g_pObjectManager->GetPC(rankings[i].first);
		if (pPlayer && pPlayer->IsInitialized())
		{
			unsigned int bonus = m_cfg.mvpBonusMudosa;
			if (i == 1) bonus = (unsigned int)(bonus * 0.7f); // 2nd place: 70%
			else if (i == 2) bonus = (unsigned int)(bonus * 0.5f); // 3rd place: 50%

			pPlayer->UpdateMudosaPoints(pPlayer->GetMudosaPoints() + bonus, true);

			wchar_t msg[128];
			const wchar_t* rank = (i == 0) ? L"MVP" : (i == 1) ? L"2nd Place" : L"3rd Place";
			swprintf_s(msg, L"[EVENT] %s Bonus: +%u Mudosa!", rank, bonus);
			SendSystemTo(pPlayer, msg);
		}
	}
}

void CEventManager::CheckTimeAttackBonus()
{
	if (!m_cfg.enableTimeAttack || m_roundStartTime == 0)
		return;

	unsigned long elapsed = GetTickCount() - m_roundStartTime;
	unsigned int elapsedSeconds = (unsigned int)(elapsed / 1000);

	const wchar_t* tier = nullptr;
	unsigned int bonusMudosa = 0;

	if (elapsedSeconds <= m_cfg.timeAttackGoldSeconds)
	{
		tier = L"GOLD";
		bonusMudosa = m_cfg.timeAttackBonusMudosa;
	}
	else if (elapsedSeconds <= m_cfg.timeAttackSilverSeconds)
	{
		tier = L"SILVER";
		bonusMudosa = (unsigned int)(m_cfg.timeAttackBonusMudosa * 0.7f);
	}
	else if (elapsedSeconds <= m_cfg.timeAttackBronzeSeconds)
	{
		tier = L"BRONZE";
		bonusMudosa = (unsigned int)(m_cfg.timeAttackBonusMudosa * 0.5f);
	}

	if (tier && bonusMudosa > 0)
	{
		wchar_t msg[256];
		swprintf_s(msg, L"[EVENT] TIME ATTACK: %s Tier! (%u seconds) +%u Mudosa for all!",
			tier, elapsedSeconds, bonusMudosa);
		BroadcastSystem(msg);
		SendNotice(msg, SERVER_TEXT_SYSNOTICE);

		// Award time bonus to all active participants
		for (unsigned int charId : m_participants)
		{
			auto it = m_roundContribution.find(charId);
			if (it != m_roundContribution.end() && it->second > 0)
			{
				CPlayer* pPlayer = g_pObjectManager->GetPC(charId);
				if (pPlayer && pPlayer->IsInitialized())
				{
					pPlayer->UpdateMudosaPoints(pPlayer->GetMudosaPoints() + bonusMudosa, true);
				}
			}
		}
	}
}

unsigned int CEventManager::GetDynamicMobCount(unsigned int baseCount)
{
	if (!m_cfg.enableDynamicDifficulty)
		return baseCount;

	unsigned int participantCount = (unsigned int)m_participants.size();
	if (participantCount <= 1)
		return baseCount;

	// Scale based on participant count
	float multiplier = 1.0f + (m_cfg.difficultyPerPlayer * (participantCount - 1));
	unsigned int scaledCount = (unsigned int)(baseCount * multiplier);

	EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Dynamic difficulty: %u participants, %u -> %u mobs (x%.2f)",
		participantCount, baseCount, scaledCount, multiplier);

	return scaledCount;
}

float CEventManager::GetPartyBonusMultiplier(CPlayer* pPlayer)
{
	if (!m_cfg.enablePartyBonus || !pPlayer)
		return 1.0f;

	// Check if player is in party (use existing Party API)
	if (pPlayer->GetParty() && pPlayer->GetParty()->GetPartyMemberCount() > 1)
	{
		return m_cfg.partyBonusMultiplier;
	}

	return 1.0f;
}

//--------------------------------------------------------------------------------------//
//  TEAM COMPETITION MODE
//--------------------------------------------------------------------------------------//

void CEventManager::AssignTeams()
{
	m_playerTeam.clear();

	// Convert participants to vector for shuffling
	std::vector<unsigned int> participantList(m_participants.begin(), m_participants.end());

	// Shuffle for random team assignment (use rand for simplicity)
	for (size_t i = participantList.size() - 1; i > 0; --i)
	{
		size_t j = rand() % (i + 1);
		std::swap(participantList[i], participantList[j]);
	}

	// Split evenly into Red and Blue teams
	size_t halfPoint = participantList.size() / 2;

	for (size_t i = 0; i < participantList.size(); i++)
	{
		Team assignedTeam = (i < halfPoint) ? Team::RED : Team::BLUE;
		m_playerTeam[participantList[i]] = assignedTeam;

		// Notify player of their team
		CPlayer* pPlayer = g_pObjectManager->GetPC(participantList[i]);
		if (pPlayer && pPlayer->IsInitialized())
		{
			wchar_t msg[128];
			swprintf_s(msg, L"[EVENT] You are on %s! Work together to win!", GetTeamName(assignedTeam));
			SendSystemTo(pPlayer, msg);
		}
	}

	// Announce team setup
	wchar_t msg[256];
	unsigned int redCount = (unsigned int)halfPoint;
	unsigned int blueCount = (unsigned int)(participantList.size() - halfPoint);
	swprintf_s(msg, L"[EVENT] RED TEAM (%u players) vs BLUE TEAM (%u players)!", redCount, blueCount);
	BroadcastSystem(msg);
	SendNotice(msg, SERVER_TEXT_SYSNOTICE);
}

void CEventManager::ShowTeamScores()
{
	if (m_teamRedDamage == 0 && m_teamBlueDamage == 0)
		return;

	BroadcastSystem(L"============= TEAM RESULTS =============");
	BroadcastSystem(L"         TEAM COMPETITION RESULTS        ");
	BroadcastSystem(L"=========================================");

	wchar_t msg[256];

	// Red team stats
	swprintf_s(msg, L"RED TEAM: %u damage | %u kills", m_teamRedDamage, m_teamRedKills);
	BroadcastSystem(msg);

	// Blue team stats
	swprintf_s(msg, L"BLUE TEAM: %u damage | %u kills", m_teamBlueDamage, m_teamBlueKills);
	BroadcastSystem(msg);

	// Determine winner
	Team winningTeam = Team::NONE;
	if (m_teamRedDamage > m_teamBlueDamage)
		winningTeam = Team::RED;
	else if (m_teamBlueDamage > m_teamRedDamage)
		winningTeam = Team::BLUE;

	if (winningTeam != Team::NONE)
	{
		swprintf_s(msg, L"%s WINS! Bonus rewards awarded!", GetTeamName(winningTeam));
		BroadcastSystem(msg);
		SendNotice(msg, SERVER_TEXT_SYSNOTICE);
	}
	else
	{
		BroadcastSystem(L"IT'S A TIE! Both teams performed equally!");
	}
}

void CEventManager::AwardTeamBonuses()
{
	if (m_cfg.teamCompetitionBonusMudosa == 0)
		return;

	// Determine winning team
	Team winningTeam = Team::NONE;
	if (m_teamRedDamage > m_teamBlueDamage)
		winningTeam = Team::RED;
	else if (m_teamBlueDamage > m_teamRedDamage)
		winningTeam = Team::BLUE;

	if (winningTeam == Team::NONE)
		return; // No bonus on tie

	// Award bonus to winning team members
	unsigned int bonusAwarded = 0;
	for (const auto& entry : m_playerTeam)
	{
		if (entry.second != winningTeam)
			continue;

		// Check if player participated (dealt damage)
		auto damageIt = m_roundContribution.find(entry.first);
		if (damageIt == m_roundContribution.end() || damageIt->second == 0)
			continue;

		CPlayer* pPlayer = g_pObjectManager->GetPC(entry.first);
		if (pPlayer && pPlayer->IsInitialized())
		{
			pPlayer->UpdateMudosaPoints(pPlayer->GetMudosaPoints() + m_cfg.teamCompetitionBonusMudosa, true);

			wchar_t msg[128];
			swprintf_s(msg, L"[EVENT] Team Victory Bonus: +%u Mudosa!", m_cfg.teamCompetitionBonusMudosa);
			SendSystemTo(pPlayer, msg);
			bonusAwarded++;
		}
	}

	EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Team competition: %s won, %u players awarded bonus",
		GetTeamName(winningTeam) == L"RED TEAM" ? "RED" : "BLUE", bonusAwarded);
}

const wchar_t* CEventManager::GetTeamName(Team team)
{
	switch (team)
	{
	case Team::RED: return L"RED TEAM";
	case Team::BLUE: return L"BLUE TEAM";
	default: return L"NO TEAM";
	}
}

const wchar_t* CEventManager::GetTeamColor(Team team)
{
	switch (team)
	{
	case Team::RED: return L"[RED] ";
	case Team::BLUE: return L"[BLUE] ";
	default: return L"";
	}
}

// ========================================
// Event Helpers System
// ========================================

void CEventManager::SpawnEventHelpers()
{
	if (!m_cfg.eventHelpersEnabled || m_cfg.helpersPerPlayer == 0)
	{
		EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Helper spawn skipped: enabled=%d helpersPerPlayer=%u",
			m_cfg.eventHelpersEnabled, m_cfg.helpersPerPlayer);
		return;
	}

	if (m_cfg.helperMobId == 0)
	{
		ERR_LOG(LOG_GENERAL, _T("%s"), _T("[EVENT] Cannot spawn helpers: helperMobId is 0. Please configure a valid mob ID."));
		return;
	}

	CGameServer* app = (CGameServer*)g_pApp;
	if (!app || m_eventWorldId == 0)
	{
		ERR_LOG(LOG_GENERAL, _T("[EVENT] Cannot spawn helpers: app=%p eventWorldId=%u"), app, m_eventWorldId);
		return;
	}

	CWorld* pWorld = app->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)m_eventWorldId);
	if (!pWorld)
	{
		ERR_LOG(LOG_GENERAL, _T("[EVENT] Cannot spawn helpers: event world %u not found"), m_eventWorldId);
		return;
	}

	EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Starting helper spawn: world=%u participants=%u helperMobId=%u",
		m_eventWorldId, (unsigned)m_participants.size(), m_cfg.helperMobId);

	unsigned int spawnedCount = 0;
	unsigned int skippedCount = 0;
	for (unsigned int charId : m_participants)
	{
		CPlayer* pPlayer = g_pObjectManager->FindByChar((CHARACTERID)charId);
		if (!pPlayer || !pPlayer->IsInitialized())
		{
			EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Skipping helper for charId=%u: player not found or not initialized", charId);
			skippedCount++;
			continue;
		}

		// Check if player already has a helper
		if (m_playerHelpers.find(charId) != m_playerHelpers.end())
		{
			EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Skipping helper for %s: already has helper", pPlayer->GetCharName());
			skippedCount++;
			continue;
		}

		// Spawn helpers for this player
		for (unsigned int i = 0; i < m_cfg.helpersPerPlayer; i++)
		{
			CNtlVector spawnPos = pPlayer->GetCurLoc();
			// Offset slightly to avoid spawning on top of player
			float angle = RandomRangeF(0.0f, 6.28318530718f);
			spawnPos.x += cosf(angle) * m_cfg.helperFollowDistance;
			spawnPos.z += sinf(angle) * m_cfg.helperFollowDistance;

			CNtlVector dir(0.0f, 0.0f, 0.0f);
			CMonster* pHelper = pWorld->Add_Monster(m_cfg.helperMobId, spawnPos, dir, 0xFF);

			if (pHelper)
			{
				// Link helper to player (using charId and handle)
				pHelper->SetLinkPc(charId, pPlayer->GetID());

				// Track helper
				m_eventHelpers.push_back(pHelper->GetID());
				m_playerHelpers[charId] = pHelper->GetID();
				spawnedCount++;

				EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Spawned helper (mobId=%u handle=%u) for player %s (charId=%u)",
					m_cfg.helperMobId, pHelper->GetID(), pPlayer->GetCharName(), charId);

				// Send follow command (method is on CNpc, not BotAiController)
				sVECTOR3 vDest = { pPlayer->GetCurLoc().x, pPlayer->GetCurLoc().y, pPlayer->GetCurLoc().z };
				pHelper->SendCharStateFollowing(pPlayer->GetID(), m_cfg.helperFollowDistance,
					DBO_MOVE_FOLLOW_FRIENDLY, vDest, true);
			}
			else
			{
				ERR_LOG(LOG_GENERAL, _T("[EVENT] Failed to spawn helper (mobId=%u) for player %s"),
					m_cfg.helperMobId, pPlayer->GetCharName());
			}
		}
	}

	EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Helper spawn complete: spawned=%u skipped=%u total=%u",
		spawnedCount, skippedCount, (unsigned)m_participants.size());

	if (spawnedCount > 0)
	{
		wchar_t msg[256];
		swprintf_s(msg, L"[EVENT] %u NPC helper%s have joined the battle!", spawnedCount, spawnedCount == 1 ? L"" : L"s");
		BroadcastSystem(msg);
	}
	else if (m_participants.size() > 0)
	{
		ERR_LOG(LOG_GENERAL, _T("[EVENT] Failed to spawn any helpers despite having %u participants"), (unsigned)m_participants.size());
	}
}

void CEventManager::DespawnEventHelpers()
{
	if (m_eventHelpers.empty())
		return;

	unsigned int despawnedCount = 0;
	for (HOBJECT helperHandle : m_eventHelpers)
	{
		CNpc* pHelper = g_pObjectManager->GetNpc(helperHandle);
		if (pHelper && pHelper->IsInitialized())
		{
			// Use bot controller to despawn
			CBotAiController* pAI = (CBotAiController*)pHelper->GetBotController();
			if (pAI)
			{
				pAI->ChangeControlState_Despawn();
				despawnedCount++;
			}
		}
	}

	m_eventHelpers.clear();
	m_playerHelpers.clear();

	EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Despawned %u helpers", despawnedCount);
}

void CEventManager::DespawnPlayerHelper(unsigned int charId)
{
	auto it = m_playerHelpers.find(charId);
	if (it == m_playerHelpers.end())
		return;

	HOBJECT helperHandle = it->second;
	CNpc* pHelper = g_pObjectManager->GetNpc(helperHandle);
	if (pHelper && pHelper->IsInitialized())
	{
		CBotAiController* pAI = (CBotAiController*)pHelper->GetBotController();
		if (pAI)
		{
			pAI->ChangeControlState_Despawn();
			EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Despawned helper for player charId=%u", charId);
		}
	}

	// Remove from tracking
	m_playerHelpers.erase(it);
	auto vecIt = std::find(m_eventHelpers.begin(), m_eventHelpers.end(), helperHandle);
	if (vecIt != m_eventHelpers.end())
		m_eventHelpers.erase(vecIt);
}

void CEventManager::DespawnAllEventMobs()
{
	if (m_spawnedMobs.empty())
		return;

	unsigned int despawnedCount = 0;
	unsigned int alreadyDeadCount = 0;

	for (HOBJECT mobHandle : m_spawnedMobs)
	{
		// Skip if already killed
		if (m_killedMobs.find(mobHandle) != m_killedMobs.end())
		{
			alreadyDeadCount++;
			continue;
		}

		CCharacter* pChar = g_pObjectManager->GetChar(mobHandle);
		CMonster* pMob = dynamic_cast<CMonster*>(pChar);
		if (pMob && pMob->IsInitialized() && pMob->GetCurWorld())
		{
			// Use bot controller to safely despawn the mob (same as Arena)
			if (pMob->GetBotController())
			{
				pMob->GetBotController()->ChangeControlState_Despawn();
				despawnedCount++;
			}
		}
	}

	// Clear tracking lists
	m_spawnedMobs.clear();
	m_killedMobs.clear();

	EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Mob cleanup: despawned=%u dead=%u total=%u",
		despawnedCount, alreadyDeadCount, despawnedCount + alreadyDeadCount);
}
