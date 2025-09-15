// Cleaned up and extended to support healing a linked PC when no NPC party exists
#ifndef __DBOG_SKILL_CONDITION_GIVE__
#define __DBOG_SKILL_CONDITION_GIVE__

#include "SkillCondition.h"
#include "ObjectManager.h"
#include "HelperNpcManager.h"

class CSkillBot;
class CNpc;

class CSkillCondition_Give : public CSkillCondition
{
public:
	CSkillCondition_Give() : m_pPartyMemberLowLP(NULL) {}
	virtual ~CSkillCondition_Give() {}

public:
	virtual CSkillBot* OnUpdate(DWORD dwTickTime);
	virtual void       AppointTargetSelf_ApplyTargetParty(sSKILL_TARGET_LIST& rTargetList);
	virtual void       AppointTargetTarget_ApplyTargetParty(HOBJECT& hTarget, sSKILL_TARGET_LIST& rTargetList);

private:
	CCharacter*        m_pPartyMemberLowLP;
};

inline CSkillBot* CSkillCondition_Give::OnUpdate(DWORD dwTickTime)
{
	CSkillBot* pSkill = CSkillCondition::OnUpdate(dwTickTime);
	if (pSkill)
	{
		if (m_wUse_Skill_LP == INVALID_WORD)
		{
			ERR_LOG(LOG_GENERAL, "fail : INVALID_WORD == m_wUse_Skill_LP");
			return NULL;
		}
		// Prefer NPC party member with lowest LP
		m_pPartyMemberLowLP = NULL;
		if (CNpcParty* pParty = GetBot()->GetNpcParty())
		{
			CNpc* pNpcLow = pParty->GetMember_LowLP();
			if (pNpcLow)
				m_pPartyMemberLowLP = pNpcLow;
		}

		// If no NPC party or no low member found, try linked PC (dungeon leader)
		if (!m_pPartyMemberLowLP)
		{
			HOBJECT hLink = GetBot()->GetLinkPc();
			if (hLink != INVALID_HOBJECT)
			{
				CCharacter* pLinked = g_pObjectManager->GetChar(hLink);
				if (pLinked && pLinked->IsInitialized())
					m_pPartyMemberLowLP = pLinked;
			}
		}

		WORD wThreshold = m_wUse_Skill_LP;
		// If this bot is a helper linked to a PC and a global override exists, prefer that threshold
		if (GetBot()->GetLinkPc() != INVALID_HOBJECT)
		{
			const sHELPER_NPC_CONFIG& cfg = GetHelperNpcManager()->GetConfig();
			if (cfg.wHealLpThresholdOverride > 0)
				wThreshold = cfg.wHealLpThresholdOverride;
		}

		// If override is 0 => heal whenever missing any LP
		if (GetHelperNpcManager()->GetConfig().wHealLpThresholdOverride == 0)
		{
			if (m_pPartyMemberLowLP && m_pPartyMemberLowLP->GetCurLP() < m_pPartyMemberLowLP->GetMaxLP())
			{
				ERR_LOG(LOG_BOTAI, "Give: healing target %u (missing LP mode)", m_pPartyMemberLowLP->GetID());
				return pSkill;
			}
		}
		else if (m_pPartyMemberLowLP && m_pPartyMemberLowLP->ConsiderLPLow((float)wThreshold))
		{
			ERR_LOG(LOG_BOTAI, "Give: healing target %u with threshold %u (curLP=%u)", m_pPartyMemberLowLP->GetID(), wThreshold, m_pPartyMemberLowLP->GetCurLP());
			return pSkill;
		}
	}

	return NULL;
}

inline void CSkillCondition_Give::AppointTargetSelf_ApplyTargetParty(sSKILL_TARGET_LIST& rTargetList)
{
	if (GetApplyRangeType() && GetTargetMaxCount() != 1)
	{
		// Prefer party-based selection when an NPC party exists
		GetTarget_ApplyRange_Party_LPLow(m_pPartyMemberLowLP, rTargetList, GetTargetMaxCount());
		// Fallback: if we have no NPC party (or the linked PC isn't in the NPC party),
		// ensure we at least target the linked PC so heals actually fire.
		if (rTargetList.byTargetCount == 0 && m_pPartyMemberLowLP)
		{
			rTargetList.Init();
			rTargetList.AddTarget(m_pPartyMemberLowLP->GetID());
		}
	}
	else
	{
		rTargetList.Init();
		if (m_pPartyMemberLowLP)
			rTargetList.AddTarget(m_pPartyMemberLowLP->GetID());
	}
}

inline void CSkillCondition_Give::AppointTargetTarget_ApplyTargetParty(HOBJECT& hTarget, sSKILL_TARGET_LIST& rTargetList)
{
	hTarget = m_pPartyMemberLowLP ? m_pPartyMemberLowLP->GetID() : GetBot()->GetID();

	if (GetApplyRangeType() && GetTargetMaxCount() != 1)
	{
		// Prefer party-based selection when an NPC party exists
		GetTarget_ApplyRange_Party_LPLow(m_pPartyMemberLowLP, rTargetList, GetTargetMaxCount());
		// Fallback: if we have no NPC party (or the linked PC isn't in the NPC party),
		// ensure we at least target the linked PC so heals actually fire.
		if (rTargetList.byTargetCount == 0 && m_pPartyMemberLowLP)
		{
			rTargetList.Init();
			rTargetList.AddTarget(m_pPartyMemberLowLP->GetID());
		}
	}
	else
	{
		rTargetList.Init();
		if (m_pPartyMemberLowLP)
			rTargetList.AddTarget(m_pPartyMemberLowLP->GetID());
	}
}

#endif