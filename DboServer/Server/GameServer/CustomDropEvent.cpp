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
	m_cfgPath = ".\\config\\CustomDropEvent.cfg";
	LoadConfigInternal(m_cfgPath.c_str());
}

bool CCustomDropEvent::ReloadConfig(const char *path)
{
	if (!path)
		path = m_cfgPath.c_str();
	return LoadConfigInternal(path);
}

bool CCustomDropEvent::LoadConfigInternal(const char *path)
{
	m_mobDrops.clear();
	m_mobMods.clear();
	m_mobSpawns.clear();

	FILE *f = nullptr;
	errno_t e = fopen_s(&f, path, "rt");
	if (e != 0 || !f)
		return false;

	// Line formats:
	// mobId: itemId@rate, itemId@rate, ...
	// mobId modifiers: hp=1.5 physAtk=1.2 engAtk=1.0 physDef=1.1 engDef=1.0
	// Lines starting with # are comments
	char line[1024];
	while (fgets(line, sizeof(line), f))
	{
		// trim leading spaces
		char *p = line;
		while (*p == ' ' || *p == '\t')
			++p;
		if (*p == '\0' || *p == '\n' || *p == '#')
			continue;

		// detect optional sections: "modifiers", "spawn" or "spawns"
		const char *modsKw = "modifiers";
		const char *spawnKw = "spawn";
		const char *spawnsKw = "spawns";
		bool isMods = false;
		bool isSpawn = false;
		char *colon = strchr(p, ':');
		if (!colon)
			continue;
		*colon = '\0';
		// check for "<id> modifiers|spawn" key (supports "all" as wildcard)
		unsigned int mobId = 0;
		{
			// split p by spaces to detect keyword
			char *sp = p;
			while (*sp == ' ' || *sp == '\t')
				++sp;
			// find space
			char *sp2 = strchr(sp, ' ');
			if (sp2)
			{
				*sp2 = '\0';
				if (_stricmp(sp, "all") == 0)
					mobId = 0; // wildcard: apply to all mobs
				else
					mobId = (unsigned int)strtoul(sp, nullptr, 10);
				const char *tail = sp2 + 1;
				while (*tail == ' ' || *tail == '\t')
					++tail;
				if (_stricmp(tail, modsKw) == 0)
					isMods = true;
				else if (_stricmp(tail, spawnKw) == 0 || _stricmp(tail, spawnsKw) == 0)
					isSpawn = true;
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
			char *s = colon + 1;
			// tokenize by whitespace
			char *t = strtok(s, " \t\n\r");
			while (t)
			{
				char *eq = strchr(t, '=');
				if (eq)
				{
					*eq = '\0';
					const char *key = t;
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
			char *list = colon + 1;
			std::vector<SpawnEntry> entries;
			// split by comma
			char *tok = strtok(list, ",\n\r");
			while (tok)
			{
				// trim
				while (*tok == ' ' || *tok == '\t')
					++tok;
				unsigned int toSpawn = 0;
				float rate = 100.f;
				BYTE cnt = 1;
				char *at = strchr(tok, '@');
				if (at)
				{
					*at = '\0';
					toSpawn = (unsigned int)strtoul(tok, nullptr, 10);
					char *rx = at + 1;
					char *x = strchr(rx, 'x');
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
					entries.push_back(SpawnEntry{toSpawn, rate, cnt});
				}
				tok = strtok(nullptr, ",\n\r");
			}
			if (!entries.empty())
			{
				m_mobSpawns[mobId] = entries;
			}
		}
		else
		{
			char *list = colon + 1;
			std::vector<DropEntry> entries;
			// split by comma
			char *tok = strtok(list, ",\n\r");
			while (tok)
			{
				// trim
				while (*tok == ' ' || *tok == '\t')
					++tok;
				// token format itemId@ratexcount OR itemId@rate OR itemId (default rate=100, count=1)
				unsigned int itemId = 0;
				float rate = 100.f;
				BYTE count = 1;
				char *at = strchr(tok, '@');
				if (at)
				{
					*at = '\0';
					itemId = (unsigned int)strtoul(tok, nullptr, 10);
					char *rx = at + 1;
					char *x = strchr(rx, 'x');
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

	CGameServer *app = (CGameServer *)g_pApp;

	m_bOn = true;
	m_timeStart = app->GetTime();
	m_timeEnd = m_timeStart + (byHours * 3600);

	CNtlStringW msg;

	CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	sGU_SYSTEM_DISPLAY_TEXT *res = (sGU_SYSTEM_DISPLAY_TEXT *)packet.GetPacketData();
	res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
	res->wMessageLengthInUnicode = (WORD)msg.Format(L"Custom Drop Event Started! Duration: %u Hours.", byHours);
	res->byDisplayType = SERVER_TEXT_EMERGENCY;
	NTL_SAFE_WCSCPY(res->awchMessage, msg.c_str());
	packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	g_pObjectManager->SendPacketToAll(&packet);
}

void CCustomDropEvent::TickProcess(DWORD dwTick)
{
	CGameServer *app = (CGameServer *)g_pApp;

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

void CCustomDropEvent::Update(CMonster *pMob, CCharacter *pPlayer)
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
			for (const SpawnEntry &se : spawns)
			{
				if (se.mobTblidx == INVALID_TBLIDX)
					continue;
				// Enforce level-gap gating only for sub-100% rates
				if (se.rate < 100.f && levelGap > 10)
					continue;
				if (se.rate >= 100.f || Dbo_CheckProbabilityF(se.rate))
				{
					sMOB_TBLDAT *pTbldat = (sMOB_TBLDAT *)g_pTableContainer->GetMobTable()->FindData(se.mobTblidx);
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

						if (CMonster *pNewMob = (CMonster *)g_pObjectManager->CreateCharacter(OBJTYPE_MOB))
						{
							if (pNewMob->CreateDataAndSpawn(data, pTbldat))
							{
								// Dynamically scale spawned mobs based on killer mob level
								{
									BYTE baseLvl = pMob->GetLevel();
									CCharacterAtt *att = pNewMob->GetCharAtt();
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
	for (const DropEntry &d : list)
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
	sGU_SYSTEM_DISPLAY_TEXT *res = (sGU_SYSTEM_DISPLAY_TEXT *)packet.GetPacketData();
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
	sGU_SYSTEM_DISPLAY_TEXT *resMsg = (sGU_SYSTEM_DISPLAY_TEXT *)packetMsg.GetPacketData();
	resMsg->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
	resMsg->byDisplayType = SERVER_TEXT_EMERGENCY;
	resMsg->wMessageLengthInUnicode = (WORD)msg.Format(L"Custom Drop Event is currently running!");
	NTL_SAFE_WCSCPY(resMsg->awchMessage, msg.c_str());
	g_pApp->Send(hSession, &packetMsg);
}

void CCustomDropEvent::CreateSingleDrop(CMonster *pMob, CCharacter *pPlayer, unsigned int dropId)
{
	ERR_LOG(LOG_GENERAL, "[DropTrace] CustomDropEvent CreateSingleDrop mob=%u player=%u item=%u", pMob->GetTblidx(), pPlayer->GetID(), dropId);
	CItemDrop *pDrop = g_pItemManager->CreateSingleDrop(100.f, dropId);
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

void CCustomDropEvent::CreateStackedDrop(CMonster *pMob, CCharacter *pPlayer, unsigned int dropId, BYTE count)
{
	if (count <= 1)
	{
		CreateSingleDrop(pMob, pPlayer, dropId);
		return;
	}

	sITEM_TBLDAT *pItemData = (sITEM_TBLDAT *)g_pTableContainer->GetItemTable()->FindData(dropId);
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
			CItemDrop *pDrop = g_pItemManager->CreateSingleDrop(100.f, dropId);
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

void CCustomDropEvent::ApplyModifiers(CMonster *pMob)
{
	if (!m_bOn)
		return;
	auto it = m_mobMods.find(pMob->GetTblidx());
	if (it == m_mobMods.end())
		return;
	const Modifiers &m = it->second;
	if (m.IsIdentity())
		return;

	CCharacterAtt *att = pMob->GetCharAtt();
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
