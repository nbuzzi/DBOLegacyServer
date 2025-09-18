#ifndef __DBOG_SKILL_CONDITION_LP__
#define __DBOG_SKILL_CONDITION_LP__

#include "SkillCondition.h"
#include "ObjectManager.h"
#include "HelperNpcManager.h"

class CSkillBot;

class CSkillCondition_LP : public CSkillCondition
{
public:

	CSkillCondition_LP() : m_pPartyMemberLowLP(NULL) {}
	virtual ~CSkillCondition_LP() {}

public:

	virtual CSkillBot*      OnUpdate(DWORD dwTickTime);
	virtual void            AppointTargetSelf_ApplyTargetParty(sSKILL_TARGET_LIST& rTargetList);
	virtual void            AppointTargetTarget_ApplyTargetParty(HOBJECT& hTarget, sSKILL_TARGET_LIST& rTargetList);

private:
	CCharacter*             m_pPartyMemberLowLP;
};


inline CSkillBot* CSkillCondition_LP::OnUpdate(DWORD dwTickTime)
{
	CSkillBot* pSkill = CSkillCondition::OnUpdate(dwTickTime);
	if (pSkill)
	{
		if (m_wUse_Skill_LP == INVALID_WORD)
		{
			ERR_LOG(LOG_GENERAL, "fail : INVALID_WORD == m_wUse_Skill_LP");
			return NULL;
		}

		// Reset per-tick preferred target
		m_pPartyMemberLowLP = NULL;

		const bool isHelper = GetHelperNpcManager()->IsRegisteredHelper(GetBot());

		if (isHelper)
		{
			// Helper override: if linked PC exists and is low (or simply missing any LP when override is zero), prefer healing them
			if (GetBot()->GetLinkPc() != INVALID_HOBJECT)
			{
				HOBJECT hLink = GetBot()->GetLinkPc();
				CCharacter* pLinked = g_pObjectManager->GetChar(hLink);
				if (pLinked && pLinked->IsInitialized())
				{
					WORD wThreshold = m_wUse_Skill_LP;
					const sHELPER_NPC_CONFIG& cfg = GetHelperNpcManager()->GetConfig();
					if (cfg.wHealLpThresholdOverride > 0)
						wThreshold = cfg.wHealLpThresholdOverride;

					bool bLinkedLow = pLinked->ConsiderLPLow((float)wThreshold);
					// If override is 0 => heal whenever missing any LP
					if (cfg.wHealLpThresholdOverride == 0)
						bLinkedLow = pLinked->GetCurLP() < pLinked->GetMaxLP();

					if (bLinkedLow)
					{
						m_pPartyMemberLowLP = pLinked;
						return pSkill;
					}
				}
			}

			// Helper: self LP low with override handling
			if (GetHelperNpcManager()->GetConfig().wHealLpThresholdOverride == 0)
			{
				if (GetBot()->GetCurLP() < GetBot()->GetMaxLP())
					return pSkill;
			}
			else if (GetBot()->ConsiderLPLow(m_wUse_Skill_LP))
			{
				return pSkill;
			}
		}
		else
		{
			// Non-helper legacy: only use original LP threshold on self
			if (GetBot()->ConsiderLPLow(m_wUse_Skill_LP))
			{
				return pSkill;
			}
		}
	}

	return NULL;
}

inline void CSkillCondition_LP::AppointTargetSelf_ApplyTargetParty(sSKILL_TARGET_LIST& rTargetList)
{
	if (GetApplyRangeType() && GetTargetMaxCount() != 1)
	{
		GetTarget_ApplyRange_Party_LPLow(m_pPartyMemberLowLP, rTargetList, GetTargetMaxCount());
		// Fallback to linked PC Direct only for helpers
		if (GetHelperNpcManager()->IsRegisteredHelper(GetBot()))
		{
			if (rTargetList.byTargetCount == 0 && m_pPartyMemberLowLP)
			{
				rTargetList.Init();
				rTargetList.AddTarget(m_pPartyMemberLowLP->GetID());
			}
		}
	}
	else
	{
		rTargetList.Init();
		if (GetHelperNpcManager()->IsRegisteredHelper(GetBot()) && m_pPartyMemberLowLP)
			rTargetList.AddTarget(m_pPartyMemberLowLP->GetID());
	}
}

inline void CSkillCondition_LP::AppointTargetTarget_ApplyTargetParty(HOBJECT& hTarget, sSKILL_TARGET_LIST& rTargetList)
{
	hTarget = m_pPartyMemberLowLP ? m_pPartyMemberLowLP->GetID() : GetBot()->GetID();

	if (GetApplyRangeType() && GetTargetMaxCount() != 1)
	{
		GetTarget_ApplyRange_Party_LPLow(m_pPartyMemberLowLP, rTargetList, GetTargetMaxCount());
		// Fallback to linked PC Direct only for helpers
		if (GetHelperNpcManager()->IsRegisteredHelper(GetBot()))
		{
			if (rTargetList.byTargetCount == 0 && m_pPartyMemberLowLP)
			{
				rTargetList.Init();
				rTargetList.AddTarget(m_pPartyMemberLowLP->GetID());
			}
		}
	}
	else
	{
		rTargetList.Init();
		if (GetHelperNpcManager()->IsRegisteredHelper(GetBot()) && m_pPartyMemberLowLP)
			rTargetList.AddTarget(m_pPartyMemberLowLP->GetID());
	}
}

#endif