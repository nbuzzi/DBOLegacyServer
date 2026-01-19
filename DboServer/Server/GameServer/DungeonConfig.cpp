#include "stdafx.h"
#include "DungeonConfig.h"
#include "NtlIniFile.h"
#include "NtlLog.h"

CDungeonConfig::CDungeonConfig()
{
    Init();
}

CDungeonConfig::~CDungeonConfig()
{
}

void CDungeonConfig::Init()
{
    // Initialize all dungeon settings to their default values
    m_bCCBDBossOnlyMode = false;  // Default: Normal CCBD mode (all 150 floors)

    m_strIniFilePath = ".\\config\\GameServer.ini";
}

bool CDungeonConfig::ReadBoolValue(CNtlIniFile& file, const char* section, const char* key, bool defaultValue)
{
    int value = defaultValue ? 1 : 0;
    file.Read(section, key, value);
    return (value != 0);
}

bool CDungeonConfig::LoadFromFile(const char* iniFilePath)
{
    if (iniFilePath && *iniFilePath != '\0')
    {
        m_strIniFilePath = iniFilePath;
    }

    CNtlIniFile file;
    int result = file.Create(m_strIniFilePath.c_str());
    if (result != NTL_SUCCESS)
    {
        ERR_LOG(LOG_GENERAL, "[DUNGEON_CONFIG] Failed to load configuration from %s (CNtlIniFile::Create returned %d). Using defaults.",
            m_strIniFilePath.c_str(), result);
        return false;
    }

    // Load [DUNGEONS] section settings
    m_bCCBDBossOnlyMode = ReadBoolValue(file, "DUNGEONS", "IsCCBDBossOnlyMode", false);

    ERR_LOG(LOG_GENERAL, "[DUNGEON_CONFIG] Loaded configuration from %s [DUNGEONS] section", m_strIniFilePath.c_str());
    ERR_LOG(LOG_GENERAL, "[DUNGEON_CONFIG]   CCBD Boss-Only Mode: %s", m_bCCBDBossOnlyMode ? "ENABLED" : "DISABLED");

    return true;
}
