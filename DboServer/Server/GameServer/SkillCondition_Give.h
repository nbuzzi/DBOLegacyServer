// Cleaned up and extended to support healing a linked PC when no NPC party exists
#ifndef __DBOG_SKILL_CONDITION_GIVE__
#define __DBOG_SKILL_CONDITION_GIVE__

#include "SkillCondition.h"
#include "ObjectManager.h"
#include "HelperNpcManager.h"
#include "CPlayer.h"
#include "Party.h"

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
	if (!GetBot()) return NULL;
	CSkillBot* pSkill = CSkillCondition::OnUpdate(dwTickTime);
	if (pSkill)
	{
		if (m_wUse_Skill_LP == INVALID_WORD)
		{
			ERR_LOG(LOG_GENERAL, "fail : INVALID_WORD == m_wUse_Skill_LP");
			return NULL;
		}
		// Prefer NPC party member with lowest LP first
		m_pPartyMemberLowLP = NULL;
		if (CNpcParty* pParty = GetBot()->GetNpcParty())
		{
			CNpc* pNpcLow = pParty->GetMember_LowLP();
			if (pNpcLow)
				m_pPartyMemberLowLP = pNpcLow;
		}

		// If no NPC party member found, scan the linked PC leader's party and choose the lowest-HP member
		// This enables healing of all team members instead of just the leader.
		HOBJECT hLink = GetBot()->GetLinkPc();
		if (!m_pPartyMemberLowLP && hLink != INVALID_HOBJECT)
		{
			CPlayer* pLeader = reinterpret_cast<CPlayer*>(g_pObjectManager->GetChar(hLink));
			if (pLeader && pLeader->IsInitialized())
			{
				const CWorld* pBotWorld = GetBot()->GetCurWorld();
				float bestMissingPct = -1.0f;
				CCharacter* pBest = NULL;

				auto consider = [&](CPlayer* pPlr)
				{
					if (!pPlr || !pPlr->IsInitialized()) return; // invalid
					if (pPlr->IsFainting()) return; // resurrection handled elsewhere
					if (pPlr->GetCurWorld() != pBotWorld) return; // different world
					float curPct = pPlr->GetCurLpInPercent();
					float missingPct = 100.0f - curPct;
					if (missingPct <= 0.0f) return; // full HP
					if (missingPct > bestMissingPct)
					{
						bestMissingPct = missingPct;
						pBest = pPlr;
					}
				};

				// Consider leader and party members
				consider(pLeader);
				if (pLeader->GetParty() && pLeader->GetParty()->GetPartyMemberCount() > 0)
				{
					CParty* pParty = pLeader->GetParty();
					BYTE cnt = pParty->GetPartyMemberCount();
					for (BYTE i = 0; i < cnt; ++i)
					{
						const sPARTY_MEMBER_INFO& mi = pParty->GetMemberInfo(i);
						if (mi.hHandle == hLink) continue; // already considered leader
						CPlayer* pMem = g_pObjectManager->GetPC(mi.hHandle);
						consider(pMem);
					}
				}

				if (pBest)
					m_pPartyMemberLowLP = pBest;
				else
				{
					// Fallback to the leader if nobody else is injured
					m_pPartyMemberLowLP = pLeader;
				}
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
				if (GetHelperNpcManager()->GetConfig().bVerboseLogs)
					ERR_LOG(LOG_BOTAI, "Give: healing target %u (missing LP mode)", m_pPartyMemberLowLP->GetID());
				return pSkill;
			}
		}
		else if (m_pPartyMemberLowLP && m_pPartyMemberLowLP->ConsiderLPLow((float)wThreshold))
		{
			if (GetHelperNpcManager()->GetConfig().bVerboseLogs)
				ERR_LOG(LOG_BOTAI, "Give: healing target %u with threshold %u (curLP=%u)", m_pPartyMemberLowLP->GetID(), wThreshold, m_pPartyMemberLowLP->GetCurLP());
			return pSkill;
		}
	}

	return NULL;
}

inline void CSkillCondition_Give::AppointTargetSelf_ApplyTargetParty(sSKILL_TARGET_LIST& rTargetList)
{
	if (!GetBot()) { rTargetList.Init(); return; }
	if (GetApplyRangeType() && GetTargetMaxCount() != 1)
	{
		// Prefer party-based selection when an NPC party exists
		GetTarget_ApplyRange_Party_LPLow(m_pPartyMemberLowLP, rTargetList, GetTargetMaxCount());
		// Fallback: if we have no NPC party (or the linked PC isn't in the NPC party),
		// ensure we at least target the linked PC so heals actually fire.
		if (rTargetList.byTargetCount == 0 && m_pPartyMemberLowLP)
		{
			rTargetList.Init();
			HOBJECT hid = m_pPartyMemberLowLP->GetID();
			if (hid != INVALID_HOBJECT)
				rTargetList.AddTarget(hid);
		}
	}
	else
	{
		rTargetList.Init();
		if (m_pPartyMemberLowLP)
		{
			HOBJECT hid = m_pPartyMemberLowLP->GetID();
			if (hid != INVALID_HOBJECT)
				rTargetList.AddTarget(hid);
		}
	}
}

inline void CSkillCondition_Give::AppointTargetTarget_ApplyTargetParty(HOBJECT& hTarget, sSKILL_TARGET_LIST& rTargetList)
{
	if (!GetBot()) { rTargetList.Init(); return; }
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
			HOBJECT hid = m_pPartyMemberLowLP->GetID();
			if (hid != INVALID_HOBJECT)
				rTargetList.AddTarget(hid);
		}
	}
	else
	{
		rTargetList.Init();
		if (m_pPartyMemberLowLP)
		{
			HOBJECT hid = m_pPartyMemberLowLP->GetID();
			if (hid != INVALID_HOBJECT)
				rTargetList.AddTarget(hid);
		}
	}
}

#endif