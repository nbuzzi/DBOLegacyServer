#ifndef __DUNGEON_CONFIG_H__
#define __DUNGEON_CONFIG_H__

#include "NtlSingleton.h"
#include "NtlString.h"

class CNtlIniFile;

/**
 * @class CDungeonConfig
 * @brief Manages dungeon-specific configuration loaded from GameServer.ini [DUNGEONS] section
 *
 * This singleton class handles all dungeon-related configuration settings that control
 * dungeon behavior at runtime. Unlike FeatureFlags.cfg, these settings are loaded from
 * the main GameServer.ini file to keep dungeon configuration centralized.
 *
 * Configuration is loaded from GameServer.ini under the [DUNGEONS] section:
 *
 * [DUNGEONS]
 * IsCCBDBossOnlyMode=false  ; Skip regular floors, fight only bosses (every 5 floors)
 *
 * @note This class follows the same singleton pattern as FeatureFlags but uses
 *       GameServer.ini instead of a separate config file.
 */
class CDungeonConfig : public CNtlSingleton<CDungeonConfig>
{
public:
    CDungeonConfig();
    virtual ~CDungeonConfig();

    /**
     * @brief Load dungeon configuration from GameServer.ini
     * @param iniFilePath Path to GameServer.ini (default: .\\config\\GameServer.ini)
     * @return true if configuration loaded successfully, false otherwise
     *
     * This method reads the [DUNGEONS] section from GameServer.ini and initializes
     * all dungeon-related configuration settings.
     */
    bool LoadFromFile(const char* iniFilePath = ".\\config\\GameServer.ini");

    // ---- CCBD Configuration ----

    /**
     * @brief Check if CCBD Boss-Only Mode is enabled
     * @return true if boss-only mode is active, false for normal CCBD mode
     *
     * When enabled:
     * - Players skip regular floors (1-4, 6-9, 11-14, etc.)
     * - Only boss floors are accessible (5, 10, 15, 20, etc.)
     * - Stage progression happens automatically for non-boss floors
     *
     * When disabled (default):
     * - Normal CCBD behavior (all 150 floors with regular waves)
     */
    bool IsCCBDBossOnlyModeEnabled() const { return m_bCCBDBossOnlyMode; }

    /**
     * @brief Enable or disable CCBD Boss-Only Mode at runtime
     * @param enabled true to enable boss-only mode, false for normal mode
     *
     * This allows GM commands to toggle boss-only mode without restarting the server.
     * Changes take effect immediately for new dungeon entries.
     */
    void SetCCBDBossOnlyModeEnabled(bool enabled) { m_bCCBDBossOnlyMode = enabled; }

    // ---- Future Dungeon Settings ----
    // Add more dungeon-related configuration here as needed:
    // - Custom dungeon time limits
    // - Reward multipliers
    // - Difficulty scaling
    // - etc.

private:
    /**
     * @brief Initialize all settings to their default values
     *
     * Called in constructor to ensure all settings have safe defaults
     * before configuration file is loaded.
     */
    void Init();

    /**
     * @brief Helper method to safely read boolean values from INI file
     * @param file CNtlIniFile instance to read from
     * @param section INI section name (e.g., "DUNGEONS")
     * @param key INI key name (e.g., "IsCCBDBossOnlyMode")
     * @param defaultValue Value to return if key is not found
     * @return Boolean value from INI file or defaultValue if not found
     */
    bool ReadBoolValue(CNtlIniFile& file, const char* section, const char* key, bool defaultValue);

private:
    // ---- CCBD Settings ----
    bool m_bCCBDBossOnlyMode;  ///< When true, skip regular floors and fight only bosses (every 5 floors)

    // ---- Internal State ----
    CNtlString m_strIniFilePath;  ///< Path to GameServer.ini for reference
};

// Global accessor macros (following FeatureFlags pattern)
#define GetDungeonConfig() CDungeonConfig::GetInstance()
#define g_pDungeonConfig GetDungeonConfig()

#endif // __DUNGEON_CONFIG_H__
