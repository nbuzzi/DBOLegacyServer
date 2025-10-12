#include "stdafx.h"
#include "CustomDropEvent.h"
#include "GameServer.h"
#include "NtlPacketGU.h"
#include "NtlStringW.h"
#include "ObjectManager.h"
#include "Monster.h"
#include "NtlRandom.h"
#include "ItemManager.h"
#include "ItemDrop.h"
#include "TableContainerManager.h"
#include "NtlAdmin.h"
#include "ItemTable.h"
#include "BuffManager.h"
#include "SkillTable.h"
#include "SystemEffectTable.h"
#include "NtlSkill.h"
#include "CharTitleTable.h"
#include "battle.h"
#include "calcs.h"
#include "CPlayer.h"
#include <string>

// Heal multiplier is configurable via settings; default initialized in Init().

// Helper: resolve a system effect name (e.g., "ACTIVE_STUN") to its numeric code.
// Returns -1 if not found. Case-insensitive.
static int ResolveSystemEffectCodeByName(const char* name)
{
	if (!name || !*name)
		return -1;
	for (int i = 0; i < (int)MAX_SYSTEM_EFFECT_CODE; ++i)
	{
		const char* s = NtlGetSystemEffectString((DWORD)i);
		if (s && _stricmp(s, name) == 0)
			return i;
	}
	return -1;
}

CCustomDropEvent::CCustomDropEvent()
{
	Init();
}

CCustomDropEvent::~CCustomDropEvent()
{
}

void CCustomDropEvent::Init()
{
	m_bOn = false;
	m_bVerbose = false;
	m_timeStart = 0;
	m_timeEnd = 0;
	m_dwNextUpdateTick = 0;
	m_dwNextTotemTick = 0;
	m_totemDefaultRadius = 30.0f;
	m_totemDefaultIntervalMs = 2000;
	m_totemHealMultiplier = 3.0f;
	m_totemBuffDurationOverrideMs = 0;
	m_mobDrops.clear();
	m_mobMods.clear();
	m_mobVisuals.clear();
	m_mobTotems.clear();
	m_activeTotems.clear();
	m_exceptDrops.clear();
	m_exceptMods.clear();
	m_exceptSpawns.clear();
	m_exceptBuffs.clear();
	m_exceptTitles.clear();
	m_exceptVisuals.clear();
	m_exceptTotems.clear();
	m_replaceMob.clear();
	m_exceptReplace.clear();
	m_eventSpawned.clear();
	m_cfgPath = ".\\config\\CustomDropEvent.cfg";
	m_allowChainSpawns = false; // default: prevent chain spawns
	// Debuff immunity defaults
	m_debuffImmuneEnabled = true; // default ON as requested
	m_blockDebuffEffects.clear();
	m_replaceUseTargetStats = true; // default ON so replacements feel authentic
	// Auto-start defaults
	m_autoStart = false;
	m_autoStartAllChannels = false;
	m_autoStartHours = 3;
	m_autoStartChannels.clear();
	m_autoStartPending = false;
	m_alwaysOn = false; // default: not always-on
	LoadConfigInternal(m_cfgPath.c_str());
	LoadLevelsSidecar(m_cfgPath.c_str());
}

bool CCustomDropEvent::ReloadConfig(const char* path)
{
	if (!path)
		path = m_cfgPath.c_str();
	// Remember the new path if provided so subsequent reloads use it
	if (path && *path)
		m_cfgPath = path;
	bool ok = LoadConfigInternal(m_cfgPath.c_str());
	LoadLevelsSidecar(m_cfgPath.c_str());
	return ok;
}

bool CCustomDropEvent::LoadConfigInternal(const char* path)
{
	m_mobDrops.clear();
	m_mobMods.clear();
	m_mobSpawns.clear();
	m_mobBuffs.clear();
	m_mobLevels.clear();
	m_mobTitles.clear();
	m_mobVisuals.clear();
	m_mobTotems.clear();
	m_eventSpawned.clear();
	m_exceptDrops.clear();
	m_exceptMods.clear();
	m_exceptSpawns.clear();
	m_exceptBuffs.clear();
	m_exceptTitles.clear();
	m_exceptVisuals.clear();
	m_exceptTotems.clear();
	m_replaceMob.clear();
	m_exceptReplace.clear();
	// Preserve current immunity default; allow settings section to override
	// but clear specific lists so reloading replaces them
	m_blockDebuffEffects.clear();
	// Reset auto-start flags; settings may re-enable
	m_autoStart = false;
	m_autoStartAllChannels = false;
	m_autoStartHours = 3;
	m_autoStartChannels.clear();
	m_autoStartPending = false;

	FILE* f = nullptr;
	errno_t e = fopen_s(&f, path, "rt");
	if (e != 0 || !f)
		return false;

	// Line formats:
	// mobId: itemId@rate, itemId@rate, ...
	// mobId modifiers: hp=1.5 physAtk=1.2 engAtk=1.0 physDef=1.1 engDef=1.0
	// mobId buffs: skillTblidx@durationMs, skillTblidx@durationMs, ... (duration optional; 0 or omitted uses skill default)
	// Lines starting with # are comments
	char line[1024];
	while (fgets(line, sizeof(line), f))
	{
		// trim leading spaces
		char* p = line;
		while (*p == ' ' || *p == '\t')
			++p;
		if (*p == '\0' || *p == '\n' || *p == '#')
			continue;

		// detect optional sections: "modifiers", "spawn"/"spawns", or "buffs"
		const char* modsKw = "modifiers";
		const char* spawnKw = "spawn";
		const char* spawnsKw = "spawns";
		const char* buffsKw = "buffs";
		const char* titlesKw = "titles";
		const char* visualsKw = "visuals";
		const char* totemKw = "totem";
		const char* replaceKw = "replace";
		const char* settingsKw = "settings"; // global settings for defaults
		bool isMods = false;
		bool isSpawn = false;
		bool isBuffs = false;
		bool isTitles = false;
		bool isVisuals = false;
		bool isTotem = false;
		bool isReplace = false;
		bool isSettings = false;
		bool isExcept = false;
		char* colon = strchr(p, ':');
		if (!colon)
			continue;
		*colon = '\0';
		// check for "<id> modifiers|spawn" key (supports "all" as wildcard)
		unsigned int mobId = 0;
		{
			// split p by spaces to detect keyword
			char* sp = p;
			while (*sp == ' ' || *sp == '\t')
				++sp;
			// find space
			char* sp2 = strchr(sp, ' ');
			if (sp2)
			{
				*sp2 = '\0';
				if (_stricmp(sp, "all") == 0)
					mobId = 0; // wildcard: apply to all mobs
				else
					mobId = (unsigned int)strtoul(sp, nullptr, 10);
				char* tail = sp2 + 1;
				while (*tail == ' ' || *tail == '\t')
					++tail;
				// allow optional second token 'except' => marks exclusion list for this section
				char* tail2 = strchr(tail, ' ');
				if (tail2)
				{
					*tail2 = '\0';
					char* extra = tail2 + 1;
					while (*extra == ' ' || *extra == '\t') ++extra;
					if (_stricmp(extra, "except") == 0)
						isExcept = true;
				}
				if (_stricmp(tail, modsKw) == 0)
					isMods = true;
				else if (_stricmp(tail, spawnKw) == 0 || _stricmp(tail, spawnsKw) == 0)
					isSpawn = true;
				else if (_stricmp(tail, buffsKw) == 0)
					isBuffs = true;
				else if (_stricmp(tail, titlesKw) == 0)
					isTitles = true;
				else if (_stricmp(tail, visualsKw) == 0)
					isVisuals = true;
				else if (_stricmp(tail, totemKw) == 0)
					isTotem = true;
				else if (_stricmp(tail, replaceKw) == 0)
					isReplace = true;
				else if (_stricmp(tail, settingsKw) == 0)
					isSettings = true;
				else if (_stricmp(tail, "except") == 0)
				{
					// special-case: "all except: ..." => drops exclusion
					isExcept = true;
				}
			}
			else
			{
				mobId = (unsigned int)strtoul(sp, nullptr, 10);
			}
		}

		// Handle exception lists: only allowed with mobId==0 (global)
		if (isExcept && mobId == 0)
		{
			// parse comma-separated IDs from RHS
			auto parseExcept = [&](std::unordered_set<unsigned int>& dst) {
				char* list = colon + 1;
				char* tok = strtok(list, ",\n\r");
				while (tok)
				{
					while (*tok == ' ' || *tok == '\t') ++tok;
					unsigned int id = (unsigned int)strtoul(tok, nullptr, 10);
					if (id)
						dst.insert(id);
					tok = strtok(nullptr, ",\n\r");
				}
				};
			if (isSpawn)          parseExcept(m_exceptSpawns);
			else if (isBuffs)     parseExcept(m_exceptBuffs);
			else if (isTitles)    parseExcept(m_exceptTitles);
			else if (isVisuals)   parseExcept(m_exceptVisuals);
			else if (isTotem)     parseExcept(m_exceptTotems);
			else if (isMods)      parseExcept(m_exceptMods);
			else if (isReplace)   parseExcept(m_exceptReplace);
			else                  parseExcept(m_exceptDrops); // default (drops)
			continue;
		}
		else if (isReplace)
		{
			// format: mobId replace: targetMobTblidx  (mobId can be 0/all for global)
			char* rhs = colon + 1;
			while (*rhs == ' ' || *rhs == '\t') ++rhs;
			unsigned int target = (unsigned int)strtoul(rhs, nullptr, 10);
			if (target != 0)
			{
				m_replaceMob[mobId] = target;
			}
			// Do not treat this line as a drop list
			continue;
		}

		if (isMods)
		{
			// parse key=value pairs separated by spaces
			Modifiers m;
			char* s = colon + 1;
			// tokenize by whitespace
			char* t = strtok(s, " \t\n\r");
			while (t)
			{
				char* eq = strchr(t, '=');
				if (eq)
				{
					*eq = '\0';
					const char* key = t;
					float val = (float)atof(eq + 1);
					if (_stricmp(key, "hp") == 0)
						m.hp = val;
					else if (_stricmp(key, "physAtk") == 0)
						m.physAtk = val;
					else if (_stricmp(key, "engAtk") == 0)
						m.engAtk = val;
					else if (_stricmp(key, "physDef") == 0)
						m.physDef = val;
					else if (_stricmp(key, "engDef") == 0)
						m.engDef = val;
					else if (_stricmp(key, "atkSpd") == 0)
						m.atkSpd = val;
					else if (_stricmp(key, "runSpd") == 0)
						m.runSpd = val;
					else if (_stricmp(key, "physCrit") == 0)
						m.physCrit = val;
					else if (_stricmp(key, "engCrit") == 0)
						m.engCrit = val;
					else if (_stricmp(key, "physCritDmg") == 0)
						m.physCritDmg = val;
					else if (_stricmp(key, "engCritDmg") == 0)
						m.engCritDmg = val;
					else if (_stricmp(key, "attackRate") == 0)
						m.attackRate = val;
					else if (_stricmp(key, "dodgeRate") == 0)
						m.dodgeRate = val;
					else if (_stricmp(key, "blockRate") == 0)
						m.blockRate = val;
					else if (_stricmp(key, "blockDmg") == 0)
						m.blockDmg = val;
					else if (_stricmp(key, "guardRate") == 0)
						m.guardRate = val;
					else if (_stricmp(key, "sizeRate") == 0)
						m.sizeRate = (int)val;
				}
				t = strtok(nullptr, " \t\n\r");
			}
			m_mobMods[mobId] = m;
		}
		else if (isSpawn)
		{
			// format: mobId spawn: mobId@ratexcount, mobId@ratexcount
			char* list = colon + 1;
			std::vector<SpawnEntry> entries;
			// split by comma
			char* tok = strtok(list, ",\n\r");
			while (tok)
			{
				// trim
				while (*tok == ' ' || *tok == '\t')
					++tok;
				unsigned int toSpawn = 0;
				float rate = 100.f;
				BYTE cnt = 1;
				char* at = strchr(tok, '@');
				if (at)
				{
					*at = '\0';
					toSpawn = (unsigned int)strtoul(tok, nullptr, 10);
					char* rx = at + 1;
					char* x = strchr(rx, 'x');
					if (x)
					{
						*x = '\0';
						rate = (float)atof(rx);
						cnt = (BYTE)strtoul(x + 1, nullptr, 10);
					}
					else
					{
						rate = (float)atof(rx);
					}
				}
				else
				{
					toSpawn = (unsigned int)strtoul(tok, nullptr, 10);
				}
				if (toSpawn != 0)
				{
					if (cnt == 0)
						cnt = 1;
					entries.push_back(SpawnEntry{ toSpawn, rate, cnt });
				}
				tok = strtok(nullptr, ",\n\r");
			}
			if (!entries.empty())
			{
				auto& dst = m_mobSpawns[mobId];
				dst.insert(dst.end(), entries.begin(), entries.end());
			}
		}
		else if (isBuffs)
		{
			// format: mobId buffs: skillTblidx@durationMs, skillTblidx@durationMs, ...
			char* list = colon + 1;
			std::vector<BuffEntry> entries;
			char* tok = strtok(list, ",\n\r");
			while (tok)
			{
				while (*tok == ' ' || *tok == '\t')
					++tok;
				unsigned int skillId = 0;
				DWORD durationMs = 0;
				char* at = strchr(tok, '@');
				if (at)
				{
					*at = '\0';
					skillId = (unsigned int)strtoul(tok, nullptr, 10);
					durationMs = (DWORD)strtoul(at + 1, nullptr, 10);
				}
				else
				{
					skillId = (unsigned int)strtoul(tok, nullptr, 10);
				}
				if (skillId != 0)
				{
					entries.push_back(BuffEntry{ skillId, durationMs });
				}
				tok = strtok(nullptr, ",\n\r");
			}
			if (!entries.empty())
			{
				auto& dst = m_mobBuffs[mobId];
				dst.insert(dst.end(), entries.begin(), entries.end());
			}
		}
		else if (isSettings)
		{
			// format: all settings: radius=50 interval=2000 healMul=3.5 duration=60000 immuneDebuff=1 debuffEffects=EFFECT1|EFFECT2|... replaceUseTargetStats=1
			// Only allowed with mobId 0/all
			if (mobId != 0)
				continue;
			char* s = colon + 1;
			char* t = strtok(s, " \t\n\r");
			while (t)
			{
				char* eq = strchr(t, '=');
				if (eq)
				{
					*eq = '\0';
					const char* key = t;
					const char* val = eq + 1;
					if (_stricmp(key, "radius") == 0)
						m_totemDefaultRadius = (float)atof(val);
					else if (_stricmp(key, "interval") == 0)
						m_totemDefaultIntervalMs = (DWORD)strtoul(val, nullptr, 10);
					else if (_stricmp(key, "healMul") == 0)
						m_totemHealMultiplier = (float)atof(val);
					else if (_stricmp(key, "duration") == 0 || _stricmp(key, "buffDuration") == 0)
						m_totemBuffDurationOverrideMs = (DWORD)strtoul(val, nullptr, 10);
					else if (_stricmp(key, "immuneDebuff") == 0 || _stricmp(key, "inmmuneDebuff") == 0)
						m_debuffImmuneEnabled = (atoi(val) != 0);
					else if (_stricmp(key, "debuffEffects") == 0)
					{
						// Parse a '|' or ',' separated list of effect names or numeric codes
						// Accept names matching eSYSTEM_EFFECT_CODE tokens (e.g., ACTIVE_POISON)
						// and numeric integers; unknown tokens are ignored.
						// Tokenize val by '|' and ','
						char buf[512];
						strncpy_s(buf, sizeof(buf), val, _TRUNCATE);
						char* tok = strtok(buf, "|, ");
						while (tok)
						{
							// Try numeric first
							bool added = false;
							char* endp = nullptr;
							long num = strtol(tok, &endp, 10);
							if (endp && *endp == '\0')
							{
								m_blockDebuffEffects.insert((int)num);
								added = true;
							}
							else
							{
								// Resolve any valid system effect name to its code
								int code = ResolveSystemEffectCodeByName(tok);
								if (code >= 0)
								{
									m_blockDebuffEffects.insert(code);
									added = true;
								}
							}
							if (!added)
							{
								ERR_LOG(LOG_GENERAL, "[CustomDropEvent] Unknown debuffEffects token '%s' (ignored)", tok);
							}
							tok = strtok(nullptr, "|, ");
						}
					}
					else if (_stricmp(key, "replaceUseTargetStats") == 0 || _stricmp(key, "replaceStats") == 0)
					{
						m_replaceUseTargetStats = (atoi(val) != 0);
					}
					else if (_stricmp(key, "autoStart") == 0)
					{
						m_autoStart = (atoi(val) != 0);
					}
					else if (_stricmp(key, "autoStartHours") == 0 || _stricmp(key, "autoHours") == 0)
					{
						int h = atoi(val); if (h <= 0) h = 1; if (h > 168) h = 168; m_autoStartHours = (BYTE)h;
					}
					else if (_stricmp(key, "autoStartChannels") == 0 || _stricmp(key, "autoChannels") == 0)
					{
						// parse CSV or '|' separated list of channel indices; special token 'all'
						m_autoStartChannels.clear();
						m_autoStartAllChannels = false;
						char buf[256]; strncpy_s(buf, sizeof(buf), val, _TRUNCATE);
						char* tok = strtok(buf, ",| ");
						while (tok)
						{
							while (*tok == ' ' || *tok == '\t') ++tok;
							if (*tok)
							{
								if (_stricmp(tok, "all") == 0)
								{
									m_autoStartAllChannels = true;
									m_autoStartChannels.clear();
									break;
								}
								int ch = atoi(tok);
								if (ch >= 0 && ch <= 50)
									m_autoStartChannels.insert((BYTE)ch);
							}
							tok = strtok(nullptr, ",| ");
						}
					}
					else if (_stricmp(key, "alwaysOn") == 0)
					{
						m_alwaysOn = (atoi(val) != 0);
					}
				}
				t = strtok(nullptr, " \t\n\r");
			}
			// If autoStart is on, schedule StartEvent on next tick to avoid init-order issues
			if (m_autoStart)
				m_autoStartPending = true;
		}
		else if (isTotem)
		{
			// format: mobId totem: beaconMob@lifeMs@radius@intervalMs: skill@dur[@period], skill@dur[@period]
			// second colon separates beacon spec from buff list
			char* rest = colon + 1;
			// find second colon
			char* colon2 = strchr(rest, ':');
			if (!colon2)
				continue;
			*colon2 = '\0';
			// parse beacon spec
			unsigned int beaconMob = 0; DWORD lifeMs = 0; float radius = 0.f; DWORD interval = 0;
			{
				char* tok = rest;
				while (*tok == ' ' || *tok == '\t') ++tok;
				char* at1 = strchr(tok, '@');
				if (at1)
				{
					*at1 = '\0';
					beaconMob = (unsigned int)strtoul(tok, nullptr, 10);
					char* at2 = strchr(at1 + 1, '@');
					if (at2)
					{
						*at2 = '\0';
						lifeMs = (DWORD)strtoul(at1 + 1, nullptr, 10);
						char* at3 = strchr(at2 + 1, '@');
						if (at3)
						{
							*at3 = '\0';
							radius = (float)atof(at2 + 1);
							interval = (DWORD)strtoul(at3 + 1, nullptr, 10);
						}
						else
						{
							radius = (float)atof(at2 + 1);
						}
					}
					else
					{
						lifeMs = (DWORD)strtoul(at1 + 1, nullptr, 10);
					}
				}
				else
				{
					beaconMob = (unsigned int)strtoul(tok, nullptr, 10);
				}
			}
			// parse buff list after colon2
			std::vector<BuffEntry> buffs;
			{
				char* list = colon2 + 1;
				char* btok = strtok(list, ",\n\r");
				while (btok)
				{
					while (*btok == ' ' || *btok == '\t') ++btok;
					if (*btok)
					{
						unsigned int skillId = 0; DWORD durationMs = 0; DWORD periodMs = 0;
						char* at = strchr(btok, '@');
						if (at)
						{
							*at = '\0';
							skillId = (unsigned int)strtoul(btok, nullptr, 10);
							char* p1 = at + 1;
							// support 'Xs' suffix for seconds
							bool secs = false;
							char* pEnd = p1;
							while (*pEnd && *pEnd != '@' && *pEnd != ',' && *pEnd != '\n' && *pEnd != '\r') ++pEnd;
							char saved = *pEnd; *pEnd = '\0';
							size_t len = strlen(p1);
							if (len > 0 && (p1[len - 1] == 's' || p1[len - 1] == 'S')) { secs = true; p1[len - 1] = '\0'; }
							durationMs = (DWORD)strtoul(p1, nullptr, 10);
							if (secs) durationMs *= 1000;
							*pEnd = saved;
							if (*pEnd == '@')
							{
								char* p2 = pEnd + 1;
								// optional period with 's' support
								bool secs2 = false;
								char* pEnd2 = p2;
								while (*pEnd2 && *pEnd2 != ',' && *pEnd2 != '\n' && *pEnd2 != '\r') ++pEnd2;
								char saved2 = *pEnd2; *pEnd2 = '\0';
								size_t len2 = strlen(p2);
								if (len2 > 0 && (p2[len2 - 1] == 's' || p2[len2 - 1] == 'S')) { secs2 = true; p2[len2 - 1] = '\0'; }
								periodMs = (DWORD)strtoul(p2, nullptr, 10);
								if (secs2) periodMs *= 1000;
								*pEnd2 = saved2;
							}
						}
						else
						{
							skillId = (unsigned int)strtoul(btok, nullptr, 10);
						}
						if (skillId)
							buffs.push_back(BuffEntry{ skillId, durationMs, periodMs });
					}
					btok = strtok(nullptr, ",\n\r");
				}
			}
			// Apply sensible defaults if some values omitted
			if (radius <= 0.f) radius = m_totemDefaultRadius; // HUGE default range
			if (interval == 0) interval = m_totemDefaultIntervalMs; // default pulse
			if (beaconMob && lifeMs > 0 && radius > 0.f && interval > 0 && !buffs.empty())
			{
				TotemRule r; r.beaconMobTblidx = beaconMob; r.lifeMs = lifeMs; r.radius = radius; r.intervalMs = interval; r.buffs = buffs;
				auto& dst = m_mobTotems[mobId];
				dst.push_back(r);
			}
		}
		else if (isTitles)
		{
			// format: mobId titles: titleTblidx, titleTblidx, ...
			char* list = colon + 1;
			std::vector<TBLIDX> entries;
			char* tok = strtok(list, ",\n\r");
			while (tok)
			{
				while (*tok == ' ' || *tok == '\t')
					++tok;
				unsigned int titleId = (unsigned int)strtoul(tok, nullptr, 10);
				if (titleId != 0)
				{
					entries.push_back((TBLIDX)titleId);
				}
				tok = strtok(nullptr, ",\n\r");
			}
			if (!entries.empty())
			{
				auto& dst = m_mobTitles[mobId];
				dst.insert(dst.end(), entries.begin(), entries.end());
			}
		}
		else if (isVisuals)
		{
			// format: mobId visuals: systemEffectTblidx[@intervalMs], systemEffectTblidx[@intervalMs], ...
			char* list = colon + 1;
			std::vector<VisualEntry> entries;
			char* tok = strtok(list, ",\n\r");
			while (tok)
			{
				while (*tok == ' ' || *tok == '\t')
					++tok;
				unsigned int effectTblidx = 0;
				DWORD intervalMs = 0;
				char* at = strchr(tok, '@');
				if (at)
				{
					*at = '\0';
					effectTblidx = (unsigned int)strtoul(tok, nullptr, 10);
					intervalMs = (DWORD)strtoul(at + 1, nullptr, 10);
				}
				else
				{
					effectTblidx = (unsigned int)strtoul(tok, nullptr, 10);
				}
				if (effectTblidx != 0)
				{
					entries.push_back(VisualEntry{ effectTblidx, intervalMs });
				}
				tok = strtok(nullptr, ",\n\r");
			}
			if (!entries.empty())
			{
				auto& dst = m_mobVisuals[mobId];
				dst.insert(dst.end(), entries.begin(), entries.end());
			}
		}
		else
		{
			char* list = colon + 1;
			std::vector<DropEntry> entries;
			// split by comma
			char* tok = strtok(list, ",\n\r");
			while (tok)
			{
				// trim
				while (*tok == ' ' || *tok == '\t')
					++tok;
				// token format itemId@ratexcount OR itemId@rate OR itemId (default rate=100, count=1)
				unsigned int itemId = 0;
				float rate = 100.f;
				BYTE count = 1;
				char* at = strchr(tok, '@');
				if (at)
				{
					*at = '\0';
					itemId = (unsigned int)strtoul(tok, nullptr, 10);
					char* rx = at + 1;
					char* x = strchr(rx, 'x');
					if (x)
					{
						*x = '\0';
						rate = (float)atof(rx);
						count = (BYTE)strtoul(x + 1, nullptr, 10);
						if (count == 0)
							count = 1;
					}
					else
					{
						rate = (float)atof(rx);
					}
				}
				else
				{
					itemId = (unsigned int)strtoul(tok, nullptr, 10);
				}
				if (itemId != 0)
				{
					DropEntry e;
					e.itemTblidx = itemId;
					e.rate = rate;
					// Apply configured title attribute effects, if any
					e.count = count;
					entries.push_back(e);
				}
				tok = strtok(nullptr, ",\n\r");
			}
			if (!entries.empty())
			{
				m_mobDrops[mobId] = entries;
			}
		}
	}

	fclose(f);
	return true;
}

void CCustomDropEvent::StartEvent(BYTE byHours /* = 3*/)
{
	if (m_bOn)
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	m_bOn = true;
	m_timeStart = app->GetTime();
	// If always-on mode, set end time to 0 (infinite)
	if (m_alwaysOn || byHours == 0)
		m_timeEnd = 0;
	else
		m_timeEnd = m_timeStart + (byHours * 3600);

	CNtlStringW msg;

	CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
	res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
	res->wMessageLengthInUnicode = (WORD)msg.Format(L"Custom Drop Event Started! Duration: %u Hours.", byHours);
	res->byDisplayType = SERVER_TEXT_EMERGENCY;
	NTL_SAFE_WCSCPY(res->awchMessage, msg.c_str());
	packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	g_pObjectManager->SendPacketToAll(&packet);
}

void CCustomDropEvent::TickProcess(DWORD dwTick)
{
	CGameServer* app = (CGameServer*)g_pApp;

	// Performance optimization: Skip custom drop event logic on non-event channels
	// if (!app->IsCustomDropEventChannel())
	// 	return;

	if (dwTick < m_dwNextUpdateTick)
		return;

	// If auto-start was requested by config, apply it once
	if (m_autoStartPending && !m_bOn)
	{
		ApplyAutoStartPolicy();
	}

	if (m_bOn)
	{
		// Skip time-based expiry if always-on mode or infinite duration (timeEnd == 0)
		if (m_timeEnd != 0 && !m_alwaysOn)
		{
			if (app->GetTime() >= m_timeEnd)
			{
				EndEvent();
			}
		}
	}

	m_dwNextUpdateTick = dwTick + 5000; // update every 5 seconds

	// Pulse active totems frequently (200ms granularity)
	if (dwTick >= m_dwNextTotemTick)
	{
		DWORD now = GetTickCount();
		if (!m_activeTotems.empty())
		{
			for (size_t i = 0; i < m_activeTotems.size(); )
			{
				ActiveTotem& t = m_activeTotems[i];
				bool remove = false;
				// Remove if beacon object no longer exists
				CCharacter* pBeacon = g_pObjectManager->GetChar(t.hBeacon);
				if (!pBeacon || !pBeacon->IsInitialized())
				{
					remove = true;
				}
				else if (now >= t.expireTick)
				{
					// expire and delete beacon
					g_pObjectManager->DestroyCharacter(pBeacon);
					remove = true;
				}
				else
				{
					// apply buffs to players in radius when each per-buff timer elapses
					CWorldCell* pCell = pBeacon->GetCurWorldCell();
					if (pCell)
					{
						CWorldCell::QUADPAGE page = pCell->GetCellQuadPage(pBeacon->GetCurLoc());
						for (int dir = CWorldCell::QUADDIR_SELF; dir <= CWorldCell::QUADDIR_VERTICAL; dir++)
						{
							CWorldCell* pSibling = pCell->GetQuadSibling(page, (CWorldCell::QUADDIR)dir);
							if (!pSibling) continue;
							CPlayer* pPlr = (CPlayer*)pSibling->GetObjectList()->GetFirst(OBJTYPE_PC);
							while (pPlr && pPlr->IsInitialized())
							{
								if (pBeacon->IsInRange(pPlr, t.radius))
								{
									for (size_t bi = 0; bi < t.buffs.size(); ++bi)
									{
										if (now < t.buffNextTicks[bi])
											continue;
										const BuffEntry& be = t.buffs[bi];
										sSKILL_TBLDAT* pSkill = (sSKILL_TBLDAT*)g_pTableContainer->GetSkillTable()->FindData(be.skillTblidx);
										if (!pSkill) continue;

										eSYSTEM_EFFECT_CODE aeEffectCode[NTL_MAX_EFFECT_IN_SKILL];
										for (int i2 = 0; i2 < NTL_MAX_EFFECT_IN_SKILL; ++i2)
										{
											if (pSkill->skill_Effect[i2] != INVALID_TBLIDX)
												aeEffectCode[i2] = g_pTableContainer->GetSystemEffectTable()->GetEffectCodeWithTblidx(pSkill->skill_Effect[i2]);
											else
												aeEffectCode[i2] = INVALID_SYSTEM_EFFECT_CODE;
										}
										sDBO_BUFF_PARAMETER aBuffParameter[NTL_MAX_EFFECT_IN_SKILL];
										for (int i3 = 0; i3 < NTL_MAX_EFFECT_IN_SKILL; ++i3)
										{
											aBuffParameter[i3].byBuffParameterType = DBO_BUFF_PARAMETER_TYPE_DEFAULT;
											// Seed parameters from table so non-HOT buffs have effect values
											float base = (float)pSkill->aSkill_Effect_Value[i3];
											aBuffParameter[i3].buffParameter.fParameter = base;
											aBuffParameter[i3].buffParameter.dwRemainValue = (DWORD)pSkill->aSkill_Effect_Value[i3];

											if (aeEffectCode[i3] == ACTIVE_HEAL_OVER_TIME || aeEffectCode[i3] == ACTIVE_EP_OVER_TIME)
											{
												aBuffParameter[i3].byBuffParameterType = DBO_BUFF_PARAMETER_TYPE_HOT;
												aBuffParameter[i3].buffParameter.dwRemainTime = (be.durationMs != 0 ? be.durationMs : pSkill->dwKeepTimeInMilliSecs);
												// Scale HOT/EP-Over-Time magnitude
												aBuffParameter[i3].buffParameter.fParameter = base * m_totemHealMultiplier;
												aBuffParameter[i3].buffParameter.dwRemainValue = (DWORD)(base * m_totemHealMultiplier);
											}
											else if (aeEffectCode[i3] == ACTIVE_BLEED || aeEffectCode[i3] == ACTIVE_POISON || aeEffectCode[i3] == ACTIVE_STOMACHACHE || aeEffectCode[i3] == ACTIVE_BURN)
											{
												aBuffParameter[i3].byBuffParameterType = DBO_BUFF_PARAMETER_TYPE_DOT;
												aBuffParameter[i3].buffParameter.dwRemainTime = (be.durationMs != 0 ? be.durationMs : pSkill->dwKeepTimeInMilliSecs);
											}
										}
										// Handle direct-heal immediately; don't register as a buff
										bool hasBuffableEffect = false;
										for (int i4 = 0; i4 < NTL_MAX_EFFECT_IN_SKILL; ++i4)
										{
											if (aeEffectCode[i4] == ACTIVE_DIRECT_HEAL)
											{
												float amt = 0.0f;
												CalcDirectHeal(pBeacon, pSkill, (BYTE)i4, amt);
												if (amt != 0.0f)
												{
													// allow heal multiplier to influence direct heals too
													float scaled = amt * m_totemHealMultiplier;
													pPlr->UpdateCurLP((int)scaled, true, false);
													pPlr->SendEffectAffected(g_pTableContainer->GetSystemEffectTable()->GetEffectTblidx(aeEffectCode[i4]), DBO_OBJECT_SOURCE_SKILL, pSkill->tblidx, scaled, 0.0f, pBeacon->GetID());
												}
												// Mark as consumed so we don't try to register it as a buff
												aeEffectCode[i4] = INVALID_SYSTEM_EFFECT_CODE;
											}
											if (aeEffectCode[i4] != INVALID_SYSTEM_EFFECT_CODE)
												hasBuffableEffect = true;
										}
										// Only register a buff if any effect remains (HoT/DoT/other sustained)
										if (hasBuffableEffect)
										{
											DWORD durMs = be.durationMs != 0 ? be.durationMs : pSkill->dwKeepTimeInMilliSecs;
											if (m_totemBuffDurationOverrideMs != 0)
												durMs = m_totemBuffDurationOverrideMs;
											if (durMs == 0) durMs = 30000;
											pPlr->GetBuffManager()->RegisterBuff(durMs, aeEffectCode, aBuffParameter, pBeacon->GetID(), BUFF_TYPE_BLESS, pSkill);
										}
										// schedule next tick for this buff
										DWORD per = be.periodMs ? be.periodMs : t.intervalMs;
										t.buffNextTicks[bi] = now + per;
									}
								}
								pPlr = (CPlayer*)pSibling->GetObjectList()->GetNext(pPlr->GetWorldCellObjectLinker());
							}
						}
					}
				}

				if (remove)
				{
					// Ensure beacon handle is no longer tracked
					m_eventSpawned.erase(t.hBeacon);
					m_activeTotems.erase(m_activeTotems.begin() + i);
				}
				else
				{
					++i;
				}
			}
		}
		m_dwNextTotemTick = dwTick + 200; // check totems every 200ms
	}
}

void CCustomDropEvent::ApplyAutoStartPolicy()
{
	if (!m_autoStart || m_bOn)
	{
		m_autoStartPending = false;
		return;
	}
	CGameServer* app = (CGameServer*)g_pApp;
	BYTE ch = app->m_config.byChannel; // channel index configured for this GameServer instance
	bool allowed = m_autoStartAllChannels || m_autoStartChannels.empty() || (m_autoStartChannels.find(ch) != m_autoStartChannels.end());
	if (allowed)
	{
		StartEvent(m_autoStartHours);
	}
	m_autoStartPending = false;
}

void CCustomDropEvent::Update(CMonster* pMob, CCharacter* pPlayer)
{
	// Performance optimization: Skip custom drop event logic on non-event channels
	// CGameServer* app = (CGameServer*)g_pApp;
	// if (!app->IsCustomDropEventChannel())
	// 	return;

	if (!pPlayer->GetCurWorld()) { m_eventSpawned.erase(pMob->GetID()); return; }

	// Allow CustomDropEvent in all worlds except competitive/PvP-only modes
	eGAMERULE_TYPE ruleType = pPlayer->GetCurWorld()->GetRuleType();
	switch (ruleType)
	{
	case GAMERULE_RANKBATTLE:
	case GAMERULE_MUDOSA:
	case GAMERULE_DOJO:
	case GAMERULE_MINORMATCH:
	case GAMERULE_MAJORMATCH:
	case GAMERULE_FINALMATCH:
	case GAMERULE_TEINKAICHIBUDOKAI:
		m_eventSpawned.erase(pMob->GetID());
		return; // skip PvP/competitive arenas entirely
	default:
		break; // all other rule types (normal, dungeons, raids, quests) are allowed
	}

	if (!m_bOn) { m_eventSpawned.erase(pMob->GetID()); return; }

	// Config-driven spawn logic: spawn configured mobs on kill
	// Skip for mobs spawned by this event to avoid infinite chains (unless explicitly allowed)
	if (m_allowChainSpawns || m_eventSpawned.find(pMob->GetID()) == m_eventSpawned.end())
	{
		int levelGap = abs((int)pPlayer->GetLevel() - (int)pMob->GetLevel());
		// Gather specific and global (all) spawn lists
		std::vector<SpawnEntry> spawns;
		auto itS = m_mobSpawns.find(pMob->GetTblidx());
		if (itS != m_mobSpawns.end())
		{
			spawns.insert(spawns.end(), itS->second.begin(), itS->second.end());
		}
		auto itAll = m_mobSpawns.find(0);
		if (itAll != m_mobSpawns.end() && m_exceptSpawns.find(pMob->GetTblidx()) == m_exceptSpawns.end())
		{
			spawns.insert(spawns.end(), itAll->second.begin(), itAll->second.end());
		}
		if (!spawns.empty())
		{
			for (const SpawnEntry& se : spawns)
			{
				if (se.mobTblidx == INVALID_TBLIDX)
					continue;
				// Enforce level-gap gating only for sub-100% rates
				if (se.rate < 100.f && levelGap > 10)
					continue;
				if (se.rate >= 100.f || Dbo_CheckProbabilityF(se.rate))
				{
					sMOB_TBLDAT* pTbldat = (sMOB_TBLDAT*)g_pTableContainer->GetMobTable()->FindData(se.mobTblidx);
					if (!pTbldat)
						continue;
					BYTE cnt = se.count ? se.count : 1;
					// capture base position once; apply slight random offsets per spawn to avoid overlap
					sVECTOR3 basePos; pMob->GetCurLoc().CopyTo(basePos);
					for (BYTE i = 0; i < cnt; ++i)
					{
						sMOB_DATA data;
						InitMobData(data);
						data.worldID = pMob->GetWorldID();
						data.worldtblidx = pMob->GetWorldTblidx();
						data.tblidx = pTbldat->tblidx;
						// slightly offset each spawn to prevent collision/merge at exact same coordinates
						sVECTOR3 pos = basePos;
						pos.x += RandomRangeF(-2.0f, 2.0f);
						pos.z += RandomRangeF(-2.0f, 2.0f);
						data.vCurLoc = pos;
						data.vSpawnLoc = pos;
						pMob->GetCurDir().CopyTo(data.vCurDir);
						pMob->GetCurDir().CopyTo(data.vSpawnDir);
						data.actionpatternTblIdx = 1;

						if (CMonster* pNewMob = (CMonster*)g_pObjectManager->CreateCharacter(OBJTYPE_MOB))
						{
							if (pNewMob->CreateDataAndSpawn(data, pTbldat))
							{
								// If a specific level is configured for this mob, set it now
								auto itLvl = m_mobLevels.find(se.mobTblidx);
								if (itLvl != m_mobLevels.end() && itLvl->second >= 1)
								{
									pNewMob->SetLevel(itLvl->second);
								}
								// Dynamically scale spawned mobs based on configured spawn level if set; otherwise killer mob level
								{
									BYTE baseLvl = pMob->GetLevel();
									if (itLvl != m_mobLevels.end() && itLvl->second >= 1)
										baseLvl = itLvl->second;
									CCharacterAtt* att = pNewMob->GetCharAtt();
									if (att)
									{
										float hpMul = 1.0f + (baseLvl * 0.015f);  // +1.5% per level
										float atkMul = 1.0f + (baseLvl * 0.010f); // +1.0% per level
										float defMul = 1.0f + (baseLvl * 0.010f); // +1.0% per level

										DWORD maxLp = att->GetMaxLP();
										DWORD newMaxLp = (DWORD)((float)maxLp * hpMul);
										att->SetMaxLP((int)newMaxLp);
										pNewMob->SetCurLP((int)newMaxLp);

										WORD physAtk = att->GetPhysicalOffence();
										WORD energyAtk = att->GetEnergyOffence();
										att->SetPhysicalOffence((WORD)((float)physAtk * atkMul));
										att->SetEnergyOffence((WORD)((float)energyAtk * atkMul));

										WORD physDef = att->GetPhysicalDefence();
										WORD energyDef = att->GetEnergyDefence();
										att->SetPhysicalDefence((WORD)((float)physDef * defMul));
										float targetED = (float)energyDef * defMul;
										float diffED = targetED - (float)energyDef;
										if (diffED > 0.0f)
											att->CalculateEnergyDefence(diffED, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
										else if (diffED < 0.0f)
											att->CalculateEnergyDefence(-diffED, SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);
									}
								}
								pNewMob->SetStandAlone(true);
								// Track event-spawned mob handle to prevent chain triggers
								m_eventSpawned.insert(pNewMob->GetID());
								// Apply configured buffs, if any
								ApplyBuffs(pNewMob);
								// Apply any configured title attribute effects, if any
								ApplyTitles(pNewMob);
								// Broadcast any configured visual effects
								ApplyVisuals(pNewMob);
							}
						}
					}
				}
			}
		}
	}

	// Special case: Devil King Piccolo guaranteed 100 Aztec Coins (315)
	// if (pMob->GetTblidx() == 46661101)
	// {
	// 	if (g_pItemManager->IsValidSingleDropIdx(315))
	// 	{
	// 		CreateStackedDrop(pMob, pPlayer, 315, 100);
	// 	}
	// }

	// Gather specific and global (all) drop lists
	std::vector<DropEntry> list;
	auto itD = m_mobDrops.find(pMob->GetTblidx());
	if (itD != m_mobDrops.end())
	{
		list.insert(list.end(), itD->second.begin(), itD->second.end());
	}
	auto itDAll = m_mobDrops.find(0);
	if (itDAll != m_mobDrops.end() && m_exceptDrops.find(pMob->GetTblidx()) == m_exceptDrops.end())
	{
		list.insert(list.end(), itDAll->second.begin(), itDAll->second.end());
	}
	// Handle totem spawning according to rules (independent of drops)
	// Skip for mobs spawned by this event to avoid chains (unless allowed)
	if (m_allowChainSpawns || m_eventSpawned.find(pMob->GetID()) == m_eventSpawned.end())
	{
		std::vector<TotemRule> rules;
		auto itT = m_mobTotems.find(pMob->GetTblidx());
		if (itT != m_mobTotems.end())
			rules.insert(rules.end(), itT->second.begin(), itT->second.end());
		auto itTAll = m_mobTotems.find(0);
		if (itTAll != m_mobTotems.end() && m_exceptTotems.find(pMob->GetTblidx()) == m_exceptTotems.end())
			rules.insert(rules.end(), itTAll->second.begin(), itTAll->second.end());
		for (const TotemRule& r : rules)
		{
			sMOB_TBLDAT* pTbldat = (sMOB_TBLDAT*)g_pTableContainer->GetMobTable()->FindData(r.beaconMobTblidx);
			if (!pTbldat) continue;

			sMOB_DATA data; InitMobData(data);
			data.worldID = pMob->GetWorldID();
			data.worldtblidx = pMob->GetWorldTblidx();
			data.tblidx = pTbldat->tblidx;
			pMob->GetCurLoc().CopyTo(data.vCurLoc);
			pMob->GetCurLoc().CopyTo(data.vSpawnLoc);
			pMob->GetCurDir().CopyTo(data.vCurDir);
			pMob->GetCurDir().CopyTo(data.vSpawnDir);
			data.actionpatternTblIdx = 1;

			if (CMonster* pBeacon = (CMonster*)g_pObjectManager->CreateCharacter(OBJTYPE_MOB))
			{
				if (pBeacon->CreateDataAndSpawn(data, pTbldat))
				{
					pBeacon->SetStandAlone(true);
					// Track event-spawned beacon
					m_eventSpawned.insert(pBeacon->GetID());
					ActiveTotem t; t.hBeacon = pBeacon->GetID(); t.radius = r.radius; t.intervalMs = r.intervalMs; t.buffs = r.buffs;
					DWORD now = GetTickCount();
					t.expireTick = now + r.lifeMs;
					t.nextPulseTick = now + r.intervalMs;
					t.buffNextTicks.resize(t.buffs.size());
					for (size_t bi = 0; bi < t.buffs.size(); ++bi)
					{
						DWORD per = t.buffs[bi].periodMs ? t.buffs[bi].periodMs : r.intervalMs;
						t.buffNextTicks[bi] = now + per;
					}
					m_activeTotems.push_back(t);
				}
			}
		}
	}
	if (!list.empty())
	{
		for (const DropEntry& d : list)
		{
			if (d.itemTblidx == 0)
				continue;
			if (g_pItemManager->IsValidSingleDropIdx(d.itemTblidx) == false)
				continue;
			if (d.rate >= 100.f || Dbo_CheckProbabilityF(d.rate))
			{
				BYTE cnt = d.count ? d.count : 1;
				if (cnt > 1)
					CreateStackedDrop(pMob, pPlayer, d.itemTblidx, cnt);
				else
					CreateSingleDrop(pMob, pPlayer, d.itemTblidx);
			}
		}
	}
	// After processing, forget this mob so set doesn't grow without bound
	m_eventSpawned.erase(pMob->GetID());
}

// Helper: load levels from a JSON sidecar next to the cfg
// JSON format: { "3131102": 50, "46661101": 70 }
bool CCustomDropEvent::LoadLevelsSidecar(const char* cfgPath)
{
	// Build sidecar path by replacing extension with .levels.json
	char sidecar[1024] = { 0 };
	strncpy_s(sidecar, sizeof(sidecar), cfgPath, _TRUNCATE);
	char* dot = strrchr(sidecar, '.');
	if (!dot)
		return false;
	size_t remain = sizeof(sidecar) - (size_t)(dot - sidecar);
	if (remain == 0)
		return false;
	strcpy_s(dot, remain, ".levels.json");

	FILE* f = nullptr;
	if (fopen_s(&f, sidecar, "rt") != 0 || !f)
		return false; // optional

	// Very small, permissive JSON reader for flat { "id": level, ... }
	// We do not bring a JSON library here to keep changes minimal.
	m_mobLevels.clear();
	char buf[2048];
	std::string content;
	while (fgets(buf, sizeof(buf), f))
		content.append(buf);
	fclose(f);

	const char* s = content.c_str();
	// find opening brace
	const char* p = strchr(s, '{');
	if (!p) return false;
	++p;
	while (*p)
	{
		while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' || *p == ',')) ++p;
		if (!*p || *p == '}') break;
		if (*p != '"') { ++p; continue; }
		++p;
		// read key until next quote
		char key[64] = { 0 };
		int ki = 0;
		while (*p && *p != '"' && ki < 63) key[ki++] = *p++;
		key[ki] = '\0';
		if (*p != '"') break;
		++p;
		// skip to colon
		while (*p && *p != ':') ++p;
		if (*p != ':') break;
		++p;
		// read value (integer)
		while (*p == ' ' || *p == '\t') ++p;
		char valbuf[16] = { 0 };
		int vi = 0;
		while (*p && ((*p >= '0' && *p <= '9'))) { if (vi < 15) valbuf[vi++] = *p; ++p; }
		valbuf[vi] = '\0';
		unsigned int id = (unsigned int)strtoul(key, nullptr, 10);
		int lvl = atoi(valbuf);
		if (id > 0 && lvl >= 1 && lvl <= 255)
			m_mobLevels[id] = (BYTE)lvl;
		// find next comma or closing brace
		while (*p && *p != ',' && *p != '}') ++p;
		if (*p == ',') ++p;
	}
	return !m_mobLevels.empty();
}

void CCustomDropEvent::EndEvent()
{
	if (!m_bOn)
		return;

	m_bOn = false;
	m_timeStart = 0;
	m_timeEnd = 0;
	m_dwNextUpdateTick = 0;
	// Cleanup active totems
	if (!m_activeTotems.empty())
	{
		for (ActiveTotem& t : m_activeTotems)
		{
			if (CCharacter* p = g_pObjectManager->GetChar(t.hBeacon))
				g_pObjectManager->DestroyCharacter(p);
			// Ensure we drop tracking for this beacon
			m_eventSpawned.erase(t.hBeacon);
		}
		m_activeTotems.clear();
	}

	// Clear any remaining tracked handles for safety
	m_eventSpawned.clear();

	CNtlStringW msg;
	CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
	res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
	res->wMessageLengthInUnicode = (WORD)msg.Format(L"Custom Drop Event ended!");
	res->byDisplayType = SERVER_TEXT_EMERGENCY;
	NTL_SAFE_WCSCPY(res->awchMessage, msg.c_str());
	packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	g_pObjectManager->SendPacketToAll(&packet);
}

void CCustomDropEvent::LoadEvent(HSESSION hSession)
{
	if (!m_bOn)
		return;

	CNtlStringW msg;
	CNtlPacket packetMsg(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	sGU_SYSTEM_DISPLAY_TEXT* resMsg = (sGU_SYSTEM_DISPLAY_TEXT*)packetMsg.GetPacketData();
	resMsg->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
	resMsg->byDisplayType = SERVER_TEXT_EMERGENCY;
	resMsg->wMessageLengthInUnicode = (WORD)msg.Format(L"Custom Drop Event is currently running!");
	NTL_SAFE_WCSCPY(resMsg->awchMessage, msg.c_str());
	g_pApp->Send(hSession, &packetMsg);
}

void CCustomDropEvent::CreateSingleDrop(CMonster* pMob, CCharacter* pPlayer, unsigned int dropId)
{
	ERR_LOG(LOG_GENERAL, "[DropTrace] CustomDropEvent CreateSingleDrop mob=%u player=%u item=%u", pMob->GetTblidx(), pPlayer->GetID(), dropId);
	CItemDrop* pDrop = g_pItemManager->CreateSingleDrop(100.f, dropId);
	if (pDrop)
	{
		sVECTOR3 pos;
		pos.x = pMob->GetCurLoc().x + RandomRangeF(-2.0f, 2.0f);
		pos.y = pMob->GetCurLoc().y;
		pos.z = pMob->GetCurLoc().z + RandomRangeF(-2.0f, 2.0f);

		pDrop->SetNeedToIdentify(false);
		pDrop->SetOwnership(pPlayer->GetID(), pPlayer->GetPartyID());
		pDrop->StartDestroyEvent();
		pDrop->AddToGround(pMob->GetWorldID(), pos);
	}
}

void CCustomDropEvent::CreateStackedDrop(CMonster* pMob, CCharacter* pPlayer, unsigned int dropId, BYTE count)
{
	if (count <= 1)
	{
		CreateSingleDrop(pMob, pPlayer, dropId);
		return;
	}

	sITEM_TBLDAT* pItemData = (sITEM_TBLDAT*)g_pTableContainer->GetItemTable()->FindData(dropId);
	if (!pItemData)
	{
		CreateSingleDrop(pMob, pPlayer, dropId);
		return;
	}

	if (pItemData->byMax_Stack > 1)
	{
		BYTE remaining = count;
		BYTE maxStack = pItemData->byMax_Stack;
		BYTE dropsSpawned = 0;
		while (remaining > 0 && dropsSpawned < 10) // safety cap to avoid spam
		{
			BYTE stack = remaining > maxStack ? maxStack : remaining;
			CItemDrop* pDrop = g_pItemManager->CreateSingleDrop(100.f, dropId);
			if (pDrop)
			{
				sVECTOR3 pos;
				pos.x = pMob->GetCurLoc().x + RandomRangeF(-2.0f, 2.0f);
				pos.y = pMob->GetCurLoc().y;
				pos.z = pMob->GetCurLoc().z + RandomRangeF(-2.0f, 2.0f);

				pDrop->SetNeedToIdentify(false);
				pDrop->SetOwnership(pPlayer->GetID(), pPlayer->GetPartyID());
				pDrop->SetStackCount(stack);
				pDrop->StartDestroyEvent();
				pDrop->AddToGround(pMob->GetWorldID(), pos);
			}
			remaining -= stack;
			dropsSpawned++;
		}
	}
	else
	{
		BYTE toSpawn = count;
		if (toSpawn > 100)
			toSpawn = 100; // cap safety
		for (BYTE i = 0; i < toSpawn; ++i)
		{
			CreateSingleDrop(pMob, pPlayer, dropId);
		}
	}
}

void CCustomDropEvent::ApplyModifiers(CMonster* pMob)
{
	if (!m_bOn)
		return;
	// Merge global (id=0) modifiers with per-mob, multiplicatively. sizeRate from specific overrides if set; otherwise use global if set.
	Modifiers m; // start with identity
	auto itAll = m_mobMods.find(0);
	if (itAll != m_mobMods.end() && m_exceptMods.find(pMob->GetTblidx()) == m_exceptMods.end())
	{
		const Modifiers& g = itAll->second;
		m.hp *= g.hp; m.physAtk *= g.physAtk; m.engAtk *= g.engAtk; m.physDef *= g.physDef; m.engDef *= g.engDef;
		m.atkSpd *= g.atkSpd; m.runSpd *= g.runSpd; m.physCrit *= g.physCrit; m.engCrit *= g.engCrit;
		m.physCritDmg *= g.physCritDmg; m.engCritDmg *= g.engCritDmg; m.attackRate *= g.attackRate; m.dodgeRate *= g.dodgeRate;
		m.blockRate *= g.blockRate; m.blockDmg *= g.blockDmg; m.guardRate *= g.guardRate;
		if (g.sizeRate > 0) m.sizeRate = g.sizeRate;
	}
	auto it = m_mobMods.find(pMob->GetTblidx());
	if (it != m_mobMods.end())
	{
		const Modifiers& s = it->second;
		m.hp *= s.hp; m.physAtk *= s.physAtk; m.engAtk *= s.engAtk; m.physDef *= s.physDef; m.engDef *= s.engDef;
		m.atkSpd *= s.atkSpd; m.runSpd *= s.runSpd; m.physCrit *= s.physCrit; m.engCrit *= s.engCrit;
		m.physCritDmg *= s.physCritDmg; m.engCritDmg *= s.engCritDmg; m.attackRate *= s.attackRate; m.dodgeRate *= s.dodgeRate;
		m.blockRate *= s.blockRate; m.blockDmg *= s.blockDmg; m.guardRate *= s.guardRate;
		if (s.sizeRate > 0) m.sizeRate = s.sizeRate;
	}
	if (itAll == m_mobMods.end() && it == m_mobMods.end())
		return;
	if (m.IsIdentity())
		return;

	CCharacterAtt* att = pMob->GetCharAtt();
	if (!att)
		return;

	// Max LP
	DWORD maxLp = att->GetMaxLP();
	DWORD newMaxLp = (DWORD)((float)maxLp * m.hp);
	att->SetMaxLP((int)newMaxLp);
	pMob->SetCurLP((int)newMaxLp);

	// Physical/Energy Offence
	WORD physAtk = att->GetPhysicalOffence();
	WORD energyAtk = att->GetEnergyOffence();
	att->SetPhysicalOffence((WORD)((float)physAtk * m.physAtk));
	att->SetEnergyOffence((WORD)((float)energyAtk * m.engAtk));

	// Physical/Energy Defence
	WORD physDef = att->GetPhysicalDefence();
	WORD energyDef = att->GetEnergyDefence();
	att->SetPhysicalDefence((WORD)((float)physDef * m.physDef));
	// Apply energy defence via calculator (no public setter provided)
	{
		float target = (float)energyDef * m.engDef;
		float diff = target - (float)energyDef;
		if (diff > 0.0f)
			att->CalculateEnergyDefence(diff, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
		else if (diff < 0.0f)
			att->CalculateEnergyDefence(-diff, SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);
	}

	// Attack Speed Rate (lower is faster) — scale around current value
	if (m.atkSpd != 1.f)
	{
		WORD cur = att->GetAttackSpeedRate();
		WORD target = (WORD)((float)cur * m.atkSpd);
		if (target > cur)
			att->CalculateAttackSpeedRate((float)(target - cur), SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
		else if (target < cur)
			att->CalculateAttackSpeedRate((float)(cur - target), SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);
	}

	// Run Speed
	if (m.runSpd != 1.f)
	{
		float base = att->GetRunSpeed();
		float target = base * m.runSpd;
		float diff = target - base;
		if (diff > 0.0f)
			att->CalculateRunSpeed(diff, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
		else if (diff < 0.0f)
			att->CalculateRunSpeed(-diff, SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);
	}

	// Rates and crits
	if (m.attackRate != 1.f)
	{
		float cur = (float)att->GetAttackRate();
		float target = cur * m.attackRate;
		float diff = target - cur;
		if (diff > 0.0f)
			att->CalculateAttackRate(diff, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
		else if (diff < 0.0f)
			att->CalculateAttackRate(-diff, SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);
	}
	if (m.dodgeRate != 1.f)
	{
		float cur = (float)att->GetDodgeRate();
		float target = cur * m.dodgeRate;
		float diff = target - cur;
		if (diff > 0.0f)
			att->CalculateDodgeRate(diff, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
		else if (diff < 0.0f)
			att->CalculateDodgeRate(-diff, SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);
	}
	if (m.blockRate != 1.f)
	{
		float cur = (float)att->GetBlockRate();
		float target = cur * m.blockRate;
		float diff = target - cur;
		if (diff > 0.0f)
			att->CalculateBlockRate(diff, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
		else if (diff < 0.0f)
			att->CalculateBlockRate(-diff, SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);
	}
	if (m.blockDmg != 1.f)
	{
		float cur = (float)att->GetBlockDamageRate();
		float target = cur * m.blockDmg;
		float diff = target - cur;
		if (diff > 0.0f)
			att->CalculateBlockDamageRate(diff, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
		else if (diff < 0.0f)
			att->CalculateBlockDamageRate(-diff, SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);
	}
	if (m.guardRate != 1.f)
	{
		float cur = (float)att->GetGuardRate();
		float target = cur * m.guardRate;
		float diff = target - cur;
		if (diff > 0.0f)
			att->CalculateGuardRate(diff, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
		else if (diff < 0.0f)
			att->CalculateGuardRate(-diff, SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);
	}

	if (m.physCrit != 1.f)
	{
		float cur = (float)att->GetPhysicalCriticalRate();
		float target = cur * m.physCrit;
		float diff = target - cur;
		if (diff > 0.0f)
			att->CalculatePhysicalCriticalRate(diff, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
		else if (diff < 0.0f)
			att->CalculatePhysicalCriticalRate(-diff, SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);
	}
	if (m.engCrit != 1.f)
	{
		float cur = (float)att->GetEnergyCriticalRate();
		float target = cur * m.engCrit;
		float diff = target - cur;
		if (diff > 0.0f)
			att->CalculateEnergyCriticalRate(diff, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
		else if (diff < 0.0f)
			att->CalculateEnergyCriticalRate(-diff, SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);
	}

	// Crit damage rates are floats
	if (m.physCritDmg != 1.f)
	{
		float cur = att->GetPhysicalCriticalDamageRate();
		float target = cur * m.physCritDmg;
		float diff = target - cur;
		if (diff > 0.0f)
			att->CalculatePhysicalCriticalDamageRate(diff, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
		else if (diff < 0.0f)
			att->CalculatePhysicalCriticalDamageRate(-diff, SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);
	}
	if (m.engCritDmg != 1.f)
	{
		float cur = att->GetEnergyCriticalDamageRate();
		float target = cur * m.engCritDmg;
		float diff = target - cur;
		if (diff > 0.0f)
			att->CalculateEnergyCriticalDamageRate(diff, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
		else if (diff < 0.0f)
			att->CalculateEnergyCriticalDamageRate(-diff, SYSTEM_EFFECT_APPLY_TYPE_VALUE, false);
	}

	// Size rate (must notify clients). Valid typical range: 1..20; default 10. Apply only if set.
	if (m.sizeRate > 0)
	{
		int rate = m.sizeRate;
		if (rate < 1)
			rate = 1;
		if (rate > 250)
			rate = 250; // hard cap for safety
		pMob->UpdateSizeRate((BYTE)rate);
	}

	// Mark mob as debuff-immune to avoid recalculation via curse-type effects
	if (m_debuffImmuneEnabled)
		pMob->SetEventDebuffImmune(true);

	// Ensure event-modified mobs are actually fightable: clear lingering script conditions that block combat
	// Common offenders observed: ATTACK_DISALLOW, INVINCIBLE, CLICK_DISABLE, CANT_BE_TARGETTED
	{
		auto sm = pMob->GetStateManager();
		bool cleared = false;
		if (sm->IsCharCondition(CHARCOND_ATTACK_DISALLOW))
		{
			sm->RemoveConditionState(CHARCOND_ATTACK_DISALLOW, NULL, true);
			cleared = true;
		}
		if (sm->IsCharCondition(CHARCOND_CANT_BE_TARGETTED))
		{
			sm->RemoveConditionState(CHARCOND_CANT_BE_TARGETTED, NULL, true);
			cleared = true;
		}
		if (sm->IsCharCondition(CHARCOND_INVINCIBLE))
		{
			sm->RemoveConditionState(CHARCOND_INVINCIBLE, NULL, true);
			cleared = true;
		}
		if (sm->IsCharCondition(CHARCOND_CLICK_DISABLE))
		{
			sm->RemoveConditionState(CHARCOND_CLICK_DISABLE, NULL, true);
			cleared = true;
		}
		if (cleared)
		{
			ERR_LOG(LOG_GENERAL, "[CustomDropEvent] Cleared combat-blocking flags on mob %u (world %u)", (unsigned)pMob->GetTblidx(), (unsigned)pMob->GetWorldID());
		}
	}
}

void CCustomDropEvent::ApplyBuffs(CMonster* pMob)
{
	if (!m_bOn)
		return;

	// Gather buffs for this mob and global buffs
	std::vector<BuffEntry> buffs;
	auto it = m_mobBuffs.find(pMob->GetTblidx());
	if (it != m_mobBuffs.end())
		buffs.insert(buffs.end(), it->second.begin(), it->second.end());
	auto itAll = m_mobBuffs.find(0);
	if (itAll != m_mobBuffs.end() && m_exceptBuffs.find(pMob->GetTblidx()) == m_exceptBuffs.end())
		buffs.insert(buffs.end(), itAll->second.begin(), itAll->second.end());

	if (buffs.empty())
		return;

	for (const BuffEntry& be : buffs)
	{
		sSKILL_TBLDAT* pSkill = (sSKILL_TBLDAT*)g_pTableContainer->GetSkillTable()->FindData(be.skillTblidx);
		if (!pSkill)
			continue;

		eSYSTEM_EFFECT_CODE aeEffectCode[NTL_MAX_EFFECT_IN_SKILL];
		for (int i = 0; i < NTL_MAX_EFFECT_IN_SKILL; ++i)
		{
			if (pSkill->skill_Effect[i] != INVALID_TBLIDX)
				aeEffectCode[i] = g_pTableContainer->GetSystemEffectTable()->GetEffectCodeWithTblidx(pSkill->skill_Effect[i]);
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
			dwDurationInMs = 30000; // fallback safety: 30s

		pMob->GetBuffManager()->RegisterBuff(dwDurationInMs, aeEffectCode, aBuffParameter, INVALID_HOBJECT, BUFF_TYPE_BLESS, pSkill);
	}
}

void CCustomDropEvent::ApplyTitles(CMonster* pMob)
{
	if (!m_bOn)
		return;

	std::vector<TBLIDX> titles;
	auto it = m_mobTitles.find(pMob->GetTblidx());
	if (it != m_mobTitles.end())
		titles.insert(titles.end(), it->second.begin(), it->second.end());
	auto itAll = m_mobTitles.find(0);
	if (itAll != m_mobTitles.end() && m_exceptTitles.find(pMob->GetTblidx()) == m_exceptTitles.end())
		titles.insert(titles.end(), itAll->second.begin(), itAll->second.end());

	if (titles.empty())
		return;

	CCharacterAtt* att = pMob->GetCharAtt();
	if (!att)
		return;

	for (TBLIDX titleIdx : titles)
	{
		sCHARTITLE_TBLDAT* pTitle = (sCHARTITLE_TBLDAT*)g_pTableContainer->GetCharTitleTable()->FindData(titleIdx);
		if (!pTitle)
			continue;
		for (BYTE i = 0; i < NTL_MAX_CHAR_TITLE_EFFECT; i++)
		{
			if (pTitle->atblSystem_Effect_Index[i] == INVALID_TBLIDX)
				continue;
			eSYSTEM_EFFECT_CODE effectcode = g_pTableContainer->GetSystemEffectTable()->GetEffectCodeWithTblidx(pTitle->atblSystem_Effect_Index[i]);
			if (effectcode == INVALID_SYSTEM_EFFECT_CODE)
				continue;
			Dbo_SetAvatarAttributeValue(att, effectcode, (float)pTitle->abySystem_Effect_Value[i], pTitle->abySystem_Effect_Type[i]);
		}
	}
}

void CCustomDropEvent::ApplyVisuals(CMonster* pMob)
{
	if (!m_bOn)
		return;

	std::vector<VisualEntry> visuals;
	auto it = m_mobVisuals.find(pMob->GetTblidx());
	if (it != m_mobVisuals.end())
		visuals.insert(visuals.end(), it->second.begin(), it->second.end());
	auto itAll = m_mobVisuals.find(0);
	if (itAll != m_mobVisuals.end() && m_exceptVisuals.find(pMob->GetTblidx()) == m_exceptVisuals.end())
		visuals.insert(visuals.end(), itAll->second.begin(), itAll->second.end());

	if (visuals.empty())
		return;

	for (const VisualEntry& ve : visuals)
	{
		// Validate effect exists; client expects system effect tblidx
		sSYSTEM_EFFECT_TBLDAT* pEff = (sSYSTEM_EFFECT_TBLDAT*)g_pTableContainer->GetSystemEffectTable()->FindData(ve.effectTblidx);
		if (!pEff)
			continue;
		// Broadcast GU_EFFECT_AFFECTED with source type SKILL and source tblidx 0 (none). Arguments left as 0.
		pMob->SendEffectAffected(ve.effectTblidx, DBO_OBJECT_SOURCE_SKILL, 0, 0.0f, 0.0f, INVALID_HOBJECT);
	}
}
