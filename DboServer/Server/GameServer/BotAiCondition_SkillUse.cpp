#include "stdafx.h"
#include "BotAiCondition_SkillUse.h"
#include "BotAiState.h"
#include "BotAiAction_SkillUse.h"
#include "SkillCondition.h"
// Follow watchdog includes
#include "HelperNpcManager.h"
#include "ObjectManager.h"
#include "CPlayer.h"


CBotAiCondition_SkillUse::CBotAiCondition_SkillUse(CNpc* pBot)
	:CBotAiCondition(pBot, BOTCONTROL_CONDITION_SKILL_USE, "BOTCONTROL_CONDITION_SKILL_USE")
{
	m_dwTime = 0;
	m_dwOutOfRangeTimeMs = 0;
}

CBotAiCondition_SkillUse::~CBotAiCondition_SkillUse()
{
}


int CBotAiCondition_SkillUse::OnUpdate(DWORD dwTickDiff, float fMultiple)
{
	m_dwTime = UnsignedSafeIncrease<DWORD>(m_dwTime, dwTickDiff);
	if (m_dwTime >= 1000)
	{
		m_dwTime = 0;

		// Ensure helpers never run out of EP: top up when low so heals/buffs keep working
		if (GetBot()->GetLinkPc() != INVALID_HOBJECT)
		{
			const WORD wEpLowThreshold = 20; // small threshold above typical skill EP costs
			if (GetBot()->GetCurEP() < wEpLowThreshold)
			{
				GetBot()->SetCurEP(GetBot()->GetMaxEP());
				ERR_LOG(LOG_BOTAI, "HelperNPC: EP auto-refill to max (%u) for helper %u", GetBot()->GetMaxEP(), GetBot()->GetID());
			}
			// Safety: if skill-use is locked but we're not casting/affecting, unlock to avoid permanent stalls
			CSkillManagerBot* pSM = (CSkillManagerBot*)GetBot()->GetSkillManager();
			if (pSM && pSM->IsSkillUseLock())
			{
				BYTE st = GetBot()->GetCharStateID();
				if (st != CHARSTATE_CASTING && st != CHARSTATE_SKILL_AFFECTING)
				{
					pSM->SetSkillUse_Unlock();
					ERR_LOG(LOG_BOTAI, "HelperNPC: cleared stale skill-use lock (state=%u)", st);
				}
			}
		}

	// Follow watchdog: if linked helper is idle and follow is enabled, resume following the leader
		if (GetBot()->GetLinkPc() != INVALID_HOBJECT && GetHelperNpcManager()->GetConfig().bFollowLeader)
		{
			// Only when not engaged
			if (GetBot()->GetTargetListManager()->GetAggroCount() == 0 && GetBot()->GetTargetHandle() == INVALID_HOBJECT)
			{
				CPlayer* pLeader = (CPlayer*)g_pObjectManager->GetChar(GetBot()->GetLinkPc());
				if (pLeader && pLeader->IsInitialized())
				{
					// Teleport/resync watchdog: if different world or far outside visibility for a sustained period, teleport to leader
					const bool bSameWorld = (pLeader->GetCurWorld() == GetBot()->GetCurWorld());
					const bool bTooFar = !GetBot()->IsInRange(pLeader, (float)NTL_MAX_RADIUS_OF_VISIBLE_AREA);
					if (!bSameWorld || bTooFar)
					{
						// Far-from-leader trigger: if enemies are nearby, auto-engage locally instead of teleporting immediately
						HOBJECT hLocalEnemy = GetBot()->ConsiderScanTarget(GetBot()->GetTbldat()->wSight_Range);
						if (hLocalEnemy != INVALID_HOBJECT)
						{
							CCharacter* pVictim = g_pObjectManager->GetChar(hLocalEnemy);
							if (pVictim && pVictim->IsInitialized() && GetBot()->IsTargetAttackble(pVictim, GetBot()->GetTbldat()->wSight_Range))
							{
								if (GetBot()->GetTargetHandle() != hLocalEnemy)
									GetBot()->SetTargetHandle(hLocalEnemy);
								// Nudge aggro so fight AI kicks in
								CObjMsg_YouKeepAggro msg;
								msg.hSource = GetBot()->GetID();
								msg.hProvoker = hLocalEnemy;
								msg.dwAggroPoint = GetBot()->GetTbldat()->wBasic_Aggro_Point + 1;
								GetBot()->SendObjectMsg(&msg);
								ERR_LOG(LOG_BOTAI, "HelperNPC: far-from-leader scan engaged local enemy=%u", hLocalEnemy);
								// While we are engaging something locally, avoid teleport countdown
								m_dwOutOfRangeTimeMs = 0;
								return m_status;
							}
						}

						// If no local enemy found and leader is low HP, teleport right away to heal
						const sHELPER_NPC_CONFIG& cfg = GetHelperNpcManager()->GetConfig();
						WORD wHealThr = cfg.wHealLpThresholdOverride > 0 ? cfg.wHealLpThresholdOverride : 35;
						if (pLeader && pLeader->GetCurLpInPercent() <= (float)wHealThr)
						{
							CWorld* pWorld = pLeader->GetCurWorld();
							if (pWorld)
							{
								CNtlVector vLoc = pLeader->GetCurLoc();
								CNtlVector vDir = pLeader->GetCurDir();
								if (GetBot()->GetBotController()->ChangeControlState_Teleporting(pWorld->GetID(), pWorld->GetIdx(), vLoc, vDir))
								{
									ERR_LOG(LOG_BOTAI, "HelperNPC: teleport-resync (leader low HP %.1f%%) to world=%u loc=(%.2f,%.2f,%.2f)", pLeader->GetCurLpInPercent(), pWorld->GetID(), vLoc.x, vLoc.y, vLoc.z);
								}
								m_dwOutOfRangeTimeMs = 0;
								return m_status;
							}
						}

						// Otherwise, keep (or start) teleport countdown
						m_dwOutOfRangeTimeMs = UnsignedSafeIncrease<DWORD>(m_dwOutOfRangeTimeMs, 1000);
						if (m_dwOutOfRangeTimeMs >= 3000) // 3s sustained out-of-range
						{
							CWorld* pWorld = pLeader->GetCurWorld();
							if (pWorld)
							{
								CNtlVector vLoc = pLeader->GetCurLoc();
								CNtlVector vDir = pLeader->GetCurDir();
								if (GetBot()->GetBotController()->ChangeControlState_Teleporting(pWorld->GetID(), pWorld->GetIdx(), vLoc, vDir))
								{
									ERR_LOG(LOG_BOTAI, "HelperNPC: teleport-resync to leader h=%u world=%u loc=(%.2f,%.2f,%.2f)", pLeader->GetID(), pWorld->GetID(), vLoc.x, vLoc.y, vLoc.z);
								}
								m_dwOutOfRangeTimeMs = 0; // reset after attempt
							}
						}
					}
					else
					{
						// In-range: reset timer and ensure follow is active
						m_dwOutOfRangeTimeMs = 0;
						if (GetBot()->GetCharStateID() != CHARSTATE_FOLLOWING)
						{
							sVECTOR3 vLeaderLoc;
							pLeader->GetCurLoc().CopyTo(vLeaderLoc);
							const float fFollowDist = 2.5f;
							GetBot()->SendCharStateFollowing(pLeader->GetID(), fFollowDist, DBO_MOVE_FOLLOW_FRIENDLY, vLeaderLoc, true);
						}
						// Proactive assist: if allowed and helper is idle, mirror leader's target to keep engaging
						if (GetHelperNpcManager()->GetConfig().bAssistLeaderTarget)
						{
							HOBJECT hVictim = pLeader->GetTargetHandle();
							if (hVictim != INVALID_HOBJECT)
							{
								CCharacter* pVictim = g_pObjectManager->GetChar(hVictim);
								if (pVictim && pVictim->IsInitialized())
								{
									if (GetBot()->IsTargetAttackble(pVictim, GetBot()->GetTbldat()->wSight_Range))
									{
										if (GetBot()->GetTargetHandle() != hVictim)
											GetBot()->SetTargetHandle(hVictim);
										// Nudge aggro so existing fight AI takes over
										CObjMsg_YouKeepAggro msg;
										msg.hSource = pLeader->GetID();
										msg.hProvoker = hVictim;
										msg.dwAggroPoint = GetBot()->GetTbldat()->wBasic_Aggro_Point + 1;
										GetBot()->SendObjectMsg(&msg);
									}
								}
							}
						}
					}
				}
			}
		}

	// For helpers linked to a PC, allow skill attempts regardless of HasNearbyPlayer().
	if (GetBot()->HasNearbyPlayer(false) || GetBot()->GetLinkPc() != INVALID_HOBJECT)
		{
			if (!GetBot()->GetStateManager()->IsCharCondition(CHARCOND_CONFUSED))
			{
				CBotAiState* pCurState = GetBot()->GetBotController()->GetCurrentState();
				if (pCurState)
				{
					CComplexState* pCurAction = pCurState->GetCurrentAction();
					if (pCurAction && pCurAction->GetControlStateID() == BOTCONTROL_ACTION_LOOK)
						return m_status;
					else
					{
						CSkillManagerBot* pBotSkillManager = (CSkillManagerBot*)GetBot()->GetSkillManager();
						if (!pBotSkillManager->IsSkillUseLock())
						{
							CSkillCondition* pSkillCondition = NULL;
							if (pBotSkillManager->GetNumberOfSkill() > 0)
								pSkillCondition = pBotSkillManager->GetSkill(dwTickDiff);

							if (pSkillCondition && pSkillCondition->GetCanUseSkill())
							{
								// Avoid casting while moving/following; stand and try next tick
								if (GetBot()->GetCharStateID() == CHARSTATE_FOLLOWING || GetBot()->GetMoveFlag() != NTL_MOVE_FLAG_INVALID)
								{
									GetBot()->SendCharStateStanding(true);
									return m_status;
								}
								// Log when we queue a skill for helper bots
								if (GetBot()->GetLinkPc() != INVALID_HOBJECT)
								{
									ERR_LOG(LOG_BOTAI, "HelperNPC: queue skill idx=%u tblidx=%u",
										pSkillCondition->GetSkillConditionIdx(), pSkillCondition->GetSkillTblidx());
								}
								CBotAiAction_SkillUse* pSkillUse = new CBotAiAction_SkillUse(GetBot(), pSkillCondition->GetSkillConditionIdx());
								if (!pCurState->AddSubControlQueue(pSkillUse, true))
								{
									m_status = FAILED;
								}
							}
						}
					}
				}
			}
		}

	}

	return m_status;
}

