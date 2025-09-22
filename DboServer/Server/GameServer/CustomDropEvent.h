#ifndef __CUSTOM_DROP_EVENT_SYSTEM__
#define __CUSTOM_DROP_EVENT_SYSTEM__

#include "NtlSingleton.h"
#include "NtlSharedType.h"
#include "NtlString.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>

class CMonster;
class CCharacter;

class CCustomDropEvent : public CNtlSingleton<CCustomDropEvent>
{
public:
    struct DropEntry
    {
        unsigned int itemTblidx;
        float rate; // 0..100
        BYTE count; // optional amount per successful roll (default 1)
    };

    struct Modifiers
    {
        float hp;          // multiplier for MaxLP
        float physAtk;     // multiplier for PhysicalOffence
        float engAtk;      // multiplier for EnergyOffence
        float physDef;     // multiplier for PhysicalDefence
        float engDef;      // multiplier for EnergyDefence
        float atkSpd;      // multiplier for AttackSpeedRate
        float runSpd;      // multiplier for RunSpeed
        float physCrit;    // multiplier for PhysicalCriticalRate
        float engCrit;     // multiplier for EnergyCriticalRate
        float physCritDmg; // multiplier for PhysicalCriticalDamageRate
        float engCritDmg;  // multiplier for EnergyCriticalDamageRate
        float attackRate;  // multiplier for AttackRate (hit)
        float dodgeRate;   // multiplier for DodgeRate
        float blockRate;   // multiplier for BlockRate
        float blockDmg;    // multiplier for BlockDamageRate
        float guardRate;   // multiplier for GuardRate
        int sizeRate;      // absolute size rate (client expects BYTE 1..20 typically, default 10)

        Modifiers()
            : hp(1.f), physAtk(1.f), engAtk(1.f), physDef(1.f), engDef(1.f),
              atkSpd(1.f), runSpd(1.f), physCrit(1.f), engCrit(1.f), physCritDmg(1.f), engCritDmg(1.f),
              attackRate(1.f), dodgeRate(1.f), blockRate(1.f), blockDmg(1.f), guardRate(1.f), sizeRate(0) {}
        bool IsIdentity() const
        {
            return hp == 1.f && physAtk == 1.f && engAtk == 1.f && physDef == 1.f && engDef == 1.f &&
                   atkSpd == 1.f && runSpd == 1.f && physCrit == 1.f && engCrit == 1.f &&
                   physCritDmg == 1.f && engCritDmg == 1.f && attackRate == 1.f && dodgeRate == 1.f &&
                   blockRate == 1.f && blockDmg == 1.f && guardRate == 1.f && sizeRate == 0;
        }
    };

    struct SpawnEntry
    {
        unsigned int mobTblidx; // mob to spawn
        float rate;              // 0..100 probability
        BYTE count;              // how many to spawn when it triggers
    };

    struct BuffEntry
    {
        unsigned int skillTblidx; // skill to apply as a buff
        DWORD durationMs;         // optional override duration in ms (0 = use skill default)
        DWORD periodMs;           // optional per-buff pulse period in ms (0 = use totem interval)
    };
    struct TitleEntry
    {
        unsigned int titleTblidx; // character title table index
    };
    struct VisualEntry
    {
        unsigned int effectTblidx; // system effect table index (from SystemEffectTable)
        DWORD intervalMs;          // optional resend interval (not yet used; reserved)
    };

    struct TotemRule
    {
        unsigned int beaconMobTblidx; // mob to spawn as the totem/beacon
        DWORD lifeMs;                  // lifetime of the beacon in ms
        float radius;                  // buff application radius (meters)
        DWORD intervalMs;              // pulse interval in ms
        std::vector<BuffEntry> buffs;  // buffs to apply to players in range every pulse
        std::vector<SpawnEntry> guards; // optional: mobs to spawn immediately near beacon
    };

    struct ActiveTotem
    {
        HOBJECT hBeacon;               // spawned beacon handle
        DWORD expireTick;              // GetTickCount time when totem expires
        DWORD nextPulseTick;           // deprecated group tick; kept for compatibility
        float radius;                  // radius copied from rule
        DWORD intervalMs;              // pulse interval
        std::vector<BuffEntry> buffs;  // buffs to apply
        std::vector<DWORD> buffNextTicks; // per-buff next pulse times
    };

public:
    CCustomDropEvent();
    virtual ~CCustomDropEvent();

private:
    void Init();
    void CreateSingleDrop(CMonster *pMob, CCharacter *pPlayer, unsigned int dropId);
    void CreateStackedDrop(CMonster *pMob, CCharacter *pPlayer, unsigned int dropId, BYTE count);
    bool LoadConfigInternal(const char *path);

public:
    bool m_bOn;
    void StartEvent(BYTE byHours = 3);
    void EndEvent();
    void LoadEvent(HSESSION hSession);
    void TickProcess(DWORD dwTick);
    void Update(CMonster *pMob, CCharacter *pPlayer);
    bool ReloadConfig(const char *path = ".\\config\\CustomDropEvent.cfg");
    void ApplyModifiers(CMonster *pMob);
    void ApplyBuffs(CMonster *pMob);
    void ApplyTitles(CMonster *pMob);
    void ApplyVisuals(CMonster *pMob);
    void SetAllowChainSpawns(bool allow) { m_allowChainSpawns = allow; }
    bool IsAllowChainSpawns() const { return m_allowChainSpawns; }
    void SetTotemHealMultiplier(float mul) { m_totemHealMultiplier = mul; }
    float GetTotemHealMultiplier() const { return m_totemHealMultiplier; }
    void SetTotemBuffDurationOverrideMs(DWORD ms) { m_totemBuffDurationOverrideMs = ms; }
    DWORD GetTotemBuffDurationOverrideMs() const { return m_totemBuffDurationOverrideMs; }
    // Debuff immunity configuration (global)
    void SetDebuffImmunityEnabled(bool on) { m_debuffImmuneEnabled = on; }
    bool IsDebuffImmunityEnabled() const { return m_debuffImmuneEnabled; }
    bool IsDebuffEffectBlocked(int code) const { return m_blockDebuffEffects.find(code) != m_blockDebuffEffects.end(); }
    size_t GetBlockedDebuffEffectCount() const { return m_blockDebuffEffects.size(); }

private:
    DBOTIME m_timeStart;
    DBOTIME m_timeEnd;
    DWORD m_dwNextUpdateTick;
    DWORD m_dwNextTotemTick;

    // mob tblidx -> list of possible drops
    std::unordered_map<unsigned int, std::vector<DropEntry>> m_mobDrops;
    // mob tblidx -> modifiers
    std::unordered_map<unsigned int, Modifiers> m_mobMods;
    // mob tblidx -> spawn entries
    std::unordered_map<unsigned int, std::vector<SpawnEntry>> m_mobSpawns;
    // mob tblidx -> buff entries
    std::unordered_map<unsigned int, std::vector<BuffEntry>> m_mobBuffs;
    // mob tblidx -> title entries (attribute effects from CharTitleTable applied to mobs)
    std::unordered_map<unsigned int, std::vector<TBLIDX>> m_mobTitles;
    // mob tblidx -> visual system effects to broadcast to clients (purely visual)
    std::unordered_map<unsigned int, std::vector<VisualEntry>> m_mobVisuals;
    // mob tblidx -> explicit level to set on spawned mobs (1..255)
    std::unordered_map<unsigned int, BYTE> m_mobLevels;
    // mob tblidx -> totem rules (spawn beacon and pulse buffs)
    std::unordered_map<unsigned int, std::vector<TotemRule>> m_mobTotems;
    // active totems currently in the world
    std::vector<ActiveTotem> m_activeTotems;
    // exceptions for global (all) rules: skip applying for these mob ids
    std::unordered_set<unsigned int> m_exceptDrops;
    std::unordered_set<unsigned int> m_exceptMods;
    std::unordered_set<unsigned int> m_exceptSpawns;
    std::unordered_set<unsigned int> m_exceptBuffs;
    std::unordered_set<unsigned int> m_exceptTitles;
    std::unordered_set<unsigned int> m_exceptVisuals;
    std::unordered_set<unsigned int> m_exceptTotems;
    // handles of mobs spawned by this event (including beacons) to prevent chain triggers
    std::unordered_set<HOBJECT> m_eventSpawned;
    // global totem defaults
    float m_totemDefaultRadius;           // default radius when not specified
    DWORD m_totemDefaultIntervalMs;       // default interval when not specified
    float m_totemHealMultiplier;          // multiplier for HoT magnitude
    DWORD m_totemBuffDurationOverrideMs;  // override duration for totem-applied buffs (0 = use skill/default)
    CNtlString m_cfgPath;
    bool m_allowChainSpawns;              // allow event-spawned mobs to trigger spawn/totem rules
    // Debuff immunity: when true, event-modified mobs get marked as debuff-immune by default
    bool m_debuffImmuneEnabled;
    // Optional filter: if non-empty, only debuff effects in this set are blocked; otherwise all curse-type debuffs are blocked
    std::unordered_set<int> m_blockDebuffEffects;

    // Mob replacement mapping: when event is ON, replace original mob tblidx with target tblidx for spawn-table spawns
    std::unordered_map<unsigned int, unsigned int> m_replaceMob; // from -> to
    std::unordered_set<unsigned int> m_exceptReplace;            // exceptions for global replaces

public:
    // Returns replacement mob tblidx for given source, or 0 if none configured or exempted
    unsigned int GetMobReplacement(unsigned int srcTblidx) const
    {
        if (srcTblidx == 0) return 0;
        // honor exception list
        if (m_exceptReplace.find(srcTblidx) != m_exceptReplace.end())
            return 0;
        auto it = m_replaceMob.find(srcTblidx);
        if (it != m_replaceMob.end()) {
            unsigned int to = it->second; return (to != srcTblidx ? to : 0);
        }
        // global default mapping (id=0) applies to all unless excepted
        auto itAll = m_replaceMob.find(0);
        if (itAll != m_replaceMob.end()) {
            unsigned int to = itAll->second; return (to != srcTblidx ? to : 0);
        }
        return 0;
    }

private:
    bool LoadLevelsSidecar(const char* cfgPath);
};

#define GetCustomDropEvent() CCustomDropEvent::GetInstance()
#define g_pCustomDropEvent GetCustomDropEvent()

#endif
