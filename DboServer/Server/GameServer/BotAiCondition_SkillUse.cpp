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
	m_dwSinceLastProactiveScanMs = 0;
	m_dwChaseNoAggroMs = 0;
	m_dwSinceLastSkillTryMs = 0;
	m_dwSinceLastFollowReassertMs = 0;
	m_fLastLeaderDist = -1.f;
	m_dwNoFollowProgressMs = 0;
	m_dwSinceLastRebuffCheckMs = 0;
	m_dwSinceRandomFollowMs = 0;
}

CBotAiCondition_SkillUse::~CBotAiCondition_SkillUse()
{
}



int CBotAiCondition_SkillUse::OnUpdate(DWORD dwTickDiff, float fMultiple)
{
	// Advance timers for responsiveness
	m_dwSinceLastProactiveScanMs = UnsignedSafeIncrease<DWORD>(m_dwSinceLastProactiveScanMs, dwTickDiff);
	m_dwSinceLastSkillTryMs = UnsignedSafeIncrease<DWORD>(m_dwSinceLastSkillTryMs, dwTickDiff);
	m_dwSinceLastFollowReassertMs = UnsignedSafeIncrease<DWORD>(m_dwSinceLastFollowReassertMs, dwTickDiff);

	m_dwTime = UnsignedSafeIncrease<DWORD>(m_dwTime, dwTickDiff);
	if (m_dwTime >= 1000)
	{
		m_dwTime = 0;

		bool bPrioritizeHealing = false;
		if (GetBot()->GetLinkPc() != INVALID_HOBJECT)
		{
			const sHELPER_NPC_CONFIG& cfg = GetHelperNpcManager()->GetConfig();
			CCharacter* pLinked = g_pObjectManager->GetChar(GetBot()->GetLinkPc());
			if (pLinked && pLinked->IsInitialized())
			{
				if (cfg.wHealLpThresholdOverride == 0)
				{
					float fMissingPct = 100.f - pLinked->GetCurLpInPercent();
					bPrioritizeHealing = (fMissingPct >= (float)cfg.wHealPriorityMinMissingPercent);
				}
				else
				{
					bPrioritizeHealing = pLinked->ConsiderLPLow((float)cfg.wHealLpThresholdOverride);
				}
			}

			// EP top-up and stale lock clear
			const WORD wEpLowThreshold = 20;
			if (GetBot()->GetCurEP() < wEpLowThreshold)
			{
				GetBot()->SetCurEP(GetBot()->GetMaxEP());
				if (GetHelperNpcManager()->GetConfig().bVerboseLogs)
					ERR_LOG(LOG_BOTAI, "HelperNPC: EP auto-refill to max (%u) for helper %u", GetBot()->GetMaxEP(), GetBot()->GetID());
			}
			CSkillManagerBot* pSM = (CSkillManagerBot*)GetBot()->GetSkillManager();
			if (pSM && pSM->IsSkillUseLock())
			{
				BYTE st = GetBot()->GetCharStateID();
				if (st != CHARSTATE_CASTING && st != CHARSTATE_SKILL_AFFECTING)
				{
					pSM->SetSkillUse_Unlock();
					if (GetHelperNpcManager()->GetConfig().bVerboseLogs)
						ERR_LOG(LOG_BOTAI, "HelperNPC: cleared stale skill-use lock (state=%u)", st);
				}
			}
		}

		// Random follow behavior when FollowLeader is disabled
		if (GetBot()->GetLinkPc() != INVALID_HOBJECT)
		{
			const sHELPER_NPC_CONFIG* pCfg = GetHelperNpcManager()->IsRegisteredHelper(GetBot())
				? GetHelperNpcManager()->GetConfigForHelper(GetBot())
				: &GetHelperNpcManager()->GetConfig();
			if (pCfg && !pCfg->bFollowLeader)
			{
				m_dwSinceRandomFollowMs = UnsignedSafeIncrease<DWORD>(m_dwSinceRandomFollowMs, 1000);
				if (m_dwSinceRandomFollowMs >= 4000 && GetBot()->GetTargetListManager()->GetAggroCount() == 0)
				{
					m_dwSinceRandomFollowMs = 0;
					CPlayer* pLeader = (CPlayer*)g_pObjectManager->GetChar(GetBot()->GetLinkPc());
					if (pLeader && pLeader->IsInitialized())
					{
						CPlayer* pFollow = pLeader; // default to leader if no party
						if (pLeader->GetParty() && pLeader->GetParty()->GetPartyMemberCount() > 0)
						{
							BYTE cnt = pLeader->GetParty()->GetPartyMemberCount();
							BYTE tries = 0;
							while (tries < cnt)
							{
								BYTE idx = (BYTE)(rand() % cnt);
								const sPARTY_MEMBER_INFO& mi = pLeader->GetParty()->GetMemberInfo(idx);
								CPlayer* pCand = g_pObjectManager->GetPC(mi.hHandle);
								if (pCand && pCand->IsInitialized() && !pCand->IsFainting() && pCand->GetCurWorld() == GetBot()->GetCurWorld())
								{
									pFollow = pCand; break;
								}
								++tries;
							}
						}
						sVECTOR3 vLoc; pFollow->GetCurLoc().CopyTo(vLoc);
						GetBot()->SendCharStateFollowing(pFollow->GetID(), 1.5f, DBO_MOVE_FOLLOW_FRIENDLY, vLoc, true);
						if (pCfg->bVerboseLogs)
							ERR_LOG(LOG_BOTAI, "HelperNPC: random-follow target set to %u", pFollow->GetID());
					}
				}
			}
		}

		// Healer resurrection: if configured, try to resurrect fainted party members
		if (GetHelperNpcManager()->IsRegisteredHelper(GetBot()))
		{
			const sHELPER_NPC_CONFIG* pCfg = GetHelperNpcManager()->GetConfigForHelper(GetBot());
			if (pCfg && pCfg->resurrectSkillTblidx != INVALID_TBLIDX)
			{
				CPlayer* pLeader = (CPlayer*)g_pObjectManager->GetChar(GetBot()->GetLinkPc());
				if (pLeader && pLeader->IsInitialized() && pLeader->GetParty())
				{
					CParty* party = pLeader->GetParty();
					BYTE cnt = party->GetPartyMemberCount();
					for (BYTE i = 0; i < cnt; ++i)
					{
						const sPARTY_MEMBER_INFO& mi = party->GetMemberInfo(i);
						CPlayer* pMem = g_pObjectManager->GetPC(mi.hHandle);
						if (pMem && pMem->IsInitialized() && pMem->IsFainting() && pMem->GetCurWorld() == GetBot()->GetCurWorld())
						{
							CSkillManagerBot* pSM = (CSkillManagerBot*)GetBot()->GetSkillManager();
							if (pSM)
							{
								CSkillCondition* pCond = pSM->FindSkillCondition(pCfg->resurrectSkillTblidx);
								if (pCond && !pSM->IsSkillUseLock())
								{
									// Temporarily set target to the fainted member and queue the skill
									if (GetHelperNpcManager()->GetConfig().bVerboseLogs)
										ERR_LOG(LOG_BOTAI, "HelperNPC: attempt resurrect skill=%u on %u", pCfg->resurrectSkillTblidx, pMem->GetID());
									CBotAiState* pCurState = GetBot()->GetBotController()->GetCurrentState();
									if (pCurState)
									{
										CBotAiAction_SkillUse* pSkillUse = new CBotAiAction_SkillUse(GetBot(), pCond->GetSkillConditionIdx());
										if (pCurState->AddSubControlQueue(pSkillUse, true))
										{
											// Set appoint target to fainted member by updating bot's target handle only for chase stop; actual target computed by condition
											GetBot()->SetTargetHandle(pMem->GetID());
										}
									}
								}
							}
							break; // one at a time
						}
					}
				}
			}
		}

		// Follow watchdog
		if (GetBot()->GetLinkPc() != INVALID_HOBJECT && GetHelperNpcManager()->GetConfig().bFollowLeader)
		{
			if (GetBot()->GetTargetListManager()->GetAggroCount() == 0)
			{
				CPlayer* pLeader = (CPlayer*)g_pObjectManager->GetChar(GetBot()->GetLinkPc());
				if (pLeader && pLeader->IsInitialized())
				{
					const bool bSameWorld = (pLeader->GetCurWorld() == GetBot()->GetCurWorld());
					const bool bTooFar = !GetBot()->IsInRange(pLeader, (float)NTL_MAX_RADIUS_OF_VISIBLE_AREA);
					if (!bSameWorld || bTooFar)
					{
						if (!bPrioritizeHealing)
						{
							HOBJECT hLocalEnemy = GetBot()->ConsiderScanTarget(GetBot()->GetTbldat()->wSight_Range);
							if (hLocalEnemy != INVALID_HOBJECT)
							{
								CCharacter* pVictim = g_pObjectManager->GetChar(hLocalEnemy);
								if (pVictim && pVictim->IsInitialized() && GetBot()->IsTargetAttackble(pVictim, GetBot()->GetTbldat()->wSight_Range))
								{
									if (GetBot()->GetTargetHandle() != hLocalEnemy)
										GetBot()->SetTargetHandle(hLocalEnemy);
									GetBot()->ChangeAggro(hLocalEnemy, DBO_AGGRO_CHANGE_TYPE_INCREASE, GetBot()->GetTbldat()->wBasic_Aggro_Point + 1);
									sVECTOR3 vDestLoc; pVictim->GetCurLoc().CopyTo(vDestLoc);
									GetBot()->SendCharStateFollowing(hLocalEnemy, GetBot()->GetAttackRange(pVictim), DBO_MOVE_FOLLOW_AUTO_ATTACK, vDestLoc, true);
									if (GetHelperNpcManager()->GetConfig().bVerboseLogs)
										ERR_LOG(LOG_BOTAI, "HelperNPC: far-from-leader proactive engage enemy=%u", hLocalEnemy);
									m_dwOutOfRangeTimeMs = 0;
									return m_status;
								}
							}
						}

						const sHELPER_NPC_CONFIG& cfg = GetHelperNpcManager()->GetConfig();
						WORD wHealThr = cfg.wHealLpThresholdOverride > 0 ? cfg.wHealLpThresholdOverride : 35;
						if (pLeader->GetCurLpInPercent() <= (float)wHealThr)
						{
							CWorld* pWorld = pLeader->GetCurWorld();
							if (pWorld)
							{
								CNtlVector vLoc = pLeader->GetCurLoc();
								CNtlVector vDir = pLeader->GetCurDir();
								if (GetBot()->GetBotController()->ChangeControlState_Teleporting(pWorld->GetID(), pWorld->GetIdx(), vLoc, vDir))
								{
									if (GetHelperNpcManager()->GetConfig().bVerboseLogs)
										ERR_LOG(LOG_BOTAI, "HelperNPC: teleport-resync (leader low HP %.1f%%) to world=%u loc=(%.2f,%.2f,%.2f)", pLeader->GetCurLpInPercent(), pWorld->GetID(), vLoc.x, vLoc.y, vLoc.z);
									// Post-teleport recovery
									if (CSkillManagerBot* pSM = (CSkillManagerBot*)GetBot()->GetSkillManager()) pSM->SetSkillUse_Unlock();
									GetBot()->SendCharStateStanding(true);
									// Reassert follow if configured
									if (GetHelperNpcManager()->GetConfig().bFollowLeader)
									{
										sVECTOR3 vFollow; pLeader->GetCurLoc().CopyTo(vFollow);
										GetBot()->SendCharStateFollowing(pLeader->GetID(), 1.5f, DBO_MOVE_FOLLOW_FRIENDLY, vFollow, true);
									}
									// Nudge assist if allowed
									if (GetHelperNpcManager()->GetConfig().bAssistLeaderTarget)
									{
										HOBJECT hVictim = pLeader->GetTargetHandle();
										if (hVictim != INVALID_HOBJECT)
										{
											if (GetBot()->GetTargetHandle() != hVictim) GetBot()->SetTargetHandle(hVictim);
										}
									}
									m_dwSinceLastSkillTryMs = 0;
								}
								m_dwOutOfRangeTimeMs = 0;
								return m_status;
							}
						}

						m_dwOutOfRangeTimeMs = UnsignedSafeIncrease<DWORD>(m_dwOutOfRangeTimeMs, 1000);
						if (m_dwOutOfRangeTimeMs >= 3000)
						{
							CWorld* pWorld = pLeader->GetCurWorld();
							if (pWorld)
							{
								CNtlVector vLoc = pLeader->GetCurLoc();
								CNtlVector vDir = pLeader->GetCurDir();
								if (GetBot()->GetBotController()->ChangeControlState_Teleporting(pWorld->GetID(), pWorld->GetIdx(), vLoc, vDir))
								{
									if (GetHelperNpcManager()->GetConfig().bVerboseLogs)
										ERR_LOG(LOG_BOTAI, "HelperNPC: teleport-resync to leader h=%u world=%u loc=(%.2f,%.2f,%.2f)", pLeader->GetID(), pWorld->GetID(), vLoc.x, vLoc.y, vLoc.z);
									if (CSkillManagerBot* pSM = (CSkillManagerBot*)GetBot()->GetSkillManager()) pSM->SetSkillUse_Unlock();
									GetBot()->SendCharStateStanding(true);
									if (GetHelperNpcManager()->GetConfig().bFollowLeader)
									{
										sVECTOR3 vFollow; pLeader->GetCurLoc().CopyTo(vFollow);
										GetBot()->SendCharStateFollowing(pLeader->GetID(), 1.5f, DBO_MOVE_FOLLOW_FRIENDLY, vFollow, true);
									}
									if (GetHelperNpcManager()->GetConfig().bAssistLeaderTarget)
									{
										HOBJECT hVictim = pLeader->GetTargetHandle();
										if (hVictim != INVALID_HOBJECT)
										{
											if (GetBot()->GetTargetHandle() != hVictim) GetBot()->SetTargetHandle(hVictim);
										}
									}
									m_dwSinceLastSkillTryMs = 0;
								}
								m_dwOutOfRangeTimeMs = 0;
							}
						}
					}
					else
					{
						m_dwOutOfRangeTimeMs = 0;
						BYTE st = GetBot()->GetCharStateID();
						// Only reassert if not casting/affecting and either not already following leader or too far from desired follow radius
						if (st != CHARSTATE_CASTING && st != CHARSTATE_SKILL_AFFECTING)
						{
							const DWORD FOLLOW_REASSERT_COOLDOWN_MS = 3000;
							bool bShouldReassert = false;
							float fDist = GetBot()->GetDistance(pLeader->GetCurLoc());

							// If helper is far but same world and idle, teleport immediately to catch up
							if (fDist > 25.0f && GetBot()->GetTargetListManager()->GetAggroCount() == 0)
							{
								CWorld* pWorld = pLeader->GetCurWorld();
								if (pWorld)
								{
									CNtlVector vLoc = pLeader->GetCurLoc();
									CNtlVector vDir = pLeader->GetCurDir();
									if (GetBot()->GetBotController()->ChangeControlState_Teleporting(pWorld->GetID(), pWorld->GetIdx(), vLoc, vDir))
									{
										if (GetHelperNpcManager()->GetConfig().bVerboseLogs)
											ERR_LOG(LOG_BOTAI, "HelperNPC: teleport-resync (far %.2fm) to leader %u", fDist, pLeader->GetID());
										if (CSkillManagerBot* pSM = (CSkillManagerBot*)GetBot()->GetSkillManager()) pSM->SetSkillUse_Unlock();
										GetBot()->SendCharStateStanding(true);
										if (GetHelperNpcManager()->GetConfig().bFollowLeader)
										{
											sVECTOR3 vFollow; pLeader->GetCurLoc().CopyTo(vFollow);
											GetBot()->SendCharStateFollowing(pLeader->GetID(), 1.5f, DBO_MOVE_FOLLOW_FRIENDLY, vFollow, true);
										}
										if (GetHelperNpcManager()->GetConfig().bAssistLeaderTarget)
										{
											HOBJECT hVictim = pLeader->GetTargetHandle();
											if (hVictim != INVALID_HOBJECT)
											{
												if (GetBot()->GetTargetHandle() != hVictim) GetBot()->SetTargetHandle(hVictim);
											}
										}
										m_dwSinceLastSkillTryMs = 0;
									}
									m_dwSinceLastFollowReassertMs = 0;
									m_dwNoFollowProgressMs = 0;
									return m_status;
								}
							}
							const float DESIRED_DIST = 1.5f;
							BYTE byMoveFlag = GetBot()->GetMoveFlag();
							bool bFollowingLeader = (GetBot()->GetFollowTarget() == pLeader->GetID());

							// Detect lack of progress toward leader if moving
							if (m_fLastLeaderDist >= 0.f)
							{
								if (fDist + 0.25f >= m_fLastLeaderDist && byMoveFlag != NTL_MOVE_FLAG_INVALID)
								{
									m_dwNoFollowProgressMs = UnsignedSafeIncrease<DWORD>(m_dwNoFollowProgressMs, 1000);
								}
								else
								{
									m_dwNoFollowProgressMs = 0;
								}
							}
							m_fLastLeaderDist = fDist;

							if (!bFollowingLeader)
							{
								// Not following correct target -> reassert
								bShouldReassert = true;
							}
							else if (byMoveFlag == NTL_MOVE_FLAG_INVALID)
							{
								// Not moving while far away -> kick follow
								if (fDist > (DESIRED_DIST + 1.5f))
									bShouldReassert = true;
							}
							else if (m_dwNoFollowProgressMs >= 4000)
							{
								// Moving but stuck for >= 4s -> reassert
								bShouldReassert = true;
							}

							// If we've been stuck making no progress for a while, do a local teleport-resync even if in range
							if (m_dwNoFollowProgressMs >= 8000 && fDist > 20.0f)
							{
								CWorld* pWorld = pLeader->GetCurWorld();
								if (pWorld)
								{
									CNtlVector vLoc = pLeader->GetCurLoc();
									CNtlVector vDir = pLeader->GetCurDir();
									if (GetBot()->GetBotController()->ChangeControlState_Teleporting(pWorld->GetID(), pWorld->GetIdx(), vLoc, vDir))
									{
										if (GetHelperNpcManager()->GetConfig().bVerboseLogs)
											ERR_LOG(LOG_BOTAI, "HelperNPC: teleport-resync (stuck %.1fs, dist=%.2f) leader h=%u world=%u", m_dwNoFollowProgressMs / 1000.0f, fDist, pLeader->GetID(), pWorld->GetID());
										if (CSkillManagerBot* pSM = (CSkillManagerBot*)GetBot()->GetSkillManager()) pSM->SetSkillUse_Unlock();
										GetBot()->SendCharStateStanding(true);
										if (GetHelperNpcManager()->GetConfig().bFollowLeader)
										{
											sVECTOR3 vFollow; pLeader->GetCurLoc().CopyTo(vFollow);
											GetBot()->SendCharStateFollowing(pLeader->GetID(), 1.5f, DBO_MOVE_FOLLOW_FRIENDLY, vFollow, true);
										}
										if (GetHelperNpcManager()->GetConfig().bAssistLeaderTarget)
										{
											HOBJECT hVictim = pLeader->GetTargetHandle();
											if (hVictim != INVALID_HOBJECT)
											{
												if (GetBot()->GetTargetHandle() != hVictim) GetBot()->SetTargetHandle(hVictim);
											}
										}
										m_dwNoFollowProgressMs = 0;
										m_dwSinceLastFollowReassertMs = 0;
										m_dwSinceLastSkillTryMs = 0;
										return m_status;
									}
									m_dwNoFollowProgressMs = 0;
									m_dwSinceLastFollowReassertMs = 0;
									return m_status;
								}
							}

							if (bShouldReassert && m_dwSinceLastFollowReassertMs >= FOLLOW_REASSERT_COOLDOWN_MS)
							{
								sVECTOR3 vLeaderLoc; pLeader->GetCurLoc().CopyTo(vLeaderLoc);
								if (GetBot()->SendCharStateFollowing(pLeader->GetID(), 1.5f, DBO_MOVE_FOLLOW_FRIENDLY, vLeaderLoc, true))
								{
									if (!bFollowingLeader || byMoveFlag == NTL_MOVE_FLAG_INVALID || m_dwNoFollowProgressMs >= 4000)
									{
										if (GetHelperNpcManager()->GetConfig().bVerboseLogs)
											ERR_LOG(LOG_BOTAI, "HelperNPC: reassert follow to leader %u (state=%u, dist=%.2f)", pLeader->GetID(), st, fDist);
									}
								}
								m_dwSinceLastFollowReassertMs = 0;
							}
						}

						if (GetBot()->GetTargetHandle() != INVALID_HOBJECT)
						{
							m_dwChaseNoAggroMs = UnsignedSafeIncrease<DWORD>(m_dwChaseNoAggroMs, 1000);
							if (m_dwChaseNoAggroMs >= 3000)
							{
								GetBot()->SetTargetHandle(INVALID_HOBJECT);
								m_dwChaseNoAggroMs = 0;
								if (GetHelperNpcManager()->GetConfig().bVerboseLogs)
									ERR_LOG(LOG_BOTAI, "HelperNPC: cleared stale target to resume follow/assist");
							}
						}
						else
						{
							m_dwChaseNoAggroMs = 0;
						}

						// Use per-helper config if this bot is a registered helper; fallback to base
						const sHELPER_NPC_CONFIG* pCfgAssist = GetHelperNpcManager()->IsRegisteredHelper(GetBot())
							? GetHelperNpcManager()->GetConfigForHelper(GetBot())
							: &GetHelperNpcManager()->GetConfig();
						if (!bPrioritizeHealing && pCfgAssist && pCfgAssist->bAssistLeaderTarget)
						{
							HOBJECT hVictim = pLeader->GetTargetHandle();
							if (hVictim != INVALID_HOBJECT)
							{
								CCharacter* pVictim = g_pObjectManager->GetChar(hVictim);
								if (pVictim && pVictim->IsInitialized())
								{
									// If proactive mode is enabled, try to split targets by attacking a different nearby enemy
									if (pCfgAssist->bProactiveAutoAttack)
									{
										WORD wRange = pCfgAssist->wAttackScanRange > 0 ? pCfgAssist->wAttackScanRange : GetBot()->GetTbldat()->wSight_Range;
										HOBJECT hAlt = GetBot()->ConsiderScanTarget(wRange);
										if (hAlt != INVALID_HOBJECT && hAlt != hVictim)
										{
											CCharacter* pAlt = g_pObjectManager->GetChar(hAlt);
											if (pAlt && pAlt->IsInitialized() && GetBot()->IsTargetAttackble(pAlt, wRange))
											{
												if (GetBot()->GetTargetHandle() != hAlt)
													GetBot()->SetTargetHandle(hAlt);
												GetBot()->ChangeAggro(hAlt, DBO_AGGRO_CHANGE_TYPE_INCREASE, GetBot()->GetTbldat()->wBasic_Aggro_Point + 1);
												sVECTOR3 vAlt; pAlt->GetCurLoc().CopyTo(vAlt);
												GetBot()->SendCharStateFollowing(hAlt, GetBot()->GetAttackRange(pAlt), DBO_MOVE_FOLLOW_AUTO_ATTACK, vAlt, true);
												return m_status; // prefer alternate target this tick
											}
										}
									}
									// Otherwise assist the leader's current target
									if (GetBot()->IsTargetAttackble(pVictim, GetBot()->GetTbldat()->wSight_Range))
									{
										if (GetBot()->GetTargetHandle() != hVictim)
											GetBot()->SetTargetHandle(hVictim);
										GetBot()->ChangeAggro(hVictim, DBO_AGGRO_CHANGE_TYPE_INCREASE, GetBot()->GetTbldat()->wBasic_Aggro_Point + 1);
										sVECTOR3 vDestLoc; pVictim->GetCurLoc().CopyTo(vDestLoc);
										GetBot()->SendCharStateFollowing(hVictim, GetBot()->GetAttackRange(pVictim), DBO_MOVE_FOLLOW_AUTO_ATTACK, vDestLoc, true);
									}
								}
							}
						}
					}

					// Proactive auto-attack (1s cadence)
					if (!bPrioritizeHealing)
					{
						const sHELPER_NPC_CONFIG* pCfg = GetHelperNpcManager()->IsRegisteredHelper(GetBot())
							? GetHelperNpcManager()->GetConfigForHelper(GetBot())
							: &GetHelperNpcManager()->GetConfig();
						if (pCfg && pCfg->bProactiveAutoAttack)
						{
							if (GetBot()->GetTargetListManager()->GetAggroCount() == 0 && GetBot()->GetTargetHandle() == INVALID_HOBJECT)
							{
								m_dwSinceLastProactiveScanMs = UnsignedSafeIncrease<DWORD>(m_dwSinceLastProactiveScanMs, 1000);
								if (m_dwSinceLastProactiveScanMs >= pCfg->dwAttackScanCooldownMs)
								{
									m_dwSinceLastProactiveScanMs = 0;
									WORD wRange = pCfg->wAttackScanRange > 0 ? pCfg->wAttackScanRange : GetBot()->GetTbldat()->wSight_Range;
									HOBJECT hEnemy = GetBot()->ConsiderScanTarget(wRange);
									if (hEnemy != INVALID_HOBJECT)
									{
										CCharacter* pVictim = g_pObjectManager->GetChar(hEnemy);
										if (pVictim && pVictim->IsInitialized() && GetBot()->IsTargetAttackble(pVictim, wRange))
										{
											if (GetBot()->GetTargetHandle() != hEnemy)
												GetBot()->SetTargetHandle(hEnemy);
											GetBot()->ChangeAggro(hEnemy, DBO_AGGRO_CHANGE_TYPE_INCREASE, GetBot()->GetTbldat()->wBasic_Aggro_Point + 1);
											sVECTOR3 vDestLoc; pVictim->GetCurLoc().CopyTo(vDestLoc);
											GetBot()->SendCharStateFollowing(hEnemy, GetBot()->GetAttackRange(pVictim), DBO_MOVE_FOLLOW_AUTO_ATTACK, vDestLoc, true);
											if (pCfg->bVerboseLogs)
												ERR_LOG(LOG_BOTAI, "HelperNPC: proactive engage enemy=%u range=%u (auto-attack)", hEnemy, wRange);
										}
									}
								}
							}
							else
							{
								m_dwSinceLastProactiveScanMs = 0;
							}
						}
					}
				}
			}
		}

		// Skill queuing
		if (GetBot()->HasNearbyPlayer(false) || GetBot()->GetLinkPc() != INVALID_HOBJECT)
		{
			if (!GetBot()->GetStateManager()->IsCharCondition(CHARCOND_CONFUSED))
			{
				CBotAiState* pCurState = GetBot()->GetBotController()->GetCurrentState();
				if (pCurState)
				{
					CComplexState* pCurAction = pCurState->GetCurrentAction();
					if (pCurAction && pCurAction->GetControlStateID() == BOTCONTROL_ACTION_LOOK)
					{
						return m_status;
					}
					else
					{
						CSkillManagerBot* pBotSkillManager = (CSkillManagerBot*)GetBot()->GetSkillManager();
						if (!pBotSkillManager->IsSkillUseLock())
						{
							const DWORD SKILL_TRY_COOLDOWN_MS = 100;
							if (GetBot()->GetLinkPc() != INVALID_HOBJECT && m_dwSinceLastSkillTryMs >= SKILL_TRY_COOLDOWN_MS)
							{
								m_dwSinceLastSkillTryMs = 0;
								CSkillCondition* pSkillCondition = NULL;
								if (pBotSkillManager->GetNumberOfSkill() > 0)
									pSkillCondition = pBotSkillManager->GetSkill(dwTickDiff);

								if (pSkillCondition && pSkillCondition->GetCanUseSkill())
								{
									if (GetBot()->GetCharStateID() == CHARSTATE_FOLLOWING || GetBot()->GetMoveFlag() != NTL_MOVE_FLAG_INVALID)
										GetBot()->SendCharStateStanding(true);

									if (GetHelperNpcManager()->GetConfig().bVerboseLogs)
										ERR_LOG(LOG_BOTAI, "HelperNPC: queue skill idx=%u tblidx=%u", pSkillCondition->GetSkillConditionIdx(), pSkillCondition->GetSkillTblidx());
									CBotAiAction_SkillUse* pSkillUse = new CBotAiAction_SkillUse(GetBot(), pSkillCondition->GetSkillConditionIdx());
									if (!pCurState->AddSubControlQueue(pSkillUse, true))
										m_status = FAILED;
								}
							}
							else if (GetBot()->GetLinkPc() == INVALID_HOBJECT)
							{
								CSkillCondition* pSkillCondition = NULL;
								if (pBotSkillManager->GetNumberOfSkill() > 0)
									pSkillCondition = pBotSkillManager->GetSkill(dwTickDiff);
								if (pSkillCondition && pSkillCondition->GetCanUseSkill())
								{
									if (GetBot()->GetCharStateID() == CHARSTATE_FOLLOWING || GetBot()->GetMoveFlag() != NTL_MOVE_FLAG_INVALID)
									{
										GetBot()->SendCharStateStanding(true);
										return m_status;
									}
									CBotAiAction_SkillUse* pSkillUse = new CBotAiAction_SkillUse(GetBot(), pSkillCondition->GetSkillConditionIdx());
									if (!pCurState->AddSubControlQueue(pSkillUse, true))
										m_status = FAILED;
								}
							}
						}
					}
				}
			}
		}
	}

	// Rebuff controller (simple cadence): check party members and reapply missing/expiring buffs
	if (GetHelperNpcManager()->IsRegisteredHelper(GetBot()))
	{
		const sHELPER_NPC_CONFIG* pCfg = GetHelperNpcManager()->GetConfigForHelper(GetBot());
		if (pCfg && pCfg->dwRebuffCooldownMs > 0 && !pCfg->vBuffSkills.empty())
		{
			m_dwSinceLastRebuffCheckMs = UnsignedSafeIncrease<DWORD>(m_dwSinceLastRebuffCheckMs, dwTickDiff);
			if (m_dwSinceLastRebuffCheckMs >= pCfg->dwRebuffCooldownMs)
			{
				m_dwSinceLastRebuffCheckMs = 0;
				CPlayer* pLeader = (CPlayer*)g_pObjectManager->GetChar(GetBot()->GetLinkPc());
				if (pLeader && pLeader->IsInitialized())
				{
					auto tryBuffOn = [&](CPlayer* pTarget) {
						if (!pTarget || !pTarget->IsInitialized() || pTarget->IsFainting() || pTarget->GetCurWorld() != GetBot()->GetCurWorld()) return false;
						CSkillManagerBot* pSM = (CSkillManagerBot*)GetBot()->GetSkillManager(); if (!pSM) return false;
						for (TBLIDX buffId : pCfg->vBuffSkills)
						{
							// Heuristic: if target already has any buff with the same tblidx active and with enough time left, skip
							CBuff* pExisting = pTarget->GetBuffManager()->FindBuff(buffId);
							if (pExisting)
							{
								if (pExisting->GetRemainTime(0) > pCfg->dwRebuffMinRemainingMs) continue;
							}
							CSkillCondition* pCond = pSM->FindSkillCondition(buffId);
							if (pCond && !pSM->IsSkillUseLock())
							{
								CBotAiState* pCurState = GetBot()->GetBotController()->GetCurrentState();
								if (pCurState)
								{
									CBotAiAction_SkillUse* pSkillUse = new CBotAiAction_SkillUse(GetBot(), pCond->GetSkillConditionIdx());
									if (pCurState->AddSubControlQueue(pSkillUse, true))
									{
										GetBot()->SetTargetHandle(pTarget->GetID());
										if (pCfg->bVerboseLogs)
											ERR_LOG(LOG_BOTAI, "HelperNPC: rebuff queued skill=%u on %u", buffId, pTarget->GetID());
										return true; // queue one buff per tick
									}
								}
							}
						}
						return false;
					};

					// Try leader first
					if (tryBuffOn(pLeader)) return m_status;
					// Then other party members
					if (pLeader->GetParty())
					{
						BYTE cnt = pLeader->GetParty()->GetPartyMemberCount();
						for (BYTE i = 0; i < cnt; ++i)
						{
							const sPARTY_MEMBER_INFO& mi = pLeader->GetParty()->GetMemberInfo(i);
							CPlayer* pMem = g_pObjectManager->GetPC(mi.hHandle);
							if (pMem == pLeader) continue;
							if (tryBuffOn(pMem)) return m_status;
						}
					}
				}
			}
		}
	}

	// Fast-path proactive scan (outside 1s gate)
	do
	{
		if (GetBot()->GetLinkPc() == INVALID_HOBJECT)
			break;

		const sHELPER_NPC_CONFIG& cfg = GetHelperNpcManager()->GetConfig();
		if (!cfg.bProactiveAutoAttack)
			break;

		if (GetBot()->GetTargetListManager()->GetAggroCount() != 0 || GetBot()->GetTargetHandle() != INVALID_HOBJECT)
		{
			m_dwSinceLastProactiveScanMs = 0;
			break;
		}

		bool bHealPriority = false;
		{
			CCharacter* pLeader = g_pObjectManager->GetChar(GetBot()->GetLinkPc());
			if (pLeader && pLeader->IsInitialized())
			{
				if (cfg.wHealLpThresholdOverride == 0)
				{
					float fMissingPct = 100.f - pLeader->GetCurLpInPercent();
					bHealPriority = (fMissingPct >= (float)cfg.wHealPriorityMinMissingPercent);
				}
				else
				{
					bHealPriority = pLeader->ConsiderLPLow((float)cfg.wHealLpThresholdOverride);
				}
			}
		}
		if (bHealPriority)
			break;

		if (m_dwSinceLastProactiveScanMs < cfg.dwAttackScanCooldownMs)
			break;

		m_dwSinceLastProactiveScanMs = 0;
		WORD wRange = cfg.wAttackScanRange > 0 ? cfg.wAttackScanRange : GetBot()->GetTbldat()->wSight_Range;
		HOBJECT hEnemy = GetBot()->ConsiderScanTarget(wRange);
		if (hEnemy == INVALID_HOBJECT)
			break;

		CCharacter* pVictim = g_pObjectManager->GetChar(hEnemy);
		if (!pVictim || !pVictim->IsInitialized())
			break;

		if (!GetBot()->IsTargetAttackble(pVictim, wRange))
			break;

		if (GetBot()->GetTargetHandle() != hEnemy)
			GetBot()->SetTargetHandle(hEnemy);

		GetBot()->ChangeAggro(hEnemy, DBO_AGGRO_CHANGE_TYPE_INCREASE, GetBot()->GetTbldat()->wBasic_Aggro_Point + 1);
		sVECTOR3 vDestLoc; pVictim->GetCurLoc().CopyTo(vDestLoc);
		GetBot()->SendCharStateFollowing(hEnemy, GetBot()->GetAttackRange(pVictim), DBO_MOVE_FOLLOW_AUTO_ATTACK, vDestLoc, true);
		if (GetHelperNpcManager()->GetConfig().bVerboseLogs)
			ERR_LOG(LOG_BOTAI, "HelperNPC: fast-scan proactive engage enemy=%u range=%u", hEnemy, wRange);
	} while (false);

	// Fast-cadence skill queuing (outside 1s gate) to improve reaction speed
	if (GetBot()->HasNearbyPlayer(false) || GetBot()->GetLinkPc() != INVALID_HOBJECT)
	{
		if (!GetBot()->GetStateManager()->IsCharCondition(CHARCOND_CONFUSED))
		{
			CBotAiState* pCurState = GetBot()->GetBotController()->GetCurrentState();
			if (pCurState)
			{
				CComplexState* pCurAction = pCurState->GetCurrentAction();
				if (!(pCurAction && pCurAction->GetControlStateID() == BOTCONTROL_ACTION_LOOK))
				{
					CSkillManagerBot* pBotSkillManager = (CSkillManagerBot*)GetBot()->GetSkillManager();
					if (!pBotSkillManager->IsSkillUseLock())
					{
						// Do not attempt to stand/cast while far from the leader and moving; prioritize catching up
						if (GetBot()->GetLinkPc() != INVALID_HOBJECT)
						{
							CCharacter* pLeader = g_pObjectManager->GetChar(GetBot()->GetLinkPc());
							if (pLeader && pLeader->IsInitialized())
							{
								float fDistL = GetBot()->GetDistance(pLeader->GetCurLoc());
								if (fDistL > 12.0f && GetBot()->GetMoveFlag() != NTL_MOVE_FLAG_INVALID)
								{
									// skip fast-queue this tick to avoid rc=605 while running
									return m_status;
								}
							}
						}
						const DWORD SKILL_TRY_COOLDOWN_MS = 100;
						if (m_dwSinceLastSkillTryMs >= SKILL_TRY_COOLDOWN_MS)
						{
							m_dwSinceLastSkillTryMs = 0;
							CSkillCondition* pSkillCondition = NULL;
							if (pBotSkillManager->GetNumberOfSkill() > 0)
								pSkillCondition = pBotSkillManager->GetSkill(dwTickDiff);
							if (pSkillCondition && pSkillCondition->GetCanUseSkill())
							{
								if (GetBot()->GetCharStateID() == CHARSTATE_FOLLOWING || GetBot()->GetMoveFlag() != NTL_MOVE_FLAG_INVALID)
									GetBot()->SendCharStateStanding(true);
								if (GetHelperNpcManager()->GetConfig().bVerboseLogs)
									ERR_LOG(LOG_BOTAI, "HelperNPC: queue skill idx=%u tblidx=%u", pSkillCondition->GetSkillConditionIdx(), pSkillCondition->GetSkillTblidx());
								CBotAiAction_SkillUse* pSkillUse = new CBotAiAction_SkillUse(GetBot(), pSkillCondition->GetSkillConditionIdx());
								if (!pCurState->AddSubControlQueue(pSkillUse, true))
									m_status = FAILED;
							}
						}
					}
				}
			}
		}
	}

	return m_status;
}


