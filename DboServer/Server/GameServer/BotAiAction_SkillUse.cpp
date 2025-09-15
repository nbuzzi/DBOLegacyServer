#include "stdafx.h"
#include "BotAiAction_SkillUse.h"
#include "SPSNodeAction_SkillUse.h"
#include "BotAiState_Fight.h"
#include "ObjectManager.h"
#include "SkillCondition.h"
#include "BotAiAction_Chase.h"
#include "CPlayer.h"
#include "NtlResultCode.h"
#include "HelperNpcManager.h"


CBotAiAction_SkillUse::CBotAiAction_SkillUse(CNpc* pBot)
	: CBotAiAction(pBot, BOTCONTROL_ACTION_SKILL_USE, "BOTCONTROL_ACTION_SKILL_USE")
{
	m_bySkillIndex = INVALID_BYTE;
}

CBotAiAction_SkillUse::CBotAiAction_SkillUse(CNpc* pBot, BYTE bySkillIndex)
	: CBotAiAction(pBot, BOTCONTROL_ACTION_SKILL_USE, "BOTCONTROL_ACTION_SKILL_USE")
{
	m_bySkillIndex = bySkillIndex;
}

CBotAiAction_SkillUse::~CBotAiAction_SkillUse()
{
}


bool CBotAiAction_SkillUse::AttachControlScriptNode(CControlScriptNode* pControlScriptNode)
{
	CSPSNodeAction_SkillUse* pAction = dynamic_cast<CSPSNodeAction_SkillUse*>(pControlScriptNode);
	if (pAction)
	{
		m_bySkillIndex = pAction->m_bySkillIndex;

		if (GetBot()->GetSkillManager()->IsSkillUseLock())
			return false;
		
		if (GetBot()->GetSkillManager()->GetNumberOfSkill() == 0)
			return false;

		if (!GetBot()->GetSkillManager()->FindSkillCondition(m_bySkillIndex))
			return false;

		return true;
	}

	ERR_LOG(LOG_BOTAI, "fail : Can't dynamic_cast from CControlScriptNode[%X] to CSPSNodeAction_SkillUse", pControlScriptNode);
	return false;
}

void CBotAiAction_SkillUse::CopyTo(CControlState* pTo)
{
	CBotAiAction_SkillUse* pSkillUse = check_cast<CBotAiAction_SkillUse*, CControlState*>(pTo);
	if (pSkillUse)
	{
		pSkillUse->m_bySkillIndex = m_bySkillIndex;
	}
	else
		ERR_LOG(LOG_BOTAI, "fail : NULL == pTo");
}

void CBotAiAction_SkillUse::OnEnter()
{
}

void CBotAiAction_SkillUse::OnExit()
{
	CBotAiState_Fight* pFight = GetBot()->GetBotController()->GetFightState();
	if (pFight) //check if is initialized.. OnExit could be called when destroying BOT.
		pFight->SetCompulsionTarget(INVALID_HOBJECT);
}


int CBotAiAction_SkillUse::OnUpdate(DWORD dwTickDiff, float fMultiple)
{
	CSkillManagerBot* pSkillManager = (CSkillManagerBot*)GetBot()->GetSkillManager();
	if (!pSkillManager)
	{
		m_status = COMPLETED;
		return m_status;
	}

	CSkillCondition* pSkillCond = pSkillManager->FindSkillCondition(m_bySkillIndex);
	if (!pSkillCond)
	{
		m_status = COMPLETED;
		return m_status;
	}

	if (UpdateSubControlQueue(dwTickDiff, fMultiple) != COMPLETED)
		return m_status;
	
	if (pSkillManager->IsSkillUseLock())
	{
		m_status = COMPLETED;
		return m_status;
	}

	// Determine intended target via skill condition, not via current target handle
	HOBJECT hTarget = INVALID_HOBJECT;
	sSKILL_TARGET_LIST targetList;
	pSkillCond->GetTarget(hTarget, targetList);

	if (hTarget == INVALID_HOBJECT)
	{
		m_status = COMPLETED;
		return m_status;
	}

	// For non-self skills, validate range and chase if out of range
	bool bChase = false;
	if (pSkillCond->GetSkill()->GetOriginalTableData()->byApply_Target != DBO_SKILL_APPLY_TARGET_SELF)
	{
		CCharacter* pRealTarget = g_pObjectManager->GetChar(hTarget);
		if (pRealTarget)
		{
			// Only set enemy target handle for harmful/enemy-targeting skills.
			// Avoid setting the player as an "enemy" when casting heals/buffs on alliance/party.
			if (pSkillCond->GetSkill()->GetOriginalTableData()->byApply_Target == DBO_SKILL_APPLY_TARGET_ENEMY)
			{
				if (GetBot()->GetTargetHandle() != hTarget)
					GetBot()->SetTargetHandle(hTarget);
			}
			float fUseRange = pSkillCond->GetSkill()->GetOriginalTableData()->fUse_Range_Max;
			if (pSkillCond->GetSkill()->GetOriginalTableData()->byApply_Target != DBO_SKILL_APPLY_TARGET_ENEMY)
			{
				// extend heal/buff use range by config bonus
				fUseRange += GetHelperNpcManager()->GetConfig().fHealUseRangeBonusMeters;
			}
			if (GetBot()->ConsiderRange(fUseRange, 30.0f / 100.0f) == false)
			{
				bChase = true;
			}
		}
	}

	if (bChase)
	{
		float fUseRange = pSkillCond->GetSkill()->GetOriginalTableData()->fUse_Range_Max;
		if (pSkillCond->GetSkill()->GetOriginalTableData()->byApply_Target != DBO_SKILL_APPLY_TARGET_ENEMY)
			fUseRange += GetHelperNpcManager()->GetConfig().fHealUseRangeBonusMeters;
		CBotAiAction_Chase* pChase = new CBotAiAction_Chase(GetBot(), CBotAiAction_Chase::ATTACKTYPE_SKILL, fUseRange);
		if (!AddSubControlQueue(pChase, true))
		{
			m_status = FAILED;
		}
		return m_status;
	}

	// If we're moving, stop first and retry next tick. It's OK to cast while in FOLLOWING state as long as we're stationary.
	// Casting while MOVING often returns GAME_SKILL_CANT_CAST_NOW (rc=605).
	if (GetBot()->GetMoveFlag() != NTL_MOVE_FLAG_INVALID)
	{
		GetBot()->SendCharStateStanding(true);
		m_status = COMPLETED; // let scheduler try again next second after state settles
		return m_status;
	}

	// Use the skill on the computed target
	GetBot()->GetTargetListManager()->SetAggroLastUpdateTime();

	pSkillManager->SetCurSkillTblidx(pSkillCond->GetSkillTblidx());
	pSkillManager->SetCurSkillConditionIdx(m_bySkillIndex);
	pSkillManager->SetSkillUse_Lock();

	WORD wTemp;
	CNtlVector vFinalSubjectLoc;
	pSkillCond->GetSkill()->UseSkill(INVALID_BYTE, hTarget, vFinalSubjectLoc, GetBot()->GetCurLoc(), targetList.byTargetCount, targetList.ahTarget, wTemp);

	if (wTemp != GAME_SUCCESS)
	{
		pSkillManager->SetCurSkillTblidx(INVALID_TBLIDX);
		pSkillManager->SetCurSkillConditionIdx(INVALID_BYTE);
		pSkillManager->SetSkillUse_Unlock();

		// Diagnostics: capture reason code when helper fails to start casting
		if (GetBot()->GetLinkPc() != INVALID_HOBJECT)
		{
			WORD wLeaderWorld = 0, wHelperWorld = 0;
			float fDist = -1.0f;
			CPlayer* pLeader = (CPlayer*)g_pObjectManager->GetChar(GetBot()->GetLinkPc());
			if (pLeader)
			{
				wLeaderWorld = pLeader->GetWorldID();
				wHelperWorld = GetBot()->GetWorldID();
				// Use vector-based overload to avoid type mismatch
				fDist = GetBot()->GetDistance(pLeader->GetCurLoc());
			}
			const bool verbose = GetHelperNpcManager()->GetConfig().bVerboseLogs;
			if (verbose && wTemp != GAME_SKILL_CANT_CAST_NOW)
			{
				ERR_LOG(LOG_BOTAI, "HelperNPC: UseSkill failed rc=%u (skill=%u target=%u) worlds L=%u H=%u dist=%.1f state=%u moveFlag=%u", wTemp, pSkillCond->GetSkillTblidx(), hTarget, wLeaderWorld, wHelperWorld, fDist, GetBot()->GetCharStateID(), GetBot()->GetMoveFlag());
			}
		}

		// Recovery: if "can't cast now" (commonly 605), only force-stand when not moving; if moving, the follow logic will handle catch-up
		if (wTemp == GAME_SKILL_CANT_CAST_NOW && GetBot()->GetMoveFlag() == NTL_MOVE_FLAG_INVALID)
		{
			GetBot()->SendCharStateStanding(true);
		}
	}

	m_status = COMPLETED;
	return m_status;

	return m_status;
}