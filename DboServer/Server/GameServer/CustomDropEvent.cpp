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
#include "calcs.h"
#include <string>

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
	m_timeStart = 0;
	m_timeEnd = 0;
	m_dwNextUpdateTick = 0;
	m_mobDrops.clear();
	m_mobMods.clear();
	m_mobVisuals.clear();
	m_cfgPath = ".\\config\\CustomDropEvent.cfg";
	LoadConfigInternal(m_cfgPath.c_str());
	LoadLevelsSidecar(m_cfgPath.c_str());
}

bool CCustomDropEvent::ReloadConfig(const char* path)
{
	if (!path)
		path = m_cfgPath.c_str();
	bool ok = LoadConfigInternal(path);
	LoadLevelsSidecar(path);
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
		bool isMods = false;
		bool isSpawn = false;
		bool isBuffs = false;
		bool isTitles = false;
		bool isVisuals = false;
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
				const char* tail = sp2 + 1;
				while (*tail == ' ' || *tail == '\t')
					++tail;
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
			}
			else
			{
				mobId = (unsigned int)strtoul(sp, nullptr, 10);
			}
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
				auto &dst = m_mobSpawns[mobId];
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
				auto &dst = m_mobBuffs[mobId];
				dst.insert(dst.end(), entries.begin(), entries.end());
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
				auto &dst = m_mobTitles[mobId];
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
				auto &dst = m_mobVisuals[mobId];
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

	if (dwTick < m_dwNextUpdateTick)
		return;

	if (m_bOn)
	{
		if (app->GetTime() >= m_timeEnd)
		{
			EndEvent();
		}
	}

	m_dwNextUpdateTick = dwTick + 5000; // update every 5 seconds
}

void CCustomDropEvent::Update(CMonster* pMob, CCharacter* pPlayer)
{
	if (!pPlayer->GetCurWorld())
		return;
	if (pPlayer->GetCurWorld()->GetRuleType() != GAMERULE_NORMAL)
		return;

	if (!m_bOn)
		return;

	// Config-driven spawn logic: spawn configured mobs on kill
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
		if (itAll != m_mobSpawns.end())
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
					for (BYTE i = 0; i < cnt; ++i)
					{
						sMOB_DATA data;
						InitMobData(data);
						data.worldID = pMob->GetWorldID();
						data.worldtblidx = pMob->GetWorldTblidx();
						data.tblidx = pTbldat->tblidx;
						pMob->GetCurLoc().CopyTo(data.vCurLoc);
						pMob->GetCurLoc().CopyTo(data.vSpawnLoc);
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
	if (itDAll != m_mobDrops.end())
	{
		list.insert(list.end(), itDAll->second.begin(), itDAll->second.end());
	}
	if (list.empty())
		return;
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

// Helper: load levels from a JSON sidecar next to the cfg
// JSON format: { "3131102": 50, "46661101": 70 }
bool CCustomDropEvent::LoadLevelsSidecar(const char* cfgPath)
{
	// Build sidecar path by replacing extension with .levels.json
	char sidecar[1024] = {0};
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
		char key[64] = {0};
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
		char valbuf[16] = {0};
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
	if (itAll != m_mobMods.end())
	{
		const Modifiers &g = itAll->second;
		m.hp *= g.hp; m.physAtk *= g.physAtk; m.engAtk *= g.engAtk; m.physDef *= g.physDef; m.engDef *= g.engDef;
		m.atkSpd *= g.atkSpd; m.runSpd *= g.runSpd; m.physCrit *= g.physCrit; m.engCrit *= g.engCrit;
		m.physCritDmg *= g.physCritDmg; m.engCritDmg *= g.engCritDmg; m.attackRate *= g.attackRate; m.dodgeRate *= g.dodgeRate;
		m.blockRate *= g.blockRate; m.blockDmg *= g.blockDmg; m.guardRate *= g.guardRate;
		if (g.sizeRate > 0) m.sizeRate = g.sizeRate;
	}
	auto it = m_mobMods.find(pMob->GetTblidx());
	if (it != m_mobMods.end())
	{
		const Modifiers &s = it->second;
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
	if (itAll != m_mobBuffs.end())
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
	if (itAll != m_mobTitles.end())
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
	if (itAll != m_mobVisuals.end())
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
