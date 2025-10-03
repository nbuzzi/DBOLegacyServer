#pragma once

#include "NtlSingleton.h"
#include "NtlString.h"
#include "NtlObject.h"
#include <vector>
#include <string>
#include <utility>
#include <unordered_set>
#include <unordered_map>

class CNtlIniFile;
class CPlayer;

class CEventManager : public CNtlSingleton<CEventManager>
{
public:
	enum class State : unsigned char
	{
		IDLE = 0,
		ENROLLMENT,      // Players joining via @participate
		PRE_ROUND,       // Players teleported, waiting for arrival
		IN_ROUND,        // Round is active, mobs spawned
		INTERMISSION,    // Between rounds
		COMPLETE         // Event finished
	};

	struct MinionGroup
	{
		std::vector<unsigned int> minionTblidxList; // Mob IDs to spawn as minions
		float spawnRadius;                           // Radius around boss to spawn
		unsigned int count;                          // How many minions (0 = spawn all types x1)

		MinionGroup() : spawnRadius(15.0f), count(0) {}
	};

	struct EventRound
	{
		std::vector<unsigned int> mobTblidxList;  // Mobs to spawn this round (bosses)
		std::vector<std::pair<unsigned int, unsigned int>> fixedRewards; // itemTblidx, count
		unsigned int durationSeconds;             // Round timer duration
		bool useCustomDropMobs;                   // Apply CustomDropEvent modifications
		bool useLootRange;                        // Use range-based loot instead of fixed
		unsigned int lootRangeItemId;             // Item to create if useLootRange=true
		unsigned int lootRangeCount;              // Number of items to create in range
		unsigned int worldTblidx;                 // World for this round (0 = use default)
		float spawnPosX, spawnPosY, spawnPosZ;    // Round-specific spawn position (optional)
		std::vector<MinionGroup> minionGroups;    // Minion groups to spawn per boss

		EventRound()
			: durationSeconds(180), useCustomDropMobs(true), useLootRange(false),
			  lootRangeItemId(0), lootRangeCount(10), worldTblidx(0),
			  spawnPosX(0), spawnPosY(0), spawnPosZ(0) {}
	};

	struct Config
	{
		bool enabled;
		// Only run on channels containing this name (case-insensitive)
		CNtlString channelNameContains;
		// World to use for events
		unsigned int eventWorldTblidx;
		float spawnPosX, spawnPosY, spawnPosZ;
		// Rounds configuration
		std::vector<EventRound> rounds;
		// Tick/usage limits
		unsigned int maxTickCount;                // Max ticks per event instance
		unsigned int tickIntervalMs;              // Tick interval
		// Enrollment
		unsigned int enrollmentSeconds;           // How long enrollment is open
		bool requireParticipateCommand;           // Must use @participate to join (default true)
		// Teleport settings
		unsigned int startDelaySeconds;           // Delay before starting after teleport
		float teleportPosX, teleportPosY, teleportPosZ;
		// Post-event teleport
		bool postEventTeleport;
		unsigned int postEventWorldTblidx;
		float postEventPosX, postEventPosY, postEventPosZ;
		unsigned int postEventTeleportDelayMs;
		// Mudosa rewards
		unsigned int mudosaPerRound;              // Mudosa points per round completion
		unsigned int mudosaEventComplete;         // Bonus for completing all rounds
		// Auto-event scheduler
		bool autoEnabled;
		unsigned int autoIntervalSeconds;         // How often to auto-start
		unsigned int autoInitialDelaySeconds;     // Initial delay on server start
		bool autoRestartOnComplete;               // Automatically restart event after completion
		unsigned int autoRestartDelaySeconds;     // Delay before restarting (after post-event teleport)
		// World rotation
		bool worldRotationEnabled;                // Enable world rotation per round
		std::vector<unsigned int> worldTblidxList; // List of worlds to rotate through
		bool randomizeWorlds;                     // Pick random world each round
		// Mob spawn configuration
		float mobSpawnRadius;                     // Radius around spawn point for mobs
		bool randomMobPositions;                  // Randomize mob positions within radius
		// Spectators
		bool spectatorsEnabled;
		// Verbose logging
		bool verboseLogs;

		Config()
			: enabled(false), channelNameContains("EVENTS"),
			  eventWorldTblidx(1), spawnPosX(0), spawnPosY(0), spawnPosZ(0),
			  maxTickCount(0), tickIntervalMs(1000),
			  enrollmentSeconds(300), requireParticipateCommand(true),
			  startDelaySeconds(10),
			  teleportPosX(0), teleportPosY(0), teleportPosZ(0),
			  postEventTeleport(true), postEventWorldTblidx(1),
			  postEventPosX(4975.609863f), postEventPosY(-48.869999f), postEventPosZ(4012.609863f),
			  postEventTeleportDelayMs(3000),
			  mudosaPerRound(1000), mudosaEventComplete(5000),
			  autoEnabled(true), autoIntervalSeconds(1800), autoInitialDelaySeconds(0),
			  autoRestartOnComplete(false), autoRestartDelaySeconds(300),
			  worldRotationEnabled(false), randomizeWorlds(false),
			  mobSpawnRadius(50.0f), randomMobPositions(true),
			  spectatorsEnabled(false), verboseLogs(false) {}
	};

public:
	CEventManager();
	~CEventManager();

	bool LoadConfigFromIniPath(const char* iniPath);
	void TickProcess(unsigned long dwTickDiff);
	void AutomationTick(unsigned long dwTickDiff);

	// Event control
	void Start();
	void Stop(bool abort = false);
	void StatusTo(CPlayer* pWho);

	// Runtime utilities
	bool ReloadConfigFromDefault();                // Reload .\\config\\Events.cfg
	void ResetAutomation(bool startIfZeroDelay);   // Reset auto state; optionally start immediately when delay=0
	void BeginNow();                                // Force close enrollment and begin pre-round immediately

	// Participant management
	bool AddParticipant(CPlayer* pPlayer);
	bool AddSpectator(CPlayer* pPlayer);
	bool RemoveParticipant(CPlayer* pPlayer);
	bool IsParticipant(CPlayer* pPlayer) const;
	bool IsParticipantId(unsigned int charId) const;

	// Round management
	void StartNextRound();
	void CompleteCurrentRound();
	void SpawnRoundMobs(const EventRound& round);
	void SpawnMinionsAroundBoss(const CNtlVector& bossPos, const MinionGroup& minionGroup, unsigned int worldId);
	void AwardRoundRewards(const EventRound& round);
	void CreateLootInRange(unsigned int itemId, unsigned int count, const CNtlVector& center, float radius);

	// State queries
	State GetState() const { return m_state; }
	const Config& GetConfig() const { return m_cfg; }
	bool IsEnabled() const { return m_cfg.enabled; }
	unsigned int GetCurrentRound() const { return m_currentRound; }
	unsigned int GetTotalRounds() const { return (unsigned int)m_cfg.rounds.size(); }

	// Player callbacks
	void OnPlayerEnterWorld(CPlayer* pPlayer);
	// Called when player has fully loaded into the world (login complete)
	void OnPlayerEnterWorldComplete(CPlayer* pPlayer);
	void OnMobKilled(unsigned int mobHandle);

private:
	// ChatServer-wide notice (like Arena): broadcasts to channel via ChatServer
	void SendNotice(const wchar_t* msg, unsigned char byType = 2);
	void BroadcastSystem(const wchar_t* msg, unsigned char byType = 2);
	void SendSystemTo(CPlayer* pPlayer, const wchar_t* msg, unsigned char byType = 3);
	void TeleportParticipants();
	void TeleportParticipantsToWorld(unsigned int worldTblidx, float x, float y, float z);
	// Arena-like teleport helpers (single-recipient)
	bool TeleportOneToWorldTblidx(CPlayer* pPlayer, unsigned int worldTblidx, float posX, float posY, float posZ);
	bool TeleportOneToWorldTblidxDir(CPlayer* pPlayer, unsigned int worldTblidx, float posX, float posY, float posZ, float dirX, float dirY, float dirZ);
	void PostEventTeleportAll();
	void StartRoundTimer(unsigned int seconds);
	void StopRoundTimer();
	void CheckRoundCompletion();
	bool IsChannelValid();
	void ParseRoundsCsv(const CNtlString& csv);
	void ParseWorldListCsv(const CNtlString& csv);
	void BroadcastDungeonStateToWorld(unsigned int worldId, unsigned char byStage, unsigned int titleTblidx = 0);
	void BroadcastRoundTimerStartToWorld(unsigned int worldId, unsigned int seconds);
	void BroadcastRoundTimerEndToWorld(unsigned int worldId);
	void BroadcastCountdownToWorld(unsigned int worldId, bool bStart);
	void SendCountdownTo(CPlayer* pPlayer, bool bStart);
	void SendRoundTimerStartTo(CPlayer* pPlayer, unsigned int seconds);
	void SendRoundTimerEndTo(CPlayer* pPlayer);
	unsigned int GetWorldForRound(unsigned int roundIndex);
	void GetSpawnPosForRound(unsigned int roundIndex, float& outX, float& outY, float& outZ);

	// Automation
	enum class AutoState : unsigned char { OFF = 0, WAIT_NEXT, ENROLLMENT_OPEN, WAIT_RESTART };
	AutoState m_autoState = AutoState::OFF;
	unsigned long m_autoRemainMs = 0;
	unsigned int m_worldRotationIndex = 0;  // Current index in world rotation list

private:
	Config m_cfg;
	State m_state;
	unsigned int m_currentRound;
	unsigned int m_eventWorldId;              // WORLDID of active event instance
	unsigned long m_tickCount;                // Current tick count
	unsigned long m_enrollmentRemainMs;       // Enrollment timer
	unsigned long m_startDelayRemainMs;       // Pre-round delay timer
	unsigned long m_roundRemainMs;            // Round timer
	bool m_roundTimerActive;

	std::unordered_set<unsigned int> m_participants;  // CHARACTERID
	std::unordered_set<unsigned int> m_spectators;    // CHARACTERID
	std::vector<HOBJECT> m_spawnedMobs;               // Handles of spawned event mobs
	std::unordered_set<HOBJECT> m_killedMobs;         // Track killed mobs this round

	// Saved locations for teleport back
	struct PrevLoc { unsigned int worldId; CNtlVector loc; CNtlVector dir; };
	std::unordered_map<unsigned int, PrevLoc> m_prevLoc;

	// Post-event teleport delay
	unsigned long m_postEventTeleportRemainMs;

	// Enrollment announcement helper to avoid spamming every tick
	unsigned int m_nextEnrollmentAnnounceSec = 0;

	// Pre-round/intermission countdown helpers
	bool m_countdownActive = false;
	unsigned int m_nextPreRoundAnnounceSec = 0;
};

#define GetEventManager() CEventManager::GetInstance()
#define g_pEventManager GetEventManager()
