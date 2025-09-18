#include "stdafx.h"
#include "BotAiCondition_SkillUse.h"
#include "BotAiState.h"
#include "BotAiAction_SkillUse.h"
#include "SkillCondition.h"
// Follow watchdog includes
#include "HelperNpcManager.h"
#include "ObjectManager.h"
#include "CPlayer.h"
#include <unordered_set>


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
	const bool isHelper = GetHelperNpcManager()->IsRegisteredHelper(GetBot());
	const sHELPER_NPC_CONFIG* pHelperCfg = isHelper ? GetHelperNpcManager()->GetConfigForHelper(GetBot()) : nullptr;
	// Advance timers for responsiveness
	m_dwSinceLastProactiveScanMs = UnsignedSafeIncrease<DWORD>(m_dwSinceLastProactiveScanMs, dwTickDiff);
	m_dwSinceLastSkillTryMs = UnsignedSafeIncrease<DWORD>(m_dwSinceLastSkillTryMs, dwTickDiff);
	m_dwSinceLastFollowReassertMs = UnsignedSafeIncrease<DWORD>(m_dwSinceLastFollowReassertMs, dwTickDiff);

	// Track time since last proactive engage/follow switch to prevent rapid oscillation (buffer jitter fix)
	static const DWORD HELPER_MIN_ENGAGE_STICK_MS = 1500; // stay committed at least 1.5s
	m_dwTime = UnsignedSafeIncrease<DWORD>(m_dwTime, dwTickDiff);

	if (m_dwTime >= 1000)
	{
		m_dwTime = 0;

		if (!isHelper)
		{
			// Non-helper: use legacy simple skill queuing
			if (GetBot()->HasNearbyPlayer(false))
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
								CSkillCondition* pSkillCondition = NULL;
								if (pBotSkillManager->GetNumberOfSkill() > 0)
									pSkillCondition = pBotSkillManager->GetSkill(dwTickDiff);

								if (pSkillCondition && pSkillCondition->GetCanUseSkill())
								{
									if (GetBot()->GetCharStateID() == CHARSTATE_FOLLOWING || GetBot()->GetMoveFlag() != NTL_MOVE_FLAG_INVALID)
										GetBot()->SendCharStateStanding(true);

									CBotAiAction_SkillUse* pSkillUse = new CBotAiAction_SkillUse(GetBot(), pSkillCondition->GetSkillConditionIdx());
									if (!pCurState->AddSubControlQueue(pSkillUse, true))
									{
										delete pSkillUse;
									}
								}
							}
						}
					}
				}
			}
			// Early exit for non-helpers to avoid helper-only logic below
			return m_status;
		}

		// Pending resurrect retry housekeeping (runs every tick pre logic)
		if (isHelper && m_hPendingResurrectTarget != INVALID_HOBJECT)
		{
			CPlayer* pPend = (CPlayer*)g_pObjectManager->GetChar(m_hPendingResurrectTarget);
			bool bClear = false;
			if (!pPend || !pPend->IsInitialized()) bClear = true; else if (!pPend->IsFainting() || pPend->GetCurWorld() != GetBot()->GetCurWorld()) bClear = true;
			if (bClear)
			{
				if (pHelperCfg && pHelperCfg->bVerboseLogs && pPend)
					ERR_LOG(LOG_BOTAI, "HelperNPC: resurrect target cleared (revived or invalid) target=%u attempts=%u", m_hPendingResurrectTarget, m_byResurrectAttemptCount);
				if (pHelperCfg && pPend && !pPend->IsFainting()) { ++pHelperCfg->dwMetricResurrectSuccess; }
				m_hPendingResurrectTarget = INVALID_HOBJECT;
				m_byResurrectAttemptCount = 0;
				m_dwSinceLastResurrectAttemptMs = 0;
				// Force a buff audit soon after revival
				m_dwSinceLastRebuffCheckMs = 0; // allow immediate coverage evaluation
			}
			else
			{
				m_dwSinceLastResurrectAttemptMs = UnsignedSafeIncrease<DWORD>(m_dwSinceLastResurrectAttemptMs, dwTickDiff);
				// Backoff schedule now configurable via helper config; defaults preserved if zero
				DWORD d1 = (pHelperCfg && pHelperCfg->dwResurrectRetryDelay1Ms > 0) ? pHelperCfg->dwResurrectRetryDelay1Ms : 1200;
				DWORD d2 = (pHelperCfg && pHelperCfg->dwResurrectRetryDelay2Ms > 0) ? pHelperCfg->dwResurrectRetryDelay2Ms : 2500;
				BYTE  maxAttempts = (pHelperCfg && pHelperCfg->byResurrectMaxAttempts > 0) ? pHelperCfg->byResurrectMaxAttempts : 3;
				if (m_byResurrectAttemptCount < maxAttempts)
				{
					DWORD waitMs = (m_byResurrectAttemptCount == 1) ? d1 : d2;
					if (m_dwSinceLastResurrectAttemptMs >= waitMs)
					{
						// Try another resurrect cast if skill free
						CSkillManagerBot* pSM = (CSkillManagerBot*)GetBot()->GetSkillManager();
						if (pSM && !pSM->IsSkillUseLock() && pHelperCfg && pHelperCfg->resurrectSkillTblidx != INVALID_TBLIDX)
						{
							CSkillCondition* pCond = pSM->FindSkillCondition(pHelperCfg->resurrectSkillTblidx);
							if (pCond && pCond->GetCanUseSkill())
							{
								if (GetBot()->GetCharStateID() == CHARSTATE_FOLLOWING || GetBot()->GetMoveFlag() != NTL_MOVE_FLAG_INVALID)
									GetBot()->SendCharStateStanding(true);
								CBotAiState* pCurState = GetBot()->GetBotController()->GetCurrentState();
								if (pCurState)
								{
									CBotAiAction_SkillUse* pSkillUse = new CBotAiAction_SkillUse(GetBot(), pCond->GetSkillConditionIdx());
									if (pCurState->AddSubControlQueue(pSkillUse, true))
									{
										GetBot()->SetTargetHandle(pPend->GetID());
										++m_byResurrectAttemptCount;
										m_dwSinceLastResurrectAttemptMs = 0;
										if (pHelperCfg->bVerboseLogs)
											ERR_LOG(LOG_BOTAI, "HelperNPC: resurrect retry attempt=%u target=%u", m_byResurrectAttemptCount, pPend->GetID());
									}
									else delete pSkillUse;
								}
							}
							else if (pHelperCfg && pHelperCfg->bVerboseLogs)
							{
								ERR_LOG(LOG_BOTAI, "HelperNPC: resurrect retry skipped (skill locked or unusable) target=%u", pPend->GetID());
							}
						}
					}
					// Stop after 3 attempts; clear to avoid indefinite tracking
					if (m_byResurrectAttemptCount >= 3)
					{
						m_hPendingResurrectTarget = INVALID_HOBJECT;
					}
				}
			}
		}

		if (isHelper)
		{
			m_dwSinceLastEngageMs = UnsignedSafeIncrease<DWORD>(m_dwSinceLastEngageMs, dwTickDiff);
			// Movement jitter suppression: if already following leader and within small radius, suppress frequent follow reasserts
			static const float HELPER_FOLLOW_HYSTERESIS = 1.2f; // meters
			static const DWORD HELPER_MIN_FOLLOW_REASSERT_MS = 1200; // ms
			if (GetBot()->GetCharStateID() == CHARSTATE_FOLLOWING && GetBot()->GetLinkPc() != INVALID_HOBJECT)
			{
				CCharacter* pLeaderChar = g_pObjectManager->GetChar(GetBot()->GetLinkPc());
				if (pLeaderChar && pLeaderChar->IsInitialized())
				{
					float dx = pLeaderChar->GetCurLoc().x - GetBot()->GetCurLoc().x;
					float dz = pLeaderChar->GetCurLoc().z - GetBot()->GetCurLoc().z;
					float dist2 = dx*dx + dz*dz;
					if (dist2 <= (HELPER_FOLLOW_HYSTERESIS * HELPER_FOLLOW_HYSTERESIS))
					{
						// Reset timer so later logic does not spam another follow packet
						m_dwSinceLastFollowReassertMs = 0;
					}
				}
			}
		}

		bool bPrioritizeHealing = false;
		if (GetBot()->GetLinkPc() != INVALID_HOBJECT && isHelper)
		{
			// Use per-helper config and only apply healing priority if helper is actually a healer (has resurrect skill)
			const sHELPER_NPC_CONFIG* pRoleCfg = pHelperCfg ? pHelperCfg : &GetHelperNpcManager()->GetConfig();
			CCharacter* pLinked = g_pObjectManager->GetChar(GetBot()->GetLinkPc());
			if (pLinked && pLinked->IsInitialized() && pRoleCfg && pRoleCfg->resurrectSkillTblidx != INVALID_TBLIDX)
			{
				if (pRoleCfg->wHealLpThresholdOverride == 0)
				{
					float fMissingPct = 100.f - pLinked->GetCurLpInPercent();
					bPrioritizeHealing = (fMissingPct >= (float)pRoleCfg->wHealPriorityMinMissingPercent);
				}
				else
				{
					bPrioritizeHealing = pLinked->ConsiderLPLow((float)pRoleCfg->wHealLpThresholdOverride);
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
		if (GetBot()->GetLinkPc() != INVALID_HOBJECT && isHelper)
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
					}
				}
			}
		}

		// Healer resurrection (500ms cadence)
		if (isHelper)
		{
			const sHELPER_NPC_CONFIG* pCfg = GetHelperNpcManager()->GetConfigForHelper(GetBot());
			if (pCfg && pCfg->resurrectSkillTblidx != INVALID_TBLIDX)
			{
				// Determine allowed max attempts from config (include first attempt). Fallback to 3.
				BYTE maxAttempts = (pCfg->byResurrectMaxAttempts > 0) ? pCfg->byResurrectMaxAttempts : 3;
				if (m_byResurrectAttemptCount >= maxAttempts)
				{
					// We've exhausted attempts for currently pending target; clear tracking
					m_hPendingResurrectTarget = INVALID_HOBJECT;
				}
				// Accumulate inside 1s gate; add 1000ms per second, but allow sub-second scans via separate fast path
				m_dwSinceLastResurrectScanMs = UnsignedSafeIncrease<DWORD>(m_dwSinceLastResurrectScanMs, 1000);
				if (m_dwSinceLastResurrectScanMs >= 500)
				{
					m_dwSinceLastResurrectScanMs = 0;
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
										// Stand / cancel follow first
										if (GetBot()->GetCharStateID() == CHARSTATE_FOLLOWING || GetBot()->GetMoveFlag() != NTL_MOVE_FLAG_INVALID)
										{
											GetBot()->SendCharStateStanding(true);
											if (GetHelperNpcManager()->GetConfig().bVerboseLogs)
												ERR_LOG(LOG_BOTAI, "HelperNPC: stand before resurrect target=%u", pMem->GetID());
										}
										CBotAiState* pCurState = GetBot()->GetBotController()->GetCurrentState();
										if (pCurState)
										{
											CBotAiAction_SkillUse* pSkillUse = new CBotAiAction_SkillUse(GetBot(), pCond->GetSkillConditionIdx());
											if (pCurState->AddSubControlQueue(pSkillUse, true))
											{
												GetBot()->SetTargetHandle(pMem->GetID());
												if (pCfg->bVerboseLogs)
													ERR_LOG(LOG_BOTAI, "HelperNPC: queued resurrect skill=%u on %u", pCfg->resurrectSkillTblidx, pMem->GetID());
											}
											else
											{
												delete pSkillUse;
												if (pCfg->bVerboseLogs)
													ERR_LOG(LOG_BOTAI, "HelperNPC: failed queue resurrect skill=%u", pCfg->resurrectSkillTblidx);
											}
										}
									}
								}
								break; // one attempt per scan
							}
						}
					}
				}
			}
		}

		// Follow watchdog
		if (GetBot()->GetLinkPc() != INVALID_HOBJECT && isHelper && GetHelperNpcManager()->GetConfig().bFollowLeader)
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
									m_dwSinceLastEngageMs = 0; // start stick window
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

							// Suppress reasserting follow if helper recently engaged an enemy (stick window)
							bool bWithinStick = (isHelper && m_dwSinceLastEngageMs < HELPER_MIN_ENGAGE_STICK_MS && GetBot()->GetTargetHandle() != INVALID_HOBJECT);
							if (!bWithinStick && bShouldReassert && m_dwSinceLastFollowReassertMs >= FOLLOW_REASSERT_COOLDOWN_MS)
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
											m_dwSinceLastEngageMs = 0; // start stick window
										}
									}
								}
							}

							// Tank aggro enforcement pulse (1s gate accumulation only)
							if (GetHelperNpcManager()->IsRegisteredHelper(GetBot()))
							{
								const sHELPER_NPC_CONFIG* pTankCfg = GetHelperNpcManager()->GetConfigForHelper(GetBot());
								if (pTankCfg && pTankCfg->bEnforceTankAggro)
								{
									m_dwSinceLastTankAggroPulseMs = UnsignedSafeIncrease<DWORD>(m_dwSinceLastTankAggroPulseMs, 1000);
									DWORD pulseMs = pTankCfg->dwTankAggroPulseMs > 0 ? pTankCfg->dwTankAggroPulseMs : 500;
									if (m_dwSinceLastTankAggroPulseMs >= pulseMs)
									{
										m_dwSinceLastTankAggroPulseMs = 0;
										// Simple strategy: if we have a current offensive target, add bonus aggro; else scan current aggro list and reinforce
										HOBJECT hCurTarget = GetBot()->GetTargetHandle();
										if (hCurTarget != INVALID_HOBJECT && hCurTarget != GetBot()->GetLinkPc())
										{
											CCharacter* pVict = g_pObjectManager->GetChar(hCurTarget);
											if (pVict && pVict->IsInitialized())
											{
												if (pVict->IsNPC() || pVict->IsMonster())
												{
													// Basic throttling: skip if victim already actively targeting tank AND tank already top threat
													bool bSkip = false;
													// Fallback: no GetHighestAggroChar API; use current target handle as top-threat proxy
													HOBJECT hTop = pVict->GetTargetHandle();
													if (hTop == GetBot()->GetID())
													{
														bSkip = true;
													}
													// Avoid spamming if victim in casting or stunned state (prevent AI reset)
													BYTE vs = pVict->GetCharStateID();
													if (vs == CHARSTATE_SKILL_AFFECTING || vs == CHARSTATE_CASTING)
														bSkip = true;
													if (!bSkip)
													{
														((CCharacter*)pVict)->ChangeAggro(GetBot()->GetID(), DBO_AGGRO_CHANGE_TYPE_INCREASE, pTankCfg->dwTankAggroBonus);
														if (pTankCfg->bVerboseLogs)
															ERR_LOG(LOG_BOTAI, "HelperNPC: tank aggro pulse target=%u bonus=%u", hCurTarget, pTankCfg->dwTankAggroBonus);
													}
													else if (pTankCfg->bVerboseLogs)
													{
														ERR_LOG(LOG_BOTAI, "HelperNPC: tank aggro pulse skipped target=%u (topThreat/casting)", hCurTarget);
													}
												}
											}
										}
										else
										{
											// If no explicit target, optionally iterate enemies already aggroed on party (lightweight: use scan)
											WORD wRange = pTankCfg->wAttackScanRange > 0 ? pTankCfg->wAttackScanRange : GetBot()->GetTbldat()->wSight_Range;
											HOBJECT hEnemy = GetBot()->ConsiderScanTarget(wRange);
											if (hEnemy != INVALID_HOBJECT)
											{
												CCharacter* pScan = g_pObjectManager->GetChar(hEnemy);
												if (pScan && (pScan->IsNPC() || pScan->IsMonster()))
												{
													HOBJECT hTop = pScan->GetTargetHandle();
													if (hTop != GetBot()->GetID())
													{
														pScan->ChangeAggro(GetBot()->GetID(), DBO_AGGRO_CHANGE_TYPE_INCREASE, pTankCfg->dwTankAggroBonus);
														if (pTankCfg->bVerboseLogs)
															ERR_LOG(LOG_BOTAI, "HelperNPC: tank aggro pulse (scan) enemy=%u bonus=%u", hEnemy, pTankCfg->dwTankAggroBonus);
													}
													else if (pTankCfg->bVerboseLogs)
													{
														ERR_LOG(LOG_BOTAI, "HelperNPC: tank aggro pulse (scan) skipped enemy=%u (already top)", hEnemy);
													}
												}
											}
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
		if ((GetBot()->HasNearbyPlayer(false) || GetBot()->GetLinkPc() != INVALID_HOBJECT) && isHelper)
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
							// Forced skill priority: attempt any forced skill first if flagged
							if (GetHelperNpcManager()->IsRegisteredHelper(GetBot()))
							{
								const sHELPER_NPC_CONFIG* pCfgPrio = GetHelperNpcManager()->GetConfigForHelper(GetBot());
								if (pCfgPrio && pCfgPrio->bPrioritizeForcedSkills && !pCfgPrio->vForcedSkills.empty())
								{
									for (TBLIDX fsId : pCfgPrio->vForcedSkills)
									{
										CSkillCondition* pForcedCond = pBotSkillManager->FindSkillCondition(fsId);
										if (pForcedCond && pForcedCond->GetCanUseSkill())
										{
											if (GetBot()->GetCharStateID() == CHARSTATE_FOLLOWING || GetBot()->GetMoveFlag() != NTL_MOVE_FLAG_INVALID)
												GetBot()->SendCharStateStanding(true);
											if (pCfgPrio->bVerboseLogs)
												ERR_LOG(LOG_BOTAI, "HelperNPC: prioritized forced skill tblidx=%u", fsId);
											CBotAiAction_SkillUse* pSkillUse = new CBotAiAction_SkillUse(GetBot(), pForcedCond->GetSkillConditionIdx());
											if (!pCurState->AddSubControlQueue(pSkillUse, true))
												delete pSkillUse;
											return m_status; // do not attempt generic skill this tick
										}
									}
								}
							}
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

	// From here onward we run helper-only fast paths (rebuff, proactive fast scan, fast skill cadence).
	// Non-helpers exit; helpers continue. Keep this narrow so helpers never bail unexpectedly.
	if (!isHelper)
		return m_status;
	// One-time trace that helper reached fast path (stored in reserved DWORD using map if needed; simple static guard here).
	static std::unordered_set<HOBJECT> s_loggedHelpers; // NOTE: acceptable minor static; if disallowed convert to bitset in manager.
	if (pHelperCfg && pHelperCfg->bVerboseLogs && s_loggedHelpers.find(GetBot()->GetID()) == s_loggedHelpers.end())
	{
		ERR_LOG(LOG_BOTAI, "HelperNPC: fast-path activated helper=%u", GetBot()->GetID());
		s_loggedHelpers.insert(GetBot()->GetID());
	}

	// Rebuff controller (full coverage audit with rotation)
	if (isHelper)
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

	// Fast-path proactive scan (outside 1s gate) - helpers only (redundant gating for safety)
	if (isHelper)
	{
		// If helper has an attack target but no aggro entries for some time, clear it so scans resume
		if (GetBot()->GetTargetHandle() != INVALID_HOBJECT && GetBot()->GetTargetListManager()->GetAggroCount() == 0)
		{
			m_dwChaseNoAggroMs = UnsignedSafeIncrease<DWORD>(m_dwChaseNoAggroMs, dwTickDiff);
			if (m_dwChaseNoAggroMs >= 2500)
			{
				GetBot()->SetTargetHandle(INVALID_HOBJECT);
				m_dwChaseNoAggroMs = 0;
				if (pHelperCfg && pHelperCfg->bVerboseLogs)
					ERR_LOG(LOG_BOTAI, "HelperNPC: cleared stale target (no aggro) to resume proactive scan");
			}
		}
		else
		{
			m_dwChaseNoAggroMs = 0;
		}
		do
		{
			if (GetBot()->GetLinkPc() == INVALID_HOBJECT)
				break;
			const sHELPER_NPC_CONFIG* pCfg = GetHelperNpcManager()->GetConfigForHelper(GetBot());
			if (!pCfg || !pCfg->bProactiveAutoAttack)
				break;
			if (GetBot()->GetTargetListManager()->GetAggroCount() != 0 || (GetBot()->GetTargetHandle() != INVALID_HOBJECT && GetBot()->GetTargetHandle() != GetBot()->GetLinkPc()))
			{
				m_dwSinceLastProactiveScanMs = 0; break;
			}
			m_dwSinceLastProactiveScanMs = UnsignedSafeIncrease<DWORD>(m_dwSinceLastProactiveScanMs, dwTickDiff);
			bool bHealPriority = false;
			{
				CCharacter* pLeader = g_pObjectManager->GetChar(GetBot()->GetLinkPc());
				if (pLeader && pLeader->IsInitialized())
				{
					if (pCfg->wHealLpThresholdOverride == 0)
					{
						float fMissingPct = 100.f - pLeader->GetCurLpInPercent();
						bHealPriority = (fMissingPct >= (float)pCfg->wHealPriorityMinMissingPercent);
					}
					else
					{
						bHealPriority = pLeader->ConsiderLPLow((float)pCfg->wHealLpThresholdOverride);
					}
				}
			}
			// Only block proactive attack for heal priority if this helper can actually heal/resurrect (healer role). SPEED or others without resurrect skill should still engage.
			if (bHealPriority)
			{
				if (!(pCfg->resurrectSkillTblidx == INVALID_TBLIDX))
				{
					// Healer-capable: respect heal priority and pause attack scan
					break;
				}
				else if (pCfg->bVerboseLogs)
				{
					ERR_LOG(LOG_BOTAI, "HelperNPC: ignoring heal priority (no resurrect skill) continuing proactive scan");
				}
			}
			DWORD dwCooldown = pCfg->dwAttackScanCooldownMs > 0 ? pCfg->dwAttackScanCooldownMs : 500;
			if (m_dwSinceLastProactiveScanMs < dwCooldown) break;
			m_dwSinceLastProactiveScanMs = 0;
			WORD wRange = pCfg->wAttackScanRange > 0 ? pCfg->wAttackScanRange : GetBot()->GetTbldat()->wSight_Range;
			HOBJECT hEnemy = GetBot()->ConsiderScanTarget(wRange);
			if (hEnemy == INVALID_HOBJECT) break;
			CCharacter* pVictim = g_pObjectManager->GetChar(hEnemy);
			if (!pVictim || !pVictim->IsInitialized()) break;
			if (!GetBot()->IsTargetAttackble(pVictim, wRange)) break;
			if (GetBot()->GetTargetHandle() != hEnemy) GetBot()->SetTargetHandle(hEnemy);
			GetBot()->ChangeAggro(hEnemy, DBO_AGGRO_CHANGE_TYPE_INCREASE, GetBot()->GetTbldat()->wBasic_Aggro_Point + 1);
			sVECTOR3 vDestLoc; pVictim->GetCurLoc().CopyTo(vDestLoc);
			GetBot()->SendCharStateFollowing(hEnemy, GetBot()->GetAttackRange(pVictim), DBO_MOVE_FOLLOW_AUTO_ATTACK, vDestLoc, true);
			if (pCfg->bVerboseLogs) ERR_LOG(LOG_BOTAI, "HelperNPC: fast-scan proactive engage enemy=%u range=%u", hEnemy, wRange);
			m_dwSinceLastEngageMs = 0; // start stick window
		} while (false);
	}

	// Fast-cadence skill queuing (outside 1s gate) to improve reaction speed (helpers only)
	if (isHelper && (GetBot()->HasNearbyPlayer(false) || GetBot()->GetLinkPc() != INVALID_HOBJECT))
	{
		// Fast resurrection scan (independent of 1s gate)
		if (GetHelperNpcManager()->IsRegisteredHelper(GetBot()))
		{
			const sHELPER_NPC_CONFIG* pCfg = GetHelperNpcManager()->GetConfigForHelper(GetBot());
			if (pCfg && pCfg->resurrectSkillTblidx != INVALID_TBLIDX)
			{
				// Use dwTickDiff via accumulating m_dwSinceLastResurrectScanMs (already exists) here as true fast path
				m_dwSinceLastResurrectScanMs = UnsignedSafeIncrease<DWORD>(m_dwSinceLastResurrectScanMs, dwTickDiff);
				DWORD cadence = 500; // 500ms target
				if (m_dwSinceLastResurrectScanMs >= cadence)
				{
					m_dwSinceLastResurrectScanMs = 0;
					CPlayer* pLeader = (CPlayer*)g_pObjectManager->GetChar(GetBot()->GetLinkPc());
					if (pLeader && pLeader->IsInitialized())
					{
						auto considerMember = [&](CPlayer* pMem) -> bool {
							if (!pMem || !pMem->IsInitialized() || !pMem->IsFainting()) return false;
							if (pMem->GetCurWorld() != GetBot()->GetCurWorld()) return false;
							CSkillManagerBot* pSM = (CSkillManagerBot*)GetBot()->GetSkillManager(); if (!pSM) return false;
							CSkillCondition* pCond = pSM->FindSkillCondition(pCfg->resurrectSkillTblidx);
							if (!pCond)
							{
								if (pCfg->bVerboseLogs) ERR_LOG(LOG_BOTAI, "HelperNPC: resurrect skill condition missing skill=%u", pCfg->resurrectSkillTblidx);
								return false;
							}
							if (pSM->IsSkillUseLock()) return false; // busy
							if (!pCond->GetCanUseSkill()) {
								if (pCfg->bVerboseLogs) ERR_LOG(LOG_BOTAI, "HelperNPC: resurrect skill not usable (cd/resources) skill=%u", pCfg->resurrectSkillTblidx);
								return false;
							}
							// Basic range diagnostics (approx): use bot to target distance; if too far beyond 35m skip with log
							float fDist = GetBot()->GetDistance(pMem->GetCurLoc());
							if (fDist > 40.0f)
							{
								if (pCfg->bVerboseLogs) ERR_LOG(LOG_BOTAI, "HelperNPC: resurrect target out of hard range dist=%.1f target=%u", fDist, pMem->GetID());
								return false;
							}
							if (GetBot()->GetCharStateID() == CHARSTATE_FOLLOWING || GetBot()->GetMoveFlag() != NTL_MOVE_FLAG_INVALID)
								GetBot()->SendCharStateStanding(true);
							CBotAiState* pCurState = GetBot()->GetBotController()->GetCurrentState(); if (!pCurState) return false;
							CBotAiAction_SkillUse* pSkillUse = new CBotAiAction_SkillUse(GetBot(), pCond->GetSkillConditionIdx());
							if (pCurState->AddSubControlQueue(pSkillUse, true))
							{
								GetBot()->SetTargetHandle(pMem->GetID());
								// Initialize retry tracking
								m_hPendingResurrectTarget = pMem->GetID();
								m_byResurrectAttemptCount = 1; // first attempt
								m_dwSinceLastResurrectAttemptMs = 0;
								if (pCfg->bVerboseLogs) ERR_LOG(LOG_BOTAI, "HelperNPC: fast resurrect queued skill=%u target=%u dist=%.1f", pCfg->resurrectSkillTblidx, pMem->GetID(), fDist);
								if (pCfg) { ++pCfg->dwMetricResurrectAttempts; }
								return true;
							}
							else if (pCfg->bVerboseLogs)
							{
								ERR_LOG(LOG_BOTAI, "HelperNPC: failed to queue resurrect skill=%u target=%u dist=%.1f", pCfg->resurrectSkillTblidx, pMem->GetID(), fDist);
							}
							delete pSkillUse; return false;
							};
						// Solo case (no party) revive leader if fainted (edge scenario)
						if (!pLeader->GetParty())
						{
							considerMember(pLeader);
						}
						else
						{
							CParty* party = pLeader->GetParty();
							BYTE cnt = party->GetPartyMemberCount();
							for (BYTE i = 0; i < cnt; ++i)
							{
								const sPARTY_MEMBER_INFO& mi = party->GetMemberInfo(i);
								CPlayer* pMem = g_pObjectManager->GetPC(mi.hHandle);
								if (considerMember(pMem)) break;
							}
						}
					}
				}
			}
		}
		if (isHelper && !GetBot()->GetStateManager()->IsCharCondition(CHARCOND_CONFUSED))
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
