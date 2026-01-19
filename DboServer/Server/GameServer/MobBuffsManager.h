#ifndef __MOB_BUFFS_MANAGER_H__
#define __MOB_BUFFS_MANAGER_H__

#include "NtlSingleton.h"
#include "NtlString.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>

class CMonster;

class CMobBuffsManager : public CNtlSingleton<CMobBuffsManager>
{
public:
    struct BuffEntry
    {
        unsigned int skillTblidx; // Skill ID to use for buff
        unsigned int durationMs;  // Optional override duration (0 = use skill default)
        BuffEntry() : skillTblidx(0), durationMs(0) {}
        BuffEntry(unsigned int s, unsigned int d) : skillTblidx(s), durationMs(d) {}
    };

public:
    CMobBuffsManager();
    virtual ~CMobBuffsManager();

    // Load text config from path (default .\\config\\MobBuffs.cfg)
    bool LoadConfigFromIniPath(const char* path = ".\\config\\MobBuffs.cfg");

    // Apply configured buffs to a monster according to its worldTblidx and mob tblidx
    void ApplyBuffs(CMonster* pMob);

    void SetEnabled(bool on) { m_enabled = on; }
    bool IsEnabled() const { return m_enabled; }
    void SetVerbose(bool on) { m_verbose = on; }
    bool IsVerbose() const { return m_verbose; }

private:
    void Init();
    void Clear();

    // Internal: composite key world<<32 | mob
    static unsigned long long MakeKey(unsigned int worldTblidx, unsigned int mobTblidx)
    {
        return (static_cast<unsigned long long>(worldTblidx) << 32) | static_cast<unsigned long long>(mobTblidx);
    }

private:
    // Mapping: (worldTblidx, mobTblidx) -> list of buffs. Use 0 for wildcard.
    std::unordered_map<unsigned long long, std::vector<BuffEntry>> m_worldMobBuffs;
    // Exceptions for world-level global rules (mob=0). worldTblidx -> set of mob tblidx to skip.
    std::unordered_map<unsigned int, std::unordered_set<unsigned int>> m_exceptWorldGlobals;
    
    CNtlString m_cfgPath;
    bool m_enabled;
    bool m_verbose;
};

#define GetMobBuffsManager() CMobBuffsManager::GetInstance()
#define g_pMobBuffsManager GetMobBuffsManager()

#endif // __MOB_BUFFS_MANAGER_H__
