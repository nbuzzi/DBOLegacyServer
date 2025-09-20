#ifndef __DBOG_SKILL_CONDITION_LP__
#define __DBOG_SKILL_CONDITION_LP__

#include "SkillCondition.h"
#include "ObjectManager.h"
#include "HelperNpcManager.h"
#include "CPlayer.h"
#include "Party.h"
#include "NtlParty.h"

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

		CNpc* pBot = GetBot();
		if (!pBot)
		{
			return NULL;
		}

		auto pHelperMgr = GetHelperNpcManager();
		bool isHelper = false;
		bool isActiveLinkedHelper = false;
		if (pHelperMgr)
		{
			isHelper = pHelperMgr->IsRegisteredHelper(pBot);
			isActiveLinkedHelper = pHelperMgr->IsActiveLinkedHelper(pBot);
		}

		if (isActiveLinkedHelper)
		{
			// Helper override: if linked PC exists and is low (or simply missing any LP when override is zero), prefer healing them
			if (pBot->GetLinkPc() != INVALID_HOBJECT)
			{
				HOBJECT hLink = pBot->GetLinkPc();
				CCharacter* pLinked = g_pObjectManager->GetChar(hLink);
				if (pLinked && pLinked->IsInitialized())
				{
					WORD wThreshold = m_wUse_Skill_LP;
					const sHELPER_NPC_CONFIG& cfg = pHelperMgr ? pHelperMgr->GetConfig() : sHELPER_NPC_CONFIG();
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

			// If leader is fine, scan the leader's party for the lowest-LP member and prefer them
			// This enables single-target heals to cover the entire party, not only the leader
			if (pBot->GetLinkPc() != INVALID_HOBJECT)
			{
				CPlayer* pLeader = g_pObjectManager->GetPC(pBot->GetLinkPc());
				if (pLeader && pLeader->IsInitialized() && pLeader->GetParty())
				{
					WORD wThreshold = m_wUse_Skill_LP;
					const sHELPER_NPC_CONFIG& cfg = pHelperMgr ? pHelperMgr->GetConfig() : sHELPER_NPC_CONFIG();
					bool bMissingLpMode = false; // when override==0 heal anyone missing LP
					if (cfg.wHealLpThresholdOverride > 0)
						wThreshold = cfg.wHealLpThresholdOverride;
					else if (cfg.wHealLpThresholdOverride == 0)
						bMissingLpMode = true;

					CCharacter* pBest = NULL;
					float bestMissingPct = -1.0f;
					BYTE cnt = pLeader->GetParty()->GetPartyMemberCount();
					for (BYTE i = 0; i < cnt; ++i)
					{
						const sPARTY_MEMBER_INFO& mi = pLeader->GetParty()->GetMemberInfo(i);
						CPlayer* pMem = g_pObjectManager->GetPC(mi.hHandle);
						if (!pMem || !pMem->IsInitialized()) continue;
						if (pMem->IsFainting()) continue; // resurrection handled elsewhere
						if (!pMem->GetCurWorld() || !pBot->GetCurWorld()) continue; // world must be valid
						if (pMem->GetCurWorld() != pBot->GetCurWorld()) continue; // must be in same world
						bool qualifies = bMissingLpMode ? (pMem->GetCurLP() < pMem->GetMaxLP()) : pMem->ConsiderLPLow((float)wThreshold);
						if (!qualifies) continue;
						float missingPct = 100.0f - pMem->GetCurLpInPercent();
						if (missingPct > bestMissingPct)
						{
							bestMissingPct = missingPct;
							pBest = pMem;
						}
					}
					if (pBest)
					{
						m_pPartyMemberLowLP = pBest;
						return pSkill;
					}
				}
			}

			// Helper: self LP low with override handling
			if (pHelperMgr && pHelperMgr->GetConfig().wHealLpThresholdOverride == 0)
			{
				if (pBot->GetCurLP() < pBot->GetMaxLP())
					return pSkill;
			}
			else if (pBot->ConsiderLPLow(m_wUse_Skill_LP))
			{
				return pSkill;
			}
		}
		else
		{
			// Non-helper legacy: only use original LP threshold on self
			if (pBot->ConsiderLPLow(m_wUse_Skill_LP))
			{
				return pSkill;
			}
		}
	}

	return NULL;
}

inline void CSkillCondition_LP::AppointTargetSelf_ApplyTargetParty(sSKILL_TARGET_LIST& rTargetList)
{
	CNpc* pBot = GetBot();
	if (GetApplyRangeType() && GetTargetMaxCount() != 1)
	{
		GetTarget_ApplyRange_Party_LPLow(m_pPartyMemberLowLP, rTargetList, GetTargetMaxCount());
		// Fallback to linked PC Direct only for helpers
		if (pBot && GetHelperNpcManager() && GetHelperNpcManager()->IsRegisteredHelper(pBot))
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
		if (pBot && GetHelperNpcManager() && GetHelperNpcManager()->IsRegisteredHelper(pBot) && m_pPartyMemberLowLP)
			rTargetList.AddTarget(m_pPartyMemberLowLP->GetID());
	}
}

inline void CSkillCondition_LP::AppointTargetTarget_ApplyTargetParty(HOBJECT& hTarget, sSKILL_TARGET_LIST& rTargetList)
{
	CNpc* pBot = GetBot();
	auto pHelperMgr = GetHelperNpcManager();
	bool bVerbose = pHelperMgr ? pHelperMgr->GetConfig().bVerboseLogs : false;
	hTarget = m_pPartyMemberLowLP ? m_pPartyMemberLowLP->GetID() : (pBot ? pBot->GetID() : INVALID_HOBJECT);
	if(hTarget == INVALID_HOBJECT)
	{
		if(bVerbose)
			ERR_LOG(LOG_BOTAI, "fail : INVALID_HOBJECT == hTarget");
		rTargetList.Init();
		return;
	}

	if (GetApplyRangeType() && GetTargetMaxCount() != 1)
	{
		GetTarget_ApplyRange_Party_LPLow(m_pPartyMemberLowLP, rTargetList, GetTargetMaxCount());
		// Fallback to linked PC Direct only for helpers
		if (pHelperMgr && pBot && pHelperMgr->IsRegisteredHelper(pBot))
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
		if (pHelperMgr && pBot && pHelperMgr->IsRegisteredHelper(pBot) && m_pPartyMemberLowLP)
			rTargetList.AddTarget(m_pPartyMemberLowLP->GetID());
	}
}

#endif