#include "stdafx.h"
#include "MobAppearanceOverrideManager.h"
#include "NtlLog.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

CMobAppearanceOverrideManager::CMobAppearanceOverrideManager()
{
    Init();
}

CMobAppearanceOverrideManager::~CMobAppearanceOverrideManager()
{
}

void CMobAppearanceOverrideManager::Init()
{
    m_cfgPath = ".\\config\\MobAppearanceOverrides.cfg";
    m_enabled = false;
    m_verbose = false;
    m_hasGlobalRule = false;
    Clear();
}

void CMobAppearanceOverrideManager::Clear()
{
    m_rules.clear();
    m_hasGlobalRule = false;
    m_globalRule = ReplacementRule();
}

bool CMobAppearanceOverrideManager::LoadConfigFromPath(const char* path)
{
    if (path && *path)
        m_cfgPath = path;

    Clear();

    FILE* f = nullptr;
    errno_t e = fopen_s(&f, m_cfgPath.c_str(), "rt");
    if (e != 0 || !f)
    {
        ERR_LOG(LOG_GENERAL, "[MobAppearance] Config missing or cannot open: %s (errno=%d)", m_cfgPath.c_str(), (int)e);
        m_enabled = false;
        return false;
    }

    m_enabled = true;

    char lineBuf[1024];
    unsigned int lineNum = 0;
    while (fgets(lineBuf, sizeof(lineBuf), f))
    {
        ++lineNum;
        // Trim leading whitespace
        char* p = lineBuf;
        while (*p == ' ' || *p == '\t')
            ++p;

        if (*p == '\0' || *p == '\n' || *p == '#')
            continue;
        if (p[0] == '/' && p[1] == '/')
            continue;
        if (*p == ';')
            continue;

        // Handle inline comments: stop at '#' or ';' or "//"
        char* comment = strchr(p, '#');
        if (comment) *comment = '\0';
        comment = strchr(p, ';');
        if (comment) *comment = '\0';
        char* slash = strstr(p, "//");
        if (slash) *slash = '\0';

        if (_strnicmp(p, "settings:", 9) == 0)
        {
            char* settings = p + 9;
            char* tok = strtok(settings, " \t\r\n");
            while (tok)
            {
                char* eq = strchr(tok, '=');
                if (eq)
                {
                    *eq = '\0';
                    const char* key = tok;
                    const char* val = eq + 1;
                    if (_stricmp(key, "Enabled") == 0 || _stricmp(key, "Enable") == 0)
                        m_enabled = (atoi(val) != 0);
                    else if (_stricmp(key, "Verbose") == 0)
                        m_verbose = (atoi(val) != 0);
                }
                tok = strtok(nullptr, " \t\r\n");
            }
            continue;
        }

        unsigned int srcId = 0;
        ReplacementRule rule;
        // Make a copy because ParseReplacementLine mutates the buffer with strtok
        char workingBuf[1024];
        strncpy_s(workingBuf, sizeof(workingBuf), p, _TRUNCATE);

        if (!ParseReplacementLine(workingBuf, srcId, rule))
        {
            ERR_LOG(LOG_GENERAL, "[MobAppearance] Invalid line %u in %s (ignored)", lineNum, m_cfgPath.c_str());
            continue;
        }

        if (rule.targetTblidx == 0)
        {
            ERR_LOG(LOG_GENERAL, "[MobAppearance] Line %u missing valid target tblidx (ignored)", lineNum);
            continue;
        }

        if (srcId == 0)
        {
            m_globalRule = rule;
            m_hasGlobalRule = true;
            if (m_verbose)
            {
                ERR_LOG(LOG_GENERAL, "[MobAppearance] Global rule: display tblidx %u (useTargetStats=%d)",
                    rule.targetTblidx, (int)rule.useTargetStats);
            }
        }
        else
        {
            m_rules[srcId] = rule;
            if (m_verbose)
            {
                ERR_LOG(LOG_GENERAL, "[MobAppearance] Rule mob %u -> %u (useTargetStats=%d)",
                    srcId, rule.targetTblidx, (int)rule.useTargetStats);
            }
        }
    }

    fclose(f);

    if (!IsEnabled())
    {
        ERR_LOG(LOG_GENERAL, "[MobAppearance] No valid overrides found in %s - manager disabled", m_cfgPath.c_str());
    }
    else
    {
        ERR_LOG(LOG_GENERAL, "[MobAppearance] Loaded overrides: %u specific %s global (config %s)",
            (unsigned)m_rules.size(), m_hasGlobalRule ? "with" : "without", m_cfgPath.c_str());
    }

    return IsEnabled();
}

bool CMobAppearanceOverrideManager::GetReplacement(unsigned int sourceTblidx, ReplacementRule& outRule) const
{
    if (!IsEnabled())
        return false;

    auto it = m_rules.find(sourceTblidx);
    if (it != m_rules.end())
    {
        outRule = it->second;
        return true;
    }

    if (m_hasGlobalRule)
    {
        outRule = m_globalRule;
        return true;
    }

    return false;
}

bool CMobAppearanceOverrideManager::ParseReplacementLine(char* line, unsigned int& outSource, ReplacementRule& outRule)
{
    outSource = 0;
    outRule = ReplacementRule();

    bool hasTarget = false;
    bool hasSource = false;

    for (char* tok = strtok(line, " \t\r\n"); tok != nullptr; tok = strtok(nullptr, " \t\r\n"))
    {
        size_t len = strlen(tok);
        while (len > 0 && (tok[len - 1] == ',' || tok[len - 1] == ';' || tok[len - 1] == ':'))
        {
            tok[--len] = '\0';
        }

        if (*tok == '\0')
            continue;

        if (_stricmp(tok, "->") == 0 || _stricmp(tok, "=>") == 0)
        {
            char* next = strtok(nullptr, " \t\r\n");
            if (next)
            {
                size_t nlen = strlen(next);
                while (nlen > 0 && (next[nlen - 1] == ',' || next[nlen - 1] == ';' || next[nlen - 1] == ':'))
                {
                    next[--nlen] = '\0';
                }
                outRule.targetTblidx = (unsigned int)strtoul(next, nullptr, 10);
                hasTarget = (outRule.targetTblidx != 0);
            }
            continue;
        }

        char* eq = strchr(tok, '=');
        if (!eq)
        {
            // Token without '=' but numeric and target not yet set -> treat as target
            if (!hasTarget)
            {
                unsigned int maybeTarget = (unsigned int)strtoul(tok, nullptr, 10);
                if (maybeTarget != 0)
                {
                    outRule.targetTblidx = maybeTarget;
                    hasTarget = true;
                }
            }
            continue;
        }

        *eq = '\0';
        const char* key = tok;
        const char* val = eq + 1;

        if (_stricmp(key, "mob") == 0 || _stricmp(key, "source") == 0 || _stricmp(key, "src") == 0)
        {
            outSource = (unsigned int)strtoul(val, nullptr, 10);
            hasSource = true;
        }
        else if (_stricmp(key, "replace") == 0 || _stricmp(key, "target") == 0 || _stricmp(key, "with") == 0 || _stricmp(key, "display") == 0)
        {
            outRule.targetTblidx = (unsigned int)strtoul(val, nullptr, 10);
            hasTarget = (outRule.targetTblidx != 0);
        }
        else if (_stricmp(key, "useTargetStats") == 0 || _stricmp(key, "useStats") == 0 || _stricmp(key, "stats") == 0)
        {
            outRule.useTargetStats = (atoi(val) != 0);
        }
        else if (_stricmp(key, "preserveStats") == 0 || _stricmp(key, "keepStats") == 0)
        {
            outRule.useTargetStats = (atoi(val) == 0);
        }
    }

    return hasTarget && (hasSource || outRule.targetTblidx != 0);
}
