#include "stdafx.h"
#include "FeatureFlags.h"
#include "NtlIniFile.h"
#include "NtlLog.h"

CFeatureFlags::CFeatureFlags()
{
    Init();
}

CFeatureFlags::~CFeatureFlags()
{
}

void CFeatureFlags::Init()
{
    // Default: all features enabled
    m_bEnableVirtualTransformations = true;
    m_bEnableBudokai = true;
    m_bEnableDojo = true;
    m_bEnableRankBattle = true;
    m_bEnableDWC = true;
    m_bEnableTMQ = true;
    m_bEnableQuickSlot = true;
    m_bEnableItemUpgrade = true;
    m_bEnableItemExchange = true;
    m_bEnablePartyMatchmaking = true;
    m_bEnableMobBuffs = true;

    m_configPath = ".\\config\\FeatureFlags.cfg";
}

bool CFeatureFlags::ReadBoolFlag(CNtlIniFile& file, const char* section, const char* key, bool defaultValue)
{
    int value = defaultValue ? 1 : 0;
    file.Read(section, key, value);
    return (value != 0);
}

bool CFeatureFlags::LoadFromFile(const char* configPath)
{
    if (configPath && *configPath != '\0')
    {
        m_configPath = configPath;
    }

    CNtlIniFile file;
    int result = file.Create(m_configPath.c_str());
    if (result != NTL_SUCCESS)
    {
        ERR_LOG(LOG_GENERAL, "[FEATURE_FLAGS] Failed to load config from %s (CNtlIniFile::Create returned %d). Using defaults.",
            m_configPath.c_str(), result);
        return false;
    }

    // Load feature flags
    m_bEnableVirtualTransformations = ReadBoolFlag(file, "Features", "EnableVirtualTransformations", true);
    m_bEnableBudokai = ReadBoolFlag(file, "Features", "EnableBudokai", true);
    m_bEnableDojo = ReadBoolFlag(file, "Features", "EnableDojo", true);
    m_bEnableRankBattle = ReadBoolFlag(file, "Features", "EnableRankBattle", true);
    m_bEnableDWC = ReadBoolFlag(file, "Features", "EnableDWC", true);
    m_bEnableTMQ = ReadBoolFlag(file, "Features", "EnableTMQ", true);
    m_bEnableQuickSlot = ReadBoolFlag(file, "Features", "EnableQuickSlot", true);
    m_bEnableItemUpgrade = ReadBoolFlag(file, "Features", "EnableItemUpgrade", true);
    m_bEnableItemExchange = ReadBoolFlag(file, "Features", "EnableItemExchange", true);
    m_bEnablePartyMatchmaking = ReadBoolFlag(file, "Features", "EnablePartyMatchmaking", true);
    m_bEnableMobBuffs = ReadBoolFlag(file, "Features", "EnableMobBuffs", true);

    ERR_LOG(LOG_GENERAL, "[FEATURE_FLAGS] Loaded configuration from %s", m_configPath.c_str());
    ERR_LOG(LOG_GENERAL, "[FEATURE_FLAGS]   VirtualTransformations: %s", m_bEnableVirtualTransformations ? "ENABLED" : "DISABLED");
    ERR_LOG(LOG_GENERAL, "[FEATURE_FLAGS]   Budokai: %s", m_bEnableBudokai ? "ENABLED" : "DISABLED");
    ERR_LOG(LOG_GENERAL, "[FEATURE_FLAGS]   Dojo: %s", m_bEnableDojo ? "ENABLED" : "DISABLED");
    ERR_LOG(LOG_GENERAL, "[FEATURE_FLAGS]   RankBattle: %s", m_bEnableRankBattle ? "ENABLED" : "DISABLED");
    ERR_LOG(LOG_GENERAL, "[FEATURE_FLAGS]   DWC: %s", m_bEnableDWC ? "ENABLED" : "DISABLED");
    ERR_LOG(LOG_GENERAL, "[FEATURE_FLAGS]   TMQ: %s", m_bEnableTMQ ? "ENABLED" : "DISABLED");
    ERR_LOG(LOG_GENERAL, "[FEATURE_FLAGS]   QuickSlot: %s", m_bEnableQuickSlot ? "ENABLED" : "DISABLED");
    ERR_LOG(LOG_GENERAL, "[FEATURE_FLAGS]   ItemUpgrade: %s", m_bEnableItemUpgrade ? "ENABLED" : "DISABLED");
    ERR_LOG(LOG_GENERAL, "[FEATURE_FLAGS]   ItemExchange: %s", m_bEnableItemExchange ? "ENABLED" : "DISABLED");
    ERR_LOG(LOG_GENERAL, "[FEATURE_FLAGS]   PartyMatchmaking: %s", m_bEnablePartyMatchmaking ? "ENABLED" : "DISABLED");
    ERR_LOG(LOG_GENERAL, "[FEATURE_FLAGS]   MobBuffs: %s", m_bEnableMobBuffs ? "ENABLED" : "DISABLED");

    return true;
}
