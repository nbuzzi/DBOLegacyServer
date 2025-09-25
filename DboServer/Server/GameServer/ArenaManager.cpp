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
#include <tchar.h>
#include <set>

static unsigned long ToMs(unsigned int seconds) { return seconds * 1000UL; }

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

	CNtlString worldsCsv = file.Read("Arena", "WorldTblidxList");
	ParseWorldListCsv(worldsCsv);

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
	int stopOnTimeout = 1;
	if (file.Read("Arena", "StopOnTimeout", stopOnTimeout)) m_cfg.stopOnTimeout = (stopOnTimeout != 0);
	int reviveOnFaint = 0;
	if (file.Read("Arena", "ReviveOnFaint", reviveOnFaint)) m_cfg.reviveOnFaint = (reviveOnFaint != 0);
	int faintBecomeSpectator = 1;
	if (file.Read("Arena", "FaintBecomeSpectator", faintBecomeSpectator)) m_cfg.faintBecomeSpectator = (faintBecomeSpectator != 0);
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

	// CC Battle Mode (RankBattle style)
	int ccBattleMode = 0;
	if (file.Read("Arena", "CCBattleMode", ccBattleMode)) m_cfg.ccBattleMode = (ccBattleMode != 0);

	// Allow custom world overrides (GM commands, config WorldTblidxList)
	int allowCustomWorlds = 0;
	if (file.Read("Arena", "AllowCustomWorlds", allowCustomWorlds)) m_cfg.allowCustomWorlds = (allowCustomWorlds != 0);

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

	// Always try to load available worlds from RankBattle table first (they're proven to work)
	// If AllowCustomWorlds is enabled and that fails, fall back to config WorldTblidxList
	size_t originalWorldCount = m_cfg.worldTblidxList.size();
	NTL_PRINT(PRINT_APP, _T("[ARENA] Before LoadAvailableWorlds: %u worlds in config, AllowCustomWorlds=%d"),
		(unsigned)originalWorldCount, m_cfg.allowCustomWorlds ? 1 : 0);

	LoadAvailableWorlds();

	NTL_PRINT(PRINT_APP, _T("[ARENA] After LoadAvailableWorlds: %u worlds loaded"), (unsigned)m_cfg.worldTblidxList.size());

	// If no worlds loaded from RankBattle table, fall back to config worlds to keep Arena functional
	if (m_cfg.worldTblidxList.empty() && originalWorldCount > 0)
	{
		NTL_PRINT(PRINT_APP, _T("[ARENA] No RankBattle worlds found. Falling back to config WorldTblidxList (AllowCustomWorlds=%d)"), m_cfg.allowCustomWorlds ? 1 : 0);
		CNtlString worldsCsv = file.Read("Arena", "WorldTblidxList");
		ParseWorldListCsv(worldsCsv);
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
	}
	else
	{
		NTL_PRINT(PRINT_APP, _T("[ARENA] Warning: No worlds available - arena will not function properly"));
	}

	return true;
}

// Broadcast RANKBATTLE_BATTLE_PLAYER_STATE_NFY with ATTACKABLE for current participants in world
static void ArenaBroadcastRankPlayerAttackableToWorld(CWorld* pWorld, const std::unordered_set<unsigned int>& participants)
{
	if (!pWorld) return;
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
		rd->eState = RANKBATTLE_MEMBER_STATE_ATTACKABLE;

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

void CArenaManager::TickProcess(unsigned long dwTickDiff)
{
	// Do nothing unless the Arena feature is enabled and actively started
	if (!m_cfg.enabled || m_state == State::IDLE)
		return;

	// Decrement run-settle window so we don't prematurely end rounds at start
	if (m_runSettleMs > 0)
	{
		if (m_runSettleMs > dwTickDiff) m_runSettleMs -= dwTickDiff; else m_runSettleMs = 0;
	}

	// Auto-detect if participants are already in arena world while in ENROLLMENT state
	// This handles cases where players were teleported manually or the state got stuck
	if (m_state == State::ENROLLMENT && !m_participants.empty())
	{
		unsigned int arenaWorldId = EnsureCurrentWorldId();
		if (arenaWorldId > 0)
		{
			unsigned int participantsInArena = CountParticipantsInWorld(arenaWorldId);
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
				SendNotice(L"Arena battle sequence starting...", m_cfg.noticeType);
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
					SendNotice(L"Arena canceled: no participants online.", m_cfg.noticeType);
					NTL_PRINT(PRINT_APP, _T("[ARENA] Start canceled: no participants online"));
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
		}
	}
	// OLD SEQUENCE SYSTEM - DISABLED in favor of proper state-based approach
	// The old sequence system is replaced by MATCH_READY -> STAGE_READY -> IN_ROUND states
	/*
	// Drive timed rank-like sequence before starting dungeon UI
	// First, honor any extra post-ready delay before beginning the sequence
	if (m_state == State::IN_ROUND && m_pendingStartWorldId == 0 && m_seqStep == 0 && m_postStartDelayMs > 0)
	{
		if (m_postStartDelayMs > dwTickDiff)
			m_postStartDelayMs -= dwTickDiff;
		else
		{
			m_postStartDelayMs = 0;
			// Initialize timed rank-like opening sequence (or skip if Rank UI disabled)
			m_seqStep = m_cfg.rankUiEnabled ? 1 : 5;
			m_seqRemainMs = m_cfg.rankSequenceStepMs;
		}
	}
	if (m_state == State::IN_ROUND && m_seqStep > 0 && m_pendingStartWorldId == 0)
	{
		if (m_seqRemainMs > dwTickDiff)
		{
			m_seqRemainMs -= dwTickDiff;
		}
		else
		{
			m_seqRemainMs = m_cfg.rankSequenceStepMs;
			unsigned int worldId = EnsureCurrentWorldId();
			switch (m_seqStep)
			{
			case 1: // DIRECTION (after implicit WAIT)
				BroadcastRankStateToWorld(worldId, 1, 0); // RANKBATTLE_BATTLESTATE_DIRECTION
				m_seqStep = 2;
				break;
			case 2: // STAGE_PREPARE
				BroadcastRankStateToWorld(worldId, 2, 0); // RANKBATTLE_BATTLESTATE_STAGE_PREPARE
				m_seqStep = 3;
				break;
			case 3: // STAGE_READY
				BroadcastRankStateToWorld(worldId, 3, 0); // RANKBATTLE_BATTLESTATE_STAGE_READY (first round -> stage 0)
				m_seqStep = 4;
				break;
			case 4: // MATCH_START
				BroadcastRankMatchStartToWorld(worldId);
				BroadcastRankStateToWorld(worldId, 4, 0); // RANKBATTLE_BATTLESTATE_STAGE_RUN (first round -> stage 0)
				m_seqStep = 5;
				break;
			case 5: // RUN -> now start dungeon UI and other start effects
				m_seqStep = 0;
				// Do not send dungeon UI yet; wait until battle actually starts
				// BroadcastDungeonStateToWorld(worldId);
				if (m_cfg.roundTimerSeconds > 0)
					StartRoundTimerUI(m_cfg.roundTimerSeconds);
				SendNotice(L"Arena round started!", m_cfg.noticeType);
				if (m_cfg.mobsAllowed) SpawnArenaMobs();
				if (m_cfg.telecastEnabled) BroadcastTelecastToWorld(worldId);
				break;
			}
		}
	}
	*/
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

		if (m_waitAllArriveMs <= dwTickDiff)
		{
			// Grace expired: cancel if nobody present, otherwise proceed
			if (present == 0)
			{
				m_waitAllArriveMs = 0;
				m_pendingStartWorldId = 0;
				m_state = State::ENROLLMENT;
				SendNotice(L"Arena canceled: nobody arrived to the arena.", m_cfg.noticeType);
				NTL_PRINT(PRINT_APP, _T("[ARENA] Start canceled: present=0 at timeout"));
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
				// RankBattle opening: WAIT(stage) -> TeamInfo -> DIRECTION(stage)
				BroadcastRankStateToWorld(worldId, RANKBATTLE_BATTLESTATE_WAIT, m_rankBattleStage);
				BroadcastRankTeamInfoToWorld(worldId);
				UpdateRankBattleState(RANKBATTLE_BATTLESTATE_DIRECTION, m_rankBattleStage, 3000);
				// For the first round, some clients expect an early MATCH_START after DIRECTION
				if (m_rankBattleStage == 0)
					BroadcastRankMatchStartToWorld(worldId);
				SendNotice(L"Arena match starting...", m_cfg.noticeType);
				NTL_PRINT(PRINT_APP, _T("[ARENA] PRE_ROUND -> MATCH_READY: present=%u ready=%u worldId=%u (grace expired)"), present, ready, worldId);
			}
		}
		else if (present > 0 && ready >= present)
		{
			// Everyone present and ready within grace period
			m_waitAllArriveMs = 0;
			m_pendingStartWorldId = 0;
			m_state = State::MATCH_READY;
			m_directionTimeMs = 5000; // direction/intro
			// Bind current world id just in case it wasn't set yet
			m_currentWorldId = worldId;
			// RankBattle opening: WAIT(stage) -> TeamInfo -> DIRECTION(stage)
			BroadcastRankStateToWorld(worldId, RANKBATTLE_BATTLESTATE_WAIT, m_rankBattleStage);
			BroadcastRankTeamInfoToWorld(worldId);
			UpdateRankBattleState(RANKBATTLE_BATTLESTATE_DIRECTION, m_rankBattleStage, 3000);
			if (m_rankBattleStage == 0)
				BroadcastRankMatchStartToWorld(worldId);
			SendNotice(L"Arena match starting...", m_cfg.noticeType);
			NTL_PRINT(PRINT_APP, _T("[ARENA] PRE_ROUND -> MATCH_READY: present=%u ready=%u worldId=%u"), present, ready, worldId);
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
		// Early start if everyone already accepted
		unsigned int worldIdNow = m_currentWorldId ? m_currentWorldId : EnsureCurrentWorldId();
		if (worldIdNow && CountParticipantsInWorld(worldIdNow) == m_participants.size() && m_participants.size() > 0)
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
			unsigned accepted = CountParticipantsInWorld(worldId);

			NTL_PRINT(PRINT_APP, _T("[ARENA] Invite timeout: worldId=%u, accepted=%u, total participants=%u"),
				worldId, accepted, (unsigned)m_participants.size());

			if (accepted == 0)
			{
				SendNotice(L"Arena invite timed out. No participants accepted.", m_cfg.noticeType);
				m_state = State::ENROLLMENT;
				// Clear invite UI on clients
				for (auto cid : m_participants)
				{
					if (CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid))
					{
						// Cancel any Arena invite proposals (RB or Dojo)
						p->CancelTeleportProposal(TELEPORT_TYPE_RANKBATTLE);
						p->CancelTeleportProposal(TELEPORT_TYPE_DOJO);
					}
				}
				NTL_PRINT(PRINT_APP, _T("[ARENA] Invite timeout: no acceptors. Reset to ENROLLMENT"));
			}
			else
			{
				// Require at least 2 participants to start a rank-like match
				if (accepted < 2)
				{
					SendNotice(L"Arena invite concluded: not enough participants accepted.", m_cfg.noticeType);
					m_state = State::ENROLLMENT;
					// Cancel any open proposals and keep players in place
					for (auto cid : m_participants)
					{
						if (CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid))
						{
							p->CancelTeleportProposal(TELEPORT_TYPE_RANKBATTLE);
							p->CancelTeleportProposal(TELEPORT_TYPE_DOJO);
						}
					}
					NTL_PRINT(PRINT_APP, _T("[ARENA] Invite end: accepted=%u < 2. Reset to ENROLLMENT"), accepted);
					return;
				}

				// Remove participants who didn't accept from the arena participant list
				std::unordered_set<unsigned int> acceptedParticipants;
				for (auto cid : m_participants)
				{
					CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid);
					if (p && p->IsInitialized() && (unsigned int)p->GetWorldID() == worldId)
					{
						acceptedParticipants.insert(cid);
					}
				}

				// Keep a copy of all invited participants before narrowing
				auto invitedBefore = m_participants;
				// Update participant list to only include those who accepted
				m_participants = acceptedParticipants;

				// Begin RankBattle-like opening: WAIT(0) -> TeamInfo -> DIRECTION(0) -> MATCH_START
				m_state = State::MATCH_READY;
				m_currentWorldId = worldId;

				// Ensure HUD is initialized just before start
				BroadcastRankStateToWorld(worldId, RANKBATTLE_BATTLESTATE_WAIT, 0);
				// Inform clients of RankBattle room context to satisfy HUD expectations
				if (m_cfg.rankUiEnabled) BroadcastRankJoinToWorld(worldId);

				// Send team composition
				BroadcastRankTeamInfoToWorld(worldId);
				// Enter DIRECTION state and start timer from RankBattle table if available
				DWORD dirMs = 3000; // default
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
				UpdateRankBattleState(RANKBATTLE_BATTLESTATE_DIRECTION, 0, dirMs);
				BroadcastRankMatchStartToWorld(worldId);

				wchar_t msg[128];
				swprintf_s(msg, _countof(msg), L"Arena starting with %u participants...", accepted);
				SendNotice(msg, m_cfg.noticeType);

				NTL_PRINT(PRINT_APP, _T("[ARENA] MATCH_READY: TeamInfo sent; DIRECTION for %ums"), (unsigned)dirMs);

				// Cancel any remaining pending proposals for players who did not accept
				for (auto cid : invitedBefore)
				{
					if (acceptedParticipants.find(cid) != acceptedParticipants.end())
						continue; // accepted; already moved
					if (CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid))
					{
						p->CancelTeleportProposal(TELEPORT_TYPE_RANKBATTLE);
						p->CancelTeleportProposal(TELEPORT_TYPE_DOJO);
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
						// Reset players (buffs/cc/targets/stats) only for rounds after the first
						if (m_rankBattleStage > 0)
						{
							ResetParticipantsBetweenRounds();
						}
						EnsureParticipantsStanding(worldId);
						// For party-only rank maps, mirror RankBattle UpdatePlayersAttackable
						if (IsPartyModeWorld(m_currentWorldTblidx))
						{
							for (auto cid : m_participants)
							{
								CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid);
								if (!p || !p->IsInitialized() || (unsigned int)p->GetWorldID() != worldId) continue;
								sRANK_BATTLE_DATA* rd = p->GetRankBattleData();
								rd->eState = RANKBATTLE_MEMBER_STATE_ATTACKABLE;
								p->SendCharStateStanding();
								CNtlPacket pkt(sizeof(sGU_RANKBATTLE_BATTLE_PLAYER_STATE_NFY));
								sGU_RANKBATTLE_BATTLE_PLAYER_STATE_NFY* res = (sGU_RANKBATTLE_BATTLE_PLAYER_STATE_NFY*)pkt.GetPacketData();
								res->wOpCode = GU_RANKBATTLE_BATTLE_PLAYER_STATE_NFY;
								res->hPc = p->GetID();
								res->byPCState = RANKBATTLE_MEMBER_STATE_ATTACKABLE;
								pkt.SetPacketLen(sizeof(sGU_RANKBATTLE_BATTLE_PLAYER_STATE_NFY));
								p->SendPacket(&pkt);
							}
						}
						CGameServer* app = (CGameServer*)g_pApp;
						if (CWorld* pWorld = app->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)worldId))
						{
							// Set server-side rank-battle state to ATTACKABLE for all participants prior to notify
							for (auto cid : m_participants)
							{
								CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid);
								if (p && p->IsInitialized() && (unsigned int)p->GetWorldID() == worldId)
									p->GetRankBattleData()->eState = RANKBATTLE_MEMBER_STATE_ATTACKABLE;
							}
							ArenaBroadcastRankPlayerAttackableToWorld(pWorld, m_participants);
						}
						// Send MATCH_START notify to fully unlock camera/input every round
						BroadcastRankMatchStartToWorld(worldId);
					}
					// Enter RUN for the current stage; preserve stage index for clients
					UpdateRankBattleState(RANKBATTLE_BATTLESTATE_STAGE_RUN, m_rankBattleStage, runMs);
					// Allow a short settle period before evaluating alive-count logic
					m_runSettleMs = 3000;
					m_state = State::IN_ROUND;
					// Announce dungeon stage after RUN so the HUD reflects the active round
					BroadcastDungeonStateToWorld(worldId);
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
						UpdateRankBattleState(RANKBATTLE_BATTLESTATE_STAGE_FINISH, m_rankBattleStage, stageFinishMs);
					}
					else
					{
						UpdateRankBattleState(RANKBATTLE_BATTLESTATE_MATCH_FINISH, m_rankBattleStage, matchFinishMs);
					}
					break;
				}
				case RANKBATTLE_BATTLESTATE_STAGE_FINISH:
				{
					// Ensure the previous round UI is fully cleared before advancing/rotating
					StopRoundTimerUI();
					if (m_currentWorldId)
						BroadcastCountdownToWorld(m_currentWorldId, false);
					// Advance to next round
					m_rankBattleStage = (BYTE)(m_rankBattleStage + 1);
					// If configured, rotate map between rounds
					if (m_cfg.mapPerRound && !m_cfg.worldTblidxList.empty())
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
									loc.x += RandomRangeF(-3.0f, 3.0f);
									loc.z += RandomRangeF(-3.0f, 3.0f);
									TeleportOneToWorldTblidx(p, m_currentWorldTblidx, loc.x, loc.y, loc.z);
									// Update team state hint for attack rules/UI
									p->GetRankBattleData()->eTeamType = (i < half) ? RANKBATTLE_TEAM_OWNER : RANKBATTLE_TEAM_CHALLENGER;
								}
							}
							// Bind to new world id for subsequent RB packets
							m_currentWorldId = newWorldId;
							// Initialize HUD in the new world before the next round
							BroadcastRankStateToWorld(newWorldId, RANKBATTLE_BATTLESTATE_WAIT, m_rankBattleStage);
							// Send JOIN again after map rotation so clients rebind to the room context
							if (m_cfg.rankUiEnabled) BroadcastRankJoinToWorld(newWorldId);
							BroadcastRankTeamInfoToWorld(newWorldId);
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
							BroadcastRankStateToWorld(worldId, RANKBATTLE_BATTLESTATE_WAIT, m_rankBattleStage);
							BroadcastRankTeamInfoToWorld(worldId);
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
							break;
						}
					}
					// Reset players between rounds (buffs/cc/targets/stats) before we start gating next round
					ReviveParticipantsForNextRound();
					// Fall back to PRE_ROUND gating path (handled above) which will drive the intro for the new stage
					break;
				}
				case RANKBATTLE_BATTLESTATE_MATCH_FINISH:
				{
					// Cleanup like RankBattle end; guard to prevent duplicate finalization
					if (m_state == State::IN_ROUND)
					{
						// Clear any timer/countdown HUD first
						StopRoundTimerUI();
						if (m_currentWorldId)
							BroadcastCountdownToWorld(m_currentWorldId, false);
						FinishMatch(false);
					}
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
			if (m_cfg.stopOnTimeout)
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
	}
}

void CArenaManager::Start(Mode mode)
{
	m_mode = mode;
	m_state = State::ENROLLMENT; // placeholder entry
	m_participants.clear();
	m_spectators.clear();
	m_winners.clear();
	m_killPoints.clear();
	// Reset settle window
	m_runSettleMs = 0;
	
	// Reset rank battle state for new match
	m_rankBattleState = INVALID_RANKBATTLE_BATTLESTATE;
	m_rankBattleStage = 0;
	m_rankStateTimeMs = 0;
	
	// Randomize current map from available list if configured
	if (m_cfg.randomizeMapOnStart && !m_cfg.worldTblidxList.empty())
	{
		m_worldIndex = (size_t)(rand() % m_cfg.worldTblidxList.size());
		m_currentWorldTblidx = m_cfg.worldTblidxList[m_worldIndex];
		m_currentWorldId = 0; // force fresh world
		NTL_PRINT(PRINT_APP, _T("[ARENA] Start: randomized map tblidx=%u (index=%u)"), m_currentWorldTblidx, (unsigned)m_worldIndex);
	}

	BroadcastSystem(L"[Arena] Started. Use @arena status for details.");

	// Mode-specific enrollment instructions
	const wchar_t* enrollMsg = L"Arena enrollment started. Use @arena join to participate.";
	switch (mode)
	{
	case Mode::PARTY_VS_PARTY:
		enrollMsg = L"Party vs Party Arena started. Use @arena joinparty to join with your party.";
		break;
	case Mode::GUILD_VS_GUILD:
		enrollMsg = L"Guild vs Guild Arena started. Use @arena joinguild to join with your guild.";
		break;
	case Mode::FREE_FOR_ALL:
		enrollMsg = L"Free For All Arena started. Use @arena join to participate.";
		break;
	case Mode::OPEN:
		enrollMsg = L"Open Arena started. Use @arena join to participate.";
		break;
	}

	SendNotice(enrollMsg, m_cfg.noticeType);
}

void CArenaManager::Stop(bool abort)
{
	m_state = State::COMPLETE;
	m_runSettleMs = 0;
	StopRoundTimerUI();
	// Notify RankBattle LEAVE for HUD cleanup
	if (m_cfg.rankUiEnabled && m_currentWorldId)
		BroadcastRankLeaveToWorld(m_currentWorldId);
	
	// Reset rank battle state
	m_rankBattleState = INVALID_RANKBATTLE_BATTLESTATE;
	m_rankBattleStage = 0;
	m_rankStateTimeMs = 0;
	
	if (m_cfg.rewardsEnabled && !abort)
	{
		AwardRewards(true);   // winners
		AwardRewards(false);  // all participants
	}
	BroadcastSystem(abort ? L"[Arena] Stopped (abort)." : L"[Arena] Completed.");
	// Also announce via Notice channel as requested
	SendNotice(abort ? L"Arena stopped." : L"Arena completed.", m_cfg.noticeType);
	// Always despawn arena mobs
	DespawnArenaMobs();
	// Always teleport everyone out: if configured world provided, use it; otherwise fallback to previous/bind
	if (m_cfg.postFinishTeleport)
		PostFinishTeleportAll();
	else
		PostFinishTeleportDefault();
	m_state = State::IDLE;
}

void CArenaManager::RotateMapNow()
{
	if (m_cfg.worldTblidxList.empty()) return;
	StopRoundTimerUI();
	m_worldIndex = (m_worldIndex + 1) % m_cfg.worldTblidxList.size();
	m_currentWorldTblidx = m_cfg.worldTblidxList[m_worldIndex];
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
	res->byDisplayType = SERVER_TEXT_SYSTEM;
	res->wMessageLengthInUnicode = (WORD)wcslen(buf);
	wcscpy_s(res->awchMessage, NTL_MAX_LENGTH_OF_CHAT_MESSAGE + 1, buf);
	packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	pWho->SendPacket(&packet);
}

void CArenaManager::BroadcastSystem(const wchar_t* msg)
{
	CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
	res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
	res->byDisplayType = SERVER_TEXT_SYSTEM;
	res->wMessageLengthInUnicode = (WORD)wcslen(msg);
	wcscpy_s(res->awchMessage, NTL_MAX_LENGTH_OF_CHAT_MESSAGE + 1, msg);
	packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	g_pObjectManager->SendPacketToAll(&packet);
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
				g_pItemManager->CreateItem(p, (TBLIDX)item, (BYTE)cnt);
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

void CArenaManager::TryRotateByTime(unsigned long dwTickDiff)
{
	if (m_cfg.rotationSeconds == 0 || m_cfg.worldTblidxList.size() <= 1)
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
	SendNotice(buf, m_cfg.noticeType);
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
	SendNotice(buf, m_cfg.noticeType);
	return true;
}

bool CArenaManager::Remove(CPlayer* pPlayer)
{
	if (!pPlayer) return false;
	m_participants.erase(pPlayer->GetCharID());
	m_spectators.erase(pPlayer->GetCharID());
	m_winners.erase(pPlayer->GetCharID());
	return true;
}

void CArenaManager::TeleportParticipants()
{
	TeleportParticipants(false); // Use normal flow
}

void CArenaManager::TeleportParticipants(bool forceDirect)
{
	// Validate team composition before starting the battle
	if (!ValidateTeamComposition())
	{
		// Do not Stop(true) here (which sets COMPLETE); stay in enrollment
		m_state = State::ENROLLMENT;
		return;
	}

	if (m_currentWorldTblidx == 0)
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
					BroadcastRankStateToWorld(existingWorldId, 2, 1); // PREPARE
					BroadcastRankStateToWorld(existingWorldId, 3, 1); // READY
				}
				SendNotice(L"Get ready for battle!", m_cfg.noticeType);
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
	// If invite flow enabled and not forced direct, send proposals to participants and defer start
	if (m_cfg.useInviteFlow && !forceDirect)
	{
		CGameServer* app = (CGameServer*)g_pApp;
		sWORLD_TBLDAT* pWorldTbldat = (sWORLD_TBLDAT*)g_pTableContainer->GetWorldTable()->FindData((TBLIDX)m_currentWorldTblidx);
		if (!pWorldTbldat) return;

		CWorld* pWorld = app->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)m_currentWorldId);
		if (!pWorld)
		{
			// In CC battle mode, always create a new world instance per match
			if (m_cfg.ccBattleMode)
			{
				pWorld = app->GetGameMain()->GetWorldManager()->CreateWorld(pWorldTbldat);
			}
			else
			{
				// In normal mode, try to find or create a shared world
				pWorld = app->GetGameMain()->GetWorldManager()->CreateWorld(pWorldTbldat);
			}
			if (!pWorld) return;
			m_currentWorldId = (unsigned int)pWorld->GetID();
		}

		// Choose teleport type
		// In CC battle mode, always use RANKBATTLE proposal to mirror RankBattle invites (more reliable client UI)
		BYTE tpType = m_cfg.ccBattleMode ? TELEPORT_TYPE_RANKBATTLE : ((pWorldTbldat->byWorldRuleType == GAMERULE_RANKBATTLE) ? TELEPORT_TYPE_RANKBATTLE : TELEPORT_TYPE_DOJO);
		// Use same index as type (previous working behavior)
		BYTE tpIndex = tpType;

		// CC Battle Mode: Assign teams to different spawn positions like RankBattle
			if (m_cfg.ccBattleMode)
		{
			size_t teamSize = m_participants.size() / 2;
			size_t teamIndex = 0;

			for (auto cid : m_participants)
			{
				CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid);
				if (!p || !p->IsInitialized()) continue;

				// Assign teams: first half to team 1 (start1), second half to team 2 (start2)
				CNtlVector destLoc, destDir;
					if (teamIndex < teamSize)
				{
					// Team 1 - use Start1 position
					destLoc = pWorldTbldat->vStart1Loc;
					destDir = pWorldTbldat->vStart1Dir;
					// Add some randomization to spawn positions
					destLoc.x += RandomRangeF(-3.0f, 3.0f);
					destLoc.z += RandomRangeF(-3.0f, 3.0f);
						p->GetRankBattleData()->eTeamType = RANKBATTLE_TEAM_OWNER;
				}
				else
				{
					// Team 2 - use Start2 position  
					destLoc = pWorldTbldat->vStart2Loc;
					destDir = pWorldTbldat->vStart2Dir;
					// Add some randomization to spawn positions
					destLoc.x += RandomRangeF(-3.0f, 3.0f);
					destLoc.z += RandomRangeF(-3.0f, 3.0f);
						p->GetRankBattleData()->eTeamType = RANKBATTLE_TEAM_CHALLENGER;
				}

				bool ok = p->StartTeleportProposal(NULL, (WORD)m_cfg.inviteWaitSeconds, tpType, tpIndex, (TBLIDX)m_currentWorldTblidx, pWorld->GetID(), destLoc, destDir);
				if (!ok)
				{
					bool dynamic = (p->GetCurWorld() && p->GetCurWorld()->GetTbldat()->bDynamic);
					NTL_PRINT(PRINT_APP, _T("[ARENA] Invite FAILED: char=%u type=%u idx=%u dynamic=%d hasWorld=%d. Fallback to direct teleport."),
						(unsigned)cid, (unsigned)tpType, (unsigned)tpIndex, dynamic ? 1 : 0, p->GetCurWorld() ? 1 : 0);
					// Fallback: direct-teleport if proposal cannot be shown (player in dynamic world, etc.)
					p->StartTeleport(destLoc, destDir, pWorld->GetID(), TELEPORT_TYPE_DOJO);
				}
				else
				{
					NTL_PRINT(PRINT_APP, _T("[ARENA] CC Battle invite sent: char=%u team=%u worldId=%u type=%u idx=%u"),
						(unsigned)cid, (teamIndex < teamSize ? 1 : 2), (unsigned)pWorld->GetID(), (unsigned)tpType, (unsigned)tpIndex);
				}
				teamIndex++;
			}
		}
		else
		{
			// Normal arena mode: all participants spawn at same location
			CNtlVector destLoc = pWorldTbldat->vStart1Loc;
			CNtlVector destDir = pWorldTbldat->vStart1Dir;

			for (auto cid : m_participants)
			{
				CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid);
				if (!p || !p->IsInitialized()) continue;

				bool ok = p->StartTeleportProposal(NULL, (WORD)m_cfg.inviteWaitSeconds, tpType, tpIndex, (TBLIDX)m_currentWorldTblidx, pWorld->GetID(), destLoc, destDir);
				if (!ok)
				{
					bool dynamic = (p->GetCurWorld() && p->GetCurWorld()->GetTbldat()->bDynamic);
					NTL_PRINT(PRINT_APP, _T("[ARENA] Invite FAILED: char=%u type=%u idx=%u dynamic=%d hasWorld=%d. Fallback to direct teleport."),
						(unsigned)cid, (unsigned)tpType, (unsigned)tpIndex, dynamic ? 1 : 0, p->GetCurWorld() ? 1 : 0);
					p->StartTeleport(destLoc, destDir, pWorld->GetID(), TELEPORT_TYPE_DOJO);
				}
				else
				{
					NTL_PRINT(PRINT_APP, _T("[ARENA] Invite sent: char=%u worldTblidx=%u worldId=%u type=%u idx=%u wait=%us"),
						(unsigned)cid, (unsigned)m_currentWorldTblidx, (unsigned)pWorld->GetID(), (unsigned)tpType, (unsigned)tpIndex, (unsigned)m_cfg.inviteWaitSeconds);
				}
			}
		}
		m_inviting = true;
		m_inviteRemainMs = ToMs(m_cfg.inviteWaitSeconds);
		m_state = State::PRE_ROUND;
		SendNotice(L"Arena invites sent. Please accept to join.", m_cfg.noticeType);
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
			p->StartTeleport(pGm->GetCurLoc(), pGm->GetCurDir(), pGm->GetWorldID(), TELEPORT_TYPE_DOJO);
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
			p->StartTeleport(pGm->GetCurLoc(), pGm->GetCurDir(), pGm->GetWorldID(), TELEPORT_TYPE_DOJO);
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
	unsigned lastAliveCharId = 0;
	std::vector<CPlayer*> toSpectate;
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
		}
	}
	// Do not move fainted players to spectator mid-round in CC mode; leave them as-is

	if (aliveCount <= 1)
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
					SendNotice(msg, m_cfg.noticeType);
				}
			}
			// Avoid re-entering MATCH_FINISH if already there
			if (m_rankBattleState != RANKBATTLE_BATTLESTATE_MATCH_FINISH)
				UpdateRankBattleState(RANKBATTLE_BATTLESTATE_MATCH_FINISH, m_rankBattleStage, matchFinishMs);
			return;
		}

		// Non-CC mode or rank UI disabled: end immediately
		if (aliveCount == 1)
			FinishWithWinner(lastAliveCharId);
		else
			FinishMatch(false);
	}
}

void CArenaManager::FinishMatch(bool aborted)
{
	if (m_state != State::IN_ROUND && !aborted)
		return;
	StopRoundTimerUI();
	// Notify RankBattle LEAVE for HUD cleanup
	if (m_cfg.rankUiEnabled && m_currentWorldId)
		BroadcastRankLeaveToWorld(m_currentWorldId);
	if (!aborted && m_cfg.rewardsEnabled)
	{
		AwardRewards(true);
		AwardRewards(false);
	}
	SendNotice(aborted ? L"Arena stopped." : L"Arena finished.", m_cfg.noticeType);
	// Despawn arena mobs
	DespawnArenaMobs();
	// Post-finish teleport if configured
	if (!aborted)
	{
		if (m_cfg.postFinishTeleport)
			PostFinishTeleportAll();
		else
			PostFinishTeleportDefault();
	}
	m_state = aborted ? State::IDLE : State::COMPLETE;
}

void CArenaManager::FinishWithWinner(unsigned int winnerCharId)
{
	if (m_state != State::IN_ROUND)
		return; // already finished
	StopRoundTimerUI();
	if (winnerCharId)
	{
		if (CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)winnerCharId))
		{
			MarkWinner(p);
			wchar_t msg[256];
			ComposeWinnerText(p, msg, _countof(msg));
			SendNotice(msg, m_cfg.noticeType);
		}
	}
	if (m_cfg.rewardsEnabled)
	{
		AwardRewards(true);
		AwardRewards(false);
	}
	// Telecast at finish as well
	if (m_cfg.telecastEnabled)
		BroadcastTelecastToWorld(EnsureCurrentWorldId());
	// Rank UI end state
	if (m_cfg.rankUiEnabled)
		BroadcastRankStateToWorld(EnsureCurrentWorldId(), 5, 1); // RANKBATTLE_BATTLESTATE_MATCH_FINISH
	// Ensure HUD cleans up like RankBattle: send LEAVE
	if (m_cfg.rankUiEnabled && m_currentWorldId)
		BroadcastRankLeaveToWorld(m_currentWorldId);
	// Despawn arena mobs on finish
	DespawnArenaMobs();
	// Post-finish teleport if configured
	if (m_cfg.postFinishTeleport)
		PostFinishTeleportAll();
	else
		PostFinishTeleportDefault();
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

// Notify clients that they "joined" a rank battle room for HUD consistency
void CArenaManager::BroadcastRankJoinToWorld(unsigned int worldId)
{
	if (worldId == 0) return;
	TBLIDX roomTblidx = FindRankBattleTblidxForWorld((TBLIDX)m_currentWorldTblidx);
	if (roomTblidx == INVALID_TBLIDX) return;
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
	if (worldId == 0) return;
	for (auto cid : m_participants)
	{
		CPlayer* pPlayer = g_pObjectManager->FindByChar((CHARACTERID)cid);
		if (!pPlayer || !pPlayer->IsInitialized() || (unsigned int)pPlayer->GetWorldID() != worldId) continue;
		CNtlPacket packet(sizeof(sGU_RANKBATTLE_LEAVE_NFY));
		sGU_RANKBATTLE_LEAVE_NFY* res = (sGU_RANKBATTLE_LEAVE_NFY*)packet.GetPacketData();
		res->wOpCode = GU_RANKBATTLE_LEAVE_NFY;
		packet.SetPacketLen(sizeof(sGU_RANKBATTLE_LEAVE_NFY));
		pPlayer->SendPacket(&packet);
	}
}

// Provide RankBattle HUD with team/member info to render UI bars
void CArenaManager::BroadcastRankTeamInfoToWorld(unsigned int worldId)
{
	if (worldId == 0) return;
	CGameServer* app = (CGameServer*)g_pApp;
	CWorld* pWorld = app->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)worldId);
	if (!pWorld) return;

	sRANKBATTLE_MATCH_MEMBER_INFO memberInfo[NTL_MAX_MEMBER_IN_PARTY * 2];
	::ZeroMemory(memberInfo, sizeof(memberInfo));

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
	for (auto cid : m_participants)
	{
		CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid);
		if (!p || !p->IsInitialized()) continue;
		if ((unsigned int)p->GetWorldID() != worldId) continue;
		// Ensure client exits any locked state; do not touch RankBattleData in Arena
		p->SendCharStateStanding();
	}
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

	// Simple guard: if a world was just created and cached, reuse it to avoid creation loops
	if (m_currentWorldId != 0)
	{
		CGameServer* appCheck = (CGameServer*)g_pApp;
		if (CWorld* w = appCheck->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)m_currentWorldId))
			return m_currentWorldId;
	}

	// If custom worlds are disabled, validate that current world is from RankBattle table
	if (!m_cfg.allowCustomWorlds)
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

		NTL_PRINT(PRINT_APP, _T("[ARENA] Attempting to create world: tblidx=%u name='%s'"),
			m_currentWorldTblidx, pWorldTbldat->wszName);

		CWorld* pWorld = app->GetGameMain()->GetWorldManager()->CreateWorld(pWorldTbldat);
		if (pWorld)
		{
			m_currentWorldId = (unsigned int)pWorld->GetID();
			NTL_PRINT(PRINT_APP, _T("[ARENA] CC Mode: Created fresh world instance ID %u for tblidx %u"),
				m_currentWorldId, m_currentWorldTblidx);
		}
		else
		{
			m_currentWorldId = 0;
			NTL_PRINT(PRINT_APP, _T("[ARENA] CC Mode: Failed to create world for tblidx %u - CreateWorld returned NULL"), m_currentWorldTblidx);
			NTL_PRINT(PRINT_APP, _T("[ARENA] World data: name='%s' mapName='%s'"),
				pWorldTbldat->wszName, pWorldTbldat->szName);

			// Try fallback worlds if configured world fails
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
						m_currentWorldId = (unsigned int)pFallbackWorld->GetID();
						m_currentWorldTblidx = fallbackTblidx; // Update the current tblidx to the working one
						NTL_PRINT(PRINT_APP, _T("[ARENA] Using fallback world: tblidx=%u name='%s' worldId=%u"),
							fallbackTblidx, pFallbackTbldat->wszName, m_currentWorldId);
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
			return m_currentWorldId;
		// fallthrough to recreate
	}

	sWORLD_TBLDAT* pWorldTbldat = (sWORLD_TBLDAT*)g_pTableContainer->GetWorldTable()->FindData((TBLIDX)m_currentWorldTblidx);
	if (!pWorldTbldat)
	{
		NTL_PRINT(PRINT_APP, _T("[ARENA] ERROR: World tblidx %u not found in World Table!"), m_currentWorldTblidx);
		return 0;
	}

	CWorld* pWorld = app->GetGameMain()->GetWorldManager()->CreateWorld(pWorldTbldat);
	if (pWorld)
	{
		m_currentWorldId = (unsigned int)pWorld->GetID();
	}
	else
	{
		// Try fallback worlds if configured world fails
		m_currentWorldId = 0;
		NTL_PRINT(PRINT_APP, _T("[ARENA] Normal Mode: Failed to create world for tblidx %u"), m_currentWorldTblidx);

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
	if (!worldId)
		return;
	CGameServer* app = (CGameServer*)g_pApp;
	CWorld* pWorld = app->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)worldId);
	if (!pWorld)
		return;
	CNtlPacket packet(sizeof(sGU_RANKBATTLE_BATTLE_STATE_UPDATE_NFY));
	sGU_RANKBATTLE_BATTLE_STATE_UPDATE_NFY* res = (sGU_RANKBATTLE_BATTLE_STATE_UPDATE_NFY*)packet.GetPacketData();
	res->wOpCode = GU_RANKBATTLE_BATTLE_STATE_UPDATE_NFY;
	res->byBattleState = byState;
	res->byStage = byStage;
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
	// WAIT (0) with stage 0
	BroadcastRankStateToWorld(worldId, RANKBATTLE_BATTLESTATE_WAIT, 0);
	// DIRECTION phase (shows VS)
	BroadcastRankStateToWorld(worldId, RANKBATTLE_BATTLESTATE_DIRECTION, 0);
	// STAGE_PREPARE (gear up)
	BroadcastRankStateToWorld(worldId, RANKBATTLE_BATTLESTATE_STAGE_PREPARE, 0);
	// STAGE_READY (round ready)
	BroadcastRankStateToWorld(worldId, RANKBATTLE_BATTLESTATE_STAGE_READY, 0);
	// MATCH START notification
	BroadcastRankMatchStartToWorld(worldId);
	// Unlock attack/movement
	MakeParticipantsAttackable(worldId);
	// STAGE_RUN
	BroadcastRankStateToWorld(worldId, RANKBATTLE_BATTLESTATE_STAGE_RUN, 0);
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

		// Prepare state for next round: keep NORMAL during READY; ATTACKABLE will be set at STAGE_READY→RUN
		pRankData->eState = RANKBATTLE_MEMBER_STATE_NORMAL;
		pPlayer->SendCharStateStanding();
	}
}

void CArenaManager::PostFinishTeleportDefault()
{
	// send players back to their previous location if we have it; else to bind
	for (auto cid : m_participants)
	{
		if (CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid))
		{
			auto it = m_prevLoc.find(cid);
			if (it != m_prevLoc.end() && it->second.worldId != INVALID_WORLDID)
			{
				p->StartTeleport(it->second.loc, it->second.dir, (WORLDID)it->second.worldId, TELEPORT_TYPE_COMMAND);
			}
			else
			{
				TeleportToBind(p);
			}
		}
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
				p->StartTeleport(it->second.loc, it->second.dir, (WORLDID)it->second.worldId, TELEPORT_TYPE_COMMAND);
			else
				TeleportToBind(p);
		}
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
		// final round: finish the match UX then cleanup
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
			// Announce team winner using ComposeWinnerText helpers
			wchar_t msg[256];
			ComposeWinnerText(rep, msg, _countof(msg));
			SendNotice(msg, m_cfg.noticeType);
		}
		// RankBattle-like finish UX
		if (m_cfg.telecastEnabled)
			BroadcastTelecastToWorld(EnsureCurrentWorldId());
		if (m_cfg.rankUiEnabled)
		{
			BroadcastRankStateToWorld(EnsureCurrentWorldId(), 5, 1); // MATCH_FINISH
			if (m_currentWorldId)
				BroadcastRankLeaveToWorld(m_currentWorldId);
		}
		if (m_cfg.rewardsEnabled)
		{
			AwardRewards(true);
			AwardRewards(false);
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

void CArenaManager::OnPlayerFaint(unsigned int killerCharId, unsigned int victimCharId)
{
	if (!m_cfg.enabled) return;
	if (killerCharId == 0 || killerCharId == victimCharId) return;
	if (m_participants.find(killerCharId) == m_participants.end()) return;
	// Only count if not reviving on faint (points-based mode)
	if (!m_cfg.reviveOnFaint)
	{
		m_killPoints[killerCharId] += 1;
	}
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
	for (auto cid : m_participants)
	{
		if (CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid))
		{
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
		if (worldTblidx == m_currentWorldTblidx)
			m_currentWorldId = (unsigned int)pWorld->GetID();
	}

	CNtlVector destLoc = pWorldTbldat->vStart1Loc;
	if (!(posX == 0.f && posY == 0.f && posZ == 0.f))
	{
		destLoc.x = posX; destLoc.y = posY; destLoc.z = posZ;
	}

	// Use DOJO teleport type consistently; we emulate RankBattle UI separately
	pPlayer->StartTeleport(destLoc, pPlayer->GetCurDir(), pWorld->GetID(), TELEPORT_TYPE_DOJO);
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
	CNtlPacket packet(sizeof(sGU_RANKBATTLE_BATTLE_STATE_UPDATE_NFY));
	sGU_RANKBATTLE_BATTLE_STATE_UPDATE_NFY* res = (sGU_RANKBATTLE_BATTLE_STATE_UPDATE_NFY*)packet.GetPacketData();
	res->wOpCode = GU_RANKBATTLE_BATTLE_STATE_UPDATE_NFY;
	res->byBattleState = byState;
	res->byStage = byStage;
	packet.SetPacketLen(sizeof(sGU_RANKBATTLE_BATTLE_STATE_UPDATE_NFY));
	pPlayer->SendPacket(&packet);
}

void CArenaManager::SendRankMatchStartTo(CPlayer* pPlayer)
{
	if (!pPlayer) return;
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
	if (newState == m_rankBattleState && byStage == m_rankBattleStage)
	{
		if (m_rankStateTimeMs > 0)
		{
			// Do not spam the same state; keep current timer running
			return;
		}
		// If timer is zero but state is terminal, also avoid re-entry
		if (newState == RANKBATTLE_BATTLESTATE_STAGE_FINISH || newState == RANKBATTLE_BATTLESTATE_MATCH_FINISH)
		{
			return;
		}
	}

	m_rankBattleState = newState;
	m_rankBattleStage = byStage;
	m_rankStateTimeMs = durationMs;

	NTL_PRINT(PRINT_APP, _T("[ARENA] RankBattle State: %d -> Stage: %d, Duration: %ums"), 
		(int)newState, (int)byStage, durationMs);

	// Broadcast the state only to arena participants present in the arena world
	if (m_currentWorldId != 0)
	{
		CGameServer* app = (CGameServer*)g_pApp;
		CWorld* pWorld = app->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)m_currentWorldId);
		if (pWorld)
		{
			CNtlPacket packet(sizeof(sGU_RANKBATTLE_BATTLE_STATE_UPDATE_NFY));
			sGU_RANKBATTLE_BATTLE_STATE_UPDATE_NFY* res = (sGU_RANKBATTLE_BATTLE_STATE_UPDATE_NFY*)packet.GetPacketData();
			res->wOpCode = GU_RANKBATTLE_BATTLE_STATE_UPDATE_NFY;
			res->byBattleState = newState;
			res->byStage = byStage;
			packet.SetPacketLen(sizeof(sGU_RANKBATTLE_BATTLE_STATE_UPDATE_NFY));
			for (auto cid : m_participants)
			{
				CPlayer* pRecv = g_pObjectManager->FindByChar((CHARACTERID)cid);
				if (!pRecv || !pRecv->IsInitialized() || (unsigned int)pRecv->GetWorldID() != (unsigned int)m_currentWorldId) continue;
				pRecv->SendPacket(&packet);
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
		return;
	}
	
	if (m_state != State::IN_ROUND && m_state != State::PRE_ROUND &&
		m_state != State::MATCH_READY && m_state != State::STAGE_READY) 
	{
		NTL_PRINT(PRINT_APP, _T("[ARENA] OnPlayerEnterWorld: char=%u wrong arena state=%d, ignoring"), 
			pPlayer->GetCharID(), (int)m_state);
		return;
	}
	
	// Only resync if he is in the arena world when using configured world
	if (m_currentWorldTblidx != 0 && (unsigned int)pPlayer->GetWorldTblidx() != m_currentWorldTblidx)
	{
		NTL_PRINT(PRINT_APP, _T("[ARENA] OnPlayerEnterWorld: char=%u not in arena world (current=%u, arena=%u), ignoring"), 
			pPlayer->GetCharID(), pPlayer->GetWorldTblidx(), m_currentWorldTblidx);
		return;
	}

	NTL_PRINT(PRINT_APP, _T("[ARENA] OnPlayerEnterWorld: Processing char=%u in arena world"), pPlayer->GetCharID());

	// Schedule readiness with optional per-player delay
	unsigned int cid = pPlayer->GetCharID();
	if (m_cfg.enterReadyDelayMs > 0)
		m_readyDelayMs[cid] = m_cfg.enterReadyDelayMs;
	else
		m_readyParticipants.insert(cid);

	if (m_state == State::PRE_ROUND)
	{
		// In PRE_ROUND: send RankBattle room JOIN + WAIT(0) like RankBattle does on enter and unlock locally
		if (m_cfg.rankUiEnabled)
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
			auto teamKey = [&](CPlayer* x){ return useParty ? (unsigned int)x->GetPartyID() : (useGuild ? (unsigned int)x->GetGuildID() : 0); };
			for (CPlayer* x : present){ unsigned int k = teamKey(x); if (k == 0) continue; if (ownerKey == 0) ownerKey = k; else if (k != ownerKey) { /* found a different team */ break; } }
			for (size_t i=0;i<present.size();++i)
			{
				CPlayer* x = present[i];
				BYTE t = RANKBATTLE_TEAM_OWNER;
				if (useParty || useGuild)
				{
					unsigned int k = teamKey(x);
					t = (k!=0 && k!=ownerKey) ? RANKBATTLE_TEAM_CHALLENGER : RANKBATTLE_TEAM_OWNER;
				}
				else
				{
					t = (i < (present.size()+1)/2) ? RANKBATTLE_TEAM_OWNER : RANKBATTLE_TEAM_CHALLENGER;
				}
				x->GetRankBattleData()->eTeamType = (eRANKBATTLE_TEAM_TYPE)t;
			}
		}
		SendRankStateTo(pPlayer, RANKBATTLE_BATTLESTATE_WAIT, 0);
		pPlayer->SendCharStateStanding();
		return;
	}

	// STAGE_READY/IN_ROUND resync path: ensure unlock and send UI
	pPlayer->SendCharStateStanding();
	// If player enters during STAGE_READY, mirror per-round unlock sequence for this client
	if (m_state == State::STAGE_READY)
	{
		// Set attackable state and notify just for this player/world
		CGameServer* app = (CGameServer*)g_pApp;
		CWorld* pWorld = app->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)pPlayer->GetWorldID());
		if (pWorld)
		{
			std::unordered_set<unsigned int> one{ pPlayer->GetCharID() };
			ArenaBroadcastRankPlayerAttackableToWorld(pWorld, one);
		}
		// Do not start a new timer here; STAGE_READY -> RUN path will start the timer.
		// If a timer is already active for some reason, just sync the remaining time.
		if (m_roundUiActive && m_roundRemainMs > 0)
			SendRoundTimerStartTo(pPlayer, (unsigned int)(m_roundRemainMs / 1000));
	}
	// Also re-broadcast attackable for this player to ensure client unlocks in RUN
	if (m_state == State::IN_ROUND)
	{
		CGameServer* app = (CGameServer*)g_pApp;
		CWorld* pWorld = app->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)pPlayer->GetWorldID());
		if (pWorld)
		{
			std::unordered_set<unsigned int> one{ pPlayer->GetCharID() };
			ArenaBroadcastRankPlayerAttackableToWorld(pWorld, one);
		}
		// Sync timer as well if running
		if (m_roundUiActive && m_roundRemainMs > 0)
			SendRoundTimerStartTo(pPlayer, (unsigned int)(m_roundRemainMs / 1000));
	}
	// Send rank UI packets only if explicitly enabled and safe
	if (m_cfg.rankUiEnabled && m_cfg.ccBattleMode)
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
		}
		SendRankTeamInfoTo(pPlayer);
	}
	else
	{
		NTL_PRINT(PRINT_APP, _T("[ARENA] Skipping rank packets: rankUi=%d ccBattle=%d"), 
			m_cfg.rankUiEnabled ? 1 : 0, m_cfg.ccBattleMode ? 1 : 0);
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
			SendNotice(L"Arena canceled - not enough participants.", m_cfg.noticeType);
			NTL_PRINT(PRINT_APP, _T("[ARENA] Validation failed: only %u participants online for non-team mode"), onlineParticipants);
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
				SendNotice(L"Arena canceled - all participants must be in guilds for Guild vs Guild mode.", m_cfg.noticeType);
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
		SendNotice(L"Arena canceled - not enough participants.", m_cfg.noticeType);
		NTL_PRINT(PRINT_APP, _T("[ARENA] Validation failed: only %u valid participants for team mode"), validParticipants);
		return false;
	}

	// Validate team composition based on mode
	if (m_mode == Mode::PARTY_VS_PARTY)
	{
		if (uniqueParties.size() < 2)
		{
			BroadcastSystem(L"[Arena] Party vs Party mode requires at least 2 different parties. Arena canceled.");
			SendNotice(L"Arena canceled - need at least 2 different parties.", m_cfg.noticeType);
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
			SendNotice(L"Arena canceled - need at least 2 different guilds.", m_cfg.noticeType);
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
