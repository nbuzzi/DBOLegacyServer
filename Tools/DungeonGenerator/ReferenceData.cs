using System.Collections.ObjectModel;

namespace DungeonGenerator;

internal static class ReferenceData
{
    internal readonly record struct RewardItemOption(int ItemId, string Label)
    {
        public override string ToString()
        {
            return $"{ItemId} - {Label}";
        }
    }

    internal static readonly ReadOnlyCollection<string> ArenaOptions = new(
        new List<string>
        {
            "ARENA_A",
            "ARENA_B",
            "ARENA_C",
            "ARENA_D",
            "ARENA_FIRE",
            "ARENA_ICE",
            "ARENA_LIGHTNING",
            "ARENA_FOREST",
            "ARENA_RED",
            "ARENA_BLUE",
            "ARENA_YELLOW",
            "ARENA_GREEN"
        });

    internal static readonly ReadOnlyCollection<RewardItemOption> RewardItems = new(
        new List<RewardItemOption>
        {
            new(7000002, "Reward Chest Tier 1"),
            new(7000003, "Reward Chest Tier 2"),
            new(7000004, "Reward Chest Tier 3"),
            new(7000005, "Reward Chest Tier 4")
        });

    internal static int FindRewardItemIndex(int itemId)
    {
        for (int i = 0; i < RewardItems.Count; i++)
        {
            if (RewardItems[i].ItemId == itemId)
            {
                return i;
            }
        }

        return -1;
    }
}
