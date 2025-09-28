#pragma once

// Shared helpers to recognize Arena worlds by tblidx/ID.
// We treat a contiguous range starting at 900043 as Arena worlds.
// If you need a different size, adjust ARENA_WORLD_COUNT accordingly.

namespace ArenaWorld
{
    // Base world tblidx for Arena
    static const unsigned int ARENA_WORLD_BASE = 900043u;
    // Number of arena worlds in the contiguous range (supports 900043..900142 by default)
    static const unsigned int ARENA_WORLD_COUNT = 100u;

    inline bool IsWorldTblidx(unsigned int worldTblidx)
    {
        return worldTblidx >= ARENA_WORLD_BASE && worldTblidx < (ARENA_WORLD_BASE + ARENA_WORLD_COUNT);
    }

    // In some places legacy code compares GetWorldID() directly; if your engine uses
    // separate runtime world IDs, prefer checking the table index instead.
    inline bool IsWorldId(unsigned int worldId)
    {
        return IsWorldTblidx(worldId);
    }
}
