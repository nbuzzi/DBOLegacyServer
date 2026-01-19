#ifndef __MOB_APPEARANCE_OVERRIDE_MANAGER_H__
#define __MOB_APPEARANCE_OVERRIDE_MANAGER_H__

#include "NtlSingleton.h"
#include "NtlString.h"
#include <unordered_map>

class CMobAppearanceOverrideManager : public CNtlSingleton<CMobAppearanceOverrideManager>
{
public:
    struct ReplacementRule
    {
        unsigned int targetTblidx;   // mob tblidx to display
        bool useTargetStats;         // true = use target mob stats/template, false = keep original stats

        ReplacementRule() : targetTblidx(0), useTargetStats(true) {}
        ReplacementRule(unsigned int target, bool useStats) : targetTblidx(target), useTargetStats(useStats) {}
    };

public:
    CMobAppearanceOverrideManager();
    virtual ~CMobAppearanceOverrideManager();

    // Load config from disk. When file missing, manager stays disabled.
    bool LoadConfigFromPath(const char* path = ".\\config\\MobAppearanceOverrides.cfg");

    bool IsEnabled() const { return m_enabled && (!m_rules.empty() || m_hasGlobalRule); }
    bool IsVerbose() const { return m_verbose; }
    void SetEnabled(bool on) { m_enabled = on; }
    void SetVerbose(bool on) { m_verbose = on; }

    // Returns true when there is a replacement rule for the given source tblidx.
    // When multiple rules exist, specific mob id wins over global (mob=0).
    bool GetReplacement(unsigned int sourceTblidx, ReplacementRule& outRule) const;

private:
    void Init();
    void Clear();
    bool ParseReplacementLine(char* line, unsigned int& outSource, ReplacementRule& outRule);

private:
    std::unordered_map<unsigned int, ReplacementRule> m_rules; // per-mob overrides (source -> rule)
    ReplacementRule m_globalRule;
    bool m_hasGlobalRule;

    CNtlString m_cfgPath;
    bool m_enabled;
    bool m_verbose;
};

#define GetMobAppearanceOverrideManager() CMobAppearanceOverrideManager::GetInstance()
#define g_pMobAppearanceOverrideManager GetMobAppearanceOverrideManager()

#endif // __MOB_APPEARANCE_OVERRIDE_MANAGER_H__

