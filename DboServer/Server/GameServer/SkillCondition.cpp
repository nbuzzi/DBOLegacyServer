#include "stdafx.h"
#include "SkillCondition.h"
#include "Monster.h"
#include "SpellAreaChecker.h"
#include "ObjectManager.h"
#include "HelperNpcManager.h"
// Added for PC party access when helper is a MOB/NPC
#include "CPlayer.h"
#include "Party.h"
#include <algorithm>



CSkillCondition::CSkillCondition()
{
	m_pBot = NULL;
	m_pSkill = NULL;
	m_Skill_Tblidx = INVALID_TBLIDX;
	m_byUse_Skill_Basis = INVALID_BYTE;
	m_wUse_Skill_LP = INVALID_WORD;
	m_wUse_Skill_Time = INVALID_WORD;
	m_dwTime = 0;
	m_bySkillConditionIdx = INVALID_BYTE;

	m_bCanUse = true;
}

CSkillCondition::~CSkillCondition()
{
	Destroy();
}


void CSkillCondition::Destroy()
{
	this->m_pBot = NULL;
	this->m_pSkill = NULL;
	this->m_Skill_Tblidx = INVALID_TBLIDX;
	this->m_byUse_Skill_Basis = INVALID_BYTE;
	this->m_wUse_Skill_LP = INVALID_WORD;
	this->m_wUse_Skill_Time = INVALID_WORD;
	this->m_dwTime = 0;
	m_bCanUse = false;
}


CSkillBot* CSkillCondition::OnUpdate(DWORD dwTickTime)
{
	m_dwTime = UnsignedSafeIncrease<DWORD>(m_dwTime, dwTickTime + 1000);

	if (m_dwTime >= m_wUse_Skill_Time)
	{
		m_dwTime = 0;
		if (GetBot()->GetCurEP() >= m_pSkill->GetOriginalTableData()->wRequire_EP)
		{
			if (m_pSkill->GetCoolTimeRemaining() == 0)
				return m_pSkill;
		}
	}

	return NULL;
}


bool CSkillCondition::GetTarget(HOBJECT & hTarget, sSKILL_TARGET_LIST & rTargetList)
{
	if (!m_pSkill || !GetBot())
	{
		ERR_LOG(LOG_BOTAI, "fail : NULL skill/bot in GetTarget");
		rTargetList.Init();
		return false;
	}
	switch (m_pSkill->GetOriginalTableData()->byAppoint_Target)
	{
		case DBO_SKILL_APPOINT_TARGET_SELF:
		{
			HOBJECT self = GetBot()->GetID();
			if (self == INVALID_HOBJECT)
			{
				ERR_LOG(LOG_BOTAI, "fail : INVALID_HOBJECT self in GetTarget");
				rTargetList.Init();
				return false;
			}
			hTarget = self;
			SkillAppointTargetSelf(rTargetList);
		}
		break;
		case DBO_SKILL_APPOINT_TARGET_TARGET:
		{
			SkillAppointTargetTarget(hTarget, rTargetList);
		}
		break;
		default: 
		{
			ERR_LOG(LOG_USER, "fail : switch( m_pSkill->GetOriginalTableData()->byAppoint_Target(%u) )", m_pSkill->GetOriginalTableData()->byAppoint_Target);
			return false;
		}
		break;
	}

	return true;
}


void CSkillCondition::SkillAppointTargetSelf(sSKILL_TARGET_LIST & rTargetList)
{
	switch (m_pSkill->GetOriginalTableData()->byApply_Target)
	{
		case DBO_SKILL_APPLY_TARGET_SELF:
		{
			AppointTargetSelf_ApplyTargetSelf(rTargetList);
		}
		break;
		case DBO_SKILL_APPLY_TARGET_ENEMY:
		{
			AppointTargetSelf_ApplyTargetEnemy(rTargetList);
		}
		break;

		case DBO_SKILL_APPLY_TARGET_ALLIANCE:
		case DBO_SKILL_APPLY_TARGET_PARTY:
		case DBO_SKILL_APPLY_TARGET_MOB_PARTY:
		{
			AppointTargetSelf_ApplyTargetParty(rTargetList);
		}
		break;

		case DBO_SKILL_APPLY_TARGET_ANY:
		{
			if (GetBot() && m_pSkill && GetHelperNpcManager()->IsRegisteredHelper(GetBot()))
			{
				GetTarget_ApplyRange_Party(GetBot(), rTargetList, GetTargetMaxCount());
			}
		}
		break;

		case DBO_SKILL_APPLY_TARGET_SUMMON:
		{
			ERR_LOG(LOG_SYSTEM, "DBO_SKILL_APPLY_TARGET_SUMMON value is used by in NPC Server.");
		}
		break;

		case DBO_SKILL_APPLY_TARGET_ANY_NPC:
		{
			ERR_LOG(LOG_SYSTEM, "DBO_SKILL_APPLY_TARGET_ANY_NPC value is used by in NPC Server.");
		}
		break;

		case DBO_SKILL_APPLY_TARGET_ANY_MOB:
		{
			ERR_LOG(LOG_SYSTEM, "DBO_SKILL_APPLY_TARGET_ANY_MOB value is used by in NPC Server.");
		}
		break;

		case DBO_SKILL_APPLY_TARGET_ANY_ALLIANCE:
		{
			ERR_LOG(LOG_SYSTEM, "DBO_SKILL_APPLY_TARGET_ANY_ALLIANCE value is used by in NPC Server.");
		}
		break;

		default:
		{
			ERR_LOG(LOG_SYSTEM, "fail : switch( m_pSkill->GetOriginalTableData()->byApply_Target(%u) )", m_pSkill->GetOriginalTableData()->byApply_Target);
		}
		break;
	}
}


void CSkillCondition::SkillAppointTargetTarget(HOBJECT & hTarget, sSKILL_TARGET_LIST & rTargetList)
{
	switch (m_pSkill->GetOriginalTableData()->byApply_Target)
	{
	case DBO_SKILL_APPLY_TARGET_SELF:
	{
		AppointTargetTarget_ApplyTargetSelf(hTarget, rTargetList);
	}
	break;
	case DBO_SKILL_APPLY_TARGET_ENEMY:
	{
		AppointTargetTarget_ApplyTargetEnemy(hTarget, rTargetList);
	}
	break;

	case DBO_SKILL_APPLY_TARGET_ALLIANCE:
	case DBO_SKILL_APPLY_TARGET_PARTY:
	case DBO_SKILL_APPLY_TARGET_MOB_PARTY:
	{
		AppointTargetTarget_ApplyTargetParty(hTarget, rTargetList);
	}
	break;

	case DBO_SKILL_APPLY_TARGET_ANY:
	{
		if (GetBot() && m_pSkill && GetHelperNpcManager()->IsRegisteredHelper(GetBot()))
		{
			GetTarget_ApplyRange_Party(GetBot(), rTargetList, GetTargetMaxCount());
		}
	}
	break;


	case DBO_SKILL_APPLY_TARGET_SUMMON:
	{
		ERR_LOG(LOG_SYSTEM, "DBO_SKILL_APPLY_TARGET_SUMMON value is used by in NPC Server.");
	}
	break;

	case DBO_SKILL_APPLY_TARGET_ANY_NPC:
	{
		ERR_LOG(LOG_SYSTEM, "DBO_SKILL_APPLY_TARGET_ANY_NPC value is used by in NPC Server.");
	}
	break;

	case DBO_SKILL_APPLY_TARGET_ANY_MOB:
	{
		ERR_LOG(LOG_SYSTEM, "DBO_SKILL_APPLY_TARGET_ANY_MOB value is used by in NPC Server.");
	}
	break;

	case DBO_SKILL_APPLY_TARGET_ANY_ALLIANCE:
	{
		ERR_LOG(LOG_SYSTEM, "DBO_SKILL_APPLY_TARGET_ANY_ALLIANCE value is used by in NPC Server.");
	}
	break;

	default:
	{
		ERR_LOG(LOG_SYSTEM, "fail : switch( m_pSkill->GetOriginalTableData()->byApply_Target(%u) )", m_pSkill->GetOriginalTableData()->byApply_Target);
	}
	break;
	}
}


void CSkillCondition::AppointTargetSelf_ApplyTargetSelf(sSKILL_TARGET_LIST & rTargetList)
{
	if (IsApplyNotMe())
	{
		ERR_LOG(LOG_BOTAI, "fail : true == bIsApplyNotMe()");
		rTargetList.Init();
	}
	else
	{
		// Helper enhancement: if this is a helper casting a non-enemy skill, expand to party
		bool handled = false;
		if (GetBot() && m_pSkill && GetHelperNpcManager()->IsRegisteredHelper(GetBot()))
		{
			BYTE applyTarget = m_pSkill->GetOriginalTableData()->byApply_Target;
			bool nonEnemy = (applyTarget != DBO_SKILL_APPLY_TARGET_ENEMY);
			if (nonEnemy)
			{
				GetTarget_ApplyRange_Party(GetBot(), rTargetList, GetTargetMaxCount());
				handled = true;
			}
		}
		if (!handled && GetBot())
		{
			HOBJECT self = GetBot()->GetID();
			if (self != INVALID_HOBJECT)
				rTargetList.AddTarget(self);
		}
	}
}


void CSkillCondition::AppointTargetSelf_ApplyTargetEnemy(sSKILL_TARGET_LIST & rTargetList)
{
	if (!GetBot()) { rTargetList.Init(); return; }
	HOBJECT hT = GetBot()->GetTargetHandle();
	if (hT != INVALID_HOBJECT)
		rTargetList.AddTarget(hT);

	if (GetApplyRangeType())
	{
		if (GetBot()->GetObjType() == OBJTYPE_MOB)
			GetTarget_ApplyRange_PCandNPC(GetBot(), rTargetList);
		else
			GetTarget_ApplyRange_Bot(GetBot(), rTargetList);
	}
}


void CSkillCondition::AppointTargetSelf_ApplyTargetParty(sSKILL_TARGET_LIST & rTargetList)
{
	if (GetApplyRangeType())
	{
		GetTarget_ApplyRange_Party(GetBot(), rTargetList, GetTargetMaxCount());
	}
	else
	{
		GetTarget_ApplyRange_Party(GetBot(), rTargetList, 1);
	}
}


void CSkillCondition::AppointTargetTarget_ApplyTargetSelf(HOBJECT & hTarget, sSKILL_TARGET_LIST & rTargetList)
{
	if (!GetBot()) { rTargetList.Init(); return; }
	hTarget = GetBot()->GetID();
	if (IsApplyNotMe())
	{
		ERR_LOG(LOG_BOTAI, "fail : true == bIsApplyNotMe()");
		rTargetList.Init();
	}
	else
	{
		bool handled = false;
		if (m_pSkill && GetHelperNpcManager()->IsRegisteredHelper(GetBot()))
		{
			BYTE applyTarget = m_pSkill->GetOriginalTableData()->byApply_Target;
			bool nonEnemy = (applyTarget != DBO_SKILL_APPLY_TARGET_ENEMY);
			if (nonEnemy)
			{
				GetTarget_ApplyRange_Party(GetBot(), rTargetList, GetTargetMaxCount());
				handled = true;
			}
		}
		if (!handled && hTarget != INVALID_HOBJECT)
			rTargetList.AddTarget(hTarget);
	}
}


void CSkillCondition::AppointTargetTarget_ApplyTargetEnemy(HOBJECT & hTarget, sSKILL_TARGET_LIST & rTargetList)
{
	if (!GetBot()) { rTargetList.Init(); return; }
	hTarget = GetBot()->GetTargetHandle();
	if (hTarget != INVALID_HOBJECT)
		rTargetList.AddTarget(hTarget);

	if (GetApplyRangeType())
	{
		if (GetBot()->GetObjType() == OBJTYPE_MOB)
			GetTarget_ApplyRange_PCandNPC(GetBot(), rTargetList);
		else
			GetTarget_ApplyRange_Bot(GetBot(), rTargetList);
	}
}


void CSkillCondition::AppointTargetTarget_ApplyTargetParty(HOBJECT & hTarget, sSKILL_TARGET_LIST & rTargetList)
{
	if (!GetBot()) { rTargetList.Init(); return; }
	hTarget = GetBot()->GetID();

	if (GetApplyRangeType())
	{
		GetTarget_ApplyRange_Party(GetBot(), rTargetList, GetTargetMaxCount());
	}
	else
	{
		GetTarget_ApplyRange_Party(GetBot(), rTargetList, 1);
	}
}


HOBJECT CSkillCondition::IsObjectInApplyRingRange()
{
	if (!GetBot())
		return INVALID_HOBJECT;
	if (GetBot()->GetObjType() == OBJTYPE_NPC)
		return (GetBot())->ConsiderScanTargetRingRange(m_wUse_Skill_LP);
	else if (GetBot()->GetObjType() == OBJTYPE_MOB)
		return ((CMonster*)GetBot())->ConsiderScanTargetRingRange(m_wUse_Skill_LP);

	return INVALID_HOBJECT;
}


void CSkillCondition::GetTargetApplyRange_Cell(eOBJTYPE byObjType, CWorldCell *pCell, CSpellAreaChecker& cSpellAreaChecker, sSKILL_TARGET_LIST& rTargetList, BYTE byMaxTargetCount)
{
	for (CSpawnObject* pObject = pCell->GetObjectList()->GetFirst(byObjType);
		rTargetList.byTargetCount < (BYTE)byMaxTargetCount && pObject;
		pObject = pCell->GetObjectList()->GetNext(pObject->GetWorldCellObjectLinker()))
	{
		if (cSpellAreaChecker.IsObjectInApplyRange(pObject, NULL))
		{
			// If the caster is a registered helper and this is an ENEMY skill, exclude PCs from AoE.
			// For non-enemy (heal/buff) skills, allow PCs so party-wide effects can include players.
			if (GetHelperNpcManager()->IsRegisteredHelper(GetBot()))
			{
				if (pObject->GetObjType() == OBJTYPE_PC)
				{
					if (m_pSkill && m_pSkill->GetOriginalTableData()->byApply_Target == DBO_SKILL_APPLY_TARGET_ENEMY)
						continue;
				}
			}
			if (!rTargetList.IsExist(pObject->GetID()))
			{
				HOBJECT hid = pObject->GetID();
				if (hid != INVALID_HOBJECT)
					rTargetList.AddTarget(hid);
			}
		}
	}
}


void CSkillCondition::GetTargetApplyRange_Cell(CWorldCell *pCell, CSpellAreaChecker& cSpellAreaChecker, sSKILL_TARGET_LIST& rTargetList, BYTE byMaxTargetCount)
{
	for (CNpc* pObject = (CNpc*)pCell->GetObjectList()->GetFirst(OBJTYPE_NPC);
		rTargetList.byTargetCount < (BYTE)byMaxTargetCount && pObject;
		pObject = (CNpc*)pCell->GetObjectList()->GetNext(pObject->GetWorldCellObjectLinker()))
	{
		if (cSpellAreaChecker.IsObjectInApplyRange(pObject, NULL))
		{
			if (!rTargetList.IsExist(pObject->GetID()))
			{
				if (pObject->HasFunction(NPC_FUNC_FLAG_SCAN_BY_MOB))
				{
					HOBJECT hid = pObject->GetID();
					if (hid != INVALID_HOBJECT)
						rTargetList.AddTarget(hid);
				}
			}
		}
	}
}


void CSkillCondition::GetTarget_ApplyRange_PCandNPC(CCharacter *pAppointTarget, sSKILL_TARGET_LIST& rTargetList)
{
	if (!GetBot()) { rTargetList.Init(); return; }
	CSpellAreaChecker rSpellAreaChecker;
	rSpellAreaChecker.Create();

	{
		float a1 = (float)GetApplyAreaSize1();
		float a2 = (float)GetApplyAreaSize2();
		// If not enemy-targeting, extend apply area by config bonus (helpers only)
		if (m_pSkill && m_pSkill->GetOriginalTableData()->byApply_Target != DBO_SKILL_APPLY_TARGET_ENEMY)
		{
			if (GetHelperNpcManager()->IsRegisteredHelper(GetBot()))
			{
				const sHELPER_NPC_CONFIG* pcfg = GetHelperNpcManager()->GetConfigForHelper(GetBot());
				if (pcfg)
				{
					a1 += pcfg->fHealApplyAreaBonusMeters;
					a2 += pcfg->fHealApplyAreaBonusMeters;
				}
			}
		}
		rSpellAreaChecker.PrepareForSelection(GetBot(), pAppointTarget, GetApplyRangeType(), (int)a1, (int)a2);
	}

	if (GetBot())
	{
		if (GetBot()->GetTargetListManager())
		{
			GetBot()->GetTargetListManager()->GetTargetApplyRange(rSpellAreaChecker, rTargetList, GetTargetMaxCount());

			CWorldCell* pWorldCell = GetBot()->GetCurWorldCell();
			if (pWorldCell)
			{
				CWorldCell::QUADPAGE page = pWorldCell->GetCellQuadPage(GetBot()->GetCurLoc());

				for (int dir = CWorldCell::QUADDIR_SELF; dir < CWorldCell::QUADDIR_COUNT; dir++)
				{
					CWorldCell* pCellSibling = pWorldCell->GetQuadSibling(page, (CWorldCell::QUADDIR)dir);
					if (pCellSibling)
					{
						GetTargetApplyRange_Cell(OBJTYPE_PC, pCellSibling, rSpellAreaChecker, rTargetList, GetTargetMaxCount());

						if (GetTargetMaxCount() <= rTargetList.byTargetCount)
							return;

						GetTargetApplyRange_Cell(pCellSibling, rSpellAreaChecker, rTargetList, GetTargetMaxCount());

						if (GetTargetMaxCount() <= rTargetList.byTargetCount)
							return;

						GetTargetApplyRange_Cell(OBJTYPE_SUMMON_PET, pCellSibling, rSpellAreaChecker, rTargetList, GetTargetMaxCount());

						if (GetTargetMaxCount() <= rTargetList.byTargetCount)
							return;
					}
				}
			}
		}
		else
			ERR_LOG(LOG_BOTAI, "fail : NULL == pTargetListManager");
	}
	else
		ERR_LOG(LOG_BOTAI, "fail : NULL == m_pChar");
}


void CSkillCondition::GetTarget_ApplyRange_Bot(CCharacter *pAppointTarget, sSKILL_TARGET_LIST& rTargetList)
{
	if (!GetBot()) { rTargetList.Init(); return; }
	CSpellAreaChecker rSpellAreaChecker;
	rSpellAreaChecker.Create();

	rSpellAreaChecker.PrepareForSelection(GetBot(), pAppointTarget, GetApplyRangeType(), GetApplyAreaSize1(), GetApplyAreaSize2());

	if (GetBot())
	{
		if (GetBot()->GetTargetListManager())
		{
			GetBot()->GetTargetListManager()->GetTargetApplyRange(rSpellAreaChecker, rTargetList, GetTargetMaxCount());

			CWorldCell* pWorldCell = GetBot()->GetCurWorldCell();
			if (pWorldCell)
			{
				CWorldCell::QUADPAGE page = pWorldCell->GetCellQuadPage(GetBot()->GetCurLoc());

				for (int dir = CWorldCell::QUADDIR_SELF; dir < CWorldCell::QUADDIR_COUNT; dir++)
				{
					CWorldCell* pCellSibling = pWorldCell->GetQuadSibling(page, (CWorldCell::QUADDIR)dir);
					if (pCellSibling)
					{
						GetTargetApplyRange_Cell(OBJTYPE_MOB, pCellSibling, rSpellAreaChecker, rTargetList, GetTargetMaxCount());

						if (GetTargetMaxCount() <= rTargetList.byTargetCount)
							return;
					}
				}
			}
		}
		else
			ERR_LOG(LOG_BOTAI, "fail : NULL == pTargetListManager");
	}
	else
		ERR_LOG(LOG_BOTAI, "fail : NULL == m_pChar");
}


void CSkillCondition::GetTarget_ApplyRange_Party(CCharacter *pAppointTarget, sSKILL_TARGET_LIST& rTargetList, BYTE byMaxTargetCount)
{
	if (!GetBot()) { rTargetList.Init(); return; }
	CSpellAreaChecker rSpellAreaChecker;
	rSpellAreaChecker.Create();

	{
		float a1 = (float)GetApplyAreaSize1();
		float a2 = (float)GetApplyAreaSize2();
		bool nonEnemy = m_pSkill && (m_pSkill->GetOriginalTableData()->byApply_Target != DBO_SKILL_APPLY_TARGET_ENEMY);
		bool partyWide = false;
		if (nonEnemy && GetHelperNpcManager()->IsRegisteredHelper(GetBot()))
		{
			const sHELPER_NPC_CONFIG* pcfg = GetHelperNpcManager()->GetConfigForHelper(GetBot());
			if (pcfg)
			{
				partyWide = pcfg->bBuffPartyWide;
				// Prefer buff-wide override for party coverage, else add heal bonus
				if (pcfg->fBuffApplyAreaMeters > 0.0f)
				{
					a1 = (a1 > pcfg->fBuffApplyAreaMeters) ? a1 : pcfg->fBuffApplyAreaMeters;
					a2 = (a2 > pcfg->fBuffApplyAreaMeters) ? a2 : pcfg->fBuffApplyAreaMeters;
				}
				a1 += pcfg->fHealApplyAreaBonusMeters;
				a2 += pcfg->fHealApplyAreaBonusMeters;
				// When party-wide buffs are enabled, lift the target cap to include the whole party
				if (pcfg->bBuffPartyWide)
				{
					byMaxTargetCount = 36; // generous upper bound for party + npc allies
				}
			}
		}
		rSpellAreaChecker.PrepareForSelection(GetBot(), pAppointTarget, GetApplyRangeType(), (int)a1, (int)a2);
		// When party-wide buffing is enabled, we won't rely on area checks below
		// We'll still keep byMaxTargetCount as adjusted, and same-world filtering.
	}

	std::map<HOBJECT, HOBJECT> mapCandidate;

	if (GetBot()->GetObjType() != OBJTYPE_MOB && GetBot()->GetObjType() != OBJTYPE_NPC)
	{
		if (GetBot()->GetObjType() == OBJTYPE_SUMMON_PET)
		{
			CWorldCell* pCell = GetBot()->GetCurWorldCell();
			if (pCell)
			{
				for (int dir = 0; dir < CWorldCell::MAX_CELLDIR; dir++)
				{
					CWorldCell* pSibling = pCell->GetSibling((CWorldCell::CELLDIR)dir);
					if (pSibling)
					{
						CSpawnObject* pSpawnObject = NULL;

						for (pSpawnObject = pSibling->GetObjectList()->GetFirst(OBJTYPE_PC);
							pSpawnObject;
							pSpawnObject = pSibling->GetObjectList()->GetNext(pSpawnObject->GetWorldCellObjectLinker()))
						{
							HOBJECT hid = pSpawnObject->GetID();
							if (hid != INVALID_HOBJECT)
								mapCandidate.insert(std::make_pair(hid, hid));
						}

						for (pSpawnObject = pSibling->GetObjectList()->GetFirst(OBJTYPE_NPC);
							pSpawnObject;
							pSpawnObject = pSibling->GetObjectList()->GetNext(pSpawnObject->GetWorldCellObjectLinker()))
						{
							HOBJECT hid2 = pSpawnObject->GetID();
							if (hid2 != INVALID_HOBJECT)
								mapCandidate.insert(std::make_pair(hid2, hid2));
						}

						for (pSpawnObject = pSibling->GetObjectList()->GetFirst(OBJTYPE_SUMMON_PET);
							pSpawnObject;
							pSpawnObject = pSibling->GetObjectList()->GetNext(pSpawnObject->GetWorldCellObjectLinker()))
						{
							HOBJECT hid3 = pSpawnObject->GetID();
							if (hid3 != INVALID_HOBJECT)
								mapCandidate.insert(std::make_pair(hid3, hid3));
						}
					}
				}
			}
		}
	}
	else
	{
		CNpcParty* pParty = GetBot()->GetNpcParty();
		// Collect NPC party members if any
		if (pParty)
		{
			for (CNpcParty::MEMBER_MAP::iterator it = pParty->Begin(); it != pParty->End(); it++)
			{
				if (it->first != INVALID_HOBJECT)
					mapCandidate.insert(std::make_pair(it->first, it->first));
			}
		}

		// Also include PCs from linked leader's party so helper can heal full player party
		HOBJECT hLink = GetBot()->GetLinkPc();
		if (hLink != INVALID_HOBJECT)
		{
			CPlayer* pLeader = g_pObjectManager->GetPC(hLink);
			if (pLeader && pLeader->GetParty())
			{
				CParty* pPcParty = pLeader->GetParty();
				for (BYTE i = 0; i < pPcParty->GetPartyMemberCount(); i++)
				{
					const sPARTY_MEMBER_INFO& mi = pPcParty->GetMemberInfo(i);
					if (mi.hHandle != INVALID_HOBJECT)
						mapCandidate.insert(std::make_pair(mi.hHandle, mi.hHandle));
				}
			}
		}

		// If still no candidates, fallback to self so the skill has a legal target
		if (mapCandidate.empty())
		{
			HOBJECT self = GetBot()->GetID();
			if (self != INVALID_HOBJECT)
				mapCandidate.insert(std::make_pair(self, self));
		}
	}

	float pfSquaredLength = 0.0;
	std::set<HOBJECT> mapSelectedBot;

	for (std::map<HOBJECT, HOBJECT>::iterator it = mapCandidate.begin(); it != mapCandidate.end(); it++)
	{
		CCharacter* pObject = g_pObjectManager->GetChar(it->first);
		if (pObject && pObject->IsInitialized() && pObject->GetCurWorld() == GetBot()->GetCurWorld())
		{
			// If party-wide is enabled for helper non-enemy buffs, include all candidates without area checks
			bool nonEnemy = m_pSkill && (m_pSkill->GetOriginalTableData()->byApply_Target != DBO_SKILL_APPLY_TARGET_ENEMY);
			bool isHelper = GetHelperNpcManager()->IsRegisteredHelper(GetBot());
			const sHELPER_NPC_CONFIG* pcfg = isHelper ? GetHelperNpcManager()->GetConfigForHelper(GetBot()) : NULL;
			bool partyWide = (nonEnemy && pcfg && pcfg->bBuffPartyWide);
			if (partyWide)
			{
				if (GetBot()->GetID() != pObject->GetID() || IsApplyNotMe() != true)
					mapSelectedBot.insert(it->second);
			}
			else if (rSpellAreaChecker.IsObjectInApplyRange(pObject, &pfSquaredLength))
			{
				if (GetBot()->GetID() != pObject->GetID() || IsApplyNotMe() != true)
					mapSelectedBot.insert(it->second);
			}
		}
		else
		{
			if (GetHelperNpcManager()->GetConfig().bVerboseLogs)
				ERR_LOG(LOG_GENERAL, "LP_LOW: invalid candidate handle=%u (null/uninit or diff world)", it->first);
		}
	}

	for (std::set<HOBJECT>::iterator it = mapSelectedBot.begin(); it != mapSelectedBot.end(); it++)
	{
		if (rTargetList.byTargetCount < byMaxTargetCount)
		{
			if (*it != INVALID_HOBJECT)
				rTargetList.AddTarget(*it);
		}
		else
			break;
	}
}


void CSkillCondition::GetTarget_ApplyRange_Party_LPLow(CCharacter *pAppointTarget, sSKILL_TARGET_LIST& rTargetList, BYTE byMaxTargetCount)
{
	if (!GetBot()) { rTargetList.Init(); return; }
	CSpellAreaChecker rSpellAreaChecker;
	rSpellAreaChecker.Create();
	// Resolve a non-null appoint target for area checks: prefer provided, then linked PC, then self
	CCharacter* pResolvedAppoint = pAppointTarget;
	if (!pResolvedAppoint)
	{
		if (GetBot())
		{
			HOBJECT hLink = GetBot()->GetLinkPc();
			if (hLink != INVALID_HOBJECT)
			{
				CCharacter* pLinked = g_pObjectManager->GetChar(hLink);
				if (pLinked && pLinked->IsInitialized())
					pResolvedAppoint = pLinked;
			}
			if (!pResolvedAppoint)
				pResolvedAppoint = GetBot();
		}
	}

	{
		float a1 = (float)GetApplyAreaSize1();
		float a2 = (float)GetApplyAreaSize2();
		bool nonEnemy = m_pSkill && (m_pSkill->GetOriginalTableData()->byApply_Target != DBO_SKILL_APPLY_TARGET_ENEMY);
		bool partyWide = false;
		if (nonEnemy && GetHelperNpcManager()->IsRegisteredHelper(GetBot()))
		{
			const sHELPER_NPC_CONFIG* pcfg = GetHelperNpcManager()->GetConfigForHelper(GetBot());
			if (pcfg)
			{
				partyWide = pcfg->bBuffPartyWide;
				if (pcfg->fBuffApplyAreaMeters > 0.0f)
				{
					a1 = (a1 > pcfg->fBuffApplyAreaMeters) ? a1 : pcfg->fBuffApplyAreaMeters;
					a2 = (a2 > pcfg->fBuffApplyAreaMeters) ? a2 : pcfg->fBuffApplyAreaMeters;
				}
				a1 += pcfg->fHealApplyAreaBonusMeters;
				a2 += pcfg->fHealApplyAreaBonusMeters;
				if (pcfg->bBuffPartyWide)
				{
					byMaxTargetCount = 36;
				}
			}
		}
		rSpellAreaChecker.PrepareForSelection(GetBot(), pResolvedAppoint, GetApplyRangeType(), (int)a1, (int)a2);
		// Party-wide mode skips area checks below when selecting candidates
	}

	std::map<int, HOBJECT> mapCandidate;

	// Determine LP threshold, allowing helper override when linked to a PC
	WORD wThreshold = m_wUse_Skill_LP;
	bool bMissingLpMode = false; // when override==0 heal anyone missing LP
	if (GetBot() && GetBot()->GetLinkPc() != INVALID_HOBJECT)
	{
		const sHELPER_NPC_CONFIG& cfg = GetHelperNpcManager()->GetConfig();
		if (cfg.wHealLpThresholdOverride > 0)
			wThreshold = cfg.wHealLpThresholdOverride;
		else if (cfg.wHealLpThresholdOverride == 0)
			bMissingLpMode = true;
	}

	// First, if an appointed target was provided/resolved (e.g., linked PC), consider it as a candidate
	if (pResolvedAppoint && pResolvedAppoint->IsInitialized() && GetBot() && pResolvedAppoint->GetCurWorld() == GetBot()->GetCurWorld())
	{
		if (rSpellAreaChecker.IsObjectInApplyRange(pResolvedAppoint, NULL))
		{
			if ((bMissingLpMode && pResolvedAppoint->GetCurLP() < pResolvedAppoint->GetMaxLP()) ||
				(!bMissingLpMode && pResolvedAppoint->ConsiderLPLow((float)wThreshold)))
			{
				if (GetBot()->GetID() != pResolvedAppoint->GetID() || IsApplyNotMe() != true)
				{
					HOBJECT hid = pResolvedAppoint->GetID();
					if (hid != INVALID_HOBJECT)
						mapCandidate.insert(std::make_pair(pResolvedAppoint->GetCurLP(), hid));
				}
			}
		}
	}

	// Consider both mobs/npcs in the NPC party and PCs in the linked player's party for LP-low selection
	if (GetBot()->GetObjType() == OBJTYPE_MOB || GetBot()->GetObjType() == OBJTYPE_NPC)
	{
		// 1) NPC party members
		if (CNpcParty* pParty = GetBot()->GetNpcParty())
		{
			for (CNpcParty::MEMBER_MAP::iterator it = pParty->Begin(); it != pParty->End(); it++)
			{
				CNpc* pObject = g_pObjectManager->GetNpc(it->first);
				if (pObject && pObject->IsInitialized() && GetBot() && pObject->GetCurWorld() == GetBot()->GetCurWorld())
				{
					if (rSpellAreaChecker.IsObjectInApplyRange(pObject, NULL))
					{
						if ((bMissingLpMode && pObject->GetCurLP() < pObject->GetMaxLP()) ||
							(!bMissingLpMode && pObject->ConsiderLPLow((float)wThreshold)))
						{
							if (GetBot()->GetID() != pObject->GetID() || IsApplyNotMe() != true)
							{
								if (it->first != INVALID_HOBJECT)
									mapCandidate.insert(std::make_pair(pObject->GetCurLP(), it->first));
							}
						}
					}
				}
			}
		}

		// 2) PC party members via linked PC
		HOBJECT hLink = GetBot()->GetLinkPc();
		if (hLink != INVALID_HOBJECT)
		{
			CPlayer* pLeader = g_pObjectManager->GetPC(hLink);
			if (pLeader && pLeader->IsInitialized() && pLeader->GetParty())
			{
				CParty* pPcParty = pLeader->GetParty();
				for (BYTE i = 0; i < pPcParty->GetPartyMemberCount(); i++)
				{
					const sPARTY_MEMBER_INFO& mi = pPcParty->GetMemberInfo(i);
					CPlayer* pMember = g_pObjectManager->GetPC(mi.hHandle);
					if (pMember && pMember->IsInitialized() && GetBot() && pMember->GetCurWorld() == GetBot()->GetCurWorld())
					{
						if (rSpellAreaChecker.IsObjectInApplyRange(pMember, NULL))
						{
							if ((bMissingLpMode && pMember->GetCurLP() < pMember->GetMaxLP()) ||
								(!bMissingLpMode && pMember->ConsiderLPLow((float)wThreshold)))
							{
								if (GetBot()->GetID() != pMember->GetID() || IsApplyNotMe() != true)
								{
									HOBJECT hid = pMember->GetID();
									if (hid != INVALID_HOBJECT)
										mapCandidate.insert(std::make_pair(pMember->GetCurLP(), hid));
								}
							}
						}
					}
				}
			}
		}
	}

	// Add up to byMaxTargetCount candidates (lowest LP first)
	for (std::map<int, HOBJECT>::iterator it = mapCandidate.begin(); it != mapCandidate.end(); it++)
	{
		if (rTargetList.byTargetCount < byMaxTargetCount)
		{
			if (it->second != INVALID_HOBJECT)
				rTargetList.AddTarget(it->second);
		}
		else
			break;
	}
}

