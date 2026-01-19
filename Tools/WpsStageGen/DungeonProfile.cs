using System.Text.Json;
using System.Text.Json.Serialization;

namespace WpsStageGen;

/// <summary>
/// Wave configuration for regular (non-boss) floors
/// </summary>
public class WaveConfig
{
    /// <summary>
    /// Pattern list for mob spawning with weighted probabilities
    /// Format: "(pattern_id, percentage), (pattern_id, percentage), ..."
    /// Example: "(1, 35%), (2, 35%), (3, 15%), (4, 15%)"
    /// </summary>
    [JsonPropertyName("patternList")]
    public string PatternList { get; set; } = "(1, 35%), (2, 35%), (3, 10%), (4, 10%), (6, 10%)";

    /// <summary>
    /// Auto level scaling for mobs (0 = use default from mob list)
    /// </summary>
    [JsonPropertyName("autoLevel")]
    public int AutoLevel { get; set; } = 0;

    /// <summary>
    /// Optional: Custom mob group override for specific waves
    /// </summary>
    [JsonPropertyName("mobGroupOverride")]
    public int MobGroupOverride { get; set; } = 0;

    /// <summary>
    /// Optional: Path to custom template for regular waves
    /// </summary>
    [JsonPropertyName("customTemplate")]
    public string? CustomTemplate { get; set; }
}

/// <summary>
/// Individual boss stage configuration (for per-boss customization)
/// </summary>
public class IndividualBossConfig
{
    /// <summary>
    /// Floor number for this specific boss
    /// </summary>
    [JsonPropertyName("floor")]
    public int Floor { get; set; }

    /// <summary>
    /// Boss mob group ID for this specific boss
    /// </summary>
    [JsonPropertyName("bossGroup")]
    public int BossGroup { get; set; }

    /// <summary>
    /// Mechanics template for this specific boss (overrides global template)
    /// </summary>
    [JsonPropertyName("mechanicsTemplate")]
    public string? MechanicsTemplate { get; set; }

    /// <summary>
    /// Arena world ID for this boss (overrides rotation)
    /// </summary>
    [JsonPropertyName("arena")]
    public string? Arena { get; set; }

    /// <summary>
    /// Reward item for this boss (overrides reward rotation)
    /// </summary>
    [JsonPropertyName("rewardItem")]
    public int? RewardItem { get; set; }

    /// <summary>
    /// Custom variables for this boss's template
    /// </summary>
    [JsonPropertyName("variables")]
    public Dictionary<string, string> Variables { get; set; } = new();

    /// <summary>
    /// Description/notes for this boss
    /// </summary>
    [JsonPropertyName("description")]
    public string Description { get; set; } = "";
}

/// <summary>
/// Boss configuration for boss floors
/// </summary>
public class BossConfig
{
    /// <summary>
    /// Base boss mob group ID (can be incremented for different bosses)
    /// </summary>
    [JsonPropertyName("bossGroupBase")]
    public int BossGroupBase { get; set; } = 9999;

    /// <summary>
    /// Default boss mechanics template path (used when no individual config)
    /// Options:
    /// - "templates/boss_phases_91_71_61_41_25_20.wps" (Multi-phase raid boss)
    /// - "templates/boss_simple_enrage.wps" (Simple enrage at 30%)
    /// - null/empty (No template, basic boss spawn)
    /// </summary>
    [JsonPropertyName("mechanicsTemplate")]
    public string? MechanicsTemplate { get; set; }

    /// <summary>
    /// Arena world IDs for boss rotation
    /// If empty, uses default CCBD arenas
    /// Example: ["ARENA_FIRE", "ARENA_ICE", "ARENA_LIGHTNING"]
    /// </summary>
    [JsonPropertyName("arenaRotation")]
    public List<string> ArenaRotation { get; set; } = new();

    /// <summary>
    /// Reward item table IDs (cycles through for different boss tiers)
    /// Example: [7000002, 7000003, 7000004] for increasing quality rewards
    /// </summary>
    [JsonPropertyName("rewardItems")]
    public List<int> RewardItems { get; set; } = new() { 7000002 };

    /// <summary>
    /// Whether to increment boss group ID for each boss stage
    /// If true: Boss 1 uses bossGroupBase, Boss 2 uses bossGroupBase+1, etc.
    /// </summary>
    [JsonPropertyName("incrementBossGroup")]
    public bool IncrementBossGroup { get; set; } = false;

    /// <summary>
    /// Individual boss configurations (per-boss customization)
    /// Allows mixing different mechanics, arenas, and rewards per boss
    /// If specified, overrides global settings for those floors
    /// </summary>
    [JsonPropertyName("individualBosses")]
    public List<IndividualBossConfig> IndividualBosses { get; set; } = new();
}

/// <summary>
/// Template variable configuration
/// </summary>
public class VariableConfig
{
    /// <summary>
    /// Custom variables for template substitution
    /// Key: Variable name (e.g., "INVINCIBLE_BUFF")
    /// Value: Variable value (e.g., "1900101")
    /// </summary>
    [JsonPropertyName("variables")]
    public Dictionary<string, string> Variables { get; set; } = new();

    /// <summary>
    /// Optional: Path to external variables file (.ini or .json)
    /// </summary>
    [JsonPropertyName("varsFilePath")]
    public string? VarsFilePath { get; set; }
}

/// <summary>
/// Complete dungeon profile configuration
/// </summary>
public class DungeonProfile
{
    /// <summary>
    /// Dungeon profile name
    /// </summary>
    [JsonPropertyName("name")]
    public string Name { get; set; } = "Custom Dungeon";

    /// <summary>
    /// Description of the dungeon
    /// </summary>
    [JsonPropertyName("description")]
    public string Description { get; set; } = "";

    /// <summary>
    /// WPS ID for this dungeon (must be unique across all server instances)
    /// Example: 83001, 83002, 83003, etc.
    /// IMPORTANT: Each dungeon MUST have unique WPS ID
    /// </summary>
    [JsonPropertyName("wpsId")]
    public int WpsId { get; set; } = 0;

    /// <summary>
    /// Base WPS file to extend (typically 83000.wps for CCBD)
    /// </summary>
    [JsonPropertyName("baseWpsFile")]
    public string BaseWpsFile { get; set; } = "";

    /// <summary>
    /// Total number of floors to generate
    /// </summary>
    [JsonPropertyName("floorCount")]
    public int FloorCount { get; set; } = 50;

    /// <summary>
    /// Boss appears every N floors (5 = boss at floors 5, 10, 15, 20...)
    /// </summary>
    [JsonPropertyName("bossInterval")]
    public int BossInterval { get; set; } = 5;

    /// <summary>
    /// Starting floor number (0 = auto-detect from base WPS file)
    /// </summary>
    [JsonPropertyName("startFloor")]
    public int StartFloor { get; set; } = 0;

    /// <summary>
    /// Wave configuration for regular floors
    /// </summary>
    [JsonPropertyName("waveConfig")]
    public WaveConfig WaveConfig { get; set; } = new();

    /// <summary>
    /// Boss configuration for boss floors
    /// </summary>
    [JsonPropertyName("bossConfig")]
    public BossConfig BossConfig { get; set; } = new();

    /// <summary>
    /// Template variables
    /// </summary>
    [JsonPropertyName("variables")]
    public VariableConfig Variables { get; set; } = new();

    /// <summary>
    /// Convert this profile to command-line arguments for WpsStageGen
    /// </summary>
    public string ToCommandLineArgs()
    {
        var args = new List<string>();

        // Required parameters
        args.Add($"in=\"{BaseWpsFile}\"");

        // Basic dungeon parameters
        args.Add($"add={FloorCount}");
        args.Add($"bossEvery={BossInterval}");

        if (StartFloor > 0)
            args.Add($"start={StartFloor}");

        // Wave configuration
        if (!string.IsNullOrWhiteSpace(WaveConfig.PatternList))
            args.Add($"pattern=\"{WaveConfig.PatternList}\"");

        if (!string.IsNullOrWhiteSpace(WaveConfig.CustomTemplate))
            args.Add($"regularTemplate=\"{WaveConfig.CustomTemplate}\"");

        // Boss configuration
        args.Add($"bossGroup={BossConfig.BossGroupBase}");

        if (!string.IsNullOrWhiteSpace(BossConfig.MechanicsTemplate))
            args.Add($"bossTemplate=\"{BossConfig.MechanicsTemplate}\"");

        if (BossConfig.ArenaRotation.Count > 0)
            args.Add($"bossWorlds=\"{string.Join(",", BossConfig.ArenaRotation)}\"");

        if (BossConfig.RewardItems.Count > 0)
            args.Add($"rewardItem={BossConfig.RewardItems[0]}");

        // Variables
        if (!string.IsNullOrWhiteSpace(Variables.VarsFilePath))
            args.Add($"varsFile=\"{Variables.VarsFilePath}\"");

        foreach (var kv in Variables.Variables)
            args.Add($"var.{kv.Key}={kv.Value}");

        return string.Join(" ", args);
    }

    /// <summary>
    /// Save profile to JSON file
    /// </summary>
    public void SaveToFile(string path)
    {
        var options = new JsonSerializerOptions
        {
            WriteIndented = true,
            PropertyNamingPolicy = JsonNamingPolicy.CamelCase
        };

        string json = JsonSerializer.Serialize(this, options);
        File.WriteAllText(path, json);
    }

    /// <summary>
    /// Load profile from JSON file
    /// </summary>
    public static DungeonProfile LoadFromFile(string path)
    {
        if (!File.Exists(path))
            throw new FileNotFoundException($"Profile file not found: {path}");

        string json = File.ReadAllText(path);
        var options = new JsonSerializerOptions
        {
            PropertyNameCaseInsensitive = true,
            AllowTrailingCommas = true
        };

        var profile = JsonSerializer.Deserialize<DungeonProfile>(json, options);
        if (profile == null)
            throw new InvalidOperationException("Failed to deserialize profile");

        return profile;
    }

    /// <summary>
    /// Create a default profile for quick start
    /// </summary>
    public static DungeonProfile CreateDefault(string baseWpsFile = "")
    {
        return new DungeonProfile
        {
            Name = "Default CCBD Extension",
            Description = "50 additional floors with bosses every 5 floors",
            BaseWpsFile = baseWpsFile,
            FloorCount = 50,
            BossInterval = 5,
            StartFloor = 0,
            WaveConfig = new WaveConfig
            {
                PatternList = "(1, 35%), (2, 35%), (3, 10%), (4, 10%), (6, 10%)",
                AutoLevel = 0
            },
            BossConfig = new BossConfig
            {
                BossGroupBase = 9999,
                MechanicsTemplate = null,
                ArenaRotation = new List<string>(),
                RewardItems = new List<int> { 7000002 },
                IncrementBossGroup = false
            },
            Variables = new VariableConfig
            {
                Variables = new Dictionary<string, string>(),
                VarsFilePath = null
            }
        };
    }

    /// <summary>
    /// Create a sample hard mode profile with advanced mechanics
    /// </summary>
    public static DungeonProfile CreateHardModeTemplate(string baseWpsFile = "")
    {
        return new DungeonProfile
        {
            Name = "Extreme Challenge Tower",
            Description = "100 floors with bosses every 10 floors. Multi-phase boss mechanics with add spawns.",
            BaseWpsFile = baseWpsFile,
            FloorCount = 100,
            BossInterval = 10,
            StartFloor = 0,
            WaveConfig = new WaveConfig
            {
                PatternList = "(1, 30%), (2, 30%), (3, 20%), (4, 20%)",
                AutoLevel = 55
            },
            BossConfig = new BossConfig
            {
                BossGroupBase = 9000,
                MechanicsTemplate = "templates/boss_phases_91_71_61_41_25_20.wps",
                ArenaRotation = new List<string> { "ARENA_FIRE", "ARENA_ICE", "ARENA_LIGHTNING" },
                RewardItems = new List<int> { 7000002, 7000003, 7000004 },
                IncrementBossGroup = true
            },
            Variables = new VariableConfig
            {
                Variables = new Dictionary<string, string>
                {
                    ["INVINCIBLE_BUFF"] = "1900101",
                    ["PHASE91_GROUP"] = "301",
                    ["PHASE71_GROUP"] = "302",
                    ["PHASE61_GROUP"] = "303",
                    ["PHASE41_GROUP"] = "304",
                    ["PHASE25_GROUP"] = "305",
                    ["PHASE20_GROUP"] = "306",
                    ["PROTECT_PLATFORM_GROUP"] = "350",
                    ["PROTECT_ADDONS_GROUP"] = "351"
                },
                VarsFilePath = null
            }
        };
    }

    /// <summary>
    /// Get output WPS file path based on WPS ID
    /// </summary>
    public string GetOutputWpsPath(string wpsDirectory = "")
    {
        if (WpsId == 0)
            throw new InvalidOperationException("WPS ID must be set before generating output path");

        string filename = $"{WpsId}.wps";
        return string.IsNullOrWhiteSpace(wpsDirectory)
            ? filename
            : Path.Combine(wpsDirectory, filename);
    }

    /// <summary>
    /// Validate WPS ID is unique in directory
    /// </summary>
    public static bool IsWpsIdAvailable(int wpsId, string wpsDirectory)
    {
        if (wpsId < 83001) // Reserve 83000 for original CCBD
            return false;

        string path = Path.Combine(wpsDirectory, $"{wpsId}.wps");
        return !File.Exists(path);
    }

    /// <summary>
    /// Find next available WPS ID in directory
    /// </summary>
    public static int FindNextAvailableWpsId(string wpsDirectory, int startFrom = 83001)
    {
        int wpsId = startFrom;
        while (!IsWpsIdAvailable(wpsId, wpsDirectory) && wpsId < 99999)
        {
            wpsId++;
        }
        return wpsId;
    }

    /// <summary>
    /// Get all existing WPS IDs in directory
    /// </summary>
    public static List<int> GetExistingWpsIds(string wpsDirectory)
    {
        var wpsIds = new List<int>();
        if (!Directory.Exists(wpsDirectory))
            return wpsIds;

        foreach (var file in Directory.GetFiles(wpsDirectory, "*.wps"))
        {
            string filename = Path.GetFileNameWithoutExtension(file);
            if (int.TryParse(filename, out int wpsId))
            {
                wpsIds.Add(wpsId);
            }
        }

        return wpsIds.OrderBy(x => x).ToList();
    }

    /// <summary>
    /// Get individual boss configuration for a specific floor, or null if using default
    /// </summary>
    public IndividualBossConfig? GetBossConfigForFloor(int floor)
    {
        return BossConfig.IndividualBosses.FirstOrDefault(b => b.Floor == floor);
    }

    /// <summary>
    /// Add or update individual boss configuration
    /// </summary>
    public void SetBossConfigForFloor(int floor, IndividualBossConfig bossConfig)
    {
        var existing = GetBossConfigForFloor(floor);
        if (existing != null)
        {
            BossConfig.IndividualBosses.Remove(existing);
        }
        bossConfig.Floor = floor;
        BossConfig.IndividualBosses.Add(bossConfig);
        BossConfig.IndividualBosses = BossConfig.IndividualBosses.OrderBy(b => b.Floor).ToList();
    }

    /// <summary>
    /// Remove individual boss configuration for a floor
    /// </summary>
    public void RemoveBossConfigForFloor(int floor)
    {
        var existing = GetBossConfigForFloor(floor);
        if (existing != null)
        {
            BossConfig.IndividualBosses.Remove(existing);
        }
    }

    /// <summary>
    /// Get all boss floors based on boss interval
    /// </summary>
    public List<int> GetBossFloors()
    {
        var bossFloors = new List<int>();
        int startStage = StartFloor > 0 ? StartFloor : 1;

        for (int i = 0; i < FloorCount; i++)
        {
            int floor = startStage + i;
            if (floor % BossInterval == 0)
            {
                bossFloors.Add(floor);
            }
        }

        return bossFloors;
    }

    /// <summary>
    /// Create a profile with mixed boss mechanics (example template)
    /// </summary>
    public static DungeonProfile CreateMixedMechanicsTemplate(string baseWpsFile = "", int wpsId = 83001)
    {
        var profile = new DungeonProfile
        {
            Name = "Mixed Mechanics Gauntlet",
            Description = "50 floors with different boss mechanics every 5 floors. Mix of simple, enrage, and multi-phase bosses.",
            WpsId = wpsId,
            BaseWpsFile = baseWpsFile,
            FloorCount = 50,
            BossInterval = 5,
            BossConfig = new BossConfig
            {
                BossGroupBase = 9100,
                IncrementBossGroup = true,
                // Default mechanics (used for bosses without individual config)
                MechanicsTemplate = null,
                RewardItems = new List<int> { 7000002, 7000003, 7000004 }
            }
        };

        // Floor 5: Simple boss (no mechanics)
        profile.SetBossConfigForFloor(5, new IndividualBossConfig
        {
            Floor = 5,
            BossGroup = 9100,
            MechanicsTemplate = null,
            Arena = null,
            RewardItem = 7000002,
            Description = "Warm-up boss - no special mechanics"
        });

        // Floor 10: Enrage boss
        profile.SetBossConfigForFloor(10, new IndividualBossConfig
        {
            Floor = 10,
            BossGroup = 9101,
            MechanicsTemplate = "templates/boss_simple_enrage.wps",
            Arena = "ARENA_FIRE",
            RewardItem = 7000002,
            Variables = new Dictionary<string, string>
            {
                ["ENRAGE_BUFF"] = "1900555",
                ["ENRAGE_HP_THRESHOLD"] = "30"
            },
            Description = "Fire boss with enrage at 30% HP"
        });

        // Floor 15: Multi-phase boss
        profile.SetBossConfigForFloor(15, new IndividualBossConfig
        {
            Floor = 15,
            BossGroup = 9102,
            MechanicsTemplate = "templates/boss_phases_91_71_61_41_25_20.wps",
            Arena = "ARENA_ICE",
            RewardItem = 7000003,
            Variables = new Dictionary<string, string>
            {
                ["INVINCIBLE_BUFF"] = "1900101",
                ["PHASE91_GROUP"] = "311",
                ["PHASE71_GROUP"] = "312",
                ["PHASE61_GROUP"] = "313",
                ["PHASE41_GROUP"] = "314",
                ["PHASE25_GROUP"] = "315",
                ["PHASE20_GROUP"] = "316"
            },
            Description = "Ice boss with 6-phase mechanics and adds"
        });

        // Additional bosses at 20, 25, 30... can be configured similarly
        // If not configured, they'll use default settings from BossConfig

        return profile;
    }

    /// <summary>
    /// Validate profile configuration
    /// </summary>
    public List<string> Validate()
    {
        var errors = new List<string>();

        if (WpsId == 0)
            errors.Add("WPS ID must be set (must be unique, e.g., 83001, 83002)");

        if (WpsId > 0 && WpsId < 83001)
            errors.Add("WPS ID should be 83001 or higher (83000 is reserved for original CCBD)");

        if (string.IsNullOrWhiteSpace(BaseWpsFile))
            errors.Add("Base WPS file must be specified");

        if (!string.IsNullOrWhiteSpace(BaseWpsFile) && !File.Exists(BaseWpsFile))
            errors.Add($"Base WPS file not found: {BaseWpsFile}");

        if (FloorCount < 1 || FloorCount > 255)
            errors.Add("Floor count must be between 1 and 255");

        if (BossInterval < 1)
            errors.Add("Boss interval must be at least 1");

        // Validate individual boss configurations
        foreach (var boss in BossConfig.IndividualBosses)
        {
            if (boss.Floor % BossInterval != 0)
                errors.Add($"Individual boss at floor {boss.Floor} is not a boss floor (boss interval is {BossInterval})");

            if (!string.IsNullOrWhiteSpace(boss.MechanicsTemplate) && !File.Exists(boss.MechanicsTemplate))
                errors.Add($"Boss mechanics template not found for floor {boss.Floor}: {boss.MechanicsTemplate}");
        }

        return errors;
    }
}
