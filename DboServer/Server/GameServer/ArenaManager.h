#pragma once

#include "NtlSingleton.h"
#include "NtlString.h"
#include <vector>
#include <utility>
#include <unordered_set>

class CNtlIniFile;
class CPlayer;

class CArenaManager : public CNtlSingleton<CArenaManager>
{
public:
    enum class Mode : unsigned char
    {
        GUILD_VS_GUILD = 0,
        PARTY_VS_PARTY = 1,
        FREE_FOR_ALL   = 2,
        OPEN           = 3
    };

    struct Config
    {
        bool enabled;
        // comma-separated list of world tblidx values
        std::vector<unsigned int> worldTblidxList;
        unsigned int rotationSeconds;
        unsigned int roundTimerSeconds; // visible UI countdown duration
        // spectators
        bool spectatorsEnabled;
        bool spectatorsUseSameWorld;
        unsigned int spectatorWorldTblidx; // used when not using same world
        float spectatorPosX;
        float spectatorPosY;
        float spectatorPosZ;
        // rewards
        bool rewardsEnabled;
        // pair<itemTblidx,count>
        std::vector<std::pair<unsigned int, unsigned int>> winnerRewards;
        std::vector<std::pair<unsigned int, unsigned int>> participantRewards;

        Config()
            : enabled(false), rotationSeconds(0), roundTimerSeconds(180), spectatorsEnabled(false), spectatorsUseSameWorld(true),
              spectatorWorldTblidx(0), spectatorPosX(0), spectatorPosY(0), spectatorPosZ(0),
              rewardsEnabled(false) {}
    };

    enum class State : unsigned char
    {
        IDLE = 0,
        ENROLLMENT,
        PRE_ROUND,
        IN_ROUND,
        INTERMISSION,
        COMPLETE
    };

public:
    CArenaManager();
    ~CArenaManager();

    bool LoadConfigFromIniPath(const char* iniPath);

    void TickProcess(unsigned long dwTickDiff);

    // GM control
    void Start(Mode mode);
    void Stop(bool abort = false);
    void RotateMapNow();
    void StatusTo(CPlayer* pWho);
    bool SetCurrentWorld(unsigned int worldTblidx);

    // Enrollment
    bool AddParticipant(CPlayer* pPlayer);
    bool AddSpectator(CPlayer* pPlayer);
    bool Remove(CPlayer* pPlayer);
    void TeleportParticipants();
    void TeleportSpectators();
    void TeleportParticipantsHere(CPlayer* pGm);
    void TeleportSpectatorsHere(CPlayer* pGm);
    void MarkWinner(CPlayer* pPlayer);
    void ClearWinners();
    void AwardRewards(bool winnersOnly);

    // Round timer UI
    void StartRoundTimerUI(unsigned int seconds);
    void StopRoundTimerUI();

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

private:
    void BroadcastSystem(const wchar_t* msg);
    void TryRotateByTime(unsigned long dwTickDiff);
    void ParseWorldListCsv(const CNtlString& csv);
    void ParseRewardsCsv(const CNtlString& csv, std::vector<std::pair<unsigned int, unsigned int>>& out);
    bool TeleportOneToWorldTblidx(CPlayer* pPlayer, unsigned int worldTblidx, float posX, float posY, float posZ);
    void BroadcastRoundTimerStartToWorld(unsigned int worldId, unsigned int seconds);
    void BroadcastRoundTimerEndToWorld(unsigned int worldId);

private:
    Config m_cfg;
    State m_state;
    Mode m_mode;
    unsigned int m_currentWorldTblidx;
    size_t m_worldIndex;
    unsigned long m_rotationRemainMs;

    // Round timer state
    bool m_roundUiActive = false;
    unsigned long m_roundRemainMs = 0;
    unsigned int m_roundWorldId = 0; // WORLDID

    std::unordered_set<unsigned int> m_participants; // CHARACTERID
    std::unordered_set<unsigned int> m_spectators;   // CHARACTERID
    std::unordered_set<unsigned int> m_winners;      // CHARACTERID
};

#define GetArenaManager() CArenaManager::GetInstance()
#define g_pArenaManager GetArenaManager()
