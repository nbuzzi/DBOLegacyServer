#include "stdafx.h"
#include "HelperNpcManager.h"
#include "GameServer.h"
#include "CPlayer.h"
#include "Party.h"
#include "World.h"
#include "Npc.h"
#include "Monster.h"
#include "ObjectManager.h"
#include "BotAiController.h"
#include "TableContainerManager.h"
#include "NtlIniFile.h"
#include "ObjectMsg.h"
#include "NtlResultCode.h"
#include "Npc.h"
#include <string>
#include <sstream>
#include <cctype>
#include <algorithm>
#include <cstdarg>
#include <cstdio>

// simple string trim helper (both ends)
static inline void str_trim(std::string& x)
{
	auto notspace = [](int c) { return !std::isspace(c); };
	x.erase(x.begin(), std::find_if(x.begin(), x.end(), notspace));
	x.erase(std::find_if(x.rbegin(), x.rend(), notspace).base(), x.end());
}

// verbose logging helper
static inline void VLog(bool enabled, const char* fmt, ...)
{
	if (!enabled)
		return;
	char buf[1024];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	ERR_LOG(LOG_GENERAL, "%s", buf);
}

CHelperNpcManager* CHelperNpcManager::Instance()
{
	// Meyers' singleton to allow private constructor
	static CHelperNpcManager s_instance;
	return &s_instance;
}

bool CHelperNpcManager::LoadConfig(CNtlIniFile& file)
{
	// Optional section; keep defaults if read fails
	// Read booleans via int then cast to avoid type/overload pitfalls
	{
		int v = m_config.bEnabled ? 1 : 0;
		if (file.Read("HELPER_NPC", "Enable", v)) m_config.bEnabled = (v != 0);
	}
	{
		int v = m_config.bAllowUltimate ? 1 : 0;
		if (file.Read("HELPER_NPC", "AllowUltimate", v)) m_config.bAllowUltimate = (v != 0);
	}
	{
		int v = m_config.bAllowBattleDungeon ? 1 : 0;
		if (file.Read("HELPER_NPC", "AllowBattleDungeon", v)) m_config.bAllowBattleDungeon = (v != 0);
	}
	{
		int v = m_config.bAllowTimeQuest ? 1 : 0;
		if (file.Read("HELPER_NPC", "AllowTimeQuest", v)) m_config.bAllowTimeQuest = (v != 0);
	}
	file.Read("HELPER_NPC", "MinPartySizeToAvoid", m_config.byMinPartySizeToAvoidSpawn);
	file.Read("HELPER_NPC", "PrimaryNpcId", m_config.primaryNpcTblidx);
	file.Read("HELPER_NPC", "FallbackNpcId", m_config.fallbackNpcTblidx);
	file.Read("HELPER_NPC", "SpawnOffset", m_config.fSpawnOffset);
	{
		int v = m_config.bFollowLeader ? 1 : 0;
		if (file.Read("HELPER_NPC", "FollowLeader", v)) m_config.bFollowLeader = (v != 0);
	}
	{
		int v = m_config.bAssistLeaderTarget ? 1 : 0;
		if (file.Read("HELPER_NPC", "AssistLeaderTarget", v)) m_config.bAssistLeaderTarget = (v != 0);
	}
	file.Read("HELPER_NPC", "HealLpThresholdOverride", m_config.wHealLpThresholdOverride);
	file.Read("HELPER_NPC", "HealPriorityMinMissingPercent", m_config.wHealPriorityMinMissingPercent);
	file.Read("HELPER_NPC", "DamageMultiplier", m_config.fDamageMultiplier);
	file.Read("HELPER_NPC", "HealPowerMultiplier", m_config.fHealPowerMultiplier);
	file.Read("HELPER_NPC", "MoveSpeedMultiplier", m_config.fMoveSpeedMultiplier);
	file.Read("HELPER_NPC", "AttackSpeedPercent", m_config.wAttackSpeedPercent);
	file.Read("HELPER_NPC", "EpRegenPercent", m_config.wEpRegenPercent);
	{
		int v = m_config.bInvincibleHelper ? 1 : 0;
		if (file.Read("HELPER_NPC", "InvincibleHelper", v)) m_config.bInvincibleHelper = (v != 0);
	}

	// Proactive attack behavior
	{
		int v = m_config.bProactiveAutoAttack ? 1 : 0;
		if (file.Read("HELPER_NPC", "ProactiveAutoAttack", v)) m_config.bProactiveAutoAttack = (v != 0);
		if (file.Read("HELPER_NPC", "VerboseLogs", v)) m_config.bVerboseLogs = (v != 0);
		file.Read("HELPER_NPC", "HealUseRangeBonusMeters", m_config.fHealUseRangeBonusMeters);
		file.Read("HELPER_NPC", "HealApplyAreaBonusMeters", m_config.fHealApplyAreaBonusMeters);
	}
	file.Read("HELPER_NPC", "AttackScanRange", m_config.wAttackScanRange);
	file.Read("HELPER_NPC", "AttackScanCooldownMs", m_config.dwAttackScanCooldownMs);

	// Attribute modifiers (helpers only)
	file.Read("HELPER_NPC", "MaxLPPercent", m_config.wMaxLPPercent);
	file.Read("HELPER_NPC", "MaxEPPercent", m_config.wMaxEPPercent);
	file.Read("HELPER_NPC", "PhysicalOffensePercent", m_config.wPhysicalOffensePercent);
	file.Read("HELPER_NPC", "EnergyOffensePercent", m_config.wEnergyOffensePercent);
	file.Read("HELPER_NPC", "PhysicalDefensePercent", m_config.wPhysicalDefensePercent);
	file.Read("HELPER_NPC", "EnergyDefensePercent", m_config.wEnergyDefensePercent);
	file.Read("HELPER_NPC", "AttackRangePercent", m_config.wAttackRangePercent);
	file.Read("HELPER_NPC", "AttackRangeBonusMeters", m_config.fAttackRangeBonusMeters);
	file.Read("HELPER_NPC", "SkillAnimSpeedPercent", m_config.wSkillAnimSpeedPercent);

	// Resurrection + rebuff controller
	file.Read("HELPER_NPC", "ResurrectSkillTblidx", m_config.resurrectSkillTblidx);
	file.Read("HELPER_NPC", "RebuffCooldownMs", m_config.dwRebuffCooldownMs);
	file.Read("HELPER_NPC", "RebuffMinRemainingMs", m_config.dwRebuffMinRemainingMs);
	// Tank aggro enforcement (global defaults)
	{
		int v = m_config.bEnforceTankAggro ? 1 : 0;
		if (file.Read("HELPER_NPC", "EnforceTankAggro", v)) m_config.bEnforceTankAggro = (v != 0);
	}
	file.Read("HELPER_NPC", "TankAggroPulseMs", m_config.dwTankAggroPulseMs);
	file.Read("HELPER_NPC", "TankAggroBonus", m_config.dwTankAggroBonus);
	{
		int v = m_config.bAllowGMHelpers ? 1 : 0; if (file.Read("HELPER_NPC", "AllowGMHelpers", v)) m_config.bAllowGMHelpers = (v != 0);
	}
	{
		int v = m_config.bAllowMultipleHelpersPerWorld ? 1 : 0; if (file.Read("HELPER_NPC", "AllowMultipleHelpersPerWorld", v)) m_config.bAllowMultipleHelpersPerWorld = (v != 0);
	}
	{
		int v = m_config.bDisallowDuplicateHelperKindPerWorld ? 1 : 0; if (file.Read("HELPER_NPC", "DisallowDuplicateHelperKindPerWorld", v)) m_config.bDisallowDuplicateHelperKindPerWorld = (v != 0);
	}

	// Resurrection retry/backoff + buff audit burst limits
	file.Read("HELPER_NPC", "ResurrectRetryDelay1Ms", m_config.dwResurrectRetryDelay1Ms);
	file.Read("HELPER_NPC", "ResurrectRetryDelay2Ms", m_config.dwResurrectRetryDelay2Ms);
	{
		int attempts = m_config.byResurrectMaxAttempts; if (file.Read("HELPER_NPC", "ResurrectMaxAttempts", attempts)) m_config.byResurrectMaxAttempts = (BYTE)attempts;
	}
	{
		int n = m_config.byMaxBuffsPerAudit; if (file.Read("HELPER_NPC", "MaxBuffsPerAudit", n)) { if (n < 1) n = 1; else if (n > 10) n = 10; m_config.byMaxBuffsPerAudit = (BYTE)n; }
	}
	{
		int n = m_config.byMaxBuffsPerTargetPerAudit; if (file.Read("HELPER_NPC", "MaxBuffsPerTargetPerAudit", n)) { if (n < 1) n = 1; else if (n > 10) n = 10; m_config.byMaxBuffsPerTargetPerAudit = (BYTE)n; }
	}

	// Optional global base modifiers section
	{
		// Only modifier keys are read from [NPC_MODIFIERS], leaving behavior keys to [HELPER_NPC]
		file.Read("NPC_MODIFIERS", "MaxLPPercent", m_config.wMaxLPPercent);
		file.Read("NPC_MODIFIERS", "MaxEPPercent", m_config.wMaxEPPercent);
		file.Read("NPC_MODIFIERS", "PhysicalOffensePercent", m_config.wPhysicalOffensePercent);
		file.Read("NPC_MODIFIERS", "EnergyOffensePercent", m_config.wEnergyOffensePercent);
		file.Read("NPC_MODIFIERS", "PhysicalDefensePercent", m_config.wPhysicalDefensePercent);
		file.Read("NPC_MODIFIERS", "EnergyDefensePercent", m_config.wEnergyDefensePercent);
		file.Read("NPC_MODIFIERS", "AttackRangePercent", m_config.wAttackRangePercent);
		file.Read("NPC_MODIFIERS", "AttackRangeBonusMeters", m_config.fAttackRangeBonusMeters);
		file.Read("NPC_MODIFIERS", "SkillAnimSpeedPercent", m_config.wSkillAnimSpeedPercent);
	}

	// Force skill (optional)
	// ForcedSkillTblidx supports single value or comma-separated list; fill both forcedSkillTblidx and vForcedSkills
	{
		// Try reading as string first
		CNtlString cs;
		if (file.Read("HELPER_NPC", "ForcedSkillTblidx", cs))
		{
			m_config.vForcedSkills.clear();
			std::string sfs = cs.c_str();
			std::stringstream ss(sfs);
			std::string token;
			bool any = false;
			while (std::getline(ss, token, ','))
			{
				TBLIDX id = (TBLIDX)std::strtoul(token.c_str(), NULL, 10);
				if (id != 0 && id != INVALID_TBLIDX)
				{
					m_config.vForcedSkills.push_back(id);
					any = true;
				}
			}
			if (any)
			{
				m_config.forcedSkillTblidx = m_config.vForcedSkills.front();
			}
			else
			{
				m_config.forcedSkillTblidx = INVALID_TBLIDX;
			}
		}
		else
		{
			// Fallback: read as single numeric
			file.Read("HELPER_NPC", "ForcedSkillTblidx", m_config.forcedSkillTblidx);
			m_config.vForcedSkills.clear();
			if (m_config.forcedSkillTblidx != INVALID_TBLIDX)
				m_config.vForcedSkills.push_back(m_config.forcedSkillTblidx);
		}
	}
	int forcedBasis = m_config.forcedSkillBasis;
	if (file.Read("HELPER_NPC", "ForcedSkillBasis", forcedBasis))
		m_config.forcedSkillBasis = (BYTE)forcedBasis;
	file.Read("HELPER_NPC", "ForcedSkillLP", m_config.forcedSkillLP);
	file.Read("HELPER_NPC", "ForcedSkillTime", m_config.forcedSkillTime);
	// Optional MOB helper configuration
	{
		int v = m_config.bUseMobAsHelper ? 1 : 0;
		if (file.Read("HELPER_NPC", "UseMobAsHelper", v)) m_config.bUseMobAsHelper = (v != 0);
	}
	file.Read("HELPER_NPC", "MobId", m_config.helperMobTblidx);

	// Buff list (comma-separated): e.g., "420141,420142"
	{
		CNtlString cs;
		if (file.Read("HELPER_NPC", "BuffSkills", cs))
		{
			m_config.vBuffSkills.clear();
			std::string s = cs.c_str();
			// Support both comma and semicolon separators
			for (char& ch : s)
			{
				if (ch == ';') ch = ',';
			}
			std::istringstream iss(s);
			std::string tok;
			auto parseTblidxToken = [&](const std::string& raw, TBLIDX& out) -> bool
				{
					std::string t = raw;
					// strip optional prefixes like "S:" or "B:" (GM-like), case-insensitive
					if (t.size() > 2 && t[1] == ':')
					{
						char pfx = (char)std::toupper((unsigned char)t[0]);
						if (pfx == 'S' || pfx == 'B')
							t = t.substr(2);
					}
					str_trim(t);
					if (t.empty()) return false;
					// hex support (0x...)
					if (t.size() > 2 && (t[0] == '0') && (t[1] == 'x' || t[1] == 'X'))
					{
						char* endp = nullptr;
						unsigned long val = std::strtoul(t.c_str(), &endp, 16);
						if (endp && *endp == '\0' && val != 0 && val != INVALID_TBLIDX)
						{
							out = (TBLIDX)val;
							return true;
						}
						return false;
					}
					// decimal
					char* endp = nullptr;
					unsigned long val = std::strtoul(t.c_str(), &endp, 10);
					if (endp && *endp == '\0' && val != 0 && val != INVALID_TBLIDX)
					{
						out = (TBLIDX)val;
						return true;
					}
					return false;
				};
			while (std::getline(iss, tok, ','))
			{
				str_trim(tok);
				if (tok.empty()) continue;
				TBLIDX id = INVALID_TBLIDX;
				bool ok = parseTblidxToken(tok, id);
				if (ok)
				{
					m_config.vBuffSkills.push_back(id);
					continue;
				}
				// Try alias map forms such as "1:..." or plain number to be resolved later
				// If token is a plain integer and not a valid SkillTable id, treat it as external buff index; we'll map it below if aliases exist
				char* endp = nullptr;
				unsigned long ext = std::strtoul(tok.c_str(), &endp, 10);
				if (endp && *endp == '\0' && ext != 0)
				{
					auto it = m_config.buffIndexAlias.find((DWORD)ext);
					if (it != m_config.buffIndexAlias.end())
					{
						m_config.vBuffSkills.push_back(it->second);
						continue;
					}
				}
				ERR_LOG(LOG_GENERAL, "HelperNPC: BuffSkills token '%s' could not be resolved to a skill (check SkillTable id or BuffIndexMap)", tok.c_str());
			}
		}
		// Parse optional alias map entries: BuffIndexMap=1:420141, 2=420142
		CNtlString csMap;
		if (file.Read("HELPER_NPC", "BuffIndexMap", csMap))
		{
			m_config.buffIndexAlias.clear();
			std::string ms = csMap.c_str();
			for (char& ch : ms) { if (ch == ';') ch = ','; }
			std::istringstream miss(ms);
			std::string pair;
			while (std::getline(miss, pair, ','))
			{
				str_trim(pair);
				if (pair.empty()) continue;
				size_t pos = pair.find_first_of("=:");
				if (pos == std::string::npos) { ERR_LOG(LOG_GENERAL, "HelperNPC: BuffIndexMap entry '%s' missing ':' or '='", pair.c_str()); continue; }
				std::string k = pair.substr(0, pos);
				std::string v = pair.substr(pos + 1);
				str_trim(k); str_trim(v);
				if (k.empty() || v.empty()) { ERR_LOG(LOG_GENERAL, "HelperNPC: BuffIndexMap entry '%s' invalid (empty key/value)", pair.c_str()); continue; }
				char* endk = nullptr; unsigned long ext = std::strtoul(k.c_str(), &endk, 10);
				if (!(endk && *endk == '\0') || ext == 0) { ERR_LOG(LOG_GENERAL, "HelperNPC: BuffIndexMap key '%s' is not a valid number", k.c_str()); continue; }
				TBLIDX skill = INVALID_TBLIDX; char* endv = nullptr; unsigned long vv = std::strtoul(v.c_str(), &endv, 10);
				if (!(endv && *endv == '\0') || vv == 0 || vv == INVALID_TBLIDX) { ERR_LOG(LOG_GENERAL, "HelperNPC: BuffIndexMap value '%s' is not a valid SkillTable id", v.c_str()); continue; }
				// Verify skill exists to avoid silent typos
				if (!g_pTableContainer->GetSkillTable()->FindData((TBLIDX)vv))
				{
					ERR_LOG(LOG_GENERAL, "HelperNPC: BuffIndexMap value %lu not found in SkillTable", vv);
					continue;
				}
				m_config.buffIndexAlias[(DWORD)ext] = (TBLIDX)vv;
			}
		}
		int basis = m_config.buffBasis;
		if (file.Read("HELPER_NPC", "BuffBasis", basis)) m_config.buffBasis = (BYTE)basis;
		file.Read("HELPER_NPC", "BuffLP", m_config.buffLP);
		file.Read("HELPER_NPC", "BuffTime", m_config.buffTime);
	}

	// Verbose config summary
	VLog(m_config.bVerboseLogs,
		"HelperNPC: Config loaded: Enabled=%d AllowUltimate=%d AllowBattleDungeon=%d AllowTimeQuest=%d FollowLeader=%d AssistLeaderTarget=%d Invincible=%d ProactiveAutoAttack=%d",
		m_config.bEnabled ? 1 : 0,
		m_config.bAllowUltimate ? 1 : 0,
		m_config.bAllowBattleDungeon ? 1 : 0,
		m_config.bAllowTimeQuest ? 1 : 0,
		m_config.bFollowLeader ? 1 : 0,
		m_config.bAssistLeaderTarget ? 1 : 0,
		m_config.bInvincibleHelper ? 1 : 0,
		m_config.bProactiveAutoAttack ? 1 : 0);
	VLog(m_config.bVerboseLogs,
		"HelperNPC: IDs: PrimaryNpcId=%u FallbackNpcId=%u MobId=%u MinPartyNoHelper=%u",
		m_config.primaryNpcTblidx,
		m_config.fallbackNpcTblidx,
		m_config.helperMobTblidx,
		m_config.byMinPartySizeToAvoidSpawn);
	VLog(m_config.bVerboseLogs,
		"HelperNPC: Multipliers: Damage=%.2f Heal=%.2f Move=%.2f AttackSpeed%%=%u EPRegen%%=%u LP%%=%u EP%%=%u OffP%%=%u OffE%%=%u DefP%%=%u DefE%%=%u Range%%=%u Range+%.1fm SkillAnim%%=%u",
		m_config.fDamageMultiplier,
		m_config.fHealPowerMultiplier,
		m_config.fMoveSpeedMultiplier,
		m_config.wAttackSpeedPercent,
		m_config.wEpRegenPercent,
		m_config.wMaxLPPercent,
		m_config.wMaxEPPercent,
		m_config.wPhysicalOffensePercent,
		m_config.wEnergyOffensePercent,
		m_config.wPhysicalDefensePercent,
		m_config.wEnergyDefensePercent,
		m_config.wAttackRangePercent,
		m_config.fAttackRangeBonusMeters,
		m_config.wSkillAnimSpeedPercent);
	VLog(m_config.bVerboseLogs,
		"HelperNPC: Skills: ForcedCount=%u ForcedBasis=%u LP=%u Time=%u BuffCount=%u BuffBasis=%u LP=%u Time=%u AliasMap=%u Resurrect=%u RebuffCd=%u RebuffMinRemain=%u",
		(unsigned)m_config.vForcedSkills.size(),
		m_config.forcedSkillBasis,
		m_config.forcedSkillLP,
		m_config.forcedSkillTime,
		(unsigned)m_config.vBuffSkills.size(),
		m_config.buffBasis,
		m_config.buffLP,
		m_config.buffTime,
		(unsigned)m_config.buffIndexAlias.size(),
		m_config.resurrectSkillTblidx,
		m_config.dwRebuffCooldownMs,
		m_config.dwRebuffMinRemainingMs);
	VLog(m_config.bVerboseLogs,
		"HelperNPC: Scan: AttackScanRange=%u AttackScanCooldownMs=%u HealUseRange+%.1fm HealApplyArea+%.1fm SpawnOffset=%.1fm GMHelpers=%d MultiWorldHelpers=%d",
		m_config.wAttackScanRange,
		m_config.dwAttackScanCooldownMs,
		m_config.fHealUseRangeBonusMeters,
		m_config.fHealApplyAreaBonusMeters,
		m_config.fSpawnOffset,
		m_config.bAllowGMHelpers ? 1 : 0,
		m_config.bAllowMultipleHelpersPerWorld ? 1 : 0);

	// Optional per-dungeon override sections: [HELPER_NPC_UD], [HELPER_NPC_BD], [HELPER_NPC_TMQ]
	m_cfgUD = m_config; m_cfgBD = m_config; m_cfgTMQ = m_config;
	int udCount = LoadConfigSection(file, "HELPER_NPC_UD", m_cfgUD);
	int bdCount = LoadConfigSection(file, "HELPER_NPC_BD", m_cfgBD);
	int tmqCount = LoadConfigSection(file, "HELPER_NPC_TMQ", m_cfgTMQ);
	m_hasUDOverride = (udCount > 0);
	m_hasBDOverride = (bdCount > 0);
	m_hasTMQOverride = (tmqCount > 0);
	if (m_hasUDOverride) VLog(m_config.bVerboseLogs, "HelperNPC: UD override loaded with %d keys", udCount);
	if (m_hasBDOverride) VLog(m_config.bVerboseLogs, "HelperNPC: BD override loaded with %d keys", bdCount);
	if (m_hasTMQOverride) VLog(m_config.bVerboseLogs, "HelperNPC: TMQ override loaded with %d keys", tmqCount);

	// Extra allowlists for custom worlds
	{
		CNtlString cs;
		if (file.Read("HELPER_NPC", "ExtraUDWorldIDs", cs))
		{
			m_extraUDWorldIDs.clear();
			std::string s = cs.c_str();
			for (char& ch : s) { if (ch == ';') ch = ','; }
			std::istringstream iss(s); std::string tok;
			while (std::getline(iss, tok, ','))
			{
				str_trim(tok); if (tok.empty()) continue; WORLDID wid = (WORLDID)std::strtoul(tok.c_str(), NULL, 10);
				if (wid != 0 && wid != INVALID_WORLDID) m_extraUDWorldIDs.insert(wid);
			}
			if (!m_extraUDWorldIDs.empty()) VLog(m_config.bVerboseLogs, "HelperNPC: Extra UD worlds loaded: %zu", m_extraUDWorldIDs.size());
		}
	}
	{
		CNtlString cs;
		if (file.Read("HELPER_NPC", "ExtraBDWorldIDs", cs))
		{
			m_extraBDWorldIDs.clear();
			std::string s = cs.c_str(); for (char& ch : s) { if (ch == ';') ch = ','; }
			std::istringstream iss(s); std::string tok;
			while (std::getline(iss, tok, ',')) { str_trim(tok); if (tok.empty()) continue; WORLDID wid = (WORLDID)std::strtoul(tok.c_str(), NULL, 10); if (wid != 0 && wid != INVALID_WORLDID) m_extraBDWorldIDs.insert(wid); }
			if (!m_extraBDWorldIDs.empty()) VLog(m_config.bVerboseLogs, "HelperNPC: Extra BD worlds loaded: %zu", m_extraBDWorldIDs.size());
		}
	}
	{
		CNtlString cs;
		if (file.Read("HELPER_NPC", "ExtraTMQWorldIDs", cs))
		{
			m_extraTMQWorldIDs.clear();
			std::string s = cs.c_str(); for (char& ch : s) { if (ch == ';') ch = ','; }
			std::istringstream iss(s); std::string tok;
			while (std::getline(iss, tok, ',')) { str_trim(tok); if (tok.empty()) continue; WORLDID wid = (WORLDID)std::strtoul(tok.c_str(), NULL, 10); if (wid != 0 && wid != INVALID_WORLDID) m_extraTMQWorldIDs.insert(wid); }
			if (!m_extraTMQWorldIDs.empty()) VLog(m_config.bVerboseLogs, "HelperNPC: Extra TMQ worlds loaded: %zu", m_extraTMQWorldIDs.size());
		}
	}

	// Role sections: [HEALER], [TANK], [BUFFER], [SPEED]
	LoadRoleSection(file, "HEALER", m_roleHealer);
	LoadRoleSection(file, "TANK", m_roleTank);
	LoadRoleSection(file, "BUFFER", m_roleBuffer);
	LoadRoleSection(file, "SPEED", m_roleSpeed);
	if (m_roleTank.enabled && m_roleTank.coveredClasses.empty())
	{
		// Default tank classes if not specified
		m_roleTank.coveredClasses.insert(13);
		m_roleTank.coveredClasses.insert(14);
		m_roleTank.coveredClasses.insert(17);
		m_roleTank.coveredClasses.insert(18);
	}
	return true;
}

bool CHelperNpcManager::LoadRoleSection(CNtlIniFile& file, const char* sectionName, sROLE_DEF& outRole)
{
	int en = 0;
	if (!file.Read(sectionName, "Enabled", en))
		return false;
	outRole.enabled = (en != 0);
	int mc = 1;
	file.Read(sectionName, "MaxCount", mc);
	if (mc < 1) mc = 1; if (mc > 3) mc = 3;
	outRole.maxCount = (BYTE)mc;

	// Load role behavior config using existing section loader
	outRole.cfg = m_config;
	LoadConfigSection(file, sectionName, outRole.cfg);

	// Parse covered class IDs
	CNtlString cs;
	if (file.Read(sectionName, "Classes", cs))
	{
		outRole.coveredClasses.clear();
		std::string s = cs.c_str();
		for (char& ch : s) { if (ch == ';') ch = ','; }
		std::istringstream iss(s); std::string tok;
		while (std::getline(iss, tok, ','))
		{
			str_trim(tok); if (tok.empty()) continue;
			int cid = (int)std::strtol(tok.c_str(), NULL, 10);
			if (cid > 0) outRole.coveredClasses.insert(cid);
		}
	}
	VLog(m_config.bVerboseLogs, "HelperNPC: Loaded role [%s] enabled=%d max=%u classes=%zu",
		sectionName, outRole.enabled ? 1 : 0, (unsigned)outRole.maxCount, outRole.coveredClasses.size());
	return true;
}

int CHelperNpcManager::LoadConfigSection(CNtlIniFile& file, const char* sectionName, sHELPER_NPC_CONFIG& out)
{
	int readCount = 0;
	// Booleans via int
	{ int v = out.bEnabled ? 1 : 0; if (file.Read(sectionName, "Enable", v)) { out.bEnabled = (v != 0); ++readCount; } }
	{ int v = out.bAllowUltimate ? 1 : 0; if (file.Read(sectionName, "AllowUltimate", v)) { out.bAllowUltimate = (v != 0); ++readCount; } }
	{ int v = out.bAllowBattleDungeon ? 1 : 0; if (file.Read(sectionName, "AllowBattleDungeon", v)) { out.bAllowBattleDungeon = (v != 0); ++readCount; } }
	{ int v = out.bAllowTimeQuest ? 1 : 0; if (file.Read(sectionName, "AllowTimeQuest", v)) { out.bAllowTimeQuest = (v != 0); ++readCount; } }
	if (file.Read(sectionName, "MinPartySizeToAvoid", out.byMinPartySizeToAvoidSpawn)) ++readCount;
	if (file.Read(sectionName, "PrimaryNpcId", out.primaryNpcTblidx)) ++readCount;
	if (file.Read(sectionName, "FallbackNpcId", out.fallbackNpcTblidx)) ++readCount;
	if (file.Read(sectionName, "SpawnOffset", out.fSpawnOffset)) ++readCount;
	{ int v = out.bFollowLeader ? 1 : 0; if (file.Read(sectionName, "FollowLeader", v)) { out.bFollowLeader = (v != 0); ++readCount; } }
	{ int v = out.bAssistLeaderTarget ? 1 : 0; if (file.Read(sectionName, "AssistLeaderTarget", v)) { out.bAssistLeaderTarget = (v != 0); ++readCount; } }
	if (file.Read(sectionName, "HealLpThresholdOverride", out.wHealLpThresholdOverride)) ++readCount;
	if (file.Read(sectionName, "HealPriorityMinMissingPercent", out.wHealPriorityMinMissingPercent)) ++readCount;
	if (file.Read(sectionName, "DamageMultiplier", out.fDamageMultiplier)) ++readCount;
	if (file.Read(sectionName, "HealPowerMultiplier", out.fHealPowerMultiplier)) ++readCount;
	if (file.Read(sectionName, "MoveSpeedMultiplier", out.fMoveSpeedMultiplier)) ++readCount;
	if (file.Read(sectionName, "AttackSpeedPercent", out.wAttackSpeedPercent)) ++readCount;
	if (file.Read(sectionName, "EpRegenPercent", out.wEpRegenPercent)) ++readCount;
	{ int v = out.bInvincibleHelper ? 1 : 0; if (file.Read(sectionName, "InvincibleHelper", v)) { out.bInvincibleHelper = (v != 0); ++readCount; } }
	{ int v = out.bProactiveAutoAttack ? 1 : 0; if (file.Read(sectionName, "ProactiveAutoAttack", v)) { out.bProactiveAutoAttack = (v != 0); ++readCount; } }
	// Accept 0/1 for verbose
	{ int v = out.bVerboseLogs ? 1 : 0; if (file.Read(sectionName, "VerboseLogs", v)) { out.bVerboseLogs = (v != 0); ++readCount; } }
	if (file.Read(sectionName, "HealUseRangeBonusMeters", out.fHealUseRangeBonusMeters)) ++readCount;
	if (file.Read(sectionName, "HealApplyAreaBonusMeters", out.fHealApplyAreaBonusMeters)) ++readCount;
	if (file.Read(sectionName, "AttackScanRange", out.wAttackScanRange)) ++readCount;
	if (file.Read(sectionName, "AttackScanCooldownMs", out.dwAttackScanCooldownMs)) ++readCount;
	// Attribute modifiers (helpers only)
	if (file.Read(sectionName, "MaxLPPercent", out.wMaxLPPercent)) ++readCount;
	if (file.Read(sectionName, "MaxEPPercent", out.wMaxEPPercent)) ++readCount;
	if (file.Read(sectionName, "PhysicalOffensePercent", out.wPhysicalOffensePercent)) ++readCount;
	if (file.Read(sectionName, "EnergyOffensePercent", out.wEnergyOffensePercent)) ++readCount;
	if (file.Read(sectionName, "PhysicalDefensePercent", out.wPhysicalDefensePercent)) ++readCount;
	if (file.Read(sectionName, "EnergyDefensePercent", out.wEnergyDefensePercent)) ++readCount;
	if (file.Read(sectionName, "AttackRangePercent", out.wAttackRangePercent)) ++readCount;
	if (file.Read(sectionName, "AttackRangeBonusMeters", out.fAttackRangeBonusMeters)) ++readCount;
	if (file.Read(sectionName, "SkillAnimSpeedPercent", out.wSkillAnimSpeedPercent)) ++readCount;
	// Resurrection + rebuff controller (per-section overrides)
	if (file.Read(sectionName, "ResurrectSkillTblidx", out.resurrectSkillTblidx)) ++readCount;
	if (file.Read(sectionName, "RebuffCooldownMs", out.dwRebuffCooldownMs)) ++readCount;
	if (file.Read(sectionName, "RebuffMinRemainingMs", out.dwRebuffMinRemainingMs)) ++readCount;
	{ int v = out.bPrioritizeForcedSkills ? 1 : 0; if (file.Read(sectionName, "PrioritizeForcedSkills", v)) { out.bPrioritizeForcedSkills = (v != 0); ++readCount; } }
	// Tank aggro enforcement overrides
	{ int v = out.bEnforceTankAggro ? 1 : 0; if (file.Read(sectionName, "EnforceTankAggro", v)) { out.bEnforceTankAggro = (v != 0); ++readCount; } }
	if (file.Read(sectionName, "TankAggroPulseMs", out.dwTankAggroPulseMs)) ++readCount;
	if (file.Read(sectionName, "TankAggroBonus", out.dwTankAggroBonus)) ++readCount;
	if (file.Read(sectionName, "ResurrectRetryDelay1Ms", out.dwResurrectRetryDelay1Ms)) ++readCount;
	if (file.Read(sectionName, "ResurrectRetryDelay2Ms", out.dwResurrectRetryDelay2Ms)) ++readCount;
	{ int v = out.byResurrectMaxAttempts; if (file.Read(sectionName, "ResurrectMaxAttempts", v)) { out.byResurrectMaxAttempts = (BYTE)v; ++readCount; } }
	{ int v = out.byMaxBuffsPerAudit; if (file.Read(sectionName, "MaxBuffsPerAudit", v)) { if (v < 1) v = 1; else if (v > 10) v = 10; out.byMaxBuffsPerAudit = (BYTE)v; ++readCount; } }
	{ int v = out.byMaxBuffsPerTargetPerAudit; if (file.Read(sectionName, "MaxBuffsPerTargetPerAudit", v)) { if (v < 1) v = 1; else if (v > 10) v = 10; out.byMaxBuffsPerTargetPerAudit = (BYTE)v; ++readCount; } }
	{ int v = out.bUseMobAsHelper ? 1 : 0; if (file.Read(sectionName, "UseMobAsHelper", v)) { out.bUseMobAsHelper = (v != 0); ++readCount; } }
	if (file.Read(sectionName, "MobId", out.helperMobTblidx)) ++readCount;

	// Buff skills list
	{
		CNtlString cs;
		if (file.Read(sectionName, "BuffSkills", cs))
		{
			out.vBuffSkills.clear();
			std::string s = cs.c_str(); for (char& ch : s) { if (ch == ';') ch = ','; }
			std::istringstream iss(s); std::string tok;
			auto parseTblidxToken = [&](const std::string& raw, TBLIDX& val) -> bool {
				std::string t = raw; if (t.size() > 2 && t[1] == ':') { char p = (char)std::toupper((unsigned char)t[0]); if (p == 'S' || p == 'B') t = t.substr(2); }
				str_trim(t); if (t.empty()) return false;
				if (t.size() > 2 && t[0] == '0' && (t[1] == 'x' || t[1] == 'X')) { char* e = nullptr; unsigned long v = std::strtoul(t.c_str(), &e, 16); if (e && *e == '\0' && v != 0 && v != INVALID_TBLIDX) { val = (TBLIDX)v; return true; } return false; }
				char* e = nullptr; unsigned long v = std::strtoul(t.c_str(), &e, 10); if (e && *e == '\0' && v != 0 && v != INVALID_TBLIDX) { val = (TBLIDX)v; return true; }
				return false; };
			while (std::getline(iss, tok, ',')) { str_trim(tok); if (tok.empty()) continue; TBLIDX id = INVALID_TBLIDX; if (parseTblidxToken(tok, id)) out.vBuffSkills.push_back(id); }
			++readCount;
		}
	}
	// BuffIndexMap
	{
		CNtlString csMap;
		if (file.Read(sectionName, "BuffIndexMap", csMap))
		{
			out.buffIndexAlias.clear();
			std::string ms = csMap.c_str(); for (char& ch : ms) { if (ch == ';') ch = ','; }
			std::istringstream miss(ms); std::string pair;
			while (std::getline(miss, pair, ','))
			{
				str_trim(pair); if (pair.empty()) continue; size_t pos = pair.find_first_of("=:"); if (pos == std::string::npos) continue;
				std::string k = pair.substr(0, pos); std::string v = pair.substr(pos + 1); str_trim(k); str_trim(v); if (k.empty() || v.empty()) continue;
				char* ek = nullptr; unsigned long ext = std::strtoul(k.c_str(), &ek, 10); if (!(ek && *ek == '\0') || ext == 0) continue;
				char* ev = nullptr; unsigned long vv = std::strtoul(v.c_str(), &ev, 10); if (!(ev && *ev == '\0') || vv == 0 || vv == INVALID_TBLIDX) continue;
				if (!g_pTableContainer->GetSkillTable()->FindData((TBLIDX)vv)) continue;
				out.buffIndexAlias[(DWORD)ext] = (TBLIDX)vv;
			}
			++readCount;
		}
	}
	{ int basis = out.buffBasis; if (file.Read(sectionName, "BuffBasis", basis)) { out.buffBasis = (BYTE)basis; ++readCount; } }
	if (file.Read(sectionName, "BuffLP", out.buffLP)) ++readCount;
	if (file.Read(sectionName, "BuffTime", out.buffTime)) ++readCount;

	// Forced skills (single value or comma-separated list)
	{
		CNtlString cs;
		if (file.Read(sectionName, "ForcedSkillTblidx", cs))
		{
			out.vForcedSkills.clear();
			std::string sfs = cs.c_str(); std::stringstream ss(sfs); std::string token; bool any = false;
			while (std::getline(ss, token, ',')) { TBLIDX id = (TBLIDX)std::strtoul(token.c_str(), NULL, 10); if (id != 0 && id != INVALID_TBLIDX) { out.vForcedSkills.push_back(id); any = true; } }
			out.forcedSkillTblidx = any ? out.vForcedSkills.front() : INVALID_TBLIDX; ++readCount;
		}
		else if (file.Read(sectionName, "ForcedSkillTblidx", out.forcedSkillTblidx))
		{
			out.vForcedSkills.clear(); if (out.forcedSkillTblidx != INVALID_TBLIDX) out.vForcedSkills.push_back(out.forcedSkillTblidx); ++readCount;
		}
	}
	{ int basis = out.forcedSkillBasis; if (file.Read(sectionName, "ForcedSkillBasis", basis)) { out.forcedSkillBasis = (BYTE)basis; ++readCount; } }
	if (file.Read(sectionName, "ForcedSkillLP", out.forcedSkillLP)) ++readCount;
	if (file.Read(sectionName, "ForcedSkillTime", out.forcedSkillTime)) ++readCount;

	return readCount;
}

bool CHelperNpcManager::SpawnHelperIfNeededForDungeon(CPlayer* pLeader, CWorld* pWorld, bool bIsUltimateDungeon)
{
	const sHELPER_NPC_CONFIG& cfg = bIsUltimateDungeon ? (m_hasUDOverride ? m_cfgUD : m_config)
		: (m_hasBDOverride ? m_cfgBD : m_config);
	if (bIsUltimateDungeon && !cfg.bAllowUltimate) return false;
	if (!bIsUltimateDungeon && !cfg.bAllowBattleDungeon) return false;
	return SpawnIfAllowed(pLeader, pWorld, cfg);
}

bool CHelperNpcManager::SpawnHelperIfNeededForTmq(CPlayer* pLeader, CWorld* pWorld)
{
	const sHELPER_NPC_CONFIG& cfg = m_hasTMQOverride ? m_cfgTMQ : m_config;
	if (!cfg.bAllowTimeQuest) return false;
	return SpawnIfAllowed(pLeader, pWorld, cfg);
}

bool CHelperNpcManager::SpawnIfAllowed(CPlayer* pLeader, CWorld* pWorld, const sHELPER_NPC_CONFIG& cfg)
{
	if (!pLeader || !pWorld)
	{
		ERR_LOG(LOG_GENERAL, "HelperNPC: skip spawn - invalid args pLeader=%p pWorld=%p", pLeader, pWorld);
		return false;
	}

	if (!cfg.bEnabled)
	{
		VLog(cfg.bVerboseLogs, "HelperNPC: disabled by config");
		return false;
	}

	// Prevent GM-triggered helper spawns when teleporting for inspections unless explicitly allowed
	if (!cfg.bAllowGMHelpers && pLeader->IsGameMaster())
	{
		VLog(cfg.bVerboseLogs, "HelperNPC: skip - leader %u is GM and GM helpers disabled", pLeader->GetID());
		return false;
	}

	// Only spawn for actual party leader (or solo) to avoid duplicates when GM spectates or non-leaders zone in first
	if (pLeader->GetParty() && pLeader->GetParty()->GetPartyLeaderID() != pLeader->GetID())
	{
		VLog(cfg.bVerboseLogs, "HelperNPC: skip - player %u is not party leader (%u)", pLeader->GetID(), pLeader->GetParty()->GetPartyLeaderID());
		return false;
	}

	// Only allow in dungeon-like worlds (UD/BD/TMQ or extra lists)
	eGAMERULE_TYPE rule = pWorld->GetRuleType();
	WORLDID wid = pWorld->GetID();
	bool bUD = (rule == GAMERULE_HUNT) || (this->m_extraUDWorldIDs.find(wid) != this->m_extraUDWorldIDs.end());
	bool bBD = (rule == GAMERULE_CCBATTLEDUNGEON) || (this->m_extraBDWorldIDs.find(wid) != this->m_extraBDWorldIDs.end());
	bool bTMQ = (rule == GAMERULE_TIMEQUEST) || (this->m_extraTMQWorldIDs.find(wid) != this->m_extraTMQWorldIDs.end());
	if (!bUD && !bBD && !bTMQ)
	{
		VLog(cfg.bVerboseLogs, "HelperNPC: skip - world %u not a dynamic dungeon (rule=%u)", wid, (unsigned)rule);
		// Safety: ensure no stale helpers lingering in non-dynamic world
		DespawnAllHelpersForLeaderInWorld(pLeader, pWorld);
		return false;
	}

	// Treat solo (no party) as party size 1 so helper can spawn for solo entries
	BYTE byCount = 1;
	if (pLeader->GetParty())
		byCount = pLeader->GetParty()->GetPartyMemberCount();
	else
		VLog(cfg.bVerboseLogs, "HelperNPC: leader has no party - treating as solo size 1");

	if (byCount >= cfg.byMinPartySizeToAvoidSpawn)
	{
		VLog(cfg.bVerboseLogs, "HelperNPC: skip - party size %u >= threshold %u", byCount, cfg.byMinPartySizeToAvoidSpawn);
		return false; // party large enough; no helper
	}

	// Composite capacity cap: real players + existing helpers must not exceed 5
	// (helpers count as pseudo party members for slot pressure balancing)
	{
		int existingHelpersInWorld = 0;
		auto itList = m_leaderToHelpers.find(pLeader->GetID());
		if (itList != m_leaderToHelpers.end())
		{
			for (HOBJECT h : itList->second)
			{
				CNpc* hh = g_pObjectManager->GetNpc(h);
				if (hh && hh->IsInitialized() && hh->GetCurWorld() == pWorld)
					existingHelpersInWorld++;
			}
		}
		int composite = (int)byCount + existingHelpersInWorld;
		if (composite >= 5)
		{
			VLog(cfg.bVerboseLogs, "HelperNPC: skip - composite party size %d (players %u + helpers %d) >= max 5", composite, byCount, existingHelpersInWorld);
			return false;
		}
	}

	// Change dedup logic continues after helperTblidx is resolved below

	// Decide what to spawn (NPC vs MOB)
	bool bSpawnMob = false;
	TBLIDX helperTblidx = INVALID_TBLIDX;
	if (cfg.bUseMobAsHelper)
	{
		if (g_pTableContainer->GetMobTable()->FindData(cfg.helperMobTblidx))
		{
			bSpawnMob = true;
			helperTblidx = cfg.helperMobTblidx;
		}
		else
		{
			ERR_LOG(LOG_GENERAL, "HelperNPC: configured MOB %u not found", cfg.helperMobTblidx);
			return false;
		}
	}
	else
	{
		if (g_pTableContainer->GetNpcTable()->FindData(cfg.primaryNpcTblidx))
			helperTblidx = cfg.primaryNpcTblidx;
		else if (g_pTableContainer->GetNpcTable()->FindData(cfg.fallbackNpcTblidx))
			helperTblidx = cfg.fallbackNpcTblidx;
		else if (g_pTableContainer->GetMobTable()->FindData(cfg.helperMobTblidx))
		{
			// fallback to MOB healer if NPCs not found
			bSpawnMob = true;
			helperTblidx = cfg.helperMobTblidx;
		}
		else
		{
			ERR_LOG(LOG_GENERAL, "HelperNPC: no valid NPC or MOB tblidx (npc primary %u, fallback %u; mob %u)",
				cfg.primaryNpcTblidx, cfg.fallbackNpcTblidx, cfg.helperMobTblidx);
			return false;
		}
	}

	// World-level uniqueness: revised to allow one helper per distinct kind (role)
	// When multi-helpers disabled, we now only block spawning if the SAME helper kind (tblidx) already exists in the world.
	if (!cfg.bAllowMultipleHelpersPerWorld)
	{
		for (const auto& kv : m_helperKindByHelper)
		{
			CNpc* existing = g_pObjectManager->GetNpc(kv.first);
			if (!existing || !existing->IsInitialized()) continue;
			if (existing->GetCurWorld() != pWorld) continue;
			// If identical kind already present, block; otherwise allow coexistence (different role/helper kind)
			if (kv.second == helperTblidx)
			{
				VLog(cfg.bVerboseLogs, "HelperNPC: skip - world %u already has helper kind tblidx=%u (multi disabled, per-kind uniqueness)", pWorld->GetID(), helperTblidx);
				return false;
			}
		}
	}
	else if (cfg.bDisallowDuplicateHelperKindPerWorld)
	{
		for (const auto& kv : m_helperKindByHelper)
		{
			CNpc* existing = g_pObjectManager->GetNpc(kv.first);
			if (existing && existing->IsInitialized() && existing->GetCurWorld() == pWorld && kv.second == helperTblidx)
			{
				VLog(cfg.bVerboseLogs, "HelperNPC: skip - world %u already has helper kind tblidx=%u (duplicate kind disallowed)", pWorld->GetID(), helperTblidx);
				return false;
			}
		}
	}

	// Dedupe by helper ID for the same leader in the same world
	{
		auto itList = m_leaderToHelpers.find(pLeader->GetID());
		if (itList != m_leaderToHelpers.end())
		{
			for (HOBJECT h : itList->second)
			{
				CNpc* existing = g_pObjectManager->GetNpc(h);
				if (existing && existing->IsInitialized() && existing->GetCurWorld() == pWorld)
				{
					auto itKind = m_helperKindByHelper.find(h);
					if (itKind != m_helperKindByHelper.end() && itKind->second == helperTblidx)
					{
						VLog(cfg.bVerboseLogs, "HelperNPC: skip - leader %u already has helper tblidx %u in world %u", pLeader->GetID(), helperTblidx, pWorld->GetID());
						return false;
					}
				}
			}
		}
	}

	// Guard: prevent duplicate spawn for the same leader/world/helperTblidx in parallel/rapid calls
	SpawnKey key{ pLeader->GetID(), pWorld->GetID(), helperTblidx };
	if (m_pendingSpawns.find(key) != m_pendingSpawns.end())
	{
		VLog(cfg.bVerboseLogs, "HelperNPC: spawn already in progress for leader %u world %u tblidx %u", key.leader, key.world, key.tblidx);
		return false;
	}
	m_pendingSpawns.insert(key);

	// Spawn near the leader's current position (safer for custom worlds/overrides)
	// Fallback to world start only if leader location is unavailable (shouldn't happen)
	sVECTOR3 baseLoc;
	sVECTOR3 baseDir;
	{
		const auto& pl = pLeader->GetCurLoc();
		const auto& pd = pLeader->GetCurDir();
		baseLoc.x = pl.x; baseLoc.y = pl.y; baseLoc.z = pl.z;
		baseDir.x = pd.x; baseDir.y = pd.y; baseDir.z = pd.z;
		// If leader position appears defaulted (0,0,0), try world Start1 as a fallback
		if (baseLoc.x == 0.f && baseLoc.y == 0.f && baseLoc.z == 0.f)
		{
			if (const sWORLD_TBLDAT* pWorldTbl = pWorld->GetTbldat())
			{
				baseLoc.x = pWorldTbl->vStart1Loc.x;
				baseLoc.y = pWorldTbl->vStart1Loc.y;
				baseLoc.z = pWorldTbl->vStart1Loc.z;
				baseDir.x = pWorldTbl->vStart1Dir.x;
				baseDir.y = pWorldTbl->vStart1Dir.y;
				baseDir.z = pWorldTbl->vStart1Dir.z;
			}
		}
	}

	sVECTOR3 spawnLoc;
	spawnLoc.x = baseLoc.x + cfg.fSpawnOffset;
	spawnLoc.y = baseLoc.y;
	spawnLoc.z = baseLoc.z + cfg.fSpawnOffset;

	sVECTOR3 spawnDir;
	spawnDir.x = baseDir.x;
	spawnDir.y = baseDir.y;
	spawnDir.z = baseDir.z;

	VLog(cfg.bVerboseLogs, "HelperNPC: attempt spawn %s %u in world %u (partySize=%u) near leader at (%.2f, %.2f, %.2f)",
		bSpawnMob ? "mob" : "npc", helperTblidx, pWorld->GetID(), byCount, spawnLoc.x, spawnLoc.y, spawnLoc.z);
	sSPAWN_TBLDAT sSpawn;
	sSpawn.vSpawn_Loc.CopyFrom(spawnLoc);
	sSpawn.vSpawn_Dir.CopyFrom(spawnDir);
	sSpawn.dwParty_Index = INVALID_DWORD;
	sSpawn.byMove_Range = INVALID_BYTE;
	sSpawn.bySpawn_Move_Type = SPAWN_MOVE_UNKNOWN;
	sSpawn.bySpawn_Loc_Range = 0; // spawn exactly at computed location
	sSpawn.byWander_Range = INVALID_BYTE;
	sSpawn.path_Table_Index = INVALID_TBLIDX;
	sSpawn.playScript = INVALID_TBLIDX;
	sSpawn.playScriptScene = INVALID_TBLIDX;
	sSpawn.aiScript = INVALID_TBLIDX;
	sSpawn.aiScriptScene = INVALID_TBLIDX;
	sSpawn.actionPatternTblidx = 1;

	CNpc* pHelper = nullptr;
	if (!bSpawnMob)
	{
		sNPC_TBLDAT* pNpcTbl = (sNPC_TBLDAT*)g_pTableContainer->GetNpcTable()->FindData(helperTblidx);
		if (!pNpcTbl)
		{
			ERR_LOG(LOG_GENERAL, "HelperNPC: NPC table data missing for tblidx %u", helperTblidx);
			m_pendingSpawns.erase(key);
			return false;
		}
		CNpc* pNpc = (CNpc*)g_pObjectManager->CreateCharacter(OBJTYPE_NPC);
		if (!pNpc)
		{
			ERR_LOG(LOG_GENERAL, "HelperNPC: CreateCharacter(NPC) returned NULL");
			m_pendingSpawns.erase(key);
			return false;
		}
		if (!pNpc->CreateDataAndSpawn(pWorld->GetID(), pNpcTbl, &sSpawn, false, 0))
		{
			ERR_LOG(LOG_GENERAL, "HelperNPC: CreateDataAndSpawn(NPC) failed for %u in world %u", helperTblidx, pWorld->GetID());
			m_pendingSpawns.erase(key);
			return false;
		}
		pHelper = pNpc;
	}
	else
	{
		sMOB_TBLDAT* pMobTbl = (sMOB_TBLDAT*)g_pTableContainer->GetMobTable()->FindData(helperTblidx);
		if (!pMobTbl)
		{
			ERR_LOG(LOG_GENERAL, "HelperNPC: MOB table data missing for tblidx %u", helperTblidx);
			m_pendingSpawns.erase(key);
			return false;
		}
		CMonster* pMob = (CMonster*)g_pObjectManager->CreateCharacter(OBJTYPE_MOB);
		if (!pMob)
		{
			ERR_LOG(LOG_GENERAL, "HelperNPC: CreateCharacter(MOB) returned NULL");
			m_pendingSpawns.erase(key);
			return false;
		}
		if (!pMob->CreateDataAndSpawn(pWorld->GetID(), pMobTbl, &sSpawn, false, 0))
		{
			ERR_LOG(LOG_GENERAL, "HelperNPC: CreateDataAndSpawn(MOB) failed for %u in world %u", helperTblidx, pWorld->GetID());
			m_pendingSpawns.erase(key);
			return false;
		}
		pHelper = pMob; // CMonster derives from CNpc
	}

	// Keep helper independent from WPS/SPS script removals
	// Stand-alone true ensures dungeon scripts (e.g., RemoveNpc by tblidx) won't despawn it.
	pHelper->SetStandAlone(true);

	// Make the helper friendly to players and link it to the dungeon leader
	pHelper->SetPcRelation(RELATION_TYPE_ALLIENCE);
	pHelper->SetLinkPc(pLeader->GetCharID(), pLeader->GetID());

	// Optional: make helper invincible and untargettable if configured
	if (cfg.bInvincibleHelper)
	{
		pHelper->GetStateManager()->AddConditionState(CHARCOND_INVINCIBLE, NULL, true);
		pHelper->GetStateManager()->AddConditionState(CHARCOND_CANT_BE_TARGETTED, NULL, true);
		VLog(cfg.bVerboseLogs, "HelperNPC: invincible + untargettable set by config");
	}

	// Reload skills after linking so Helper skills initialize properly.
	pHelper->LoadSkillTable(INVALID_TBLIDX);
	VLog(cfg.bVerboseLogs, "HelperNPC: skills reloaded after link to leader %u", pLeader->GetID());

	// Optionally force-add a specific skill if configured
	if (!cfg.vForcedSkills.empty())
	{
		CSkillManagerBot* pSM = (CSkillManagerBot*)pHelper->GetSkillManager();
		if (pSM)
		{
			for (TBLIDX forcedId : cfg.vForcedSkills)
			{
				sSKILL_TBLDAT* pSkillTbldat = (sSKILL_TBLDAT*)g_pTableContainer->GetSkillTable()->FindData(forcedId);
				if (!pSkillTbldat)
				{
					VLog(cfg.bVerboseLogs, "HelperNPC: forced skill %u not found", forcedId);
					continue;
				}
				CSkillBot* pSkill = new CSkillBot;
				if (!pSkill->Create(pSkillTbldat, pHelper, INVALID_BYTE))
				{
					VLog(cfg.bVerboseLogs, "HelperNPC: forced skill %u create FAIL", forcedId);
					continue;
				}
				bool ok = pSM->AddSkill(0, pHelper, pSkill, forcedId,
					cfg.forcedSkillBasis, cfg.forcedSkillLP, cfg.forcedSkillTime);
				VLog(cfg.bVerboseLogs, "HelperNPC: forced skill %u add %s (basis=%u lp=%u time=%u)",
					forcedId, ok ? "OK" : "FAIL", cfg.forcedSkillBasis, cfg.forcedSkillLP, cfg.forcedSkillTime);
			}
		}
	}

	// Add configured buff skills (if any)
	{
		CSkillManagerBot* pSM = (CSkillManagerBot*)pHelper->GetSkillManager();
		if (pSM && !cfg.vBuffSkills.empty())
		{
			for (TBLIDX buffId : cfg.vBuffSkills)
			{
				sSKILL_TBLDAT* pSkillTbldat = (sSKILL_TBLDAT*)g_pTableContainer->GetSkillTable()->FindData(buffId);
				if (!pSkillTbldat)
				{
					VLog(cfg.bVerboseLogs, "HelperNPC: buff skill %u not found in SkillTable", buffId);
					continue;
				}
				CSkillBot* pSkill = new CSkillBot;
				if (!pSkill->Create(pSkillTbldat, pHelper, INVALID_BYTE))
				{
					VLog(cfg.bVerboseLogs, "HelperNPC: buff skill %u create FAIL", buffId);
					continue;
				}
				bool ok = pSM->AddSkill(0, pHelper, pSkill, buffId, cfg.buffBasis, cfg.buffLP, cfg.buffTime);
				VLog(cfg.bVerboseLogs, "HelperNPC: buff skill %u add %s (basis=%u lp=%u time=%u)", buffId, ok ? "OK" : "FAIL", cfg.buffBasis, cfg.buffLP, cfg.buffTime);
			}
		}
	}

	// Always ensure resurrect skill (if configured) is present regardless of BuffSkills list
	if (cfg.resurrectSkillTblidx != INVALID_TBLIDX)
	{
		CSkillManagerBot* pSM = (CSkillManagerBot*)pHelper->GetSkillManager();
		if (pSM && !pSM->FindSkillCondition(cfg.resurrectSkillTblidx))
		{
			sSKILL_TBLDAT* pSkillTbldat = (sSKILL_TBLDAT*)g_pTableContainer->GetSkillTable()->FindData(cfg.resurrectSkillTblidx);
			if (!pSkillTbldat)
			{
				VLog(cfg.bVerboseLogs, "HelperNPC: resurrect skill %u NOT FOUND in SkillTable", cfg.resurrectSkillTblidx);
			}
			else
			{
				CSkillBot* pSkill = new CSkillBot;
				if (!pSkill->Create(pSkillTbldat, pHelper, INVALID_BYTE))
				{
					VLog(cfg.bVerboseLogs, "HelperNPC: resurrect skill %u create FAIL", cfg.resurrectSkillTblidx);
				}
				else
				{
					bool ok = pSM->AddSkill(0, pHelper, pSkill, cfg.resurrectSkillTblidx, /*Give*/4, /*LP*/0, /*Time*/0);
					VLog(cfg.bVerboseLogs, "HelperNPC: resurrect skill %u add %s", cfg.resurrectSkillTblidx, ok ? "OK" : "FAIL");
				}
			}
		}
	}

	// Ensure helper has EP to cast skills
	if (pHelper->GetCurEP() < pHelper->GetMaxEP())
	{
		pHelper->SetCurEP(pHelper->GetMaxEP());
		VLog(cfg.bVerboseLogs, "HelperNPC: EP set to max (%u) for helper %u", pHelper->GetMaxEP(), pHelper->GetID());
	}

	if (cfg.bFollowLeader)
	{
		// Drive following directly without starting Escort action, to avoid escort-triggered Leave states
		sVECTOR3 vLeaderLoc;
		pLeader->GetCurLoc().CopyTo(vLeaderLoc);
		const float fFollowDist = 2.0f; // keep very close to the leader
		if (pHelper->SendCharStateFollowing(pLeader->GetID(), fFollowDist, DBO_MOVE_FOLLOW_FRIENDLY, vLeaderLoc, true))
		{
			VLog(cfg.bVerboseLogs, "HelperNPC: Direct follow started to leader %u at (%.2f, %.2f, %.2f)", pLeader->GetID(), vLeaderLoc.x, vLeaderLoc.y, vLeaderLoc.z);
		}
		else
		{
			VLog(cfg.bVerboseLogs, "HelperNPC: Direct follow failed to start (npc state transition rejected)");
		}
	}

	// Track mappings so we can assist leader's target and dedupe by helper ID
	// Keep legacy single mapping as the most recent helper
	m_mapLeaderToHelper[pLeader->GetID()] = pHelper->GetID();
	m_helperConfigByHelper[pHelper->GetID()] = cfg;
	m_helperKindByHelper[pHelper->GetID()] = helperTblidx;
	auto& list = m_leaderToHelpers[pLeader->GetID()];
	list.push_back(pHelper->GetID());

	VLog(cfg.bVerboseLogs, "HelperNPC: spawn success %s %u in world %u%s", bSpawnMob ? "mob" : "npc", helperTblidx, pWorld->GetID(), cfg.bInvincibleHelper ? " (invincible)" : "");

	// Prime AI: if newly spawned helper has no active control state queued (common after manual cleanup),
	// trigger a minimal follow refresh so Bot AI conditions evaluate next tick.
	{
		CBotAiController* pAI = (CBotAiController*)pHelper->GetBotController();
		// Prime only if controller exists and no active AI state
		if (pAI && !pAI->GetCurrentState())
		{
			// Re-send follow (idempotent) to ensure movement state set and AI updates run.
			if (cfg.bFollowLeader)
			{
				sVECTOR3 vLeaderLoc; pLeader->GetCurLoc().CopyTo(vLeaderLoc);
				pHelper->SendCharStateFollowing(pLeader->GetID(), 2.0f, DBO_MOVE_FOLLOW_FRIENDLY, vLeaderLoc, true);
				VLog(cfg.bVerboseLogs, "HelperNPC: AI prime follow resend for helper %u", pHelper->GetID());
			}
			else
			{
				// Force an idle look action if available (prevents inert state machine)
				pHelper->SendCharStateStanding();
				VLog(cfg.bVerboseLogs, "HelperNPC: AI prime idle stand for helper %u", pHelper->GetID());
			}
		}
	}

	// Clear pending guard on success
	m_pendingSpawns.erase(key);

	return true;
}

void CHelperNpcManager::OnWorldDestroyed(CWorld* pWorld)
{
	if (!pWorld)
		return;
	// Clean up helpers tied to this world
	std::vector<HOBJECT> toErase;
	for (const auto& kv : m_helperConfigByHelper)
	{
		HOBJECT hHelper = kv.first;
		CNpc* p = g_pObjectManager->GetNpc(hHelper);
		if (!p || !p->IsInitialized() || p->GetCurWorld() == pWorld)
			toErase.push_back(hHelper);
	}
	for (HOBJECT h : toErase)
	{
		m_helperConfigByHelper.erase(h);
		m_helperKindByHelper.erase(h);
		// Also remove from leader lists
		for (auto& lk : m_leaderToHelpers)
		{
			auto& vec = lk.second;
			vec.erase(std::remove(vec.begin(), vec.end(), h), vec.end());
		}
	}
	VLog(m_config.bVerboseLogs, "HelperNPC: world %u destroyed - cleaned helper tracking", pWorld->GetID());
}

void CHelperNpcManager::OnLeaderAttackTarget(CPlayer* pLeader, HOBJECT hTarget)
{
	if (!pLeader || hTarget == INVALID_HOBJECT)
		return;

	auto it = m_mapLeaderToHelper.find(pLeader->GetID());
	if (it == m_mapLeaderToHelper.end())
	{
		VLog(m_config.bVerboseLogs, "HelperNPC: no helper linked to leader %u - ignore attack target", pLeader->GetID());
		return;
	}

	CNpc* pHelper = g_pObjectManager->GetNpc(it->second);
	if (!pHelper || !pHelper->IsInitialized() || pHelper->GetCurWorld() != pLeader->GetCurWorld())
	{
		VLog(m_config.bVerboseLogs, "HelperNPC: helper handle invalid or not in same world for leader %u", pLeader->GetID());
		return;
	}

	const sHELPER_NPC_CONFIG* pcfg = nullptr;
	{
		auto itCfg = m_helperConfigByHelper.find(pHelper->GetID());
		pcfg = (itCfg != m_helperConfigByHelper.end()) ? &itCfg->second : &m_config;
	}
	if (!pcfg->bEnabled || !pcfg->bAssistLeaderTarget)
		return;

	// Only assist if the target is attackable by the helper
	CCharacter* pVictim = g_pObjectManager->GetChar(hTarget);
	if (!pVictim || !pVictim->IsInitialized())
	{
		VLog(pcfg->bVerboseLogs, "HelperNPC: leader %u target invalid - skip assist", pLeader->GetID());
		return;
	}

	if (!pHelper->IsTargetAttackble(pVictim, pHelper->GetTbldat()->wSight_Range))
	{
		VLog(pcfg->bVerboseLogs, "HelperNPC: helper %u cannot attack target %u - out of constraints", pHelper->GetID(), hTarget);
		return;
	}

	// Nudge helper's aggro to the leader's target so existing AI will attack
	// CObjMsg_YouKeepAggro msg;
	// msg.hSource = pLeader->GetID();
	// msg.hProvoker = hTarget;
	// msg.dwAggroPoint = pHelper->GetTbldat()->wBasic_Aggro_Point + 1;
	// pHelper->SendObjectMsg(&msg);
	// VLog(pcfg->bVerboseLogs, "HelperNPC: nudged aggro of helper %u toward target %u for leader %u", pHelper->GetID(), hTarget, pLeader->GetID());
}

void CHelperNpcManager::OnLeaderAttackEnd(CPlayer* pLeader)
{
	if (!pLeader)
		return;

	auto it = m_mapLeaderToHelper.find(pLeader->GetID());
	if (it == m_mapLeaderToHelper.end())
		return;

	CNpc* pHelper = g_pObjectManager->GetNpc(it->second);
	if (!pHelper || !pHelper->IsInitialized() || pHelper->GetCurWorld() != pLeader->GetCurWorld())
		return;

	const sHELPER_NPC_CONFIG* pcfg = nullptr;
	{
		auto itCfg = m_helperConfigByHelper.find(pHelper->GetID());
		pcfg = (itCfg != m_helperConfigByHelper.end()) ? &itCfg->second : &m_config;
	}
	if (!pcfg->bEnabled || !pcfg->bFollowLeader)
		return;

	// If helper has no aggro and no current target, resume following leader
	if (pHelper->GetTargetListManager()->GetAggroCount() == 0 && pHelper->GetTargetHandle() == INVALID_HOBJECT)
	{
		sVECTOR3 vLeaderLoc;
		pLeader->GetCurLoc().CopyTo(vLeaderLoc);
		const float fFollowDist = 1.5f;
		pHelper->SendCharStateFollowing(pLeader->GetID(), fFollowDist, DBO_MOVE_FOLLOW_FRIENDLY, vLeaderLoc, true);
		VLog(pcfg->bVerboseLogs, "HelperNPC: resumed follow to leader %u after combat", pLeader->GetID());
	}
}

float CHelperNpcManager::GetDamageMultiplierForHelper(CNpc* pNpc)
{
	if (!pNpc)
		return 1.0f;
	if (!IsRegisteredHelper(pNpc))
		return 1.0f;
	auto itCfg = m_helperConfigByHelper.find(pNpc->GetID());
	if (itCfg == m_helperConfigByHelper.end())
		return 1.0f;
	const sHELPER_NPC_CONFIG* pcfg = &itCfg->second;
	return pcfg->fDamageMultiplier > 0.f ? pcfg->fDamageMultiplier : 1.0f;
}

float CHelperNpcManager::GetHealMultiplierForHelper(CNpc* pNpc)
{
	if (!pNpc)
		return 1.0f;
	if (!IsRegisteredHelper(pNpc))
		return 1.0f;
	auto itCfg = m_helperConfigByHelper.find(pNpc->GetID());
	if (itCfg == m_helperConfigByHelper.end())
		return 1.0f;
	const sHELPER_NPC_CONFIG* pcfg = &itCfg->second;
	return pcfg->fHealPowerMultiplier > 0.f ? pcfg->fHealPowerMultiplier : 1.0f;
}

bool CHelperNpcManager::IsRegisteredHelper(CNpc* pNpc) const
{
	if (!pNpc)
		return false;
	if (pNpc->GetLinkPc() == INVALID_HOBJECT || pNpc->GetPcRelation() != RELATION_TYPE_ALLIENCE)
		return false;
	// Original single-helper mapping (latest helper per leader)
	for (const auto& kv : m_mapLeaderToHelper)
	{
		if (kv.second == pNpc->GetID())
			return true;
	}
	// Any helper we spawned is placed into m_helperConfigByHelper; treat presence as registration
	if (m_helperConfigByHelper.find(pNpc->GetID()) != m_helperConfigByHelper.end())
		return true;
	// Fallback: scan multi-helper list vectors (kept small, acceptable O(n))
	for (const auto& kv : m_leaderToHelpers)
	{
		for (HOBJECT h : kv.second)
		{
			if (h == pNpc->GetID())
				return true;
		}
	}
	// As a last resort, if we recorded its kind (tblidx) we also consider it registered
	if (m_helperKindByHelper.find(pNpc->GetID()) != m_helperKindByHelper.end())
		return true;
	return false;
}

const sHELPER_NPC_CONFIG* CHelperNpcManager::GetConfigForHelper(CNpc* pNpc) const
{
	if (!pNpc)
		return nullptr;
	auto it = m_helperConfigByHelper.find(pNpc->GetID());
	if (it == m_helperConfigByHelper.end())
		return nullptr;
	return &it->second;
}

void CHelperNpcManager::TickWatchdog(DWORD dwNow)
{
	const DWORD WATCHDOG_INTERVAL_MS = 2000; // light check
	if (m_dwLastWatchdogTick != 0 && (dwNow - m_dwLastWatchdogTick) < WATCHDOG_INTERVAL_MS)
		return;
	m_dwLastWatchdogTick = dwNow;

	// For each leader we know, ensure valid helpers exist in the same world
	for (const auto& kv : m_mapLeaderToHelper)
	{
		HOBJECT hLeader = kv.first;
		CPlayer* pLeader = (CPlayer*)g_pObjectManager->GetPC(hLeader);
		if (!pLeader || !pLeader->IsInitialized())
		{
			continue; // leader gone; let cleanup happen elsewhere
		}
		CWorld* pWorld = pLeader->GetCurWorld();
		if (!pWorld)
			continue;

		// Only allow helpers inside instance/dungeon-like worlds (HUNT/CCBATTLEDUNGEON/TIMEQUEST or extra lists)
		eGAMERULE_TYPE rule = pWorld->GetRuleType();
		WORLDID wid = pWorld->GetID();
		bool bUD = (rule == GAMERULE_HUNT) || (this->m_extraUDWorldIDs.find(wid) != this->m_extraUDWorldIDs.end());
		bool bBD = (rule == GAMERULE_CCBATTLEDUNGEON) || (this->m_extraBDWorldIDs.find(wid) != this->m_extraBDWorldIDs.end());
		bool bTMQ = (rule == GAMERULE_TIMEQUEST) || (this->m_extraTMQWorldIDs.find(wid) != this->m_extraTMQWorldIDs.end());
		if (!bUD && !bBD && !bTMQ)
		{
			// Not a dynamic world; ensure any helpers in this world for this leader are despawned
			DespawnAllHelpersForLeaderInWorld(pLeader, pWorld);
			continue;
		}

		// Check all helpers for this leader
		std::vector<TBLIDX> existingKinds;
		{
			auto itList = m_leaderToHelpers.find(hLeader);
			if (itList != m_leaderToHelpers.end())
			{
				for (HOBJECT h : itList->second)
				{
					CNpc* hh = g_pObjectManager->GetNpc(h);
					if (hh && hh->IsInitialized() && hh->GetCurWorld() == pWorld)
					{
						auto itK = m_helperKindByHelper.find(h);
						if (itK != m_helperKindByHelper.end()) existingKinds.push_back(itK->second);
					}
				}
			}
		}

		// Determine proper config for this world (UD vs BD vs TMQ) based on world rule type
		const sHELPER_NPC_CONFIG* pCfg = &m_config;
	// rule flags already computed above
		if (bUD)
			pCfg = m_hasUDOverride ? &m_cfgUD : &m_config;
		else if (bBD)
			pCfg = m_hasBDOverride ? &m_cfgBD : &m_config;
		else if (bTMQ)
			pCfg = m_hasTMQOverride ? &m_cfgTMQ : &m_config;

		if (!pCfg->bEnabled)
			continue;

		// Only care about dungeons we allow
		if (bUD && !pCfg->bAllowUltimate) continue;
		if (bBD && !pCfg->bAllowBattleDungeon) continue;
		if (bTMQ && !pCfg->bAllowTimeQuest) continue;

		// Re-run spawn rule: if party is small enough
		BYTE byCount = 1;
		if (pLeader->GetParty())
			byCount = pLeader->GetParty()->GetPartyMemberCount();
		if (byCount >= pCfg->byMinPartySizeToAvoidSpawn)
			continue;

		// Evaluate role-based helpers first (healer/tank/buffer/speed)
		EvaluateAndSpawnRoleHelpers(pLeader, pWorld);

		// If leader currently has no helper of this config’s helper ID, attempt base spawn
		TBLIDX desiredTblidx = INVALID_TBLIDX;
		if (pCfg->bUseMobAsHelper)
			desiredTblidx = pCfg->helperMobTblidx;
		else if (g_pTableContainer->GetNpcTable()->FindData(pCfg->primaryNpcTblidx))
			desiredTblidx = pCfg->primaryNpcTblidx;
		else
			desiredTblidx = pCfg->fallbackNpcTblidx;

		bool hasSame = false;
		for (TBLIDX t : existingKinds) { if (t == desiredTblidx) { hasSame = true; break; } }
		if (!hasSame)
		{
			VLog(pCfg->bVerboseLogs, "HelperNPC: watchdog spawn for leader %u world %u (missing helper tblidx %u)", pLeader->GetID(), pWorld->GetID(), desiredTblidx);
			SpawnIfAllowed(pLeader, pWorld, *pCfg);
		}

		// Done
	}
}

void CHelperNpcManager::EnsureHelperForLeaderNow(CPlayer* pLeader)
{
	if (!pLeader || !pLeader->IsInitialized())
		return;
	CWorld* pWorld = pLeader->GetCurWorld();
	if (!pWorld)
		return;

	// Prevent duplicate helper sets: only the party leader (or solo player) may trigger ensure logic.
	if (pLeader->GetParty() && pLeader->GetParty()->GetPartyLeaderID() != pLeader->GetID())
	{
		VLog(m_config.bVerboseLogs, "HelperNPC: EnsureHelperForLeaderNow skip - player %u not party leader (%u)", pLeader->GetID(), pLeader->GetParty()->GetPartyLeaderID());
		return;
	}

	// If helper is already present in this world for this leader, nothing to do
	auto it = m_mapLeaderToHelper.find(pLeader->GetID());
	if (it != m_mapLeaderToHelper.end())
	{
		CNpc* pHelper = g_pObjectManager->GetNpc(it->second);
		if (pHelper && pHelper->IsInitialized() && pHelper->GetCurWorld() == pWorld)
		{
			// Re-link to ensure the helper targets the correct leader handle after respawn
			pHelper->SetLinkPc(pLeader->GetCharID(), pLeader->GetID());
			// Reassert follow/assist to recover from any stale state after respawn
			const sHELPER_NPC_CONFIG* pcfg = GetConfigForHelper(pHelper);
			const sHELPER_NPC_CONFIG& cfg = pcfg ? *pcfg : m_config;
			if (cfg.bFollowLeader)
			{
				sVECTOR3 vLeaderLoc; pLeader->GetCurLoc().CopyTo(vLeaderLoc);
				pHelper->SendCharStateFollowing(pLeader->GetID(), 1.5f, DBO_MOVE_FOLLOW_FRIENDLY, vLeaderLoc, true);
				VLog(cfg.bVerboseLogs, "HelperNPC: EnsureHelperForLeaderNow - reassert follow to leader %u in world %u", pLeader->GetID(), pWorld->GetID());
			}
			if (cfg.bAssistLeaderTarget)
			{
				HOBJECT hVictim = pLeader->GetTargetHandle();
				if (hVictim != INVALID_HOBJECT)
				{
					if (pHelper->GetTargetHandle() != hVictim) pHelper->SetTargetHandle(hVictim);
					VLog(cfg.bVerboseLogs, "HelperNPC: EnsureHelperForLeaderNow - reassert assist target %u for leader %u", hVictim, pLeader->GetID());
				}
			}
			return;
		}
	}

	// Choose config by world rule type, with allowlists
	const sHELPER_NPC_CONFIG* pCfg = &m_config;
	eGAMERULE_TYPE rule = pWorld->GetRuleType();
	WORLDID wid = pWorld->GetID();
	bool forceUD = (this->m_extraUDWorldIDs.find(wid) != this->m_extraUDWorldIDs.end());
	bool forceBD = (this->m_extraBDWorldIDs.find(wid) != this->m_extraBDWorldIDs.end());
	bool forceTMQ = (this->m_extraTMQWorldIDs.find(wid) != this->m_extraTMQWorldIDs.end());
	if (rule == GAMERULE_HUNT || forceUD)
		pCfg = m_hasUDOverride ? &m_cfgUD : &m_config;
	else if (rule == GAMERULE_CCBATTLEDUNGEON || forceBD)
		pCfg = m_hasBDOverride ? &m_cfgBD : &m_config;
	else if (rule == GAMERULE_TIMEQUEST || forceTMQ)
		pCfg = m_hasTMQOverride ? &m_cfgTMQ : &m_config;

	if (!pCfg->bEnabled)
		return;

	if ((rule == GAMERULE_HUNT || forceUD) && !pCfg->bAllowUltimate) return;
	if ((rule == GAMERULE_CCBATTLEDUNGEON || forceBD) && !pCfg->bAllowBattleDungeon) return;
	if ((rule == GAMERULE_TIMEQUEST || forceTMQ) && !pCfg->bAllowTimeQuest) return;

	// Party size rule
	BYTE byCount = 1;
	if (pLeader->GetParty()) byCount = pLeader->GetParty()->GetPartyMemberCount();
	if (byCount >= pCfg->byMinPartySizeToAvoidSpawn) return;

	// Allow immediate repair by clearing any stale world mark and spawning now
	m_worldsWithHelper.erase(pWorld->GetID());
	VLog(pCfg->bVerboseLogs, "HelperNPC: EnsureHelperForLeaderNow - attempting immediate repair for leader %u world %u", pLeader->GetID(), pWorld->GetID());
	// Spawn role helpers first to avoid base helper blocking same-tblidx roles (e.g., HEALER)
	EvaluateAndSpawnRoleHelpers(pLeader, pWorld);
	// Then spawn base helper if allowed (will be skipped if party size threshold met)
	SpawnIfAllowed(pLeader, pWorld, *pCfg);
}

void CHelperNpcManager::DespawnAllHelpersForLeaderInWorld(CPlayer* pLeader, CWorld* pWorld)
{
	if (!pLeader || !pWorld) return;
	auto itList = m_leaderToHelpers.find(pLeader->GetID());
	if (itList == m_leaderToHelpers.end()) return;
	std::vector<HOBJECT> kept;
	for (HOBJECT h : itList->second)
	{
		CNpc* hh = g_pObjectManager->GetNpc(h);
		if (hh && hh->IsInitialized() && hh->GetCurWorld() == pWorld)
		{
			if (hh->GetBotController())
				hh->GetBotController()->ChangeControlState_Despawn();
			m_helperConfigByHelper.erase(hh->GetID());
			m_helperKindByHelper.erase(hh->GetID());
			continue;
		}
		kept.push_back(h);
	}
	itList->second.swap(kept);
	if (itList->second.empty())
		m_mapLeaderToHelper.erase(pLeader->GetID());
}

void CHelperNpcManager::OnLeaderLeaveWorld(CPlayer* pLeader, CWorld* pWorld)
{
	if (!pLeader || !pWorld) return;
	// Despawn helpers whenever the leader leaves a world. If it's a dungeon world, helpers should not persist once leader exits.
	DespawnAllHelpersForLeaderInWorld(pLeader, pWorld);
	VLog(m_config.bVerboseLogs, "HelperNPC: leader %u left world %u - despawned helpers in that world", pLeader->GetID(), pWorld->GetID());
}

void CHelperNpcManager::EvaluateAndSpawnRoleHelpers(CPlayer* pLeader, CWorld* pWorld)
{
	if (!pLeader || !pWorld) return;

	// Only allow the party leader (or solo) to evaluate and spawn role helpers to avoid one set per member.
	if (pLeader->GetParty() && pLeader->GetParty()->GetPartyLeaderID() != pLeader->GetID())
	{
		VLog(m_config.bVerboseLogs, "HelperNPC: Role evaluation skip - player %u not party leader (%u)", pLeader->GetID(), pLeader->GetParty()->GetPartyLeaderID());
		return;
	}

	auto evalRole = [&](const sROLE_DEF& role) {
		if (!role.enabled) return;

		// Respect world types using the role cfg's allow flags
		const sHELPER_NPC_CONFIG& cfg = role.cfg;
		eGAMERULE_TYPE rule = pWorld->GetRuleType();
		WORLDID wid = pWorld->GetID();
		bool bUD = (rule == GAMERULE_HUNT) || (this->m_extraUDWorldIDs.find(wid) != this->m_extraUDWorldIDs.end());
		bool bBD = (rule == GAMERULE_CCBATTLEDUNGEON) || (this->m_extraBDWorldIDs.find(wid) != this->m_extraBDWorldIDs.end());
		bool bTMQ = (rule == GAMERULE_TIMEQUEST) || (this->m_extraTMQWorldIDs.find(wid) != this->m_extraTMQWorldIDs.end());
		if ((bUD && !cfg.bAllowUltimate) || (bBD && !cfg.bAllowBattleDungeon) || (bTMQ && !cfg.bAllowTimeQuest)) return;

		// Party size rule
		BYTE byCount = 1;
		if (pLeader->GetParty()) byCount = pLeader->GetParty()->GetPartyMemberCount();
		if (byCount >= cfg.byMinPartySizeToAvoidSpawn) return;

		// Determine if party already covers this role
		bool covered = false;
		if (pLeader->GetParty())
		{
			CParty* party = pLeader->GetParty();
			BYTE mc = party->GetPartyMemberCount();
			for (BYTE i = 0; i < mc; ++i)
			{
				const sPARTY_MEMBER_INFO& info = party->GetMemberInfo(i);
				if (role.coveredClasses.find((int)info.byClass) != role.coveredClasses.end())
				{ covered = true; break; }
			}
		}
		// If no class list provided, require at least 1 party member to skip spawning (otherwise always missing)
		if (role.coveredClasses.empty())
		{
			// If only solo (no party), treat as missing
			if (!pLeader->GetParty() || pLeader->GetParty()->GetPartyMemberCount() <= 1)
				covered = false;
			else
				covered = true; // party exists; assume covered unless stated otherwise
		}
		if (covered) return;

		// Desired helper ID for this role
		TBLIDX desiredTblidx = INVALID_TBLIDX;
		bool useMob = cfg.bUseMobAsHelper;
		if (useMob)
			desiredTblidx = cfg.helperMobTblidx;
		else if (g_pTableContainer->GetNpcTable()->FindData(cfg.primaryNpcTblidx))
			desiredTblidx = cfg.primaryNpcTblidx;
		else
			desiredTblidx = cfg.fallbackNpcTblidx;
		if (desiredTblidx == INVALID_TBLIDX || desiredTblidx == 0) return;

		// Count existing helpers of this kind for this leader in this world
		int existing = 0;
		{
			auto itList = m_leaderToHelpers.find(pLeader->GetID());
			if (itList != m_leaderToHelpers.end())
			{
				for (HOBJECT h : itList->second)
				{
					CNpc* hh = g_pObjectManager->GetNpc(h);
					if (hh && hh->IsInitialized() && hh->GetCurWorld() == pWorld)
					{
						auto itK = m_helperKindByHelper.find(h);
						if (itK != m_helperKindByHelper.end() && itK->second == desiredTblidx)
							existing++;
					}
				}
			}
		}
		if (existing >= role.maxCount) return;

		VLog(cfg.bVerboseLogs, "HelperNPC: role spawn [%s] for leader %u world %u (existing=%d < max=%u)",
			(useMob ? "MOB" : "NPC"), pLeader->GetID(), pWorld->GetID(), existing, (unsigned)role.maxCount);
		SpawnIfAllowed(pLeader, pWorld, cfg);
	};

	evalRole(m_roleHealer);
	evalRole(m_roleTank);
	evalRole(m_roleBuffer);
	evalRole(m_roleSpeed);
}

void CHelperNpcManager::RemoveRoleHelpersForLeader(CPlayer* pLeader, CWorld* pWorld, bool removeHealer, bool removeTank, bool removeBuffer, bool removeSpeed)
{
	if (!pLeader || !pWorld) return;
	auto it = m_leaderToHelpers.find(pLeader->GetID());
	if (it == m_leaderToHelpers.end()) return;

	std::vector<HOBJECT> kept;
	for (HOBJECT h : it->second)
	{
		CNpc* hh = g_pObjectManager->GetNpc(h);
		if (!hh || !hh->IsInitialized() || hh->GetCurWorld() != pWorld)
		{
			// Clean dangling
			m_helperConfigByHelper.erase(h);
			m_helperKindByHelper.erase(h);
			continue;
		}
		TBLIDX kind = INVALID_TBLIDX;
		{
			auto itK = m_helperKindByHelper.find(h);
			if (itK != m_helperKindByHelper.end()) kind = itK->second;
		}
	bool isHealer = false, isTank = false, isBuffer = false, isSpeed = false;
		if (kind != INVALID_TBLIDX)
		{
			// Classify by comparing with role-config desired IDs
			auto classify = [&](const sROLE_DEF& role, bool& out) {
				if (!role.enabled) return;
				const sHELPER_NPC_CONFIG& cfg = role.cfg;
				TBLIDX desired = cfg.bUseMobAsHelper ? cfg.helperMobTblidx : (g_pTableContainer->GetNpcTable()->FindData(cfg.primaryNpcTblidx) ? cfg.primaryNpcTblidx : cfg.fallbackNpcTblidx);
				if (desired != INVALID_TBLIDX && desired == kind) out = true;
			};
			classify(m_roleHealer, isHealer);
			classify(m_roleTank, isTank);
			classify(m_roleBuffer, isBuffer);
			classify(m_roleSpeed, isSpeed);
		}

		bool shouldRemove = (removeHealer && isHealer) || (removeTank && isTank) || (removeBuffer && isBuffer) || (removeSpeed && isSpeed);
		if (shouldRemove)
		{
			VLog(m_config.bVerboseLogs, "HelperNPC: removing role helper kind %u for leader %u due to party composition change", kind, pLeader->GetID());
			// Transition helper to despawn state for a clean removal
			if (hh->GetBotController())
				hh->GetBotController()->ChangeControlState_Despawn();
			// Remove tracking immediately to prevent re-spawn loops until next watchdog cycle
			m_helperConfigByHelper.erase(hh->GetID());
			m_helperKindByHelper.erase(hh->GetID());
			continue; // do not keep
		}
		kept.push_back(h);
	}
	it->second.swap(kept);
	// Update single latest mapping if needed
	if (!it->second.empty())
		m_mapLeaderToHelper[pLeader->GetID()] = it->second.back();
	else
		m_mapLeaderToHelper.erase(pLeader->GetID());
}

void CHelperNpcManager::ResetMetrics()
{
	for (auto& kv : m_helperConfigByHelper)
	{
		sHELPER_NPC_CONFIG& cfg = kv.second;
		cfg.dwMetricResurrectAttempts = 0;
		cfg.dwMetricResurrectRetries = 0;
		cfg.dwMetricResurrectSuccess = 0;
		cfg.dwMetricBuffsQueuedMissing = 0;
		cfg.dwMetricBuffsQueuedRefresh = 0;
	}
	NTL_PRINT(PRINT_APP, "HelperNPC: metrics reset for %zu helpers", m_helperConfigByHelper.size());
}

void CHelperNpcManager::DumpMetrics()
{
	DWORD totalRA=0,totalRR=0,totalRS=0,totalBM=0,totalBR=0;
	for (auto& kv : m_helperConfigByHelper)
	{
		const sHELPER_NPC_CONFIG& cfg = kv.second;
		totalRA += cfg.dwMetricResurrectAttempts;
		totalRR += cfg.dwMetricResurrectRetries;
		totalRS += cfg.dwMetricResurrectSuccess;
		totalBM += cfg.dwMetricBuffsQueuedMissing;
		totalBR += cfg.dwMetricBuffsQueuedRefresh;
		NTL_PRINT(PRINT_APP, "HelperNPC: helper=%u stats Ra=%u Rr=%u Rs=%u Bm=%u Br=%u", kv.first, cfg.dwMetricResurrectAttempts, cfg.dwMetricResurrectRetries, cfg.dwMetricResurrectSuccess, cfg.dwMetricBuffsQueuedMissing, cfg.dwMetricBuffsQueuedRefresh);
	}
	NTL_PRINT(PRINT_APP, "HelperNPC: totals Ra=%u Rr=%u Rs=%u Bm=%u Br=%u helpers=%zu", totalRA,totalRR,totalRS,totalBM,totalBR,m_helperConfigByHelper.size());
}

void CHelperNpcManager::RefreshAllHelpers(bool bRespawn)
{
	// Capture leaders + worlds first
	std::vector<std::pair<HOBJECT, CWorld*>> leaderWorlds;
	for (auto& kv : m_leaderToHelpers)
	{
		HOBJECT leader = kv.first;
		CPlayer* pLeader = g_pObjectManager->GetPC(leader);
		if (!pLeader || !pLeader->IsInitialized()) continue;
		CWorld* pWorld = pLeader->GetCurWorld();
		if (!pWorld) continue;
		leaderWorlds.emplace_back(leader, pWorld);
	}

	// Properly despawn existing helpers using engine lifecycle (LeaveGame -> DestroyCharacter)
	for (auto& kv : m_leaderToHelpers)
	{
		for (HOBJECT h : kv.second)
		{
			CNpc* npc = g_pObjectManager->GetNpc(h);
			if (!npc) continue;
			// Ensure respawn flag cleared so ConsiderRespawn() path fully destroys
			npc->SetSpawnFuncFlag(0);
			npc->LeaveGame(); // handles party/script cleanup & ConsiderRespawn()
			// If npc still exists in object manager and not scheduled for respawn, destroy directly
			if (!npc->IsInRespawn())
			{
				g_pObjectManager->DestroyCharacter(npc);
			}
		}
	}
	// Clear tracking maps after lifecycle teardown
	m_leaderToHelpers.clear();
	m_helperConfigByHelper.clear();
	m_helperKindByHelper.clear();

	if (!bRespawn) {
		NTL_PRINT(PRINT_APP, "HelperNPC: all helpers despawned (no respawn requested)");
		return;
	}
	// Respawn based on current config
	for (auto& lw : leaderWorlds)
	{
		CPlayer* pLeader = g_pObjectManager->GetPC(lw.first);
		CWorld* pWorld = lw.second;
		if (!pLeader || !pWorld) continue;
		// Base helper (if still allowed)
		SpawnIfAllowed(pLeader, pWorld, m_config);
		// Role helpers
		EvaluateAndSpawnRoleHelpers(pLeader, pWorld);
	}
	NTL_PRINT(PRINT_APP, "HelperNPC: helpers refreshed (respawned) leaders=%zu", leaderWorlds.size());
}

void CHelperNpcManager::OnPartyMemberJoined(CParty* pParty, CPlayer* pNewMember)
{
	if (!pParty || !pNewMember) return;
	// Only act if the party is in a dungeon world where helpers can exist
	CPlayer* pLeader = g_pObjectManager->GetPC(pParty->GetPartyLeaderID());
	if (!pLeader || !pLeader->IsInitialized()) return;
	CWorld* pWorld = pLeader->GetCurWorld();
	if (!pWorld) return;

	// Determine the role coverage impact of the new member
	BYTE cls = pNewMember->GetClass();
	bool hasHealerNow = false;
	bool hasBufferTankNow = false;
	bool hasSpeedNow = false;

	// Healer: explicit class id 15 as requested
	if (cls == 15)
		hasHealerNow = true;

	// Buffer/Tank: Ultimate Majin or Grand Chef (IDs depend on server; use examples provided)
	// From earlier defaults: GrandChef=17, UltimateMajin=18. Treat either as tank+buffer coverage.
	if (cls == 17 || cls == 18)
		hasBufferTankNow = true;
	// Speed buffer classes: Poko=16, Karma=20
	if (cls == 16 || cls == 20)
		hasSpeedNow = true;

	if (!hasHealerNow && !hasBufferTankNow && !hasSpeedNow)
		return;

	// Remove appropriate role helpers
	RemoveRoleHelpersForLeader(pLeader, pWorld, /*removeHealer*/hasHealerNow, /*removeTank*/hasBufferTankNow, /*removeBuffer*/hasBufferTankNow, /*removeSpeed*/hasSpeedNow);
}
void CHelperNpcManager::OnPartyLeaderChanged(HOBJECT oldLeader, HOBJECT newLeader)
{
	if (oldLeader == INVALID_HOBJECT || newLeader == INVALID_HOBJECT || oldLeader == newLeader)
		return;

	// Move single-latest mapping if it matches oldLeader
	auto itSingle = m_mapLeaderToHelper.find(oldLeader);
	if (itSingle != m_mapLeaderToHelper.end())
	{
		HOBJECT hHelper = itSingle->second;
		m_mapLeaderToHelper.erase(itSingle);
		m_mapLeaderToHelper[newLeader] = hHelper;
	}

	// Move list of helpers to new leader
	auto itList = m_leaderToHelpers.find(oldLeader);
	if (itList != m_leaderToHelpers.end())
	{
		auto& from = itList->second;
		auto& to = m_leaderToHelpers[newLeader];
		to.insert(to.end(), from.begin(), from.end());
		m_leaderToHelpers.erase(itList);

		// Re-link each helper to the new leader and restart following
		CPlayer* pNewLeader = (CPlayer*)g_pObjectManager->GetPC(newLeader);
		if (pNewLeader && pNewLeader->IsInitialized())
		{
			for (HOBJECT h : to)
			{
				CNpc* pHelper = g_pObjectManager->GetNpc(h);
				if (!pHelper || !pHelper->IsInitialized())
					continue;
				pHelper->SetLinkPc(pNewLeader->GetCharID(), newLeader);
				pHelper->SetPcRelation(RELATION_TYPE_ALLIENCE);
				if (pHelper->GetCurWorld() == pNewLeader->GetCurWorld())
				{
					sVECTOR3 vLeaderLoc; pNewLeader->GetCurLoc().CopyTo(vLeaderLoc);
					const float fFollowDist = 2.0f;
					pHelper->SendCharStateFollowing(newLeader, fFollowDist, DBO_MOVE_FOLLOW_FRIENDLY, vLeaderLoc, true);
				}
			}
			EvaluateAndSpawnRoleHelpers(pNewLeader, pNewLeader->GetCurWorld());
		}
	}
}
