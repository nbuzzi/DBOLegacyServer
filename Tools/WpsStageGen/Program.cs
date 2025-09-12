using System.Text;
using System.Text.RegularExpressions;

// WPS Stage Generator
// - Scans a .wps file for CCBD stages
// - Adds new stages with customizable intervals, boss stages, drop items, and maps
// - Ensures LastStage is set only on the final stage

internal record GenOptions(
    string InputPath,
    int AddCount,
    int BossEvery,
    int StartStageOverride,
    int DropItem,
    int DropAmountBase,
    int DropAmountStep,
    int MobGroupBase,
    int BossGroup,
    string? BossTpWorld
);

internal class Program
{
    private static readonly Regex StageHeader = new(
        "Action\\(\\s*\\\"CCBD stage\\\"\\s*\\)\\s*--\\[\\s*Param\\(\\s*\\\"stage\\\"\\s*,\\s*(\\d+)\\s*\\)",
        RegexOptions.Compiled | RegexOptions.IgnoreCase | RegexOptions.Singleline);

    private static readonly Regex LastStageParam = new(
        "Param\\(\\s*\\\"LastStage\\\"\\s*,\\s*(true|false)\\s*\\)",
        RegexOptions.Compiled | RegexOptions.IgnoreCase);

    private static readonly Regex DropItemParam = new(
        "Param\\(\\s*\\\"drop item\\\"\\s*,\\s*(\\d+)\\s*\\)",
        RegexOptions.Compiled | RegexOptions.IgnoreCase);

    private static readonly Regex DropAmountParam = new(
        "Param\\(\\s*\\\"drop item amount\\\"\\s*,\\s*(\\d+)\\s*\\)",
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

            // Ensure all existing LastStage = false, we'll set true on the final appended stage
            text = LastStageParam.Replace(text, m => "Param( \"LastStage\", false )");

            var sb = new StringBuilder(text);
            if (!text.EndsWith("\n")) sb.AppendLine();

            for (int i = 0; i < opts.AddCount; i++)
            {
                int stage = startStage + i;
                bool isBoss = (stage % opts.BossEvery) == 0;
                int mobGroup = isBoss ? opts.BossGroup : (opts.MobGroupBase + i);
                int dropAmount = opts.DropAmountBase + (i * opts.DropAmountStep);
                int dropItem = opts.DropItem;

                sb.AppendLine();
                sb.AppendLine("-----------------------------------");
                sb.AppendLine($"-- Stage {stage}");
                sb.AppendLine("-----------------------------------");
                sb.AppendLine("Action( \"CCBD stage\" )");
                sb.AppendLine("--[");
                sb.AppendLine($"    Param( \"stage\", {stage} )");
                if (isBoss) sb.AppendLine("    Param( \"LastStage\", true )");
                sb.AppendLine("    Param( \"direct play\", \"false\" )");
                sb.AppendLine("    Action( \"add mobgroup\" )");
                sb.AppendLine("    --[");
                sb.AppendLine($"        Param( \"group\", {mobGroup} )");
                sb.AppendLine("        Param( \"no spawn wait\", \"true\" )");
                sb.AppendLine("    --]");
                sb.AppendLine("    End()");
                sb.AppendLine($"    Param( \"drop item\", {dropItem} )");
                sb.AppendLine($"    Param( \"drop item amount\", {dropAmount} )");
                if (isBoss && !string.IsNullOrWhiteSpace(opts.BossTpWorld))
                {
                    sb.AppendLine($"    Param( \"tp world\", \"{opts.BossTpWorld}\" )");
                }
                sb.AppendLine("--]");
                sb.AppendLine("End()");
            }

            File.WriteAllText(opts.InputPath, sb.ToString(), Encoding.UTF8);

            Console.WriteLine($"Added {opts.AddCount} stages starting at {startStage}. Final stage: {startStage + opts.AddCount - 1}.");
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
        int dropItem = 7000014; // default item id to drop
        int dropBase = 2;       // starting drop amount
        int dropStep = 1;       // increment per stage
        int mobGroupBase = 9101;// base mob group for regular stages
        int bossGroup = 9999;   // boss group id
        string? bossWorld = "CCBD_BOSS_WORLD";

        foreach (var a in args)
        {
            var kv = a.Split('=', 2);
            if (kv.Length != 2) continue;
            string k = kv[0].Trim().ToLowerInvariant();
            string v = kv[1].Trim();
            switch (k)
            {
                case "in": input = v; break;
                case "add": add = int.Parse(v); break;
                case "bossevery": bossEvery = int.Parse(v); break;
                case "start": startStage = int.Parse(v); break;
                case "dropitem": dropItem = int.Parse(v); break;
                case "dropbase": dropBase = int.Parse(v); break;
                case "dropstep": dropStep = int.Parse(v); break;
                case "mobbase": mobGroupBase = int.Parse(v); break;
                case "bossgroup": bossGroup = int.Parse(v); break;
                case "bossworld": bossWorld = v; break;
            }
        }

        if (string.IsNullOrWhiteSpace(input) || !File.Exists(input))
            return null;

        return new GenOptions(input!, add, bossEvery, startStage, dropItem, dropBase, dropStep, mobGroupBase, bossGroup, bossWorld);
    }

    static void PrintUsage()
    {
        Console.WriteLine("WpsStageGen - CCBD WPS Stage generator");
        Console.WriteLine("Usage: WpsStageGen in=PATH [add=5] [bossEvery=5] [start=0] [dropItem=7000014] [dropBase=2] [dropStep=1] [mobBase=9101] [bossGroup=9999] [bossWorld=CCBD_BOSS_WORLD]");
        Console.WriteLine();
        Console.WriteLine("Behavior:");
        Console.WriteLine("- Finds max existing stage, sets all LastStage=false, appends N stages");
        Console.WriteLine("- Marks every Nth stage as boss (LastStage=true) and sets TP world if provided");
        Console.WriteLine("- Adds add mobgroup group=<mobBase+i> for regular stages, bossGroup for bosses");
        Console.WriteLine("- Adds drop item and drop item amount with arithmetic progression");
    }
}
