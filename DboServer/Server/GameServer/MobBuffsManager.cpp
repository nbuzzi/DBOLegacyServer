#include "stdafx.h"
#include "MobBuffsManager.h"
#include "Monster.h"
#include "NtlLog.h"
#include "TableContainerManager.h"
#include "SkillTable.h"
#include "SystemEffectTable.h"
#include "BuffManager.h"

CMobBuffsManager::CMobBuffsManager()
{
    Init();
}

CMobBuffsManager::~CMobBuffsManager()
{
}

void CMobBuffsManager::Init()
{
    m_enabled = true;   // default ON but guarded by empty config
    m_verbose = false;
    m_cfgPath = ".\\config\\MobBuffs.cfg";
    Clear();
    LoadConfigFromIniPath(m_cfgPath.c_str());
}

void CMobBuffsManager::Clear()
{
    m_worldMobBuffs.clear();
    m_exceptWorldGlobals.clear();
}

bool CMobBuffsManager::LoadConfigFromIniPath(const char* path)
{
    if (path && *path)
        m_cfgPath = path;

    Clear();

    FILE* f = nullptr;
    errno_t e = fopen_s(&f, m_cfgPath.c_str(), "rt");
    if (e != 0 || !f)
    {
        // Not fatal: keep disabled if file missing
        ERR_LOG(LOG_GENERAL, "[MobBuffs] Config missing or cannot open: %s (errno=%d)", m_cfgPath.c_str(), (int)e);
        m_enabled = false;
        return false;
    }

    m_enabled = true;

    char line[1024];
    while (fgets(line, sizeof(line), f))
    {
        // trim leading whitespace
        char* p = line;
        while (*p == ' ' || *p == '\t') ++p;
        if (*p == '\0' || *p == '\n' || *p == '#') continue;

        // expected formats (case-insensitive keywords):
        // world=<tblidx> mob=<tblidx> buffs: skill@dur, skill@dur
        // world=<tblidx> all buffs: skill@dur, ...
        // all mob=<tblidx> buffs: ...   // applies to any world
        // all all buffs: ...            // global to any world/mob
        // all buffs except: world=<tblidx> mob=<csv>
        // settings: Enabled=1 Verbose=0

        // Handle settings section
        if (_strnicmp(p, "settings:", 9) == 0)
        {
            char* s = p + 9;
            char* tok = strtok(s, " \t\r\n");
            while (tok)
            {
                char* eq = strchr(tok, '=');
                if (eq)
                {
                    *eq = '\0';
                    const char* key = tok;
                    const char* val = eq + 1;
                    if (_stricmp(key, "Enabled") == 0) m_enabled = (atoi(val) != 0);
                    else if (_stricmp(key, "Verbose") == 0) m_verbose = (atoi(val) != 0);
                }
                tok = strtok(nullptr, " \t\r\n");
            }
            continue;
        }

        // Detect an exception rule: "all buffs except: world=<tblidx> mob=<csv>"
        if (_strnicmp(p, "all buffs except:", 17) == 0)
        {
            unsigned int worldTblidx = 0;
            std::unordered_set<unsigned int> mobs;
            char* s = p + 17;
            // Parse tokens like world=123 mob=1,2,3
            char* tok = strtok(s, " \t\r\n");
            while (tok)
            {
                char* eq = strchr(tok, '=');
                if (eq)
                {
                    *eq = '\0';
                    const char* key = tok;
                    const char* val = eq + 1;
                    if (_stricmp(key, "world") == 0)
                    {
                        worldTblidx = (unsigned int)strtoul(val, nullptr, 10);
                    }
                    else if (_stricmp(key, "mob") == 0)
                    {
                        // Comma-separated list
                        char buf[512];
                        strncpy_s(buf, sizeof(buf), val, _TRUNCATE);
                        char* idTok = strtok(buf, ",|");
                        while (idTok)
                        {
                            unsigned int id = (unsigned int)strtoul(idTok, nullptr, 10);
                            if (id) mobs.insert(id);
                            idTok = strtok(nullptr, ",|");
                        }
                    }
                }
                tok = strtok(nullptr, " \t\r\n");
            }
            if (worldTblidx != 0 && !mobs.empty())
            {
                m_exceptWorldGlobals[worldTblidx] = mobs;
            }
            continue;
        }

        // General rule parsing: split LHS and RHS by ':'
        char* colon = strchr(p, ':');
        if (!colon) continue;
        *colon = '\0';

        // Parse LHS selectors: world=..., mob=... and a keyword (must be "buffs")
        unsigned int worldTblidx = 0; // 0 = any world
        unsigned int mobTblidx = 0;   // 0 = any mob
        bool isBuffs = false;

        // Tokenize LHS by spaces
        char* lhs = p;
        char* tok = strtok(lhs, " \t");
        while (tok)
        {
            if (_stricmp(tok, "buffs") == 0) { isBuffs = true; }
            else if (_strnicmp(tok, "world=", 7) == 0)
            {
                worldTblidx = (unsigned int)strtoul(tok + 7, nullptr, 10);
            }
            else if (_strnicmp(tok, "mob=", 5) == 0)
            {
                mobTblidx = (unsigned int)strtoul(tok + 5, nullptr, 10);
            }
            else if (_stricmp(tok, "all") == 0)
            {
                // keep 0 for wildcard
            }
            tok = strtok(nullptr, " \t");
        }

        if (!isBuffs)
            continue;

        // Parse RHS list of entries: skill@durMs or just skill
        std::vector<BuffEntry> entries;
        char* list = colon + 1;
        char* ent = strtok(list, ",\n\r");
        while (ent)
        {
            while (*ent == ' ' || *ent == '\t') ++ent;
            unsigned int skillId = 0;
            unsigned int dur = 0;
            char* at = strchr(ent, '@');
            if (at)
            {
                *at = '\0';
                skillId = (unsigned int)strtoul(ent, nullptr, 10);
                dur = (unsigned int)strtoul(at + 1, nullptr, 10);
            }
            else
            {
                skillId = (unsigned int)strtoul(ent, nullptr, 10);
            }
            if (skillId)
                entries.emplace_back(skillId, dur);
            ent = strtok(nullptr, ",\n\r");
        }
        if (!entries.empty())
        {
            auto& dst = m_worldMobBuffs[MakeKey(worldTblidx, mobTblidx)];
            dst.insert(dst.end(), entries.begin(), entries.end());
        }
    }

    fclose(f);

    ERR_LOG(LOG_GENERAL, "[MobBuffs] Loaded config: %s (rules=%u)", m_cfgPath.c_str(), (unsigned)m_worldMobBuffs.size());
    return true;
}

void CMobBuffsManager::ApplyBuffs(CMonster* pMob)
{
    if (!m_enabled || !pMob || !pMob->IsInitialized())
        return;

    // We use world TBLIDX as scope id (static world type). Fall back to worldID if tblidx is 0.
    unsigned int worldTblidx = (unsigned int)pMob->GetWorldTblidx();
    unsigned int mobTblidx = (unsigned int)pMob->GetTblidx();

    std::vector<BuffEntry> buffs;

    auto append = [&](unsigned int w, unsigned int m) {
        auto it = m_worldMobBuffs.find(MakeKey(w, m));
        if (it != m_worldMobBuffs.end())
            buffs.insert(buffs.end(), it->second.begin(), it->second.end());
    };

    // Priority order:
    // 1) Exact world+mob
    append(worldTblidx, mobTblidx);
    // 2) World-specific global (mob=0) unless excepted
    {
        bool excepted = false;
        auto exIt = m_exceptWorldGlobals.find(worldTblidx);
        if (exIt != m_exceptWorldGlobals.end())
        {
            if (exIt->second.find(mobTblidx) != exIt->second.end())
                excepted = true;
        }
        if (!excepted)
            append(worldTblidx, 0);
    }
    // 3) Any-world per-mob
    append(0, mobTblidx);
    // 4) Full global (world=0, mob=0)
    append(0, 0);

    if (buffs.empty())
        return;

    CBuffManager* pBuffManager = pMob->GetBuffManager();
    if (!pBuffManager)
    {
        ERR_LOG(LOG_GENERAL, "[MobBuffs] Buff manager missing for mob=%u worldTblidx=%u", (unsigned)mobTblidx, (unsigned)worldTblidx);
        return;
    }

    if (!g_pTableContainer)
    {
        ERR_LOG(LOG_GENERAL, "[MobBuffs] Table container not ready; skip mob=%u worldTblidx=%u", (unsigned)mobTblidx, (unsigned)worldTblidx);
        return;
    }

    CSkillTable* pSkillTable = g_pTableContainer->GetSkillTable();
    CSystemEffectTable* pSystemEffectTable = g_pTableContainer->GetSystemEffectTable();
    if (!pSkillTable || !pSystemEffectTable)
    {
        ERR_LOG(LOG_GENERAL, "[MobBuffs] Missing skill/system effect tables; skip mob=%u worldTblidx=%u", (unsigned)mobTblidx, (unsigned)worldTblidx);
        return;
    }

    for (const BuffEntry& be : buffs)
    {
        sSKILL_TBLDAT* pSkill = (sSKILL_TBLDAT*)pSkillTable->FindData(be.skillTblidx);
        if (!pSkill) { if (m_verbose) ERR_LOG(LOG_GENERAL, "[MobBuffs] Unknown skill %u", be.skillTblidx); continue; }

        eSYSTEM_EFFECT_CODE aeEffectCode[NTL_MAX_EFFECT_IN_SKILL];
        for (int i = 0; i < NTL_MAX_EFFECT_IN_SKILL; ++i)
        {
            if (pSkill->skill_Effect[i] != INVALID_TBLIDX)
                aeEffectCode[i] = pSystemEffectTable->GetEffectCodeWithTblidx(pSkill->skill_Effect[i]);
            else
                aeEffectCode[i] = INVALID_SYSTEM_EFFECT_CODE;
        }

        sDBO_BUFF_PARAMETER aBuffParameter[NTL_MAX_EFFECT_IN_SKILL];
        for (int i = 0; i < NTL_MAX_EFFECT_IN_SKILL; ++i)
        {
            aBuffParameter[i].byBuffParameterType = DBO_BUFF_PARAMETER_TYPE_DEFAULT;
            aBuffParameter[i].buffParameter.fParameter = 0;
            aBuffParameter[i].buffParameter.dwRemainValue = 0;
        }

        DWORD dwDurationInMs = be.durationMs != 0 ? be.durationMs : pSkill->dwKeepTimeInMilliSecs;
        if (dwDurationInMs == 0)
            dwDurationInMs = 30000; // safety default

        pBuffManager->RegisterBuff(dwDurationInMs, aeEffectCode, aBuffParameter, INVALID_HOBJECT, BUFF_TYPE_BLESS, pSkill);

        if (m_verbose)
            ERR_LOG(LOG_GENERAL, "[MobBuffs] Applied skill=%u dur=%u to mob=%u worldTblidx=%u", be.skillTblidx, (unsigned)dwDurationInMs, (unsigned)mobTblidx, (unsigned)worldTblidx);
    }
}
