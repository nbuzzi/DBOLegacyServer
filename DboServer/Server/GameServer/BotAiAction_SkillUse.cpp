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

	// Branch: helper vs non-helper to keep original monster behavior intact
	const bool isHelper = GetHelperNpcManager()->IsRegisteredHelper(GetBot());

	// Original (legacy) path for non-helpers
	if (!isHelper)
	{
		// Only require an existing target for non-self skills. Self-target skills (e.g., bomb detonation)
		// should be allowed to execute even without a pre-set target handle.
		bool bRequiresTarget = (pSkillCond->GetSkill()->GetOriginalTableData()->byApply_Target != DBO_SKILL_APPLY_TARGET_SELF);
		CCharacter* pTarget = bRequiresTarget ? g_pObjectManager->GetChar(GetBot()->GetTargetHandle()) : nullptr;
		if (bRequiresTarget && pTarget == NULL)
		{
			m_status = COMPLETED;
			return m_status;
		}

		bool bChase = false;
		std::list<CNtlVector> rlistCollisionPos;

		if (pSkillCond->GetSkill()->GetOriginalTableData()->byApply_Target != DBO_SKILL_APPLY_TARGET_SELF) //check if we dont use skill on ourself
		{
			if (GetBot()->ConsiderRange(pSkillCond->GetSkill()->GetOriginalTableData()->fUse_Range_Max, 30.0f / 100.0f) == false)
			{
				if (GetBot()->IsReachable(pTarget, rlistCollisionPos) == false)
				{
					if (rlistCollisionPos.size() > 0)
					{
						CNtlVector rLoc(rlistCollisionPos.back());
						if (GetBot()->IsInRange(rLoc, 1.0f))
							bChase = false;
					}
					else bChase = false;
				}
				if (pTarget)
				{
					//printf("Send Skill \n");
					if (GetBot()->IsInRange(pTarget->GetCurLoc(), 2.0f))
					{
						//printf("Send Skill 2\n");
						bChase = false;
					}
					else bChase = true;
				}
				else bChase = true;
			}
		}

		if (bChase == false)
		{
			HOBJECT hTarget = INVALID_HOBJECT;
			sSKILL_TARGET_LIST targetList;

			pSkillCond->GetTarget(hTarget, targetList);

			// For self-apply skills, allow hTarget to be invalid (skill system will handle subject=bot)
			if (hTarget == INVALID_HOBJECT && pSkillCond->GetSkill()->GetOriginalTableData()->byApply_Target != DBO_SKILL_APPLY_TARGET_SELF)
			{
				m_status = COMPLETED;
				return m_status;
			}
			else
			{
				GetBot()->GetTargetListManager()->SetAggroLastUpdateTime();

				pSkillManager->SetCurSkillTblidx(pSkillCond->GetSkillTblidx());
				pSkillManager->SetCurSkillConditionIdx(m_bySkillIndex);
				pSkillManager->SetSkillUse_Lock();

				WORD wTemp;
				CNtlVector vFinalSubjectLoc;
				pSkillCond->GetSkill()->UseSkill(INVALID_BYTE, hTarget, vFinalSubjectLoc, GetBot()->GetCurLoc(), targetList.byTargetCount, targetList.ahTarget, wTemp);

				if (wTemp != 500)
				{
					pSkillManager->SetCurSkillTblidx(INVALID_TBLIDX);
					pSkillManager->SetCurSkillConditionIdx(INVALID_BYTE);
					pSkillManager->SetSkillUse_Unlock();
				}

				m_status = COMPLETED;
				return m_status;
			}
		}
		else
		{
			CBotAiAction_Chase* pChase = new CBotAiAction_Chase(GetBot(), CBotAiAction_Chase::ATTACKTYPE_SKILL, pSkillCond->GetSkill()->GetOriginalTableData()->fUse_Range_Max);
			if (!AddSubControlQueue(pChase, true))
			{
				m_status = FAILED;
			}
		}

		return m_status;
	}

	// Helper logic
	if (isHelper) {
		// Helper path: retain enhanced targeting / range / diagnostics (trimmed of monster-impacting changes)
		HOBJECT hTarget = INVALID_HOBJECT; sSKILL_TARGET_LIST targetList; pSkillCond->GetTarget(hTarget, targetList);
		if (hTarget == INVALID_HOBJECT) { m_status = COMPLETED; return m_status; }
		bool bChase = false;
		if (pSkillCond->GetSkill()->GetOriginalTableData()->byApply_Target != DBO_SKILL_APPLY_TARGET_SELF)
		{
			CCharacter* pRealTarget = g_pObjectManager->GetChar(hTarget);
			if (pRealTarget)
			{
				if (pSkillCond->GetSkill()->GetOriginalTableData()->byApply_Target == DBO_SKILL_APPLY_TARGET_ENEMY)
				{
					if (GetBot()->GetTargetHandle() != hTarget) GetBot()->SetTargetHandle(hTarget);
				}
				float fUseRange = pSkillCond->GetSkill()->GetOriginalTableData()->fUse_Range_Max;
				if (pSkillCond->GetSkill()->GetOriginalTableData()->byApply_Target != DBO_SKILL_APPLY_TARGET_ENEMY)
				{
					if (const sHELPER_NPC_CONFIG* pcfg = GetHelperNpcManager()->GetConfigForHelper(GetBot())) fUseRange += pcfg->fHealUseRangeBonusMeters;
				}
				if (GetBot()->ConsiderRange(fUseRange, 30.0f / 100.0f) == false) bChase = true;
			}
		}
		if (bChase)
		{
			float fUseRange = pSkillCond->GetSkill()->GetOriginalTableData()->fUse_Range_Max;
			if (pSkillCond->GetSkill()->GetOriginalTableData()->byApply_Target != DBO_SKILL_APPLY_TARGET_ENEMY)
			{
				if (const sHELPER_NPC_CONFIG* pcfg = GetHelperNpcManager()->GetConfigForHelper(GetBot())) fUseRange += pcfg->fHealUseRangeBonusMeters;
			}
			CBotAiAction_Chase* pChase = new CBotAiAction_Chase(GetBot(), CBotAiAction_Chase::ATTACKTYPE_SKILL, fUseRange);
			if (!AddSubControlQueue(pChase, true)) m_status = FAILED; return m_status;
		}
		if (GetBot()->GetMoveFlag() != NTL_MOVE_FLAG_INVALID)
		{
			GetBot()->SendCharStateStanding(true); m_status = COMPLETED; return m_status;
		}
		GetBot()->GetTargetListManager()->SetAggroLastUpdateTime();
		if (GetBot()->GetCharStateID() == CHARSTATE_FOLLOWING && GetBot()->GetMoveFlag() == NTL_MOVE_FLAG_INVALID)
			GetBot()->SendCharStateStanding(true);
		pSkillManager->SetCurSkillTblidx(pSkillCond->GetSkillTblidx()); pSkillManager->SetCurSkillConditionIdx(m_bySkillIndex); pSkillManager->SetSkillUse_Lock();
		WORD wTemp; CNtlVector vFinalSubjectLoc; pSkillCond->GetSkill()->UseSkill(INVALID_BYTE, hTarget, vFinalSubjectLoc, GetBot()->GetCurLoc(), targetList.byTargetCount, targetList.ahTarget, wTemp);
		if (wTemp != GAME_SUCCESS)
		{
			pSkillManager->SetCurSkillTblidx(INVALID_TBLIDX); pSkillManager->SetCurSkillConditionIdx(INVALID_BYTE); pSkillManager->SetSkillUse_Unlock();
		}
		m_status = COMPLETED; return m_status;
	}

	return m_status;
}