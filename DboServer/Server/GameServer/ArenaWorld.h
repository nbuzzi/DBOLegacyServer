#pragma once

// Shared helpers to recognize Arena worlds.
// 1) By tblidx/ID range (900043..900043 + ARENA_WORLD_COUNT)
// 2) By name containing the token "TORNEOPODER" (case-insensitive), which is
//    a robust way to detect the intended arena map regardless of numeric IDs.

#include <string>
#include <algorithm>
#include <cwctype>
#include <cctype>

// Forward decls to avoid heavy includes; definitions remain header-only and light.
struct sWORLD_TBLDAT;
class CWorld;

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

    // --- Name-based detection helpers -------------------------------------------------
    // Case-insensitive wide-string contains
    inline bool WcsIContains(const wchar_t* haystack, const wchar_t* needle)
    {
        if (!haystack || !needle || *needle == L'\0') return false;
        std::wstring h(haystack);
        std::wstring n(needle);
        std::transform(h.begin(), h.end(), h.begin(), [](wchar_t ch) { return (wchar_t)std::towupper(ch); });
        std::transform(n.begin(), n.end(), n.begin(), [](wchar_t ch) { return (wchar_t)std::towupper(ch); });
        return h.find(n) != std::wstring::npos;
    }

    // Case-insensitive narrow-string contains
    inline bool StrIContains(const char* haystack, const char* needle)
    {
        if (!haystack || !needle || *needle == '\0') return false;
        std::string h(haystack);
        std::string n(needle);
        std::transform(h.begin(), h.end(), h.begin(), [](char ch) { return (char)std::toupper((unsigned char)ch); });
        std::transform(n.begin(), n.end(), n.begin(), [](char ch) { return (char)std::toupper((unsigned char)ch); });
        return h.find(n) != std::string::npos;
    }

    // Returns true when the provided wide or narrow name clearly identifies the target arena map.
    inline bool IsArenaWorldNameW(const wchar_t* wszName)
    {
        return WcsIContains(wszName, L"TORNEOPODER");
    }
    inline bool IsArenaWorldNameA(const char* szName)
    {
        return StrIContains(szName, "TORNEOPODER");
    }

    // sWORLD_TBLDAT has the wide name field (wszName) — preferred detection path
    inline bool IsArenaWorldByTable(const sWORLD_TBLDAT* pWorldTbldat)
    {
        if (!pWorldTbldat) return false;
        // The struct is defined in WorldTable.h and includes WCHAR wszName[...]
        // We access it by reinterpretation in the consumer code, so keep it opaque here.
        // Cast to pointer-to-wchar without including the full header here.
        // NB: We rely on the conventional layout: the first wide name field is named wszName.
        // If the layout differs in your fork, adjust this function at the call site instead.
        // To avoid UB, callers should pass the actual field pointer (pWorldTbldat->wszName).
        return false; // See overload below using explicit name pointer.
    }

    // Convenience overload when only the wide name is available
    inline bool IsArenaWorldByWideName(const wchar_t* wszName)
    {
        return IsArenaWorldNameW(wszName);
    }

    // Convenience overload when only the narrow name is available
    inline bool IsArenaWorldByNarrowName(const char* szName)
    {
        return IsArenaWorldNameA(szName);
    }
}

// Optional: if you frequently have CWorld* available, you can add this helper in a .cpp
// including World.h. Keeping the header light avoids heavy includes here.
