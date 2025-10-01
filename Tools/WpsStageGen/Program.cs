using System.Text;
using System.Text.RegularExpressions;

// WPS Stage Generator
// - Scans a .wps file for CCBD stages
// - Adds new stages with customizable intervals, boss stages, optional boss arena cycle, and rewards
// - Ensures only the final boss stage has CCBD reward "last stage" flag

internal record GenOptions(
    string InputPath,
    string? OutputPath,
    int AddCount,
    int BossEvery,
    int StartStageOverride,
    int BossGroup,
    int RewardItem,
    string PatternList,
    string[] BossWorldsCycle,
    string? BossTemplatePath,
    string? RegularTemplatePath,
    IReadOnlyDictionary<string,string> CustomVars
);

internal class Program
{
    private static readonly Regex StageHeader = new(
        "Action\\(\\s*\\\"CCBD stage\\\"\\s*\\)\\s*--\\[\\s*Param\\(\\s*\\\"stage\\\"\\s*,\\s*(\\d+)\\s*\\)",
        RegexOptions.Compiled | RegexOptions.IgnoreCase | RegexOptions.Singleline);

    // In 83000.wps the terminal flag lives under the CCBD reward block as: Param( "last stage", "true" )
    private static readonly Regex RewardLastStageTrue = new(
        "Param\\(\\s*\\\"last stage\\\"\\s*,\\s*\\\"true\\\"\\s*\\)",
        RegexOptions.Compiled | RegexOptions.IgnoreCase);

    private static readonly Regex AddMobGroup = new(
        "Action\\(\\s*\\\"add mobgroup\\\"\\s*\\)\\s*--\\[\\s*Param\\(\\s*\\\"group\\\"\\s*,\\s*(\\d+)\\s*\\)",
        RegexOptions.Compiled | RegexOptions.IgnoreCase | RegexOptions.Singleline);

    static int Main(string[] args)
    {
        try
        {
            var opts = ParseArgs(args);
            if (opts is null)
            {
                PrintUsage();
                return 2;
            }

            var text = File.ReadAllText(opts.InputPath, Encoding.UTF8);

            var stages = StageHeader.Matches(text)
                .Select(m => int.Parse(m.Groups[1].Value))
                .OrderBy(x => x)
                .ToList();

            if (stages.Count == 0)
            {
                Console.Error.WriteLine("No CCBD stages found in file.");
                return 1;
            }

            int maxStage = stages.Max();
            int startStage = opts.StartStageOverride > 0 ? opts.StartStageOverride : maxStage + 1;

            // Ensure any existing CCBD reward last stage flag is set to false, we'll set true on the new terminal boss stage
            text = RewardLastStageTrue.Replace(text, m => "Param( \"last stage\", \"false\" )");

            var sb = new StringBuilder(text);
            if (!text.EndsWith("\n")) sb.AppendLine();

            // Determine the last boss stage within the appended range (used to mark 'last stage')
            int finalStage = startStage + opts.AddCount - 1;
            int lastBossStageInRange = finalStage - ((finalStage % opts.BossEvery + opts.BossEvery) % opts.BossEvery);
            if (lastBossStageInRange < startStage)
            {
                // If no boss stage falls within the appended range, fall back to the nearest lower boss stage (may be previous content)
                lastBossStageInRange = finalStage - (finalStage % opts.BossEvery);
            }

            for (int i = 0; i < opts.AddCount; i++)
            {
                int stage = startStage + i;
                bool isBoss = (stage % opts.BossEvery) == 0;
                int mobGroup = opts.BossGroup;
                bool markAsLastStage = isBoss && stage == lastBossStageInRange;
                string? bossWorld = null;
                if (isBoss && opts.BossWorldsCycle.Length > 0)
                {
                    // Determine the first boss stage within the appended range
                    int firstBoss = startStage + ((opts.BossEvery - (startStage % opts.BossEvery)) % opts.BossEvery);
                    int nthBossAppended = ((stage - firstBoss) / opts.BossEvery) + 1; // 1-based within appended
                    if (nthBossAppended < 1) nthBossAppended = 1; // safety
                    bossWorld = opts.BossWorldsCycle[(nthBossAppended - 1) % opts.BossWorldsCycle.Length];
                }
                var replacements = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
                {
                    ["STAGE"] = stage.ToString(),
                    ["IS_BOSS"] = isBoss ? "true" : "false",
                    ["BOSS_GROUP"] = mobGroup.ToString(),
                    ["REWARD_ITEM"] = opts.RewardItem.ToString(),
                    ["ARENA_WORLD"] = bossWorld ?? string.Empty,
                    ["MARK_LAST_STAGE"] = markAsLastStage ? "true" : "false",
                    ["BOSS_EVERY"] = opts.BossEvery.ToString(),
                    ["START_STAGE"] = startStage.ToString(),
                    ["END_STAGE"] = finalStage.ToString()
                };
                // Merge custom vars (explicit overrides win)
                foreach (var kv in opts.CustomVars)
                    replacements[kv.Key] = kv.Value;

                sb.AppendLine();
                sb.AppendLine("-----------------------------------");
                sb.AppendLine($"-- Stage {stage}");
                sb.AppendLine("-----------------------------------");
                sb.AppendLine("Action( \"CCBD stage\" )");
                sb.AppendLine("--[");
                sb.AppendLine($"    Param( \"stage\", {stage} )");
                if (!isBoss)
                {
                    // Regular floor: use exec pattern referencing existing pattern indexes
                    sb.AppendLine("\n    Action( \"CCBD exec pattern\" )");
                    sb.AppendLine("    --[");
                    sb.AppendLine(ReplacePlaceholders($"        Param( \"pattern list\", \"{opts.PatternList}\" )", replacements));
                    sb.AppendLine("    --]");
                    sb.AppendLine("    End()");
                    // Optional regular template injection
                    if (!string.IsNullOrWhiteSpace(opts.RegularTemplatePath) && File.Exists(opts.RegularTemplatePath))
                    {
                        InjectTemplate(sb, opts.RegularTemplatePath!, 4, replacements);
                    }
                }
                else
                {
                    // Boss floor: mark direct play, spawn boss group, then stage clear + reward
                    sb.AppendLine("    Param( \"direct play\", \"false\" )");
                    if (!string.IsNullOrWhiteSpace(bossWorld))
                    {
                        sb.AppendLine(ReplacePlaceholders($"    -- Boss arena: {bossWorld}", replacements));
                    }

                    sb.AppendLine("\n    Action( \"add mobgroup\" )");
                    sb.AppendLine("    --[");
                    sb.AppendLine(ReplacePlaceholders($"        Param( \"group\", {mobGroup} )", replacements));
                    sb.AppendLine("        Param( \"no spawn wait\", \"true\" )");
                    sb.AppendLine("    --]");
                    sb.AppendLine("    End()\n");

                    // Optional: inject a custom boss mechanics template (raw WPS snippet)
                    if (!string.IsNullOrWhiteSpace(opts.BossTemplatePath) && File.Exists(opts.BossTemplatePath))
                    {
                        InjectTemplate(sb, opts.BossTemplatePath!, 4, replacements);
                    }

                    sb.AppendLine("    Action( \"CCBD stage clear\" )");
                    sb.AppendLine("    --[");
                    sb.AppendLine("        -- Tell the client that the stage has ended.");
                    sb.AppendLine("    --]");
                    sb.AppendLine("    End()\n");

                    sb.AppendLine("    Action( \"wait\" )");
                    sb.AppendLine("    --[");
                    sb.AppendLine("        Condition( \"check time\" )");
                    sb.AppendLine("        --[");
                    sb.AppendLine("            Param( \"time\", 10 )");
                    sb.AppendLine("        --]");
                    sb.AppendLine("        End()");
                    sb.AppendLine("    --]");
                    sb.AppendLine("    End()\n");

                    sb.AppendLine("    Action( \"CCBD reward\" )");
                    sb.AppendLine("    --[");
                    sb.AppendLine(ReplacePlaceholders($"        Param( \"item tblidx\", {opts.RewardItem} )", replacements));
                    if (markAsLastStage) sb.AppendLine(ReplacePlaceholders("        Param( \"last stage\", \"true\" )", replacements));
                    sb.AppendLine("    --]");
                    sb.AppendLine("    End()");
                }
                sb.AppendLine("--]");
                sb.AppendLine("End()");
                sb.AppendLine("\t--- end Action( \"CCBD stage\" )");
            }

            var outPath = string.IsNullOrWhiteSpace(opts.OutputPath) ? opts.InputPath : opts.OutputPath!;
            File.WriteAllText(outPath, sb.ToString(), Encoding.UTF8);

            Console.WriteLine($"Wrote changes to: {outPath}");
            Console.WriteLine($"Added {opts.AddCount} stages starting at {startStage}. Final stage: {startStage + opts.AddCount - 1}. Boss every {opts.BossEvery}. Reward item {opts.RewardItem}.");
            Console.WriteLine("Note: Previous CCBD reward 'last stage' flags were cleared; the last appended boss stage was marked as last stage.");
            return 0;
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine(ex.ToString());
            return 1;
        }
    }

    static GenOptions? ParseArgs(string[] args)
    {
        // Defaults suitable for CCBD extension
        string? input = null;
    int add = 5;            // how many stages to add
        int bossEvery = 5;      // every N stage is a boss
        int startStage = 0;     // 0 = infer (max+1)
        int bossGroup = 9999;   // boss group id to spawn
    int rewardItem = 7000002;// default CCBD reward item
        string patternList = "(1, 35%), (2, 35%), (3, 10%), (4, 10%), (6, 10%)"; // default pattern mix
        string[] bossWorlds = Array.Empty<string>();
    string? outPath = null;
    string? bossTemplate = null;
    string? regularTemplate = null;
    var customVars = new Dictionary<string,string>(StringComparer.OrdinalIgnoreCase);
    string? varsFile = null;

        foreach (var a in args)
        {
            var kv = a.Split('=', 2);
            if (kv.Length != 2) continue;
            string k = kv[0].Trim().ToLowerInvariant();
            string v = kv[1].Trim();
            switch (k)
            {
                case "in": input = v; break;
                case "out": outPath = v; break;
                case "add": add = int.Parse(v); break;
                case "bossevery": bossEvery = int.Parse(v); break;
                case "start": startStage = int.Parse(v); break;
                case "bossgroup": bossGroup = int.Parse(v); break;
                case "rewarditem": rewardItem = int.Parse(v); break;
                case "pattern": patternList = v.Trim('"'); break;
                case "bossworlds": bossWorlds = v.Split(',', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries); break;
                case "bosstemplate": bossTemplate = v; break;
                case "regulartemplate": regularTemplate = v; break;
                // Dynamic variables: var.NAME=VALUE
                default:
                    if (k.StartsWith("var.", StringComparison.OrdinalIgnoreCase))
                    {
                        var name = k.Substring(4);
                        if (!string.IsNullOrWhiteSpace(name)) customVars[name] = v.Trim('"');
                    }
                    else if (k == "varsfile")
                    {
                        varsFile = v;
                    }
                    break;
            }
        }

        if (string.IsNullOrWhiteSpace(input) || !File.Exists(input))
            return null;

        // Merge vars from file if provided (simple KEY=VALUE per line or JSON object)
        if (!string.IsNullOrWhiteSpace(varsFile) && File.Exists(varsFile))
        {
            try
            {
                if (varsFile.EndsWith(".json", StringComparison.OrdinalIgnoreCase))
                {
                    var json = File.ReadAllText(varsFile);
                    var dict = System.Text.Json.JsonSerializer.Deserialize<Dictionary<string, string>>(json);
                    if (dict != null)
                    {
                        foreach (var kv in dict) customVars[kv.Key] = kv.Value;
                    }
                }
                else
                {
                    foreach (var raw in File.ReadAllLines(varsFile))
                    {
                        var line = raw.Trim();
                        if (line.Length == 0 || line.StartsWith("#") || !line.Contains('=')) continue;
                        var sp = line.Split('=', 2);
                        var key = sp[0].Trim(); var val = sp[1].Trim();
                        if (key.Length > 0) customVars[key] = val.Trim('"');
                    }
                }
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine($"Warning: Failed to read varsFile '{varsFile}': {ex.Message}");
            }
        }

        return new GenOptions(input!, outPath, add, bossEvery, startStage, bossGroup, rewardItem, patternList, bossWorlds, bossTemplate, regularTemplate, customVars);
    }

    static void PrintUsage()
    {
        Console.WriteLine("WpsStageGen - CCBD WPS Stage generator");
    Console.WriteLine("Usage: WpsStageGen in=PATH [out=PATH] [add=5] [bossEvery=5] [start=0] [bossGroup=9999] [rewardItem=7000002] [pattern=\"(1,35%),...\"] [bossWorlds=ARENA1,ARENA2,...] [bossTemplate=FILE] [regularTemplate=FILE] [varsFile=FILE] [var.NAME=VALUE ...]");
        Console.WriteLine();
        Console.WriteLine("Behavior:");
        Console.WriteLine("- Finds max existing stage, clears any CCBD reward 'last stage' flag, appends N stages");
        Console.WriteLine("- Regular floors: adds CCBD exec pattern with provided pattern list");
        Console.WriteLine("- Boss floors: sets direct play, spawns boss mobgroup, optional injected template, then stage clear + wait + CCBD reward");
        Console.WriteLine("- The last appended boss floor gets CCBD reward Param( 'last stage', 'true' )");
        Console.WriteLine("- If bossWorlds provided, cycles them and annotates each boss with a comment '-- Boss arena: WORLD'");
        Console.WriteLine("- Templates support placeholders: {{STAGE}}, {{ARENA_WORLD}}, {{BOSS_GROUP}}, {{REWARD_ITEM}}, {{START_STAGE}}, {{END_STAGE}}, etc.");
    Console.WriteLine("- You can also provide custom placeholders with var.NAME=VALUE or a varsFile; any {{NAME}} in templates will be replaced.");
    }

    static void InjectTemplate(StringBuilder sb, string path, int indentSpaces, IReadOnlyDictionary<string, string> replacements)
    {
        try
        {
            var inject = File.ReadAllText(path, Encoding.UTF8);
            string indent = new string(' ', indentSpaces);
            foreach (var raw in inject.Replace("\r\n", "\n").Split('\n'))
            {
                var line = ReplacePlaceholders(raw, replacements);
                if (string.IsNullOrWhiteSpace(line)) sb.AppendLine();
                else sb.AppendLine(indent + line);
            }
            if (!inject.EndsWith("\n")) sb.AppendLine();
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine($"Warning: Failed to inject template '{path}': {ex.Message}");
        }
    }

    static string ReplacePlaceholders(string text, IReadOnlyDictionary<string, string> replacements)
    {
        string result = text;
        foreach (var kv in replacements)
        {
            result = result.Replace("{{" + kv.Key + "}}", kv.Value);
        }
        return result;
    }
}
