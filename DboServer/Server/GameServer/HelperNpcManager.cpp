#include "stdafx.h"
#include "HelperNpcManager.h"
#include "GameServer.h"
#include "CPlayer.h"
#include "World.h"
#include "Npc.h"
#include "Monster.h"
#include "ObjectManager.h"
#include "TableContainerManager.h"
#include "NtlIniFile.h"
#include "ObjectMsg.h"
#include "NtlResultCode.h"
#include "Npc.h"
#include <string>
#include <sstream>

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
	}
	file.Read("HELPER_NPC", "AttackScanRange", m_config.wAttackScanRange);
	file.Read("HELPER_NPC", "AttackScanCooldownMs", m_config.dwAttackScanCooldownMs);

	// Force skill (optional)
	file.Read("HELPER_NPC", "ForcedSkillTblidx", m_config.forcedSkillTblidx);
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
			std::istringstream iss(s);
			std::string tok;
			while (std::getline(iss, tok, ','))
			{
				if (!tok.empty())
				{
					TBLIDX id = (TBLIDX)atoi(tok.c_str());
					if (id != 0 && id != INVALID_TBLIDX)
						m_config.vBuffSkills.push_back(id);
				}
			}
		}
		int basis = m_config.buffBasis;
		if (file.Read("HELPER_NPC", "BuffBasis", basis)) m_config.buffBasis = (BYTE)basis;
		file.Read("HELPER_NPC", "BuffLP", m_config.buffLP);
		file.Read("HELPER_NPC", "BuffTime", m_config.buffTime);
	}
	return true;
}

bool CHelperNpcManager::SpawnHelperIfNeededForDungeon(CPlayer* pLeader, CWorld* pWorld, bool bIsUltimateDungeon)
{
	if (!pLeader || !pWorld)
	{
		ERR_LOG(LOG_GENERAL, "HelperNPC: skip spawn - invalid args pLeader=%p pWorld=%p", pLeader, pWorld);
		return false;
	}

	if (!m_config.bEnabled)
	{
		ERR_LOG(LOG_GENERAL, "HelperNPC: disabled by config");
		return false;
	}

	if (bIsUltimateDungeon && !m_config.bAllowUltimate)
	{
		ERR_LOG(LOG_GENERAL, "HelperNPC: UD not allowed by config");
		return false;
	}
	if (!bIsUltimateDungeon && !m_config.bAllowBattleDungeon)
	{
		ERR_LOG(LOG_GENERAL, "HelperNPC: BattleDungeon not allowed by config");
		return false;
	}

	// Treat solo (no party) as party size 1 so helper can spawn for solo entries
	BYTE byCount = 1;
	if (pLeader->GetParty())
		byCount = pLeader->GetParty()->GetPartyMemberCount();
	else
		ERR_LOG(LOG_GENERAL, "HelperNPC: leader has no party - treating as solo size 1");

	if (byCount >= m_config.byMinPartySizeToAvoidSpawn)
	{
		ERR_LOG(LOG_GENERAL, "HelperNPC: skip - party size %u >= threshold %u", byCount, m_config.byMinPartySizeToAvoidSpawn);
		return false; // party large enough; no helper
	}

	// Prevent duplicate spawns within the same world instance
	if (m_worldsWithHelper.find(pWorld->GetID()) != m_worldsWithHelper.end())
	{
		ERR_LOG(LOG_GENERAL, "HelperNPC: skip - helper already spawned in world %u", pWorld->GetID());
		return false;
	}

	// Decide what to spawn (NPC vs MOB)
	bool bSpawnMob = false;
	TBLIDX helperTblidx = INVALID_TBLIDX;
	if (m_config.bUseMobAsHelper)
	{
		if (g_pTableContainer->GetMobTable()->FindData(m_config.helperMobTblidx))
		{
			bSpawnMob = true;
			helperTblidx = m_config.helperMobTblidx;
		}
		else
		{
			ERR_LOG(LOG_GENERAL, "HelperNPC: configured MOB %u not found", m_config.helperMobTblidx);
			return false;
		}
	}
	else
	{
		if (g_pTableContainer->GetNpcTable()->FindData(m_config.primaryNpcTblidx))
			helperTblidx = m_config.primaryNpcTblidx;
		else if (g_pTableContainer->GetNpcTable()->FindData(m_config.fallbackNpcTblidx))
			helperTblidx = m_config.fallbackNpcTblidx;
		else if (g_pTableContainer->GetMobTable()->FindData(m_config.helperMobTblidx))
		{
			// fallback to MOB healer if NPCs not found
			bSpawnMob = true;
			helperTblidx = m_config.helperMobTblidx;
		}
		else
		{
			ERR_LOG(LOG_GENERAL, "HelperNPC: no valid NPC or MOB tblidx (npc primary %u, fallback %u; mob %u)",
				m_config.primaryNpcTblidx, m_config.fallbackNpcTblidx, m_config.helperMobTblidx);
			return false;
		}
	}

	// Use world start location/direction so we spawn where players land post-teleport
	const sWORLD_TBLDAT* pWorldTbl = pWorld->GetTbldat();
	sVECTOR3 baseLoc;
	sVECTOR3 baseDir;
	if (pWorldTbl)
	{
		baseLoc.x = pWorldTbl->vStart1Loc.x;
		baseLoc.y = pWorldTbl->vStart1Loc.y;
		baseLoc.z = pWorldTbl->vStart1Loc.z;
		baseDir.x = pWorldTbl->vStart1Dir.x;
		baseDir.y = pWorldTbl->vStart1Dir.y;
		baseDir.z = pWorldTbl->vStart1Dir.z;
	}
	else
	{
		const auto& pl = pLeader->GetCurLoc();
		const auto& pd = pLeader->GetCurDir();
		baseLoc.x = pl.x; baseLoc.y = pl.y; baseLoc.z = pl.z;
		baseDir.x = pd.x; baseDir.y = pd.y; baseDir.z = pd.z;
	}

	sVECTOR3 spawnLoc;
	spawnLoc.x = baseLoc.x + m_config.fSpawnOffset;
	spawnLoc.y = baseLoc.y;
	spawnLoc.z = baseLoc.z + m_config.fSpawnOffset;

	sVECTOR3 spawnDir;
	spawnDir.x = baseDir.x;
	spawnDir.y = baseDir.y;
	spawnDir.z = baseDir.z;

	ERR_LOG(LOG_GENERAL, "HelperNPC: attempt spawn %s %u in world %u (partySize=%u, UD=%u) at Start1Loc (%.2f, %.2f, %.2f)",
		bSpawnMob ? "mob" : "npc", helperTblidx, pWorld->GetID(), byCount, bIsUltimateDungeon ? 1 : 0, spawnLoc.x, spawnLoc.y, spawnLoc.z);
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
			return false;
		}
		CNpc* pNpc = (CNpc*)g_pObjectManager->CreateCharacter(OBJTYPE_NPC);
		if (!pNpc)
		{
			ERR_LOG(LOG_GENERAL, "HelperNPC: CreateCharacter(NPC) returned NULL");
			return false;
		}
		if (!pNpc->CreateDataAndSpawn(pWorld->GetID(), pNpcTbl, &sSpawn, false, 0))
		{
			ERR_LOG(LOG_GENERAL, "HelperNPC: CreateDataAndSpawn(NPC) failed for %u in world %u", helperTblidx, pWorld->GetID());
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
			return false;
		}
		CMonster* pMob = (CMonster*)g_pObjectManager->CreateCharacter(OBJTYPE_MOB);
		if (!pMob)
		{
			ERR_LOG(LOG_GENERAL, "HelperNPC: CreateCharacter(MOB) returned NULL");
			return false;
		}
		if (!pMob->CreateDataAndSpawn(pWorld->GetID(), pMobTbl, &sSpawn, false, 0))
		{
			ERR_LOG(LOG_GENERAL, "HelperNPC: CreateDataAndSpawn(MOB) failed for %u in world %u", helperTblidx, pWorld->GetID());
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
	if (m_config.bInvincibleHelper)
	{
		pHelper->GetStateManager()->AddConditionState(CHARCOND_INVINCIBLE, NULL, true);
		pHelper->GetStateManager()->AddConditionState(CHARCOND_CANT_BE_TARGETTED, NULL, true);
		ERR_LOG(LOG_GENERAL, "HelperNPC: invincible + untargettable set by config");
	}

	// Reload skills after linking so Helper skills initialize properly.
	pHelper->LoadSkillTable(INVALID_TBLIDX);
	ERR_LOG(LOG_GENERAL, "HelperNPC: skills reloaded after link to leader %u", pLeader->GetID());

	// Optionally force-add a specific skill if configured
	if (m_config.forcedSkillTblidx != INVALID_TBLIDX)
	{
		CSkillManagerBot* pSM = (CSkillManagerBot*)pHelper->GetSkillManager();
		if (pSM)
		{
			sSKILL_TBLDAT* pSkillTbldat = (sSKILL_TBLDAT*)g_pTableContainer->GetSkillTable()->FindData(m_config.forcedSkillTblidx);
			if (pSkillTbldat)
			{
				CSkillBot* pSkill = new CSkillBot;
				if (pSkill->Create(pSkillTbldat, pHelper, INVALID_BYTE))
				{
					bool ok = pSM->AddSkill(0, pHelper, pSkill, m_config.forcedSkillTblidx,
						m_config.forcedSkillBasis, m_config.forcedSkillLP, m_config.forcedSkillTime);
					ERR_LOG(LOG_GENERAL, "HelperNPC: forced skill %u add %s (basis=%u lp=%u time=%u)",
						m_config.forcedSkillTblidx, ok ? "OK" : "FAIL", m_config.forcedSkillBasis, m_config.forcedSkillLP, m_config.forcedSkillTime);
				}
				else
				{
					ERR_LOG(LOG_GENERAL, "HelperNPC: forced skill %u create FAIL", m_config.forcedSkillTblidx);
				}
			}
			else
			{
				ERR_LOG(LOG_GENERAL, "HelperNPC: forced skill %u not found in SkillTable", m_config.forcedSkillTblidx);
			}
		}
	}

	// Optionally add a list of buff skills
	if (!m_config.vBuffSkills.empty())
	{
		CSkillManagerBot* pSM = (CSkillManagerBot*)pHelper->GetSkillManager();
		if (pSM)
		{
			for (TBLIDX buffId : m_config.vBuffSkills)
			{
				sSKILL_TBLDAT* pSkillTbldat = (sSKILL_TBLDAT*)g_pTableContainer->GetSkillTable()->FindData(buffId);
				if (!pSkillTbldat)
				{
					ERR_LOG(LOG_GENERAL, "HelperNPC: buff skill %u not found in SkillTable", buffId);
					continue;
				}
				CSkillBot* pSkill = new CSkillBot;
				if (!pSkill->Create(pSkillTbldat, pHelper, INVALID_BYTE))
				{
					ERR_LOG(LOG_GENERAL, "HelperNPC: buff skill %u create FAIL", buffId);
					continue;
				}
				bool ok = pSM->AddSkill(0, pHelper, pSkill, buffId, m_config.buffBasis, m_config.buffLP, m_config.buffTime);
				ERR_LOG(LOG_GENERAL, "HelperNPC: buff skill %u add %s (basis=%u lp=%u time=%u)",
					buffId, ok ? "OK" : "FAIL", m_config.buffBasis, m_config.buffLP, m_config.buffTime);
			}
		}
	}

	// Ensure helper has EP to cast skills
	if (pHelper->GetCurEP() < pHelper->GetMaxEP())
	{
		pHelper->SetCurEP(pHelper->GetMaxEP());
		ERR_LOG(LOG_GENERAL, "HelperNPC: EP set to max (%u) for helper %u", pHelper->GetMaxEP(), pHelper->GetID());
	}

	if (m_config.bFollowLeader)
	{
		// Drive following directly without starting Escort action, to avoid escort-triggered Leave states
		sVECTOR3 vLeaderLoc;
		pLeader->GetCurLoc().CopyTo(vLeaderLoc);
		const float fFollowDist = 2.0f; // keep very close to the leader
		if (pHelper->SendCharStateFollowing(pLeader->GetID(), fFollowDist, DBO_MOVE_FOLLOW_FRIENDLY, vLeaderLoc, true))
		{
			ERR_LOG(LOG_GENERAL, "HelperNPC: Direct follow started to leader %u at (%.2f, %.2f, %.2f)", pLeader->GetID(), vLeaderLoc.x, vLeaderLoc.y, vLeaderLoc.z);
		}
		else
		{
			ERR_LOG(LOG_GENERAL, "HelperNPC: Direct follow failed to start (npc state transition rejected)");
		}
	}

	// Track mapping so we can assist leader's target and avoid duplicates per world
	m_worldsWithHelper.insert(pWorld->GetID());
	m_mapLeaderToHelper[pLeader->GetID()] = pHelper->GetID();

	ERR_LOG(LOG_GENERAL, "HelperNPC: spawn success %s %u in world %u%s", bSpawnMob ? "mob" : "npc", helperTblidx, pWorld->GetID(), m_config.bInvincibleHelper ? " (invincible)" : "");

	return true;
}

void CHelperNpcManager::OnWorldDestroyed(CWorld* pWorld)
{
	if (!pWorld)
		return;
	m_worldsWithHelper.erase(pWorld->GetID());
	ERR_LOG(LOG_GENERAL, "HelperNPC: world %u destroyed - cleared helper mark", pWorld->GetID());
}

void CHelperNpcManager::OnLeaderAttackTarget(CPlayer* pLeader, HOBJECT hTarget)
{
	if (!m_config.bEnabled || !m_config.bAssistLeaderTarget)
		return;
	if (!pLeader || hTarget == INVALID_HOBJECT)
		return;

	auto it = m_mapLeaderToHelper.find(pLeader->GetID());
	if (it == m_mapLeaderToHelper.end())
		return;

	CNpc* pHelper = g_pObjectManager->GetNpc(it->second);
	if (!pHelper || !pHelper->IsInitialized() || pHelper->GetCurWorld() != pLeader->GetCurWorld())
		return;

	// Only assist if the target is attackable by the helper
	CCharacter* pVictim = g_pObjectManager->GetChar(hTarget);
	if (!pVictim || !pVictim->IsInitialized())
		return;

	if (!pHelper->IsTargetAttackble(pVictim, pHelper->GetTbldat()->wSight_Range))
		return;

	// Nudge helper's aggro to the leader's target so existing AI will attack
	CObjMsg_YouKeepAggro msg;
	msg.hSource = pLeader->GetID();
	msg.hProvoker = hTarget;
	msg.dwAggroPoint = pHelper->GetTbldat()->wBasic_Aggro_Point + 1;
	pHelper->SendObjectMsg(&msg);
}

void CHelperNpcManager::OnLeaderAttackEnd(CPlayer* pLeader)
{
	if (!m_config.bEnabled || !m_config.bFollowLeader)
		return;
	if (!pLeader)
		return;

	auto it = m_mapLeaderToHelper.find(pLeader->GetID());
	if (it == m_mapLeaderToHelper.end())
		return;

	CNpc* pHelper = g_pObjectManager->GetNpc(it->second);
	if (!pHelper || !pHelper->IsInitialized() || pHelper->GetCurWorld() != pLeader->GetCurWorld())
		return;

	// If helper has no aggro and no current target, resume following leader
	if (pHelper->GetTargetListManager()->GetAggroCount() == 0 && pHelper->GetTargetHandle() == INVALID_HOBJECT)
	{
		sVECTOR3 vLeaderLoc;
		pLeader->GetCurLoc().CopyTo(vLeaderLoc);
		const float fFollowDist = 1.5f;
		pHelper->SendCharStateFollowing(pLeader->GetID(), fFollowDist, DBO_MOVE_FOLLOW_FRIENDLY, vLeaderLoc, true);
	}
}

float CHelperNpcManager::GetDamageMultiplierForHelper(CNpc* pNpc)
{
	if (!pNpc)
		return 1.0f;
	// A helper is identified by having a link to a PC and being marked allied, and also being tracked in our leader->helper map.
	if (pNpc->GetLinkPc() != INVALID_HOBJECT && pNpc->GetPcRelation() == RELATION_TYPE_ALLIENCE)
	{
		// Ensure this npc is one of our registered helpers
		for (const auto& kv : m_mapLeaderToHelper)
		{
			if (kv.second == pNpc->GetID())
			{
				return m_config.fDamageMultiplier > 0.f ? m_config.fDamageMultiplier : 1.0f;
			}
		}
	}
	return 1.0f;
}

float CHelperNpcManager::GetHealMultiplierForHelper(CNpc* pNpc)
{
	if (!pNpc)
		return 1.0f;
	if (pNpc->GetLinkPc() != INVALID_HOBJECT && pNpc->GetPcRelation() == RELATION_TYPE_ALLIENCE)
	{
		for (const auto& kv : m_mapLeaderToHelper)
		{
			if (kv.second == pNpc->GetID())
			{
				return m_config.fHealPowerMultiplier > 0.f ? m_config.fHealPowerMultiplier : 1.0f;
			}
		}
	}
	return 1.0f;
}
