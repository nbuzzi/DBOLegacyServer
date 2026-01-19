#pragma once

#include "NtlSingleton.h"
#include "NtlString.h"
#include "NtlObject.h"
#include <vector>
#include <string>
#include <utility>
#include <unordered_set>
#include <unordered_map>
#include "NtlParty.h"
#include "NtlGuild.h"
#include "NtlRankBattle.h"
// For Budokai enums/packets used on Budokai worlds
#include "NtlBudokai.h"

class CNtlIniFile;
class CPlayer;

class CArenaManager : public CNtlSingleton<CArenaManager>
{
public:
	enum class Mode : unsigned char
	{
		GUILD_VS_GUILD = 0,
		PARTY_VS_PARTY = 1,
		FREE_FOR_ALL = 2,
		OPEN = 3
	};

	struct Config
	{
		bool enabled;
		// When true, Arena can only start on channels whose name contains 'ARENA' (case-insensitive) and not on Dojo channel
		bool onlyOnArenaChannel = true;
		// comma-separated list of world tblidx values
		std::vector<unsigned int> worldTblidxList;
		// worlds that require Budokai-style match notifications instead of RankBattle ones
		std::vector<unsigned int> budokaiWorldTblidxList;
		// when false (default), exclude worlds that use Budokai rules from rotation/creation
		bool allowBudokaiRuleWorlds;
		unsigned int rotationSeconds;
		unsigned int startDelaySeconds; // delay before starting round after teleport/invite accept
		unsigned int roundTimerSeconds; // visible UI countdown duration
		unsigned int roundsCount; // when >0, override RankBattle byBattleCount
		bool stopOnTimeout; // end match automatically when timer ends
		unsigned int maxWaitAllArriveSeconds; // extra grace to wait for all online participants to arrive after min delay
		unsigned int rankSequenceStepMs; // delay between rank-state steps during start sequence
		unsigned int enterReadyDelayMs;  // delay after OnPlayerEnterWorld before marking ready
		unsigned int postReadyDelayMs;   // delay after all ready before starting sequence
		bool randomizeMapOnStart; // pick a random RankBattle map when arena starts
		bool mapPerRound; // if true, change map each round (not yet changing mid-match)
		// invite flow
		bool useInviteFlow; // send teleport proposals instead of instant teleport
		unsigned int inviteWaitSeconds; // seconds to accept
		// mobs
		bool mobsAllowed;
		std::vector<unsigned int> mobTblidxList; // static list of mobs to spawn in arena world
		bool randomMobsSpawn; // when true, spawn random mobs periodically during IN_ROUND
		unsigned int randomMobsPerWave; // how many per wave
		unsigned int randomMobsWaveSeconds; // wave interval seconds
		CNtlString mobListFile; // path to Mobs.txt containing name/id mapping
		CNtlString mobPreset; // selected preset name
		std::unordered_map<std::string, std::vector<unsigned int>> mobPresets; // preset name -> list of mob tblidx
		// spectators
		bool spectatorsEnabled;
		bool spectatorsUseSameWorld;
		unsigned int spectatorWorldTblidx; // used when not using same world
		float spectatorPosX;
		float spectatorPosY;
		float spectatorPosZ;
		// faint handling
		bool reviveOnFaint; // revive fainted participants during score mode
		unsigned int reviveDelayMs; // delay before reviving (ms); 0 = instant
		unsigned int reviveProtectMs; // post-revive protection duration (ms); 0 = none
		bool faintBecomeSpectator; // move fainted players to spectators if not revive
		bool spectatorHide; // apply transparent condition to spectators
		// scoring
		bool scoreOnFaint; // when true, award points on faint regardless of revive mode
		// notices
		unsigned char noticeType; // default notice type to use (e.g., 3)
		// telecast banner (CCBD style)
		bool telecastEnabled;
		unsigned char telecastType; // eTELECAST_MESSAGE_TYPE
		unsigned int telecastSpeechTblidx; // text index to display (client-side table)
		unsigned int telecastDisplayMs; // duration visible
		// rank battle UI (optional overlay similar to rank battle)
		bool rankUiEnabled;           // enable Arena-side HUD/timers (safe)
		bool rankPacketsEnabled;      // send GU_RANKBATTLE_* packets (risky unless full context is set)
		// post-finish teleport
		bool postFinishTeleport;
		unsigned int postFinishWorldTblidx;
		float postFinishPosX;
		float postFinishPosY;
		float postFinishPosZ;
		// Optional delay before teleporting everyone out after finish (ms). 0 = immediate
		unsigned int postFinishTeleportDelayMs;
		// Optional direction for post-finish teleport destination
		float postFinishDirX;
		float postFinishDirY;
		float postFinishDirZ;
		// Keep Rank UI visible after finish (avoid sending LEAVE)
		bool keepRankUiAfterFinish;
		// When true, do not send RankBattle finish/leave packets at the end of an arena match
		// This avoids confusing the client HUD for non-Rank contexts (e.g., world_fight events)
		bool suppressRankFinishUi;
		// diagnostics
		bool verboseLogs; // when true, emit detailed ERR_LOG traces for troubleshooting
		// When enabled, set the entire arena world as PvP zone for all players while in RUN
		// This helps bypass map-level PC battle restrictions and ensures free PvP during fights
		bool worldWidePvpDuringRun;
		// CC battle mode (like RankBattle)
		bool ccBattleMode; // when true, use RankBattle logic for world creation and teleportation
		bool allowCustomWorlds; // when true, allow GM commands and config to override worlds; when false, only use RankBattle table worlds
		bool useOnlyCustomWorlds; // when true, ignore RankBattle table and use only worlds from WorldTblidxList
		// When true, do not fall back to other worlds if world creation fails; used for strict GM-run events
		bool forceExactWorld;
		// rewards
		bool rewardsEnabled;
		// pair<itemTblidx,count>
		std::vector<std::pair<unsigned int, unsigned int>> winnerRewards;
		std::vector<std::pair<unsigned int, unsigned int>> participantRewards;
		// Mudosa points awarded on arena finish (winners vs participants)
		unsigned int mudosaWinnerPoints;
		unsigned int mudosaParticipantPoints;

		// Attackability reliability tuning
		unsigned int unlockPulseSleepMs;   // sleep between unlock pulses when forcing attackable
		unsigned int unlockDefaultAttempts; // how many pulses to send by default per unlock burst

		// AutoArena scheduler (server-side automation)
		bool autoEnabled;                 // when true, scheduler opens enrollment periodically
		CNtlString autoChannelName;       // if non-empty, run only when channel name contains this (case-insensitive)
		unsigned int autoIntervalSeconds; // how often to start a new enrollment window
		unsigned int autoInitialDelaySeconds; // delay before the very first auto enrollment after server starts
		unsigned int autoEnrollmentSeconds; // how long to keep enrollment open before teleport
		unsigned int autoWorldTblidx;     // legacy: single world for automated event (kept for backward compatibility)
		std::vector<unsigned int> autoWorldTblidxList; // CSV list of worlds to use for automation
		Mode autoMode;                    // FFA or PARTY for automated events
		bool autoUseElimination;          // true = elimination, false = score
		bool autoRandomizeWorlds;         // pick random world each event (when list provided)
		bool autoMapPerRound;             // rotate world each round during an event using the list

		// Watchdog: self-heal guards to avoid stuck arena/automation
		bool watchdogEnabled;             // enable watchdog checks (safe defaults)
		unsigned int watchdogPreRoundSeconds;   // max time to stay in PRE_ROUND/STAGE_READY before forced recovery (0 = auto)
		unsigned int watchdogEnrollmentSeconds; // max time to stay in ENROLLMENT without active AutoEnrollment (0 = auto)
		unsigned int watchdogRunHardcapSeconds; // hard cap for a RUN state when no round timer is configured (0 = auto)

		Config()
			: enabled(false), rotationSeconds(0), startDelaySeconds(5), roundTimerSeconds(180), stopOnTimeout(true),
			roundsCount(0),
			maxWaitAllArriveSeconds(10), rankSequenceStepMs(3000), enterReadyDelayMs(3000), postReadyDelayMs(5000),
			randomizeMapOnStart(false), mapPerRound(false),
			useInviteFlow(false), inviteWaitSeconds(15),
			mobsAllowed(false), randomMobsSpawn(false), randomMobsPerWave(2), randomMobsWaveSeconds(25),
			spectatorsEnabled(false), spectatorsUseSameWorld(true),
			spectatorWorldTblidx(0), spectatorPosX(0), spectatorPosY(0), spectatorPosZ(0),
			reviveOnFaint(false), reviveDelayMs(0), reviveProtectMs(1500), faintBecomeSpectator(true), spectatorHide(true), scoreOnFaint(false), noticeType(3),
			telecastEnabled(false), telecastType(3), telecastSpeechTblidx(0), telecastDisplayMs(5000),
			rankUiEnabled(false), rankPacketsEnabled(false),
			postFinishTeleport(true), postFinishWorldTblidx(1), postFinishPosX(4975.609863f), postFinishPosY(-48.869999f), postFinishPosZ(4012.609863f),
			postFinishTeleportDelayMs(3000),
			postFinishDirX(0.911100f), postFinishDirY(-0.412000f), postFinishDirZ(0.0f), keepRankUiAfterFinish(true), suppressRankFinishUi(false),
			verboseLogs(false), worldWidePvpDuringRun(false),
			ccBattleMode(false), allowCustomWorlds(false), useOnlyCustomWorlds(false), allowBudokaiRuleWorlds(false), forceExactWorld(false),
			rewardsEnabled(false),
			unlockPulseSleepMs(100), unlockDefaultAttempts(2),
			autoEnabled(false), autoChannelName("ARENA"), autoIntervalSeconds(600), autoInitialDelaySeconds(300), autoEnrollmentSeconds(600),
			autoWorldTblidx(900043), autoMode(Mode::FREE_FOR_ALL), autoUseElimination(true), autoRandomizeWorlds(false), autoMapPerRound(false),
			watchdogEnabled(false), watchdogPreRoundSeconds(0), watchdogEnrollmentSeconds(0), watchdogRunHardcapSeconds(0) {
			// Default Mudosa rewards
			mudosaWinnerPoints = 3000;
			mudosaParticipantPoints = 2000;
		}
	};

	enum class State : unsigned char
	{
		IDLE = 0,
		ENROLLMENT,
		PRE_ROUND,        // Players have been teleported, waiting for arrival
		MATCH_READY,      // All players arrived, showing direction/intro
		STAGE_READY,      // Players preparing for battle, "READY" screen
		IN_ROUND,         // Battle is active
		INTERMISSION,
		COMPLETE
	};

public:
	CArenaManager();
	~CArenaManager();

	bool LoadConfigFromIniPath(const char* iniPath);

	void TickProcess(unsigned long dwTickDiff);
	// Automation scheduler (runs even when arena state is IDLE)
	void AutomationTick(unsigned long dwTickDiff);

	// GM control
	void Start(Mode mode);
	void Stop(bool abort = false);
	void RotateMapNow();
	void StatusTo(CPlayer* pWho);
	bool SetCurrentWorld(unsigned int worldTblidx);
	// Force-set the current world tblidx, bypassing allowCustomWorlds guard (for GM events)
	bool ForceCurrentWorld(unsigned int worldTblidx);
	// Configure quick world-fight parameters
	void SetupWorldFight(bool scoreMode, unsigned int roundSeconds, Mode mode);
	// Ensure world instance exists and return its WORLDID (0 on failure)
	unsigned int GetOrCreateCurrentWorldId();

	// Helpers for PvP/Arena context detection
	bool IsArenaWorldTblidx(unsigned int worldTblidx) const;
	bool IsArenaWorldId(unsigned int worldId) const { return m_currentWorldId && worldId == m_currentWorldId; }
	// Check if arena PvP rules should apply (requires: arena enabled, IN_ROUND state, arena channel, and participant)
	bool ShouldAllowArenaPvP(CPlayer* pPlayer) const;

	// Enrollment
	bool AddParticipant(CPlayer* pPlayer);
	bool AddSpectator(CPlayer* pPlayer);
	bool Remove(CPlayer* pPlayer);
	void TeleportParticipants();
	void TeleportParticipants(bool forceDirect); // Bypass invite flow when true
	void TeleportSpectators();
	void TeleportParticipantsHere(CPlayer* pGm);
	void TeleportSpectatorsHere(CPlayer* pGm);
	void MarkWinner(CPlayer* pPlayer);
	void ClearWinners();
	// World management
	void LoadAvailableWorlds(); // Load all valid worlds from RankBattle table
	void FinishWithWinner(unsigned int winnerCharId);
	void FinishByPoints();
	void FinishOnTimeout();
	bool IsParticipant(CPlayer* pPlayer) const;
	bool IsParticipantId(unsigned int charId) const;
	bool IsSpectatorId(unsigned int charId) const { return m_spectators.find(charId) != m_spectators.end(); }
	bool IsEnabled() const { return m_cfg.enabled; }
	// Called from player when finishing world load; used to re-sync UI/state
	void OnPlayerEnterWorld(CPlayer* pPlayer);

	// Round timer UI
	void StartRoundTimerUI(unsigned int seconds);
	void StopRoundTimerUI();
	// Round/Rotation timer controls (for GM commands)
	bool SetRoundTimeRemaining(unsigned int seconds, bool startIfNotActive = true);
	bool AddRoundTimeSeconds(int deltaSeconds);
	void SetRotationSecondsRemaining(unsigned int seconds);
	// GM utility: spawn a mob in the current arena world
	bool SpawnMob(unsigned int mobTblidx, const CNtlVector* pAt = nullptr, const CNtlVector* pDir = nullptr);

	// Random mob waves controls (GM-accessible)
public:
	void SetRandomMobsSpawn(bool on);
	void SetRandomMobsPerWave(unsigned int n);
	void SetRandomMobsWaveSeconds(unsigned int sec);
	bool SetRandomMobsPreset(const std::string& name);
	unsigned int ResolveMobIdByName(const std::wstring& name) const;

	// GM utility: award configured rewards (true = winners only, false = all participants)
	void AwardRewards(bool winnersOnly);
	void AwardMudosaPoints();

	// Accessors
	const Config& GetConfig() const { return m_cfg; }
	State GetState() const { return m_state; }
	Mode GetMode() const { return m_mode; }
	unsigned int GetCurrentWorldTblidx() const { return m_currentWorldTblidx; }
	bool SpectatorsUseSameWorld() const { return m_cfg.spectatorsUseSameWorld; }
	unsigned int GetSpectatorWorldTblidx() const { return m_cfg.spectatorWorldTblidx; }
	float GetSpectatorPosX() const { return m_cfg.spectatorPosX; }
	float GetSpectatorPosY() const { return m_cfg.spectatorPosY; }
	float GetSpectatorPosZ() const { return m_cfg.spectatorPosZ; }

	// Runtime configuration toggles (no server restart needed)
	void SetAllowCustomWorlds(bool on);
	void SetUseOnlyCustomWorlds(bool on);
	void SetAllowBudokaiRuleWorlds(bool on);
	void SetRandomizeMapOnStart(bool on);
	void SetRotationSeconds(unsigned int seconds);
	// Rebuild world list from RankBattle table (ignoring custom worlds) and apply current filters
	void RebuildWorldList_RankOnly();
	// Replace world list from a CSV string (e.g., "900043,900044"); applies budokai filter if disabled
	void SetWorldListCsv(const std::string& csv);
	// Show a compact configuration summary to a player
	void ShowCfgTo(CPlayer* pWho);

private:
	// Round-robin pointer for AutoArena world selection
	unsigned int m_autoWorldIndex = 0;

private:
	void BroadcastSystem(const wchar_t* msg, unsigned char byType = 2);
	void SendSystemTo(CPlayer* pPlayer, const wchar_t* msg, unsigned char byType = 3);
	void TryRotateByTime(unsigned long dwTickDiff);
	void ParseWorldListCsv(const CNtlString& csv);
	void ParseMobListCsv(const CNtlString& csv);
	void ParseRewardsCsv(const CNtlString& csv, std::vector<std::pair<unsigned int, unsigned int>>& out);
	void FilterOutBudokaiRuleWorlds();
	bool TeleportOneToWorldTblidx(CPlayer* pPlayer, unsigned int worldTblidx, float posX, float posY, float posZ);
	void BroadcastRoundTimerStartToWorld(unsigned int worldId, unsigned int seconds);
	void BroadcastRoundTimerEndToWorld(unsigned int worldId);
	void BroadcastDungeonStateToWorld(unsigned int worldId, unsigned char byStage = 1, unsigned int titleTblidx = 0, unsigned int subTitleTblidx = 0);
	void BroadcastCountdownToWorld(unsigned int worldId, bool bStart);
	void BroadcastRankStateToWorld(unsigned int worldId, unsigned char byState, unsigned char byStage);
	void BroadcastRankMatchStartToWorld(unsigned int worldId);
	void BroadcastRankStageFinishToWorld(unsigned int worldId);
	void BroadcastRankMatchFinishToWorld(unsigned int worldId);
	// Simple text-based scoreboard for participants (fallback when client has no point HUD)
	void BroadcastScoreboardToWorld(unsigned int worldId);
	// Budokai-like notifications (used only on Budokai worlds)
	void BroadcastBudokaiMatchStateToWorld(unsigned int worldId, BYTE byMatchType, BYTE byState, BUDOKAITIME tmNextStepTime, BUDOKAITIME tmRemainTime);
	void BroadcastBudokaiProgressMessageToWorld(unsigned int worldId, BYTE byMsgId);
	void BroadcastRankJoinToWorld(unsigned int worldId);
	void BroadcastRankLeaveToWorld(unsigned int worldId);
	void BroadcastRankFullStartSequence(unsigned int worldId);
	void BroadcastRankTeamInfoToWorld(unsigned int worldId);
	void MakeParticipantsAttackable(unsigned int worldId);
	void EnsureParticipantsStanding(unsigned int worldId);
	// Combat permission toggles (enable PvP/FreeBattle during RUN, clear on finish)
	void SetCombatPermittedForParticipants(bool enable);
	void SetCombatPermittedFor(class CPlayer* pPlayer, bool enable);
	// World rule override control
	bool ShouldOverrideRuleForWorld(unsigned int worldTblidx) const; // skip override for custom ARENAPODER maps
	bool ShouldRotatePerRoundForWorld(unsigned int worldTblidx) const; // skip per-round rotation for custom maps
	void RevertWorldRuleOverrides(); // clear any world rule overrides applied during match
	// Team validation for team-based modes
	bool ValidateTeamComposition();
	// Single-recipient helpers to recover when a player enters late
	void SendDungeonStateTo(CPlayer* pPlayer, unsigned char byStage = 1, unsigned int titleTblidx = 0, unsigned int subTitleTblidx = 0);
	void SendRankStateTo(CPlayer* pPlayer, unsigned char byState, unsigned char byStage);
	void SendRankMatchStartTo(CPlayer* pPlayer);
	void SendRankLeaveTo(CPlayer* pPlayer);
	void SendRankFullStartSequenceTo(CPlayer* pPlayer);
	void SendRankTeamInfoTo(CPlayer* pPlayer);
	void SendRoundTimerStartTo(CPlayer* pPlayer, unsigned int seconds);
	void SendRoundTimerEndTo(CPlayer* pPlayer);
	void SendCountdownTo(CPlayer* pPlayer, bool bStart);
	void UpdateRankBattleState(eRANKBATTLE_BATTLESTATE newState, BYTE byStage, unsigned long durationMs = 0);
	void MoveToSpectator(CPlayer* pPlayer);
	void ApplySpectatorHide(CPlayer* pPlayer, bool hide);
	void SendNotice(const wchar_t* text, unsigned char byType);
	void CheckFaintAndAliveLogic();
	void ReviveParticipantsForNextRound();
	void ResetParticipantsBetweenRounds();
	bool IsPartyModeWorld(unsigned int worldTblidx) const;
	bool IsBudokaiWorld(unsigned int worldTblidx) const;
	bool IsRankBattleWorld(unsigned int worldTblidx) const;
	void SpawnArenaMobs();
	void DespawnArenaMobs();
	// Respawn helpers
	void ReviveParticipantNow(unsigned int victimCharId, bool bApplyRespawnBuff);
	void ApplyRespawnBuff(class CPlayer* pPlayer);
	void ApplyReviveProtection(class CPlayer* pPlayer);
	void ClearReviveProtection(class CPlayer* pPlayer);
	// Random mob waves
	void SpawnRandomMobWave(unsigned int count);
	std::vector<unsigned int> GetPresetPool() const;
	// Mob name/index helpers
	void LoadMobListFile(const char* path);
	unsigned int EnsureCurrentWorldId();
	void BroadcastTelecastToWorld(unsigned int worldId);
	bool TeleportOneToWorldTblidxDir(class CPlayer* pPlayer, unsigned int worldTblidx, float posX, float posY, float posZ, float dirX, float dirY, float dirZ);
	void PostFinishTeleportAll();
	void PostFinishTeleportDefault(); // fallback to previous location or bind
	void TeleportToBind(CPlayer* pPlayer);
	void SavePrevLocation(CPlayer* pPlayer);
	unsigned int GetAnyParticipantWorldId();
	void ComposeWinnerText(CPlayer* pWinner, wchar_t* outBuf, size_t cchBuf);
	void ComposeWinnerTextTeam_Party(PARTYID partyId, wchar_t* outBuf, size_t cchBuf);
	void ComposeWinnerTextTeam_Guild(GUILDID guildId, wchar_t* outBuf, size_t cchBuf);
	void FinishMatch(bool aborted);
	void ClearCombatRestrictionsFor(class CPlayer* pPlayer);
	void ClearCombatRestrictionsForParticipants();
	// World-wide PvP toggles for the arena world during RUN
	void ApplyWorldWidePvp(unsigned int worldId);
	void RevertWorldWidePvp();
	// System status helpers
	void AnnounceRoundTimeRemaining(unsigned int secondsLeft);
	void AnnounceRotationTimeRemaining(unsigned int secondsLeft);
	static void FormatTime(unsigned int seconds, wchar_t* outBuf, size_t cchBuf);

	// Automation
	enum class AutoState : unsigned char { OFF = 0, WAIT_NEXT, ENROLLMENT_OPEN };
	AutoState m_autoState = AutoState::OFF;
	unsigned long m_autoRemainMs = 0; // time left for current auto phase

public:
	void OnPlayerFaint(unsigned int killerCharId, unsigned int victimCharId);
	// Retrieve a player's saved pre-arena location; returns true when available
	bool GetPrevLocation(unsigned int charId, unsigned int& outWorldId, CNtlVector& outLoc, CNtlVector& outDir) const;

private:
	Config m_cfg;
	State m_state;
	Mode m_mode;
	unsigned int m_currentWorldTblidx;
	unsigned int m_currentWorldId; // WORLDID of active arena instance (if created)
	size_t m_worldIndex;
	unsigned long m_rotationRemainMs;
	unsigned int m_nextRotationAnnounceSec = 0; // next rotation second mark to announce
	// Invite phase
	bool m_inviting = false;
	unsigned long m_inviteRemainMs = 0;
	unsigned CountParticipantsInWorld(unsigned int worldId);
	// Count participants present in any world instance matching the given world table index
	unsigned CountParticipantsInWorldTblidx(unsigned int worldTblidx);
	unsigned CountOnlineParticipants();
	// Returns the worldId of the first participant found in any world instance matching the given world tblidx
	unsigned GetFirstParticipantWorldIdForTblidx(unsigned int worldTblidx);

	// Round timer state
	bool m_roundUiActive = false;
	unsigned long m_roundRemainMs = 0;
	unsigned int m_roundWorldId = 0; // WORLDID
	unsigned int m_nextRoundAnnounceSec = 0; // next round second mark to announce

	// Delayed start to avoid client freeze during world load
	unsigned long m_pendingStartMs = 0; // when >0 and reaches 0, start round
	unsigned int m_pendingStartWorldId = 0; // world to use when starting
	// Gating: after pending start reaches 0, optionally wait until all online participants are present
	unsigned long m_waitAllArriveMs = 0; // remaining grace time to wait for all arrivals
	std::unordered_set<unsigned int> m_pendingStartParticipants; // snapshot of participants when scheduling start
	std::unordered_set<unsigned int> m_readyParticipants; // players who signaled world-entry complete for this round
	// Timed start sequence (rank-like) before dungeon UI/timer
	unsigned long m_seqRemainMs = 0;
	unsigned char m_seqStep = 0; // 0=idle, 1=DIRECTION, 2=STAGE_PREPARE, 3=STAGE_READY, 4=MATCH_START, 5=RUN
	unsigned long m_postStartDelayMs = 0; // extra safety delay before starting sequence
	std::unordered_map<unsigned int, unsigned long> m_readyDelayMs; // charId -> remaining ms before marking ready

	// Short grace window after entering RUN before we evaluate alive/faint logic
	unsigned long m_runSettleMs = 0;
	// Small delayed pulse to re-send ATTACKABLE shortly after RUN starts
	unsigned long m_runUnlockPulseMs = 0;
	// Watchdog to force-complete match if MATCH_FINISH stalls
	unsigned long m_matchFinishWatchdogMs = 0;

	// Watchdog bookkeeping: track durations in current states and automation phases
	State m_prevState = State::IDLE;
	unsigned long m_stateElapsedMs = 0;           // ms spent in current Arena state
	AutoState m_prevAutoState = AutoState::OFF;
	unsigned long m_autoStateElapsedMs = 0;       // ms spent in current AutoArena phase
	unsigned long m_autoEnsureRemainMs = 0;       // independent ensure timer to force reopen if scheduler stalls

	// Post-finish teleport delay timer
	unsigned long m_postFinishTeleportRemainMs = 0;

	// Battle state timers (similar to RankBattle)
	unsigned long m_directionTimeMs = 0;    // Time for direction/intro phase
	unsigned long m_matchReadyTimeMs = 0;   // Time for match ready phase
	unsigned long m_stageReadyTimeMs = 0;   // Time for stage ready phase ("READY" screen)

	// Rank Battle State Management (for CC Battle Mode)
	eRANKBATTLE_BATTLESTATE m_rankBattleState = INVALID_RANKBATTLE_BATTLESTATE;
	BYTE m_rankBattleStage = 0;
	unsigned long m_rankStateTimeMs = 0;    // Timer for current rank battle state

	std::unordered_set<unsigned int> m_participants; // CHARACTERID
	std::unordered_set<unsigned int> m_spectators;   // CHARACTERID
	std::unordered_set<unsigned int> m_winners;      // CHARACTERID
	std::vector<HOBJECT> m_spawnedMobs; // handles of spawned arena mobs

	// scoring
	std::unordered_map<unsigned int, unsigned int> m_killPoints; // charId -> kills

	struct PrevLoc { unsigned int worldId; CNtlVector loc; CNtlVector dir; };
	std::unordered_map<unsigned int, PrevLoc> m_prevLoc; // charId -> previous location

	// Random mob waves state
	unsigned long m_randomWaveRemainMs = 0;
	// Mob name -> id mapping loaded from file for @addmob by name
	std::unordered_map<std::string, unsigned int> m_mobNameToId;

	// Track which characters we force-enabled PvP zone for (to safely revert on finish)
	std::unordered_set<unsigned int> m_worldWidePvpToggled;

	// Pending delayed revive timers: charId -> ms remaining
	std::unordered_map<unsigned int, unsigned long> m_pendingReviveMs;
	// Post-revive protection timers: charId -> ms remaining
	std::unordered_map<unsigned int, unsigned long> m_reviveProtectRemainMs;
	// Track worlds where we applied a temporary rule override to restore on finish
	std::unordered_set<unsigned int> m_worldsWithOverride;
};

#define GetArenaManager() CArenaManager::GetInstance()
#define g_pArenaManager GetArenaManager()
