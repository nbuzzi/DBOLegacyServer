#include "stdafx.h"
#include "PlayerModifiers.h"
#include "CharacterAttPC.h"
#include "CharacterAtt.h"
#include "NtlSharedType.h"
#include <NtlBattle.h>
#include "CPlayer.h"
#include <cwctype>
#include <wchar.h>

static std::wstring PM_ToLowerW(const std::wstring& s)
{
    std::wstring out(s);
    for (auto &ch : out) ch = (wchar_t)towlower(ch);
    return out;
}

CPlayerModifiers::CPlayerModifiers()
{
    Init();
}

CPlayerModifiers::~CPlayerModifiers()
{
}

void CPlayerModifiers::Init()
{
    // Default disabled; can be enabled via config or GM command
    m_enabled = false;
    m_mods = Modifiers();
    m_byCharId.clear();
    m_byCharName.clear();
    m_cfgPath = ".\\config\\PlayerModifiers.cfg";

    // Initialize auto-schedule state
    m_autoScheduleCfg = AutoScheduleConfig();
    m_autoScheduleState = AutoScheduleState::IDLE;
    m_autoScheduleActive = false;
    m_autoScheduleRemainingMs = 0;

    LoadConfigInternal(m_cfgPath.c_str());
}

bool CPlayerModifiers::ReloadConfig(const char* path)
{
    if (!path)
        path = m_cfgPath.c_str();
    if (path && *path)
        m_cfgPath = path;
    return LoadConfigInternal(m_cfgPath.c_str());
}

bool CPlayerModifiers::LoadConfigInternal(const char* path)
{
    // reset to identity each load
    m_mods = Modifiers();

    FILE* f = nullptr;
    errno_t e = fopen_s(&f, path, "rt");
    if (e != 0 || !f)
        return false;

    char line[1024];
    while (fgets(line, sizeof(line), f))
    {
        char* p = line;
        while (*p == ' ' || *p == '\t') ++p;
        if (*p == '\0' || *p == '\n' || *p == '#')
            continue;

        // Supported forms:
        // - key=value pairs (global)
        // - all modifiers: ... (global)
        // - char <id> modifiers: ... (per character id)
        // - name <charName> modifiers: ... (per character name)
        char* colon = strchr(p, ':');
        char* parse = colon ? colon + 1 : p;

        // detect prefix before ':' if any
    enum Scope { Scope_Global, Scope_CharId, Scope_CharName } scope = Scope_Global;
        unsigned int scopedId = 0;
        std::wstring scopedName;
        if (colon)
        {
            *colon = '\0';
            // tokenize left part by spaces
            char idOrAll[256] = {0};
            char type[64] = {0};
            // example left: "all modifiers" | "char 123 modifiers" | "name Goku modifiers"
            // we'll scan first token to see 'char', 'name', or 'all'
            char* lp = p;
            while (*lp == ' ' || *lp == '\t') ++lp;
            char* space1 = strchr(lp, ' ');
            if (space1)
            {
                *space1 = '\0';
                const char* first = lp;
                char* rest = space1 + 1;
                while (*rest == ' ' || *rest == '\t') ++rest;
                // find next space
                char* space2 = strchr(rest, ' ');
                if (space2)
                {
                    *space2 = '\0';
                    strncpy_s(idOrAll, rest, _TRUNCATE);
                    // expect trailing token to be 'modifiers' but we don't require it
                }
                // classify scope
                if (_stricmp(first, "char") == 0)
                {
                    scope = Scope_CharId;
                    scopedId = (unsigned int)strtoul(idOrAll, nullptr, 10);
                }
                else if (_stricmp(first, "name") == 0)
                {
                    scope = Scope_CharName;
                    // convert idOrAll (ANSI) to wide
                    wchar_t wbuf[256] = {0};
                    size_t conv = 0; mbstowcs_s(&conv, wbuf, 256, idOrAll, _TRUNCATE);
                    scopedName = PM_ToLowerW(wbuf);
                }
                else
                {
                    scope = Scope_Global; // includes 'all'
                }
            }
        }

        char* t = strtok(parse, " \t\n\r");
    Modifiers temp = (scope == Scope_Global ? m_mods : Modifiers());
        while (t)
        {
            char* eq = strchr(t, '=');
            if (eq)
            {
                *eq = '\0';
                const char* key = t;
                float val = (float)atof(eq + 1);
                if (_stricmp(key, "enabled") == 0)
                {
                    // Only meaningful for global scope; non-zero enables
                    if (scope == Scope_Global)
                        m_enabled = (val != 0.f);
                }
                else if (_stricmp(key, "hp") == 0) temp.hp = val;
                else if (_stricmp(key, "physAtk") == 0) temp.physAtk = val;
                else if (_stricmp(key, "engAtk") == 0) temp.engAtk = val;
                else if (_stricmp(key, "physDef") == 0) temp.physDef = val;
                else if (_stricmp(key, "engDef") == 0) temp.engDef = val;
                else if (_stricmp(key, "atkSpd") == 0) temp.atkSpd = val;
                else if (_stricmp(key, "runSpd") == 0) temp.runSpd = val;
                else if (_stricmp(key, "physCrit") == 0) temp.physCrit = val;
                else if (_stricmp(key, "engCrit") == 0) temp.engCrit = val;
                else if (_stricmp(key, "physCritDmg") == 0) temp.physCritDmg = val;
                else if (_stricmp(key, "engCritDmg") == 0) temp.engCritDmg = val;
                else if (_stricmp(key, "attackRate") == 0) temp.attackRate = val;
                else if (_stricmp(key, "dodgeRate") == 0) temp.dodgeRate = val;
                else if (_stricmp(key, "blockRate") == 0) temp.blockRate = val;
                else if (_stricmp(key, "blockDmg") == 0) temp.blockDmg = val;
                else if (_stricmp(key, "guardRate") == 0) temp.guardRate = val;
                // Auto-schedule configuration
                else if (_stricmp(key, "AutoScheduleEnabled") == 0)
                {
                    if (scope == Scope_Global)
                        m_autoScheduleCfg.enabled = (val != 0.f);
                }
                else if (_stricmp(key, "AutoScheduleDaysPerWeek") == 0)
                {
                    if (scope == Scope_Global)
                    {
                        unsigned int days = (unsigned int)val;
                        if (days >= 1 && days <= 7)
                            m_autoScheduleCfg.daysPerWeek = days;
                    }
                }
                else if (_stricmp(key, "AutoScheduleDurationHours") == 0)
                {
                    if (scope == Scope_Global)
                        m_autoScheduleCfg.durationHours = (unsigned int)val;
                }
                else if (_stricmp(key, "AutoScheduleIntervalHours") == 0)
                {
                    if (scope == Scope_Global)
                        m_autoScheduleCfg.intervalHours = (unsigned int)val;
                }
                else if (_stricmp(key, "AutoScheduleInitialDelayMinutes") == 0)
                {
                    if (scope == Scope_Global)
                        m_autoScheduleCfg.initialDelayMinutes = (unsigned int)val;
                }
            }
            t = strtok(nullptr, " \t\n\r");
        }
        // commit parsed modifiers
        if (scope == Scope_Global)
        {
            m_mods = temp;
        }
        else if (scope == Scope_CharId)
        {
            if (scopedId != 0)
                m_byCharId[scopedId] = temp;
        }
        else if (scope == Scope_CharName)
        {
            if (!scopedName.empty())
                m_byCharName[scopedName] = temp;
        }
    }
    fclose(f);
    return true;
}

void CPlayerModifiers::ApplyTo(CCharacterAttPC* att)
{
    if (!att || !m_enabled)
        return;
    const Modifiers* pm = &m_mods;
    // resolve per-player override: prefer CharID then Name
    CPlayer* plr = static_cast<CPlayer*>(att->GetOwnerRef());
    if (plr)
    {
        unsigned int charId = plr->GetCharID();
        auto it = m_byCharId.find(charId);
        if (it != m_byCharId.end())
        {
            pm = &it->second;
        }
        else
        {
            const WCHAR* wname = plr->GetCharName();
            if (wname && *wname)
            {
                std::wstring key = PM_ToLowerW(wname);
                auto it2 = m_byCharName.find(key);
                if (it2 != m_byCharName.end())
                    pm = &it2->second;
            }
        }
    }
    const Modifiers& m = *pm;
    if (m.IsIdentity())
        return;

    // Max LP
    if (m.hp != 1.f)
    {
        DWORD maxLp = att->GetMaxLP();
        DWORD target = (DWORD)((float)maxLp * m.hp);
        if (target != maxLp)
        {
            int diff = (int)target - (int)maxLp;
            if (diff > 0)
                att->CalculateMaxLP((float)diff, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
            else if (diff < 0)
                att->CalculateMaxLP((float)(-diff), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);
        }
    }

    // Offence
    if (m.physAtk != 1.f)
    {
        WORD cur = att->GetPhysicalOffence();
        WORD target = (WORD)((float)cur * m.physAtk);
        if (target > cur) att->CalculatePhysicalOffence((float)(target - cur), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
        else if (target < cur) att->CalculatePhysicalOffence((float)(cur - target), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);
    }
    if (m.engAtk != 1.f)
    {
        WORD cur = att->GetEnergyOffence();
        WORD target = (WORD)((float)cur * m.engAtk);
        if (target > cur) att->CalculateEnergyOffence((float)(target - cur), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
        else if (target < cur) att->CalculateEnergyOffence((float)(cur - target), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);
    }

    // Defence
    if (m.physDef != 1.f)
    {
        WORD cur = att->GetPhysicalDefence();
        WORD target = (WORD)((float)cur * m.physDef);
        if (target > cur) att->CalculatePhysicalDefence((float)(target - cur), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
        else if (target < cur) att->CalculatePhysicalDefence((float)(cur - target), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);
    }
    if (m.engDef != 1.f)
    {
        WORD cur = att->GetEnergyDefence();
        WORD target = (WORD)((float)cur * m.engDef);
        if (target > cur) att->CalculateEnergyDefence((float)(target - cur), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
        else if (target < cur) att->CalculateEnergyDefence((float)(cur - target), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);
    }

    // Attack Speed Rate
    if (m.atkSpd != 1.f)
    {
        WORD cur = att->GetAttackSpeedRate();
        WORD target = (WORD)((float)cur * m.atkSpd);
        if (target > cur) att->CalculateAttackSpeedRate((float)(target - cur), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
        else if (target < cur) att->CalculateAttackSpeedRate((float)(cur - target), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);
    }

    // Run Speed
    if (m.runSpd != 1.f)
    {
        float cur = att->GetRunSpeed();
        float target = cur * m.runSpd;
        float diff = target - cur;
        if (diff > 0.0f) att->CalculateRunSpeed(diff, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
        else if (diff < 0.0f) att->CalculateRunSpeed(-diff, SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);
    }

    // Rates and crits
    if (m.attackRate != 1.f)
    {
        float cur = (float)att->GetAttackRate();
        float target = cur * m.attackRate;
        float diff = target - cur;
        if (diff > 0.0f) att->CalculateAttackRate(diff, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
        else if (diff < 0.0f) att->CalculateAttackRate(-diff, SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);
    }
    if (m.dodgeRate != 1.f)
    {
        float cur = (float)att->GetDodgeRate();
        float target = cur * m.dodgeRate;
        float diff = target - cur;
        if (diff > 0.0f) att->CalculateDodgeRate(diff, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
        else if (diff < 0.0f) att->CalculateDodgeRate(-diff, SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);
    }
    if (m.blockRate != 1.f)
    {
        float cur = (float)att->GetBlockRate();
        float target = cur * m.blockRate;
        float diff = target - cur;
        if (diff > 0.0f) att->CalculateBlockRate(diff, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
        else if (diff < 0.0f) att->CalculateBlockRate(-diff, SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);
    }
    if (m.blockDmg != 1.f)
    {
        float cur = (float)att->GetBlockDamageRate();
        float target = cur * m.blockDmg;
        float diff = target - cur;
        if (diff > 0.0f) att->CalculateBlockDamageRate(diff, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
        else if (diff < 0.0f) att->CalculateBlockDamageRate(-diff, SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);
    }
    if (m.guardRate != 1.f)
    {
        float cur = (float)att->GetGuardRate();
        float target = cur * m.guardRate;
        float diff = target - cur;
        if (diff > 0.0f) att->CalculateGuardRate(diff, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
        else if (diff < 0.0f) att->CalculateGuardRate(-diff, SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);
    }

    if (m.physCrit != 1.f)
    {
        float cur = (float)att->GetPhysicalCriticalRate();
        float target = cur * m.physCrit;
        float diff = target - cur;
        if (diff > 0.0f) att->CalculatePhysicalCriticalRate(diff, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
        else if (diff < 0.0f) att->CalculatePhysicalCriticalRate(-diff, SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);
    }
    if (m.engCrit != 1.f)
    {
        float cur = (float)att->GetEnergyCriticalRate();
        float target = cur * m.engCrit;
        float diff = target - cur;
        if (diff > 0.0f) att->CalculateEnergyCriticalRate(diff, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
        else if (diff < 0.0f) att->CalculateEnergyCriticalRate(-diff, SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);
    }

    if (m.physCritDmg != 1.f)
    {
        float cur = att->GetPhysicalCriticalDamageRate();
        float target = cur * m.physCritDmg;
        float diff = target - cur;
        if (diff > 0.0f) att->CalculatePhysicalCriticalDamageRate(diff, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
        else if (diff < 0.0f) att->CalculatePhysicalCriticalDamageRate(-diff, SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);
    }
    if (m.engCritDmg != 1.f)
    {
        float cur = att->GetEnergyCriticalDamageRate();
        float target = cur * m.engCritDmg;
        float diff = target - cur;
        if (diff > 0.0f) att->CalculateEnergyCriticalDamageRate(diff, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
        else if (diff < 0.0f) att->CalculateEnergyCriticalDamageRate(-diff, SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);
    }
}

void CPlayerModifiers::AutoScheduleTick(unsigned long dwTickDiff)
{
    if (!m_autoScheduleCfg.enabled)
        return;

    switch (m_autoScheduleState)
    {
    case AutoScheduleState::IDLE:
        // Initialize auto-schedule system on first tick
        m_autoScheduleState = AutoScheduleState::WAIT_NEXT;
        m_autoScheduleRemainingMs = m_autoScheduleCfg.initialDelayMinutes * 60 * 1000UL;
        break;

    case AutoScheduleState::WAIT_NEXT:
        if (m_autoScheduleRemainingMs > dwTickDiff)
        {
            m_autoScheduleRemainingMs -= dwTickDiff;
        }
        else
        {
            m_autoScheduleRemainingMs = 0;
            StartAutoScheduleSession();
        }
        break;

    case AutoScheduleState::ACTIVE:
        if (m_autoScheduleRemainingMs > dwTickDiff)
        {
            m_autoScheduleRemainingMs -= dwTickDiff;
        }
        else
        {
            m_autoScheduleRemainingMs = 0;
            EndAutoScheduleSession();
        }
        break;
    }
}

void CPlayerModifiers::StartAutoScheduleSession()
{
    m_autoScheduleState = AutoScheduleState::ACTIVE;
    m_autoScheduleActive = true;
    m_autoScheduleRemainingMs = m_autoScheduleCfg.durationHours * 3600 * 1000UL;

    // Enable player modifiers
    SetEnabled(true);

    // Recalculate all players
    if (g_pObjectManager)
    {
        size_t n = g_pObjectManager->RecalculateAllPlayers();
        NTL_PRINT(PRINT_APP, _T("[PlayerModifiers Auto-Schedule] Started session (duration: %u hours, %zu players recalculated)"),
            m_autoScheduleCfg.durationHours, n);
    }
}

void CPlayerModifiers::EndAutoScheduleSession()
{
    m_autoScheduleState = AutoScheduleState::WAIT_NEXT;
    m_autoScheduleActive = false;
    m_autoScheduleRemainingMs = m_autoScheduleCfg.intervalHours * 3600 * 1000UL;

    // Disable player modifiers
    SetEnabled(false);

    // Recalculate all players back to normal
    if (g_pObjectManager)
    {
        size_t n = g_pObjectManager->RecalculateAllPlayers();
        NTL_PRINT(PRINT_APP, _T("[PlayerModifiers Auto-Schedule] Ended session (next session in %u hours, %zu players recalculated)"),
            m_autoScheduleCfg.intervalHours, n);
    }
}
