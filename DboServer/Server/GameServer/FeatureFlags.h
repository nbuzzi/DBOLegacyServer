#ifndef __FEATURE_FLAGS_H__
#define __FEATURE_FLAGS_H__

#include "NtlSingleton.h"
#include "NtlString.h"

class CNtlIniFile;

class CFeatureFlags : public CNtlSingleton<CFeatureFlags>
{
public:
    CFeatureFlags();
    virtual ~CFeatureFlags();

    bool LoadFromFile(const char* configPath = ".\\config\\FeatureFlags.cfg");

    // Feature flag getters
    bool IsVirtualTransformationsEnabled() const { return m_bEnableVirtualTransformations; }
    bool IsBudokaiEnabled() const { return m_bEnableBudokai; }
    bool IsDojoEnabled() const { return m_bEnableDojo; }
    bool IsRankBattleEnabled() const { return m_bEnableRankBattle; }
    bool IsDWCEnabled() const { return m_bEnableDWC; }
    bool IsTMQEnabled() const { return m_bEnableTMQ; }
    bool IsQuickSlotEnabled() const { return m_bEnableQuickSlot; }
    bool IsItemUpgradeEnabled() const { return m_bEnableItemUpgrade; }
    bool IsItemExchangeEnabled() const { return m_bEnableItemExchange; }
    bool IsPartyMatchmakingEnabled() const { return m_bEnablePartyMatchmaking; }

    // Runtime flag modification (for testing/debugging)
    void SetVirtualTransformationsEnabled(bool enabled) { m_bEnableVirtualTransformations = enabled; }
    void SetBudokaiEnabled(bool enabled) { m_bEnableBudokai = enabled; }
    void SetDojoEnabled(bool enabled) { m_bEnableDojo = enabled; }
    void SetRankBattleEnabled(bool enabled) { m_bEnableRankBattle = enabled; }
    void SetDWCEnabled(bool enabled) { m_bEnableDWC = enabled; }
    void SetTMQEnabled(bool enabled) { m_bEnableTMQ = enabled; }
    void SetQuickSlotEnabled(bool enabled) { m_bEnableQuickSlot = enabled; }
    void SetItemUpgradeEnabled(bool enabled) { m_bEnableItemUpgrade = enabled; }
    void SetItemExchangeEnabled(bool enabled) { m_bEnableItemExchange = enabled; }
    void SetPartyMatchmakingEnabled(bool enabled) { m_bEnablePartyMatchmaking = enabled; }

private:
    void Init();
    bool ReadBoolFlag(CNtlIniFile& file, const char* section, const char* key, bool defaultValue);

private:
    bool m_bEnableVirtualTransformations;
    bool m_bEnableBudokai;
    bool m_bEnableDojo;
    bool m_bEnableRankBattle;
    bool m_bEnableDWC;
    bool m_bEnableTMQ;
    bool m_bEnableQuickSlot;
    bool m_bEnableItemUpgrade;
    bool m_bEnableItemExchange;
    bool m_bEnablePartyMatchmaking;

    CNtlString m_configPath;
};

#define GetFeatureFlags() CFeatureFlags::GetInstance()
#define g_pFeatureFlags GetFeatureFlags()

#endif // __FEATURE_FLAGS_H__
