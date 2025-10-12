#ifndef __PLAYER_MODIFIERS_SYSTEM__
#define __PLAYER_MODIFIERS_SYSTEM__

#include "NtlSingleton.h"
#include "NtlSharedType.h"
#include "NtlString.h"
#include <unordered_map>
#include <string>
#include <functional>
#include <unordered_set>

class CCharacterAttPC;

class CPlayerModifiers : public CNtlSingleton<CPlayerModifiers>
{
public:
    struct Modifiers
    {
        float hp;          // MaxLP multiplier
        float physAtk;     // PhysicalOffence multiplier
        float engAtk;      // EnergyOffence multiplier
        float physDef;     // PhysicalDefence multiplier
        float engDef;      // EnergyDefence multiplier
        float atkSpd;      // AttackSpeedRate multiplier (higher = slower, as it's a rate)
        float runSpd;      // RunSpeed multiplier
        float physCrit;    // PhysicalCriticalRate multiplier
        float engCrit;     // EnergyCriticalRate multiplier
        float physCritDmg; // PhysicalCriticalDamageRate multiplier
        float engCritDmg;  // EnergyCriticalDamageRate multiplier
        float attackRate;  // AttackRate multiplier (hit)
        float dodgeRate;   // DodgeRate multiplier
        float blockRate;   // BlockRate multiplier
        float blockDmg;    // BlockDamageRate multiplier
        float guardRate;   // GuardRate multiplier

        Modifiers()
            : hp(1.f), physAtk(1.f), engAtk(1.f), physDef(1.f), engDef(1.f),
              atkSpd(1.f), runSpd(1.f), physCrit(1.f), engCrit(1.f), physCritDmg(1.f), engCritDmg(1.f),
              attackRate(1.f), dodgeRate(1.f), blockRate(1.f), blockDmg(1.f), guardRate(1.f) {}
        bool IsIdentity() const
        {
            return hp == 1.f && physAtk == 1.f && engAtk == 1.f && physDef == 1.f && engDef == 1.f &&
                   atkSpd == 1.f && runSpd == 1.f && physCrit == 1.f && engCrit == 1.f &&
                   physCritDmg == 1.f && engCritDmg == 1.f && attackRate == 1.f && dodgeRate == 1.f &&
                   blockRate == 1.f && blockDmg == 1.f && guardRate == 1.f;
        }
    };

    struct AutoScheduleConfig
    {
        bool enabled;                      // Enable auto-scheduling
        unsigned int daysPerWeek;          // How many times per week (1-7)
        unsigned int durationHours;        // Duration of each session in hours
        unsigned int intervalHours;        // Hours between sessions
        unsigned int initialDelayMinutes;  // Initial delay after server start

        // Channel filtering
        bool channelFilterEnabled;         // Enable channel-based filtering
        std::unordered_set<BYTE> allowedChannels; // Allowed channel numbers (empty = all)
        CNtlString channelNameContains;    // Channel name filter (case-insensitive, like EventManager)

        AutoScheduleConfig()
            : enabled(false), daysPerWeek(3), durationHours(3),
              intervalHours(56), initialDelayMinutes(30),
              channelFilterEnabled(false), channelNameContains("") {}
    };

public:
    CPlayerModifiers();
    virtual ~CPlayerModifiers();

    bool ReloadConfig(const char* path = ".\\config\\PlayerModifiers.cfg");
    const Modifiers& Get() const { return m_mods; }
    bool IsEnabled() const { return m_enabled; }
    void SetEnabled(bool on) { m_enabled = on; }
    const char* GetCfgPath() const { return m_cfgPath.c_str(); }

    // Apply modifiers to a fully-calculated player attribute block
    void ApplyTo(CCharacterAttPC* att);

    // Auto-schedule system (tick-based)
    void AutoScheduleTick(unsigned long dwTickDiff);
    const AutoScheduleConfig& GetAutoScheduleConfig() const { return m_autoScheduleCfg; }
    bool IsAutoScheduleActive() const { return m_autoScheduleActive; }
    unsigned long GetAutoScheduleRemainingMs() const { return m_autoScheduleRemainingMs; }

    // Always-on mode (24/7 enabled, no auto-schedule)
    void SetAlwaysOn(bool on) { m_alwaysOn = on; }
    bool IsAlwaysOn() const { return m_alwaysOn; }

private:
    void Init();
    bool LoadConfigInternal(const char* path);
    void StartAutoScheduleSession();
    void EndAutoScheduleSession();
    bool IsChannelAllowed() const;  // Check if current channel is allowed

private:
    bool m_enabled;
    Modifiers m_mods;
    // Per-player overrides
    std::unordered_map<unsigned int, Modifiers> m_byCharId; // key: CharID
    std::unordered_map<std::wstring, Modifiers> m_byCharName; // key: wide char name (case-insensitive normalized)
    CNtlString m_cfgPath;

    // Auto-schedule state
    AutoScheduleConfig m_autoScheduleCfg;
    enum class AutoScheduleState : unsigned char { IDLE = 0, WAIT_NEXT, ACTIVE };
    AutoScheduleState m_autoScheduleState;
    bool m_autoScheduleActive;          // Is a session currently active?
    unsigned long m_autoScheduleRemainingMs; // Time remaining in current state (wait or active)
    bool m_alwaysOn;                    // If true, always enabled (24/7 mode, no scheduling)
};

#define GetPlayerModifiers() CPlayerModifiers::GetInstance()
#define g_pPlayerModifiers GetPlayerModifiers()

#endif
