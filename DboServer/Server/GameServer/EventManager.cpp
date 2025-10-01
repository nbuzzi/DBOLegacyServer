#include "stdafx.h"
#include "EventManager.h"
#include "NtlIniFile.h"
#include "GameServer.h"
#include "ObjectManager.h"
#include "CPlayer.h"
#include "NtlPacketGU.h"
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

static unsigned long ToMs(unsigned int seconds) { return seconds * 1000UL; }

#define EVENT_VLOG(cfg, category, ...) do { \
	if ((cfg).verboseLogs) { \
		char buf[1024]; \
		buf[0] = '\0'; \
		va_list ap; \
		va_start(ap, __VA_ARGS__); \
		vsnprintf_s(buf, sizeof(buf), _TRUNCATE, __VA_ARGS__, ap); \
		va_end(ap); \
		NTL_PRINT(category, _T("%S"), buf); \
	} \
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
{
}

CEventManager::~CEventManager()
{
}

bool CEventManager::LoadConfigFromIniPath(const char* iniPath)
{
	CNtlIniFile file;
	if (!file.Create(iniPath))
	{
		ERR_LOG(LOG_GENERAL, "[EVENT] Failed to load config from %s", iniPath);
		return false;
	}

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

	int spectators = 0;
	if (file.Read("Event", "SpectatorsEnabled", spectators))
		m_cfg.spectatorsEnabled = (spectators != 0);

	int verbose = 0;
	if (file.Read("Event", "VerboseLogs", verbose))
		m_cfg.verboseLogs = (verbose != 0);

	// Parse rounds configuration
	CNtlString roundsCsv = file.Read("Event", "Rounds");
	ParseRoundsCsv(roundsCsv);

	ERR_LOG(LOG_GENERAL, "[EVENT] Loaded config: enabled=%d channel='%s' rounds=%u",
		(int)m_cfg.enabled, m_cfg.channelNameContains.c_str(), (unsigned)m_cfg.rounds.size());

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
				// Mobs list: "mob1,mob2,mob3"
				std::istringstream mobSs(part);
				std::string mobToken;
				while (std::getline(mobSs, mobToken, ','))
				{
					unsigned int mobId = (unsigned int)atoi(mobToken.c_str());
					if (mobId > 0)
						round.mobTblidxList.push_back(mobId);
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
					// Optional world ID for fixed rewards
					round.worldTblidx = (unsigned int)atoi(part.c_str());
				}
			}
			else if (partIdx == 5)
			{
				// Optional world ID (for range-based loot format)
				round.worldTblidx = (unsigned int)atoi(part.c_str());
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

void CEventManager::TickProcess(unsigned long dwTickDiff)
{
	if (!m_cfg.enabled)
		return;

	switch (m_state)
	{
	case State::ENROLLMENT:
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
				BroadcastSystem(L"[EVENT] Enrollment closed. Teleporting participants...");
				TeleportParticipants();
				m_state = State::PRE_ROUND;
				m_startDelayRemainMs = ToMs(m_cfg.startDelaySeconds);
			}
			else
			{
				BroadcastSystem(L"[EVENT] No participants. Event cancelled.");
				m_state = State::IDLE;
			}
		}
		break;

	case State::PRE_ROUND:
		if (m_startDelayRemainMs > dwTickDiff)
		{
			m_startDelayRemainMs -= dwTickDiff;
		}
		else
		{
			m_startDelayRemainMs = 0;
			StartNextRound();
		}
		break;

	case State::IN_ROUND:
		// Update round timer
		if (m_roundTimerActive)
		{
			if (m_roundRemainMs > dwTickDiff)
			{
				m_roundRemainMs -= dwTickDiff;
			}
			else
			{
				m_roundRemainMs = 0;
				// Round timeout
				BroadcastSystem(L"[EVENT] Round timed out!");
				CompleteCurrentRound();
			}
		}

		// Check if all mobs killed
		CheckRoundCompletion();
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
			PostEventTeleportAll();
			m_state = State::IDLE;
			m_participants.clear();
			m_spectators.clear();
			m_spawnedMobs.clear();
			m_killedMobs.clear();
			m_prevLoc.clear();
			m_currentRound = 0;
		}
		break;

	default:
		break;
	}
}

void CEventManager::AutomationTick(unsigned long dwTickDiff)
{
	if (!m_cfg.enabled || !m_cfg.autoEnabled)
		return;

	if (!IsChannelValid())
		return;

	switch (m_autoState)
	{
	case AutoState::OFF:
		m_autoState = AutoState::WAIT_NEXT;
		m_autoRemainMs = ToMs(m_cfg.autoInitialDelaySeconds);
		break;

	case AutoState::WAIT_NEXT:
		if (m_autoRemainMs > dwTickDiff)
		{
			m_autoRemainMs -= dwTickDiff;
		}
		else
		{
			m_autoRemainMs = 0;
			if (m_state == State::IDLE)
			{
				Start();
				m_autoState = AutoState::ENROLLMENT_OPEN;
			}
			else
			{
				m_autoRemainMs = ToMs(60); // Check again in 1 minute
			}
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
		ERR_LOG(LOG_GENERAL, "[EVENT] Cannot start: invalid channel");
		return;
	}

	if (m_state != State::IDLE)
	{
		ERR_LOG(LOG_GENERAL, "[EVENT] Cannot start: already running (state=%d)", (int)m_state);
		return;
	}

	m_state = State::ENROLLMENT;
	m_enrollmentRemainMs = ToMs(m_cfg.enrollmentSeconds);
	m_currentRound = 0;
	m_participants.clear();
	m_spectators.clear();
	m_killedMobs.clear();

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
}

void CEventManager::Stop(bool abort)
{
	if (m_state == State::IDLE)
		return;

	if (abort)
		BroadcastSystem(L"[EVENT] Event has been aborted by GM.");
	else
		BroadcastSystem(L"[EVENT] Event stopped.");

	// Teleport everyone back
	PostEventTeleportAll();

	m_state = State::IDLE;
	m_participants.clear();
	m_spectators.clear();
	m_spawnedMobs.clear();
	m_killedMobs.clear();
	m_prevLoc.clear();
	m_currentRound = 0;
}

void CEventManager::StartNextRound()
{
	if (m_currentRound >= m_cfg.rounds.size())
	{
		m_state = State::COMPLETE;
		return;
	}

	const EventRound& round = m_cfg.rounds[m_currentRound];
	m_killedMobs.clear();

	// Get world for this round (supports rotation)
	unsigned int worldTblidx = GetWorldForRound(m_currentRound);

	// Teleport participants to the round's world if it changed
	if (m_cfg.worldRotationEnabled || round.worldTblidx > 0)
	{
		float x, y, z;
		GetSpawnPosForRound(m_currentRound, x, y, z);
		TeleportParticipantsToWorld(worldTblidx, x, y, z);
	}

	wchar_t msg[256];
	swprintf_s(msg, L"[EVENT] Round %u/%u starting! Kill all mobs to proceed.",
		m_currentRound + 1, (unsigned)m_cfg.rounds.size());
	BroadcastSystem(msg);

	m_state = State::IN_ROUND;
	SpawnRoundMobs(round);

	if (round.durationSeconds > 0)
	{
		StartRoundTimer(round.durationSeconds);
	}

	EVENT_VLOG(m_cfg, LOG_GENERAL, "[EVENT] Round %u started: %u mobs, %u seconds, world=%u",
		m_currentRound + 1, (unsigned)round.mobTblidxList.size(), round.durationSeconds, worldTblidx);
}

void CEventManager::CompleteCurrentRound()
{
	if (m_currentRound >= m_cfg.rounds.size())
		return;

	const EventRound& round = m_cfg.rounds[m_currentRound];

	StopRoundTimer();

	wchar_t msg[256];
	swprintf_s(msg, L"[EVENT] Round %u completed!", m_currentRound + 1);
	BroadcastSystem(msg);

	// Award rewards
	AwardRoundRewards(round);

	m_currentRound++;

	if (m_currentRound < m_cfg.rounds.size())
	{
		m_state = State::INTERMISSION;
		m_startDelayRemainMs = ToMs(5); // 5 second intermission
	}
	else
	{
		// All rounds complete
		m_state = State::COMPLETE;
		m_postEventTeleportRemainMs = m_cfg.postEventTeleportDelayMs;

		// Award completion bonus
		if (m_cfg.mudosaEventComplete > 0)
		{
			for (unsigned int charId : m_participants)
			{
				CPlayer* pPlayer = g_pObjectManager->GetPC(charId);
				if (pPlayer && pPlayer->IsInitialized())
				{
					pPlayer->UpdateMudosaToken(m_cfg.mudosaEventComplete, true);
				}
			}
		}

		BroadcastSystem(L"[EVENT] All rounds completed! Congratulations!");
	}
}

void CEventManager::SpawnRoundMobs(const EventRound& round)
{
	// Get world for current round
	unsigned int worldTblidx = GetWorldForRound(m_currentRound);

	CWorld* pWorld = g_pObjectManager->GetWorldByTblidx(worldTblidx);
	if (!pWorld)
	{
		ERR_LOG(LOG_GENERAL, "[EVENT] Failed to get event world tblidx=%u", worldTblidx);
		return;
	}
	m_eventWorldId = pWorld->GetID();

	// Get spawn position for current round
	float spawnX, spawnY, spawnZ;
	GetSpawnPosForRound(m_currentRound, spawnX, spawnY, spawnZ);
	CNtlVector spawnCenter(spawnX, spawnY, spawnZ);

	for (unsigned int mobTblidx : round.mobTblidxList)
	{
		CNtlVector spawnPos = spawnCenter;

		if (m_cfg.randomMobPositions && m_cfg.mobSpawnRadius > 0)
		{
			float angle = RandomRangeF(0.0f, 6.28318530718f); // 2*PI
			float distance = RandomRangeF(0.0f, m_cfg.mobSpawnRadius);
			spawnPos.x += cosf(angle) * distance;
			spawnPos.z += sinf(angle) * distance;
		}

		CNtlVector dir(0.0f, 0.0f, 0.0f);
		CMonster* pMob = pWorld->Add_Monster(mobTblidx, spawnPos, dir, 0xFF);

		if (pMob)
		{
			m_spawnedMobs.push_back(pMob->GetID());

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
	}
}

void CEventManager::SpawnMinionsAroundBoss(const CNtlVector& bossPos, const MinionGroup& minionGroup, unsigned int worldId)
{
	CWorld* pWorld = g_pObjectManager->GetWorld(worldId);
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
	if (round.useLootRange && round.lootRangeItemId > 0)
	{
		// Create loot in range
		CNtlVector center(m_cfg.spawnPosX, m_cfg.spawnPosY, m_cfg.spawnPosZ);
		CreateLootInRange(round.lootRangeItemId, round.lootRangeCount, center, m_cfg.mobSpawnRadius);
	}
	else
	{
		// Award fixed rewards to all participants
		for (unsigned int charId : m_participants)
		{
			CPlayer* pPlayer = g_pObjectManager->GetPC(charId);
			if (pPlayer && pPlayer->IsInitialized())
			{
				for (const auto& reward : round.fixedRewards)
				{
					pPlayer->GetItemManager()->CreateItem(reward.first, reward.second, true, "[EVENT]");
				}

				if (m_cfg.mudosaPerRound > 0)
				{
					pPlayer->UpdateMudosaToken(m_cfg.mudosaPerRound, true);
				}
			}
		}
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
			CMonster* pMob = (CMonster*)g_pObjectManager->GetObject(mobHandle);
			if (pMob && !pMob->IsDead())
			{
				allKilled = false;
				break;
			}
		}
	}

	if (allKilled && m_spawnedMobs.size() > 0)
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
		pPlayer->GetPcProfile()->wszCharName, (unsigned)m_participants.size());
	BroadcastSystem(msg);

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
	for (unsigned int charId : m_participants)
	{
		CPlayer* pPlayer = g_pObjectManager->GetPC(charId);
		if (pPlayer && pPlayer->IsInitialized())
		{
			PrevLoc loc;
			loc.worldId = pPlayer->GetWorldID();
			loc.loc = pPlayer->GetCurLoc();
			loc.dir = pPlayer->GetDirection();
			m_prevLoc[charId] = loc;
		}
	}

	// Teleport to event world
	TeleportParticipantsToWorld(m_cfg.eventWorldTblidx, m_cfg.teleportPosX, m_cfg.teleportPosY, m_cfg.teleportPosZ);
}

void CEventManager::TeleportParticipantsToWorld(unsigned int worldTblidx, float x, float y, float z)
{
	for (unsigned int charId : m_participants)
	{
		CPlayer* pPlayer = g_pObjectManager->GetPC(charId);
		if (pPlayer && pPlayer->IsInitialized())
		{
			pPlayer->Teleport_Direct(worldTblidx, 0, x, y, z, 0, 0, 0);
		}
	}
}

void CEventManager::PostEventTeleportAll()
{
	if (!m_cfg.postEventTeleport)
		return;

	for (unsigned int charId : m_participants)
	{
		CPlayer* pPlayer = g_pObjectManager->GetPC(charId);
		if (pPlayer && pPlayer->IsInitialized())
		{
			// Try to teleport to saved location first
			auto it = m_prevLoc.find(charId);
			if (it != m_prevLoc.end())
			{
				const PrevLoc& loc = it->second;
				pPlayer->Teleport_Direct(loc.worldId, 0, loc.loc.x, loc.loc.y, loc.loc.z,
					loc.dir.x, loc.dir.y, loc.dir.z);
			}
			else
			{
				// Fallback to configured post-event location
				pPlayer->Teleport_Direct(m_cfg.postEventWorldTblidx, 0,
					m_cfg.postEventPosX, m_cfg.postEventPosY, m_cfg.postEventPosZ, 0, 0, 0);
			}
		}
	}
}

void CEventManager::OnPlayerEnterWorld(CPlayer* pPlayer)
{
	if (!pPlayer || !IsParticipant(pPlayer))
		return;

	// Player entered event world, sync state if needed
	if (m_state == State::IN_ROUND && m_roundTimerActive)
	{
		// Send round timer to player
		unsigned int seconds = (unsigned int)(m_roundRemainMs / 1000);
		// Could send dungeon state or timer here
	}
}

void CEventManager::StartRoundTimer(unsigned int seconds)
{
	m_roundTimerActive = true;
	m_roundRemainMs = ToMs(seconds);

	if (m_eventWorldId > 0)
	{
		BroadcastRoundTimerStartToWorld(m_eventWorldId, seconds);
	}
}

void CEventManager::StopRoundTimer()
{
	m_roundTimerActive = false;
	m_roundRemainMs = 0;

	if (m_eventWorldId > 0)
	{
		BroadcastRoundTimerEndToWorld(m_eventWorldId);
	}
}

void CEventManager::BroadcastRoundTimerStartToWorld(unsigned int worldId, unsigned int seconds)
{
	CNtlPacket packet(sizeof(sGU_DUNGEON_TIMER_START));
	sGU_DUNGEON_TIMER_START* res = (sGU_DUNGEON_TIMER_START*)packet.GetPacketData();
	res->wOpCode = GU_DUNGEON_TIMER_START;
	res->dwLimitTime = seconds;
	packet.SetPacketLen(sizeof(sGU_DUNGEON_TIMER_START));
	g_pObjectManager->SendPacketToWorld(worldId, &packet, false);
}

void CEventManager::BroadcastRoundTimerEndToWorld(unsigned int worldId)
{
	CNtlPacket packet(sizeof(sGU_DUNGEON_TIMER_END));
	sGU_DUNGEON_TIMER_END* res = (sGU_DUNGEON_TIMER_END*)packet.GetPacketData();
	res->wOpCode = GU_DUNGEON_TIMER_END;
	packet.SetPacketLen(sizeof(sGU_DUNGEON_TIMER_END));
	g_pObjectManager->SendPacketToWorld(worldId, &packet, false);
}

void CEventManager::BroadcastDungeonStateToWorld(unsigned int worldId, unsigned char byStage, unsigned int titleTblidx)
{
	CNtlPacket packet(sizeof(sGU_DUNGEON_DUNGEON_STATE));
	sGU_DUNGEON_DUNGEON_STATE* res = (sGU_DUNGEON_DUNGEON_STATE*)packet.GetPacketData();
	res->wOpCode = GU_DUNGEON_DUNGEON_STATE;
	res->byStage = byStage;
	res->dwLimitTime = 0;
	res->dungeonState.dungeonName = titleTblidx;
	res->dungeonState.subDungeonName = 0;
	packet.SetPacketLen(sizeof(sGU_DUNGEON_DUNGEON_STATE));
	g_pObjectManager->SendPacketToWorld(worldId, &packet, false);
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
	// Check if round has custom spawn position
	if (roundIndex < m_cfg.rounds.size())
	{
		const EventRound& round = m_cfg.rounds[roundIndex];
		if (round.spawnPosX != 0 || round.spawnPosY != 0 || round.spawnPosZ != 0)
		{
			outX = round.spawnPosX;
			outY = round.spawnPosY;
			outZ = round.spawnPosZ;
			return;
		}
	}

	// Use default spawn position
	outX = m_cfg.spawnPosX;
	outY = m_cfg.spawnPosY;
	outZ = m_cfg.spawnPosZ;
}

bool CEventManager::IsChannelValid()
{
	CGameServer* app = (CGameServer*)g_pApp;
	if (!app)
		return false;

	if (m_cfg.channelNameContains.c_str()[0] == '\0')
		return true;

	std::string want = m_cfg.channelNameContains.c_str();
	std::string got = app->m_config.ChannelName.c_str();

	std::transform(want.begin(), want.end(), want.begin(), ::tolower);
	std::transform(got.begin(), got.end(), got.begin(), ::tolower);

	return got.find(want) != std::string::npos;
}

void CEventManager::BroadcastSystem(const wchar_t* msg, unsigned char byType)
{
	CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
	res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
	res->byDisplayType = byType;
	NTL_SAFE_WCSCPY(res->awchMessage, msg);
	packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	g_pApp->BroadCast(&packet);
}

void CEventManager::SendSystemTo(CPlayer* pPlayer, const wchar_t* msg, unsigned char byType)
{
	if (!pPlayer)
		return;

	CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
	res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
	res->byDisplayType = byType;
	NTL_SAFE_WCSCPY(res->awchMessage, msg);
	packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	pPlayer->SendPacket(&packet);
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
