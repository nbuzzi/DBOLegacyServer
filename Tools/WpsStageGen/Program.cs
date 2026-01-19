using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.Json;
using System.Text.RegularExpressions;

namespace WpsStageGen;

internal record BossStageOverride(
    int Stage,
    int? BossGroup,
    string? BossTemplatePath,
    int? RewardItem,
    int? ArenaSlot,
    IReadOnlyDictionary<string, string> Variables
);

internal record ProfileOverrides(
    bool IncrementBossGroup,
    IReadOnlyList<int> RewardCycle,
    IReadOnlyDictionary<int, BossStageOverride> BossStages
);

internal record GenOptions(
    string InputPath,
    string? OutputPath,
    int AddCount,
    int BossEvery,
    int StartStageOverride,
    int BossGroup,
    int DefaultRewardItem,
    string PatternList,
    string? BossTemplatePath,
    string? RegularTemplatePath,
    IReadOnlyDictionary<string, string> CustomVars,
    ProfileOverrides? ProfileOverrides,
    string[] BossWorldAliases
);

internal record GenerationSummary(string OutputPath, int StartStage, int EndStage, int AddedBossCount);

internal static class StageGenerator
{
    private static readonly Regex StageHeader = new(
        "Action\\(\\s*\\\"CCBD stage\\\"\\s*\\)\\s*--\\[\\s*Param\\(\\s*\\\"stage\\\"\\s*,\\s*(\\d+)\\s*\\)",
        RegexOptions.Compiled | RegexOptions.IgnoreCase | RegexOptions.Singleline);

    private static readonly Regex RewardLastStageTrue = new(
        "Param\\(\\s*\\\"last stage\\\"\\s*,\\s*\\\"true\\\"\\s*\\)",
        RegexOptions.Compiled | RegexOptions.IgnoreCase);

    private static readonly Dictionary<string, int> ArenaAliasLookup = new(StringComparer.OrdinalIgnoreCase)
    {
        ["ARENA_A"] = 0,
        ["ARENA_FIRE"] = 0,
        ["ARENA_RED"] = 0,
        ["ARENA_B"] = 1,
        ["ARENA_ICE"] = 1,
        ["ARENA_BLUE"] = 1,
        ["ARENA_C"] = 2,
        ["ARENA_LIGHTNING"] = 2,
        ["ARENA_YELLOW"] = 2,
        ["ARENA_D"] = 3,
        ["ARENA_FOREST"] = 3,
        ["ARENA_GREEN"] = 3
    };

    public static GenerationSummary Generate(GenOptions opts)
    {
        if (opts.AddCount <= 0)
            throw new ArgumentOutOfRangeException(nameof(opts.AddCount), "Add count must be at least 1.");
        if (opts.BossEvery <= 0)
            throw new ArgumentOutOfRangeException(nameof(opts.BossEvery), "bossEvery must be >= 1.");
        if (!File.Exists(opts.InputPath))
            throw new FileNotFoundException($"Input WPS file not found: {opts.InputPath}");

        string original = File.ReadAllText(opts.InputPath, Encoding.UTF8);
        var existingStages = StageHeader.Matches(original)
            .Select(m => int.Parse(m.Groups[1].Value))
            .OrderBy(x => x)
            .ToList();

        if (existingStages.Count == 0)
            throw new InvalidOperationException("No CCBD stages found in the source WPS file.");

        int maxExistingStage = existingStages.Max();
        int startStage = opts.StartStageOverride > 0 ? opts.StartStageOverride : maxExistingStage + 1;
        int finalStage = startStage + opts.AddCount - 1;

        var normalized = RewardLastStageTrue.Replace(original, _ => "Param( \"last stage\", \"false\" )");
        var builder = new StringBuilder(normalized);
        if (!normalized.EndsWith('\n'))
            builder.AppendLine();

        int? lastBossStage = null;
        for (int preview = startStage; preview <= finalStage; preview++)
        {
            if (preview % opts.BossEvery == 0)
                lastBossStage = preview;
        }

        int appendedBossCount = 0;
        string inputDir = Path.GetDirectoryName(Path.GetFullPath(opts.InputPath)) ?? ".";

        for (int offset = 0; offset < opts.AddCount; offset++)
        {
            int stage = startStage + offset;
            bool isBossStage = stage % opts.BossEvery == 0;

            BossStageOverride? bossOverride = null;
            if (opts.ProfileOverrides?.BossStages != null &&
                opts.ProfileOverrides.BossStages.TryGetValue(stage, out var overrideData))
            {
                bossOverride = overrideData;
                isBossStage = isBossStage || bossOverride.BossTemplatePath != null || bossOverride.BossGroup.HasValue;
            }

            int? arenaSlotOverride = bossOverride?.ArenaSlot;
            string? arenaAlias = null;
            int nthBoss = 0;

            if (isBossStage)
            {
                appendedBossCount++;
                nthBoss = appendedBossCount;

                if (arenaSlotOverride is null && opts.BossWorldAliases.Length > 0)
                {
                    arenaAlias = opts.BossWorldAliases[(nthBoss - 1) % opts.BossWorldAliases.Length];
                    arenaSlotOverride = TryParseArenaSlot(arenaAlias ?? string.Empty);
                }
            }

            bool markAsLastStage = lastBossStage.HasValue && stage == lastBossStage.Value;

            var replacements = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
            {
                ["STAGE"] = stage.ToString(),
                ["IS_BOSS"] = isBossStage ? "true" : "false",
                ["MARK_LAST_STAGE"] = markAsLastStage ? "true" : "false",
                ["BOSS_EVERY"] = opts.BossEvery.ToString(),
                ["START_STAGE"] = startStage.ToString(),
                ["END_STAGE"] = finalStage.ToString(),
                ["ARENA_SLOT"] = arenaSlotOverride?.ToString() ?? string.Empty,
                ["ARENA_ALIAS"] = arenaAlias ?? string.Empty
            };

            foreach (var kv in opts.CustomVars)
                replacements[kv.Key] = kv.Value;

            string? resolvedBossTemplate = bossOverride?.BossTemplatePath ?? opts.BossTemplatePath;
            if (!string.IsNullOrWhiteSpace(resolvedBossTemplate))
                resolvedBossTemplate = ResolveTemplatePath(resolvedBossTemplate!, inputDir);

            string? resolvedRegularTemplate = opts.RegularTemplatePath;
            if (!string.IsNullOrWhiteSpace(resolvedRegularTemplate))
                resolvedRegularTemplate = ResolveTemplatePath(resolvedRegularTemplate!, inputDir);

            int mobGroup = opts.BossGroup;
            int rewardItem = opts.DefaultRewardItem;

            if (isBossStage)
            {
                if (opts.ProfileOverrides?.IncrementBossGroup == true && nthBoss > 0)
                    mobGroup += (nthBoss - 1);

                if (opts.ProfileOverrides?.RewardCycle is { Count: > 0 } cycle && nthBoss > 0)
                    rewardItem = cycle[(nthBoss - 1) % cycle.Count];

                if (bossOverride?.BossGroup is { } groupOverride && groupOverride > 0)
                    mobGroup = groupOverride;

                if (bossOverride?.RewardItem is { } rewardOverride && rewardOverride > 0)
                    rewardItem = rewardOverride;

                replacements["BOSS_GROUP"] = mobGroup.ToString();
                replacements["REWARD_ITEM"] = rewardItem.ToString();

                if (bossOverride?.Variables != null)
                {
                    foreach (var kv in bossOverride.Variables)
                        replacements[kv.Key] = kv.Value;
                }
            }
            else
            {
                replacements["BOSS_GROUP"] = mobGroup.ToString();
                replacements["REWARD_ITEM"] = rewardItem.ToString();
            }

            builder.AppendLine();
            builder.AppendLine("-----------------------------------");
            builder.AppendLine($"-- Stage {stage}");
            builder.AppendLine("-----------------------------------");
            builder.AppendLine("Action( \"CCBD stage\" )");
            builder.AppendLine("--[");
            builder.AppendLine($"    Param( \"stage\", {stage} )");

            if (arenaSlotOverride is not null)
                builder.AppendLine($"    Param( \"boss arena slot\", {arenaSlotOverride.Value} )");

            if (!isBossStage)
            {
                builder.AppendLine();
                builder.AppendLine("    Action( \"CCBD exec pattern\" )");
                builder.AppendLine("    --[");
                builder.AppendLine($"        Param( \"pattern list\", \"{opts.PatternList}\" )");
                builder.AppendLine("    --]");
                builder.AppendLine("    End()");

                if (!string.IsNullOrWhiteSpace(resolvedRegularTemplate) && File.Exists(resolvedRegularTemplate))
                    InjectTemplate(builder, resolvedRegularTemplate!, 4, replacements);
            }
            else
            {
                builder.AppendLine("    Param( \"direct play\", \"false\" )");
                if (!string.IsNullOrWhiteSpace(arenaAlias))
                    builder.AppendLine($"    -- Boss arena alias: {arenaAlias}");

                builder.AppendLine();
                builder.AppendLine("    Action( \"add mobgroup\" )");
                builder.AppendLine("    --[");
                builder.AppendLine($"        Param( \"group\", {mobGroup} )");
                builder.AppendLine("        Param( \"no spawn wait\", \"true\" )");
                builder.AppendLine("    --]");
                builder.AppendLine("    End()");

                if (!string.IsNullOrWhiteSpace(resolvedBossTemplate) && File.Exists(resolvedBossTemplate))
                    InjectTemplate(builder, resolvedBossTemplate!, 4, replacements);

                builder.AppendLine("    Action( \"CCBD stage clear\" )");
                builder.AppendLine("    --[");
                builder.AppendLine("        -- Tell the client that the stage has ended.");
                builder.AppendLine("    --]");
                builder.AppendLine("    End()");

                builder.AppendLine("    Action( \"wait\" )");
                builder.AppendLine("    --[");
                builder.AppendLine("        Condition( \"check time\" )");
                builder.AppendLine("        --[");
                builder.AppendLine("            Param( \"time\", 10 )");
                builder.AppendLine("        --]");
                builder.AppendLine("        End()");
                builder.AppendLine("    --]");
                builder.AppendLine("    End()");

                builder.AppendLine("    Action( \"CCBD reward\" )");
                builder.AppendLine("    --[");
                builder.AppendLine($"        Param( \"item tblidx\", {rewardItem} )");
                if (markAsLastStage)
                    builder.AppendLine("        Param( \"last stage\", \"true\" )");
                builder.AppendLine("    --]");
                builder.AppendLine("    End()");
            }

            builder.AppendLine("--]");
            builder.AppendLine("End()");
            builder.AppendLine("\t--- end Action( \"CCBD stage\" )");
        }

        string outputPath = string.IsNullOrWhiteSpace(opts.OutputPath) ? opts.InputPath : opts.OutputPath;
        Directory.CreateDirectory(Path.GetDirectoryName(Path.GetFullPath(outputPath)) ?? ".");
        File.WriteAllText(outputPath, builder.ToString(), Encoding.UTF8);

        return new GenerationSummary(outputPath, startStage, finalStage, appendedBossCount);
    }

    private static string ReplacePlaceholders(string text, IReadOnlyDictionary<string, string> replacements)
    {
        string result = text;
        foreach (var kv in replacements)
            result = result.Replace($"{{{{{kv.Key}}}}}", kv.Value);
        return result;
    }

    private static void InjectTemplate(StringBuilder builder, string templatePath, int indentSpaces, IReadOnlyDictionary<string, string> replacements)
    {
        try
        {
            var raw = File.ReadAllText(templatePath, Encoding.UTF8);
            string indent = new(' ', indentSpaces);
            foreach (var line in raw.Replace("\r\n", "\n").Split('\n'))
            {
                var substituted = ReplacePlaceholders(line, replacements);
                if (string.IsNullOrWhiteSpace(substituted))
                    builder.AppendLine();
                else
                    builder.AppendLine(indent + substituted);
            }

            if (!raw.EndsWith("\n"))
                builder.AppendLine();
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine($"Warning: Failed to inject template '{templatePath}': {ex.Message}");
        }
    }

    private static string? ResolveTemplatePath(string path, string inputDirectory)
    {
        if (string.IsNullOrWhiteSpace(path))
            return null;

        if (File.Exists(path))
            return Path.GetFullPath(path);

        string candidate = Path.Combine(inputDirectory, path);
        if (File.Exists(candidate))
            return Path.GetFullPath(candidate);

        candidate = Path.Combine(AppContext.BaseDirectory, path);
        if (File.Exists(candidate))
            return Path.GetFullPath(candidate);

        return path;
    }

    internal static int? TryParseArenaSlot(string alias)
    {
        if (string.IsNullOrWhiteSpace(alias))
            return null;

        if (int.TryParse(alias, out var direct))
            return direct;

        var digitsOnly = new string(alias.Where(char.IsDigit).ToArray());
        if (!string.IsNullOrWhiteSpace(digitsOnly) && int.TryParse(digitsOnly, out var fromDigits))
            return fromDigits;

        if (ArenaAliasLookup.TryGetValue(alias.Trim(), out var mapped))
            return mapped;

        return null;
    }
}

internal class Program
{
    public static int Main(string[] args)
    {
        try
        {
            var options = ParseArgs(args);
            if (options is null)
            {
                PrintUsage();
                return 2;
            }

            var summary = StageGenerator.Generate(options);
            Console.WriteLine($"Wrote changes to: {summary.OutputPath}");
            Console.WriteLine($"Added {options.AddCount} stages starting at {summary.StartStage}. Final stage: {summary.EndStage}. Boss every {options.BossEvery}.");
            return 0;
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine(ex.ToString());
            return 1;
        }
    }

    private static GenOptions? ParseArgs(string[] args)
    {
        string? input = null;
        string? output = null;
        int addCount = 5;
        int bossEvery = 5;
        int startStage = 0;
        int bossGroup = 9999;
        int rewardItem = 7000002;
        string patternList = "(1, 35%), (2, 35%), (3, 10%), (4, 10%), (6, 10%)";
        string? bossTemplate = null;
        string? regularTemplate = null;
        string[] bossWorlds = Array.Empty<string>();
        var customVars = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        string? varsFile = null;
        string? profilePath = null;

        foreach (var arg in args)
        {
            var split = arg.Split('=', 2);
            if (split.Length != 2)
                continue;

            string key = split[0].Trim();
            string value = split[1].Trim();

            switch (key.ToLowerInvariant())
            {
                case "in":
                    input = value.Trim('"');
                    break;
                case "out":
                    output = value.Trim('"');
                    break;
                case "add":
                    addCount = int.Parse(value);
                    break;
                case "bossevery":
                    bossEvery = int.Parse(value);
                    break;
                case "start":
                    startStage = int.Parse(value);
                    break;
                case "bossgroup":
                    bossGroup = int.Parse(value);
                    break;
                case "rewarditem":
                    rewardItem = int.Parse(value);
                    break;
                case "pattern":
                    patternList = value.Trim('"');
                    break;
                case "bossworlds":
                    bossWorlds = value.Split(',', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);
                    break;
                case "bosstemplate":
                    bossTemplate = value.Trim('"');
                    break;
                case "regulartemplate":
                    regularTemplate = value.Trim('"');
                    break;
                case "varsfile":
                    varsFile = value.Trim('"');
                    break;
                case "profile":
                    profilePath = value.Trim('"');
                    break;
                default:
                    if (key.StartsWith("var.", StringComparison.OrdinalIgnoreCase))
                    {
                        string varName = key.Substring(4);
                        if (!string.IsNullOrWhiteSpace(varName))
                            customVars[varName] = value.Trim('"');
                    }
                    break;
            }
        }

        if (profilePath != null)
            return ParseProfile(profilePath, input, output, customVars, varsFile);

        if (string.IsNullOrWhiteSpace(input) || !File.Exists(input))
            return null;

        MergeVarsFromFile(customVars, varsFile);

        return new GenOptions(
            Path.GetFullPath(input),
            output,
            addCount,
            bossEvery,
            startStage,
            bossGroup,
            rewardItem,
            patternList,
            bossTemplate,
            regularTemplate,
            customVars,
            null,
            bossWorlds
        );
    }

    private static GenOptions? ParseProfile(string profilePath, string? cliInput, string? cliOutput, IDictionary<string, string> cliVars, string? cliVarsFile)
    {
        if (!File.Exists(profilePath))
        {
            Console.Error.WriteLine($"Profile JSON not found: {profilePath}");
            return null;
        }

        DungeonProfile profile;
        try
        {
            profile = DungeonProfile.LoadFromFile(profilePath);
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine($"Failed to load profile JSON: {ex.Message}");
            return null;
        }

        string profileDir = Path.GetDirectoryName(Path.GetFullPath(profilePath)) ?? ".";

        string? ResolvePath(string? candidate)
        {
            if (string.IsNullOrWhiteSpace(candidate))
                return candidate;

            if (Path.IsPathRooted(candidate))
                return candidate;

            return Path.Combine(profileDir, candidate);
        }

        string baseWps = !string.IsNullOrWhiteSpace(cliInput) ? cliInput : profile.BaseWpsFile;
        if (string.IsNullOrWhiteSpace(baseWps))
        {
            Console.Error.WriteLine("Profile missing baseWpsFile; supply via profile or in=PATH.");
            return null;
        }

        baseWps = Path.GetFullPath(ResolvePath(baseWps)!);
        if (!File.Exists(baseWps))
        {
            Console.Error.WriteLine($"Base WPS file not found: {baseWps}");
            return null;
        }

        var mergedVars = new Dictionary<string, string>(profile.Variables.Variables, StringComparer.OrdinalIgnoreCase);
        foreach (var kv in cliVars)
            mergedVars[kv.Key] = kv.Value;

        MergeVarsFromFile(mergedVars, ResolvePath(profile.Variables.VarsFilePath));
        MergeVarsFromFile(mergedVars, cliVarsFile);

        var bossOverrides = profile.BossConfig.IndividualBosses
            .Where(entry => entry is { Floor: > 0 })
            .GroupBy(entry => entry.Floor)
            .ToDictionary(
                group => group.Key,
                group =>
                {
                    var source = group.Last();
                    return new BossStageOverride(
                        source.Floor,
                        source.BossGroup > 0 ? source.BossGroup : null,
                        ResolvePath(source.MechanicsTemplate),
                        source.RewardItem,
                        StageGenerator.TryParseArenaSlot(source.Arena ?? string.Empty),
                        new Dictionary<string, string>(source.Variables ?? new Dictionary<string, string>(), StringComparer.OrdinalIgnoreCase)
                    );
                },
                comparer: EqualityComparer<int>.Default);

        var rewardCycle = profile.BossConfig.RewardItems.Count > 0
            ? profile.BossConfig.RewardItems
            : new List<int> { 7000002 };

        var overrides = new ProfileOverrides(
            profile.BossConfig.IncrementBossGroup,
            rewardCycle,
            bossOverrides
        );

        int defaultReward = rewardCycle.Count > 0 ? rewardCycle[0] : 7000002;

        return new GenOptions(
            baseWps,
            cliOutput,
            profile.FloorCount,
            profile.BossInterval,
            profile.StartFloor,
            profile.BossConfig.BossGroupBase,
            defaultReward,
            profile.WaveConfig.PatternList,
            ResolvePath(profile.BossConfig.MechanicsTemplate),
            ResolvePath(profile.WaveConfig.CustomTemplate),
            mergedVars,
            overrides,
            profile.BossConfig.ArenaRotation.ToArray()
        );
    }

    private static void MergeVarsFromFile(IDictionary<string, string> target, string? varsFile)
    {
        if (string.IsNullOrWhiteSpace(varsFile))
            return;

        try
        {
            if (varsFile.EndsWith(".json", StringComparison.OrdinalIgnoreCase))
            {
                var json = File.ReadAllText(varsFile);
                var dict = JsonSerializer.Deserialize<Dictionary<string, string>>(json);
                if (dict != null)
                {
                    foreach (var kv in dict)
                        target[kv.Key] = kv.Value;
                }
            }
            else
            {
                foreach (var raw in File.ReadAllLines(varsFile))
                {
                    var line = raw.Trim();
                    if (line.Length == 0 || line.StartsWith('#') || !line.Contains('='))
                        continue;

                    var parts = line.Split('=', 2);
                    var key = parts[0].Trim();
                    var value = parts[1].Trim();
                    if (key.Length > 0)
                        target[key] = value.Trim('"');
                }
            }
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine($"Warning: Failed to read vars file '{varsFile}': {ex.Message}");
        }
    }

    private static void PrintUsage()
    {
        Console.WriteLine("WpsStageGen - CCBD WPS Stage generator");
        Console.WriteLine("Usage: WpsStageGen in=PATH [out=PATH] [add=5] [bossEvery=5] [start=0] [bossGroup=9999] [rewardItem=7000002] [pattern=\"(1,35%),...\"] [bossWorlds=ARENA1,ARENA2,...] [bossTemplate=FILE] [regularTemplate=FILE] [varsFile=FILE] [var.NAME=VALUE ...] [profile=PROFILE.json]");
        Console.WriteLine();
        Console.WriteLine("Behavior:");
        Console.WriteLine("- Finds max existing stage, clears reward 'last stage' flags, appends N stages");
        Console.WriteLine("- Regular floors add CCBD exec pattern with provided pattern list and optional template");
        Console.WriteLine("- Boss floors spawn mob groups, optional mechanics template, and reward block");
        Console.WriteLine("- Last generated boss floor gains Param( 'last stage', 'true' ) in reward block");
        Console.WriteLine("- profile= enables JSON-driven configuration with per-boss overrides and arena slots.");
    }
}
