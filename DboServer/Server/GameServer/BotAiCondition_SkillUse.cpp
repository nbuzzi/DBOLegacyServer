#include "stdafx.h"
#include "BotAiCondition_SkillUse.h"
#include "BotAiState.h"
#include "BotAiAction_SkillUse.h"
#include "SkillCondition.h"
// Follow watchdog includes
#include "HelperNpcManager.h"
#include "ObjectManager.h"
#include "CPlayer.h"
#include "Party.h"
#include "Buff.h"
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include "SafeObjectResolve.h"
#include "TableContainer.h"
#include <float.h>
#include "TableContainerManager.h"

// No forward declaration needed; included Buff.h above

static HOBJECT ResolvePartyAssistTarget(CPlayer* pLeader, CNpc* pBot, bool preferLeaderTarget)
{
	if (!pLeader || !pBot) return INVALID_HOBJECT;

	auto isValidVictim = [&](HOBJECT hTarget) -> bool {
		if (hTarget == INVALID_HOBJECT) return false;
		CCharacter* pVictim = g_pObjectManager->GetChar(hTarget);
		if (!pVictim || !pVictim->IsInitialized()) return false;
		WORD wRange = pBot->GetTbldat() ? pBot->GetTbldat()->wSight_Range : 20;
		return pBot->IsTargetAttackble(pVictim, wRange);
		};

	if (preferLeaderTarget)
	{
		HOBJECT hLead = pLeader->GetTargetHandle();
		if (isValidVictim(hLead)) return hLead;
	}

	CParty* pParty = pLeader->GetParty();
	if (!pParty || pParty->GetPartyMemberCount() == 0)
		return INVALID_HOBJECT;

	HOBJECT hBest = INVALID_HOBJECT;
	float fBestDist = FLT_MAX;
	BYTE cnt = pParty->GetPartyMemberCount();
	for (BYTE i = 0; i < cnt; ++i)
	{
		const sPARTY_MEMBER_INFO& mi = pParty->GetMemberInfo(i);
		CPlayer* pMem = g_pObjectManager->GetPC(mi.hHandle);
		if (!pMem || !pMem->IsInitialized() || pMem->IsFainting()) continue;
		if (pMem->GetCurWorld() != pBot->GetCurWorld()) continue;
		HOBJECT hVictim = pMem->GetTargetHandle();
		if (!isValidVictim(hVictim)) continue;
		CCharacter* pVictim = g_pObjectManager->GetChar(hVictim);
		float fDist = pBot->GetDistance(pVictim->GetCurLoc());
		if (fDist < fBestDist)
		{
			fBestDist = fDist;
			hBest = hVictim;
		}
	}

	if (!preferLeaderTarget && hBest == INVALID_HOBJECT)
	{
		HOBJECT hLead = pLeader->GetTargetHandle();
		if (isValidVictim(hLead)) return hLead;
	}

	return hBest;
}

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
	// Security guards: ensure bot & core components exist before heavy logic
	CGameServer* app = (CGameServer*)g_pApp;
	CNpc* pBot = GetBot();
	if (!pBot || !pBot->IsInitialized())
	{
		if (app && app->m_config.m_bAIVerbose)
			ERR_LOG(LOG_SYSTEM, "AI_GUARD SkillUse: invalid bot pointer or uninitialized (handle=%u)", SAFE_ID(pBot));
		return m_status;
	}
	if (!pBot->GetStateManager())
	{
		if (app && app->m_config.m_bAIVerbose)
			ERR_LOG(LOG_SYSTEM, "AI_GUARD SkillUse: null StateManager tblidx=%u", pBot->GetTblidx());
		return m_status;
	}
	if (!pBot->GetBotController())
	{
		if (app && app->m_config.m_bAIVerbose)
			ERR_LOG(LOG_SYSTEM, "AI_GUARD SkillUse: null BotController tblidx=%u", pBot->GetTblidx());
		return m_status;
	}

	const bool isHelper = GetHelperNpcManager()->IsRegisteredHelper(GetBot());
	const bool isActiveLinkedHelper = GetHelperNpcManager()->IsActiveLinkedHelper(GetBot());
	const sHELPER_NPC_CONFIG* pHelperCfg = isHelper ? GetHelperNpcManager()->GetConfigForHelper(GetBot()) : nullptr;

	// Advance timers for responsiveness
	m_dwSinceLastProactiveScanMs = UnsignedSafeIncrease<DWORD>(m_dwSinceLastProactiveScanMs, dwTickDiff);
	m_dwSinceLastSkillTryMs = UnsignedSafeIncrease<DWORD>(m_dwSinceLastSkillTryMs, dwTickDiff);
	m_dwSinceLastFollowReassertMs = UnsignedSafeIncrease<DWORD>(m_dwSinceLastFollowReassertMs, dwTickDiff);

	// Track time since last proactive engage/follow switch to prevent rapid oscillation (buffer jitter fix)
	static const DWORD HELPER_MIN_ENGAGE_STICK_MS = 500; // stay committed at least 0.5s
	m_dwTime = UnsignedSafeIncrease<DWORD>(m_dwTime, dwTickDiff);

	if (m_dwTime >= 1000)
	{
		m_dwTime = 0;

		if (!isHelper)
		{
			// Legacy, unmodified logic for non-helper NPCs/monsters (restored for compatibility)
			if (GetBot()->HasNearbyPlayer(false))
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

			return m_status; // early exit for non-helper path
		}

		// Pending resurrect retry housekeeping (runs every tick pre logic)
		if (isActiveLinkedHelper && m_hPendingResurrectTarget != INVALID_HOBJECT)
		{
			CPlayer* pPend = (CPlayer*)g_pObjectManager->GetChar(m_hPendingResurrectTarget);
			bool bClear = false;

			// Check if player was successfully resurrected
			if (!pPend || !pPend->IsInitialized()) {
				bClear = true;
			}
			else if (!pPend->IsFainting()) {
				// Player is alive - but give extra time for client sync before clearing
				if (m_dwSinceLastResurrectAttemptMs >= 2000) { // 2 second delay after resurrection
					bClear = true;
				}
				else {
					// Keep tracking for a bit longer to ensure stable resurrection
					if (pHelperCfg && pHelperCfg->bVerboseLogs)
						ERR_LOG(LOG_BOTAI, "HelperNPC: resurrection detected, waiting for client sync");
				}
			}
			else if (pPend->GetCurWorld() != GetBot()->GetCurWorld()) {
				bClear = true;
			}

			if (bClear)
			{
				if (pHelperCfg && pHelperCfg->bVerboseLogs && pPend)
					ERR_LOG(LOG_BOTAI, "HelperNPC: resurrect target cleared");
				if (pHelperCfg && pPend && !pPend->IsFainting()) {
					// metrics are optional; avoid mutating const config
					if (pHelperCfg->bVerboseLogs)
						ERR_LOG(LOG_BOTAI, "HelperNPC: successful resurrection");
				}
				m_hPendingResurrectTarget = INVALID_HOBJECT;
				m_byResurrectAttemptCount = 0;
				m_dwSinceLastResurrectAttemptMs = 0;
				// Force a buff audit soon after revival
				m_dwSinceLastRebuffCheckMs = 0; // allow immediate coverage evaluation
				// Unlock any stale skill lock and stand to avoid idle stuck
				if (CSkillManagerBot* pSM = (CSkillManagerBot*)GetBot()->GetSkillManager()) pSM->SetSkillUse_Unlock();
				GetBot()->SendCharStateStanding(true);
				// Clear attack target to refocus on heal/buff
				if (GetBot()->GetTargetHandle() != INVALID_HOBJECT)
					GetBot()->SetTargetHandle(INVALID_HOBJECT);
				// Trigger immediate heal scan on next fast cadence
				DWORD healCad = (pHelperCfg && pHelperCfg->dwHealScanCooldownMs > 0) ? pHelperCfg->dwHealScanCooldownMs : 150;
				m_dwSinceLastHealScanMs = healCad;
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
										GetBot()->SetTargetHandle(pPend ? pPend->GetID() : INVALID_HOBJECT);
										++m_byResurrectAttemptCount;
										m_dwSinceLastResurrectAttemptMs = 0;
										if (pHelperCfg->bVerboseLogs)
											ERR_LOG(LOG_BOTAI, "HelperNPC: resurrect retry attempt=%u target=%u", m_byResurrectAttemptCount, SAFE_ID(pPend));
									}
									else delete pSkillUse;
								}
							}
							else if (pHelperCfg && pHelperCfg->bVerboseLogs)
							{
								ERR_LOG(LOG_BOTAI, "HelperNPC: resurrect retry skipped (skill locked or unusable) target=%u", SAFE_ID(pPend));
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
			static const float HELPER_FOLLOW_HYSTERESIS = 2.2f; // meters
			static const DWORD HELPER_MIN_FOLLOW_REASSERT_MS = 1200; // ms
			if (GetBot()->GetCharStateID() == CHARSTATE_FOLLOWING && GetBot()->GetLinkPc() != INVALID_HOBJECT)
			{
				CCharacter* pLeaderChar = g_pObjectManager->GetChar(GetBot()->GetLinkPc());
				if (pLeaderChar && pLeaderChar->IsInitialized())
				{
					float dx = pLeaderChar->GetCurLoc().x - GetBot()->GetCurLoc().x;
					float dz = pLeaderChar->GetCurLoc().z - GetBot()->GetCurLoc().z;
					float dist2 = dx * dx + dz * dz;
					if (dist2 <= (HELPER_FOLLOW_HYSTERESIS * HELPER_FOLLOW_HYSTERESIS))
					{
						// Reset timer so later logic does not spam another follow packet
						m_dwSinceLastFollowReassertMs = 0;
					}
				}
			}
		}

		bool bPrioritizeHealing = false;
		if (isActiveLinkedHelper)
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

			// Extend healing priority to any injured party member in same world
			if (!bPrioritizeHealing && pRoleCfg)
			{
				CPlayer* pLeader = (CPlayer*)g_pObjectManager->GetChar(GetBot()->GetLinkPc());
				if (pLeader && pLeader->IsInitialized() && pLeader->GetParty())
				{
					BYTE cnt = pLeader->GetParty()->GetPartyMemberCount();
					for (BYTE i = 0; i < cnt; ++i)
					{
						const sPARTY_MEMBER_INFO& mi = pLeader->GetParty()->GetMemberInfo(i);
						CPlayer* pMem = g_pObjectManager->GetPC(mi.hHandle);
						if (!pMem || !pMem->IsInitialized() || pMem->IsFainting()) continue;
						if (pMem->GetCurWorld() != GetBot()->GetCurWorld()) continue;
						bool low = false;
						if (pRoleCfg->wHealLpThresholdOverride == 0)
						{
							float miss = 100.f - pMem->GetCurLpInPercent();
							low = (miss >= (float)pRoleCfg->wHealPriorityMinMissingPercent);
						}
						else
						{
							low = pMem->ConsiderLPLow((float)pRoleCfg->wHealLpThresholdOverride);
						}
						if (low) { bPrioritizeHealing = true; break; }
					}
				}
			}

			// EP top-up and stale lock clear
			const WORD wEpLowThreshold = 20;
			if (GetBot()->GetCurEP() < wEpLowThreshold)
			{
				GetBot()->SetCurEP(GetBot()->GetMaxEP());
				if (GetHelperNpcManager()->GetConfig().bVerboseLogs)
					ERR_LOG(LOG_BOTAI, "HelperNPC: EP auto-refill to max (%u) for helper %u", GetBot()->GetMaxEP(), SAFE_ID(GetBot()));
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

		// Party-aware follow behavior: periodically follow a party member (not only leader)
		// Prefers the most injured non-fainting member in same world; falls back to random party member
		if (isActiveLinkedHelper)
		{
			const sHELPER_NPC_CONFIG* pCfg = isHelper
				? GetHelperNpcManager()->GetConfigForHelper(GetBot())
				: &GetHelperNpcManager()->GetConfig();
			// Cooldown and only when idle (no aggro) to avoid interrupting combat
			m_dwSinceRandomFollowMs = UnsignedSafeIncrease<DWORD>(m_dwSinceRandomFollowMs, dwTickDiff);
			if (m_dwSinceRandomFollowMs >= 2000 && GetBot()->GetTargetListManager() && GetBot()->GetTargetListManager()->GetAggroCount() == 0)
			{
				m_dwSinceRandomFollowMs = 0;
				CPlayer* pLeader = (CPlayer*)g_pObjectManager->GetChar(GetBot()->GetLinkPc());
				if (pLeader && pLeader->IsInitialized())
				{
					CPlayer* pFollow = pLeader; // default
					float bestMissing = -1.0f;
					if (pLeader->GetParty() && pLeader->GetParty()->GetPartyMemberCount() > 0)
					{
						BYTE cnt = pLeader->GetParty()->GetPartyMemberCount();
						// First pass: choose most injured member in same world (non-fainting)
						for (BYTE i = 0; i < cnt; ++i)
						{
							const sPARTY_MEMBER_INFO& mi = pLeader->GetParty()->GetMemberInfo(i);
							CPlayer* pCand = g_pObjectManager->GetPC(mi.hHandle);
							if (!pCand || !pCand->IsInitialized() || pCand->IsFainting()) continue;
							if (pCand->GetCurWorld() != GetBot()->GetCurWorld()) continue;
							float missing = 100.0f - pCand->GetCurLpInPercent();
							if (missing > bestMissing)
							{
								bestMissing = missing;
								pFollow = pCand;
							}
						}
						// If nobody is injured, pick a random online party member in same world
						if (bestMissing <= 0.0f)
						{
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
					}
					// Reassert follow if we are not already following this target closely
					sVECTOR3 vLoc; pFollow->GetCurLoc().CopyTo(vLoc);
					GetBot()->SendCharStateFollowing(pFollow->GetID(), 1.5f, DBO_MOVE_FOLLOW_FRIENDLY, vLoc, true);
				}
			}
		}

		// Healer resurrection (configurable cadence)
		// Removed duplicate 1s-gated resurrect scan to avoid conflicting cadence.

		// Follow watchdog
		if (isActiveLinkedHelper && GetHelperNpcManager()->GetConfig().bFollowLeader)
		{
			if (GetBot()->GetTargetListManager() && GetBot()->GetTargetListManager()->GetAggroCount() == 0)
			{
				HOBJECT hLink = GetBot()->GetLinkPc();
				CPlayer* pLeader = (CPlayer*)g_pObjectManager->GetChar(hLink);
				if (pLeader && pLeader->IsInitialized())
				{
					CWorld* pBotWorld = GetBot()->GetCurWorld();
					CWorld* pLeaderWorld = pLeader->GetCurWorld();
					if (!pBotWorld || !pLeaderWorld)
					{
						// Either side is tearing down; avoid further actions this tick
						return m_status;
					}
					const bool bSameWorld = (pLeaderWorld == pBotWorld);
					const bool bTooFar = !GetBot()->IsInRange(pLeader, (float)NTL_MAX_RADIUS_OF_VISIBLE_AREA);
					if (!bSameWorld || bTooFar)
					{
						if (!bPrioritizeHealing)
						{
							// Respect per-role proactive attack setting when deciding to engage while far from leader
							const sHELPER_NPC_CONFIG* pCfgAssist = pHelperCfg ? pHelperCfg : &GetHelperNpcManager()->GetConfig();
							if (pCfgAssist && pCfgAssist->bProactiveAutoAttack)
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
						}

						const sHELPER_NPC_CONFIG& cfg = GetHelperNpcManager()->GetConfig();
						WORD wHealThr = cfg.wHealLpThresholdOverride > 0 ? cfg.wHealLpThresholdOverride : 35;
						if (pLeader->GetCurLpInPercent() <= (float)wHealThr)
						{
							CWorld* pWorld = pLeaderWorld;
							if (pWorld && pWorld == pLeader->GetCurWorld())
							{
								CNtlVector vLoc = pLeader->GetCurLoc();
								CNtlVector vDir = pLeader->GetCurDir();
								if (GetBot()->GetBotController()->ChangeControlState_Teleporting(pWorld->GetID(), pWorld->GetIdx(), vLoc, vDir))
								{
									if (GetHelperNpcManager()->GetConfig().bVerboseLogs)
										ERR_LOG(LOG_BOTAI, "HelperNPC: teleport-resync (leader low HP %.1f%%) to world=%u loc=(%.2f,%.2f,%.2f)", pLeader->GetCurLpInPercent(), SAFE_ID(pWorld), vLoc.x, vLoc.y, vLoc.z);
									// Post-teleport recovery
									if (CSkillManagerBot* pSM = (CSkillManagerBot*)GetBot()->GetSkillManager()) pSM->SetSkillUse_Unlock();
									GetBot()->SendCharStateStanding(true);
									// Reassert follow if configured
									if (GetHelperNpcManager()->GetConfig().bFollowLeader)
									{
										sVECTOR3 vFollow; pLeader->GetCurLoc().CopyTo(vFollow);
										GetBot()->SendCharStateFollowing(pLeader->GetID(), 1.5f, DBO_MOVE_FOLLOW_FRIENDLY, vFollow, true);
									}
									// Nudge assist across party: prefer leader when configured, otherwise any member
									{
										// Use role-specific AssistLeaderTarget if available
										bool preferLeader = pHelperCfg ? pHelperCfg->bAssistLeaderTarget : GetHelperNpcManager()->GetConfig().bAssistLeaderTarget;
										HOBJECT hVictim = ResolvePartyAssistTarget(pLeader, GetBot(), preferLeader);
										if (hVictim != INVALID_HOBJECT && GetBot()->GetTargetHandle() != hVictim)
											GetBot()->SetTargetHandle(hVictim);
									}
									m_dwSinceLastSkillTryMs = 0;
								}
								m_dwOutOfRangeTimeMs = 0;
								return m_status;
							}
						}

						m_dwOutOfRangeTimeMs = UnsignedSafeIncrease<DWORD>(m_dwOutOfRangeTimeMs, 1000);
						if (m_dwOutOfRangeTimeMs >= 1000)
						{
							CWorld* pWorld = pLeaderWorld;
							if (pWorld && pWorld == pLeader->GetCurWorld())
							{
								CNtlVector vLoc = pLeader->GetCurLoc();
								CNtlVector vDir = pLeader->GetCurDir();
								if (GetBot()->GetBotController()->ChangeControlState_Teleporting(pWorld->GetID(), pWorld->GetIdx(), vLoc, vDir))
								{
									if (GetHelperNpcManager()->GetConfig().bVerboseLogs)
										ERR_LOG(LOG_BOTAI, "HelperNPC: teleport-resync to leader h=%u world=%u loc=(%.2f,%.2f,%.2f)", SAFE_ID(pLeader), SAFE_ID(pWorld), vLoc.x, vLoc.y, vLoc.z);
									if (CSkillManagerBot* pSM = (CSkillManagerBot*)GetBot()->GetSkillManager()) pSM->SetSkillUse_Unlock();
									GetBot()->SendCharStateStanding(true);
									if (GetHelperNpcManager()->GetConfig().bFollowLeader)
									{
										sVECTOR3 vFollow; pLeader->GetCurLoc().CopyTo(vFollow);
										GetBot()->SendCharStateFollowing(pLeader->GetID(), 1.5f, DBO_MOVE_FOLLOW_FRIENDLY, vFollow, true);
									}
									{
										bool preferLeader = GetHelperNpcManager()->GetConfig().bAssistLeaderTarget;
										HOBJECT hVictim = ResolvePartyAssistTarget(pLeader, GetBot(), preferLeader);
										if (hVictim != INVALID_HOBJECT && GetBot()->GetTargetHandle() != hVictim)
											GetBot()->SetTargetHandle(hVictim);
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
							if (fDist > 25.0f && GetBot()->GetTargetListManager() && GetBot()->GetTargetListManager()->GetAggroCount() == 0)
							{
								CWorld* pWorld = pLeaderWorld;
								if (pWorld && pWorld == pLeader->GetCurWorld())
								{
									CNtlVector vLoc = pLeader->GetCurLoc();
									CNtlVector vDir = pLeader->GetCurDir();
									if (GetBot()->GetBotController()->ChangeControlState_Teleporting(pWorld->GetID(), pWorld->GetIdx(), vLoc, vDir))
									{
										if (GetHelperNpcManager()->GetConfig().bVerboseLogs)
											ERR_LOG(LOG_BOTAI, "HelperNPC: teleport-resync (far %.2fm) to leader %u", fDist, SAFE_ID(pLeader));
										if (CSkillManagerBot* pSM = (CSkillManagerBot*)GetBot()->GetSkillManager()) pSM->SetSkillUse_Unlock();
										GetBot()->SendCharStateStanding(true);
										if (GetHelperNpcManager()->GetConfig().bFollowLeader)
										{
											sVECTOR3 vFollow; pLeader->GetCurLoc().CopyTo(vFollow);
											GetBot()->SendCharStateFollowing(pLeader->GetID(), 1.5f, DBO_MOVE_FOLLOW_FRIENDLY, vFollow, true);
										}
										{
											bool preferLeader = GetHelperNpcManager()->GetConfig().bAssistLeaderTarget;
											HOBJECT hVictim = ResolvePartyAssistTarget(pLeader, GetBot(), preferLeader);
											if (hVictim != INVALID_HOBJECT && GetBot()->GetTargetHandle() != hVictim)
												GetBot()->SetTargetHandle(hVictim);
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
							else if (m_dwNoFollowProgressMs >= 2000)
							{
								// Moving but stuck for >= 2s -> reassert
								bShouldReassert = true;
							}

							// If we've been stuck making no progress for a while, do a local teleport-resync even if in range
							if (m_dwNoFollowProgressMs >= 4000 && fDist > 20.0f)
							{
								CWorld* pWorld = pLeaderWorld;
								if (pWorld && pWorld == pLeader->GetCurWorld())
								{
									CNtlVector vLoc = pLeader->GetCurLoc();
									CNtlVector vDir = pLeader->GetCurDir();
									if (GetBot()->GetBotController()->ChangeControlState_Teleporting(pWorld->GetID(), pWorld->GetIdx(), vLoc, vDir))
									{
										if (GetHelperNpcManager()->GetConfig().bVerboseLogs)
											ERR_LOG(LOG_BOTAI, "HelperNPC: teleport-resync (stuck %.1fs, dist=%.2f) leader h=%u world=%u", m_dwNoFollowProgressMs / 1000.0f, fDist, SAFE_ID(pLeader), SAFE_ID(pWorld));
										if (CSkillManagerBot* pSM = (CSkillManagerBot*)GetBot()->GetSkillManager()) pSM->SetSkillUse_Unlock();
										GetBot()->SendCharStateStanding(true);
										if (GetHelperNpcManager()->GetConfig().bFollowLeader)
										{
											sVECTOR3 vFollow; pLeader->GetCurLoc().CopyTo(vFollow);
											GetBot()->SendCharStateFollowing(pLeader->GetID(), 1.5f, DBO_MOVE_FOLLOW_FRIENDLY, vFollow, true);
										}
										{
											bool preferLeader = GetHelperNpcManager()->GetConfig().bAssistLeaderTarget;
											HOBJECT hVictim = ResolvePartyAssistTarget(pLeader, GetBot(), preferLeader);
											if (hVictim != INVALID_HOBJECT && GetBot()->GetTargetHandle() != hVictim)
												GetBot()->SetTargetHandle(hVictim);
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
									if (!bFollowingLeader || byMoveFlag == NTL_MOVE_FLAG_INVALID || m_dwNoFollowProgressMs >= 2000)
									{
										if (GetHelperNpcManager()->GetConfig().bVerboseLogs)
											ERR_LOG(LOG_BOTAI, "HelperNPC: reassert follow to leader %u (state=%u, dist=%.2f)", SAFE_ID(pLeader), st, fDist);
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
						const sHELPER_NPC_CONFIG* pCfgAssist = isHelper
							? GetHelperNpcManager()->GetConfigForHelper(GetBot())
							: &GetHelperNpcManager()->GetConfig();
						if (!bPrioritizeHealing && pCfgAssist)
						{
							bool preferLeader = pCfgAssist->bAssistLeaderTarget;
							HOBJECT hVictim = ResolvePartyAssistTarget(pLeader, GetBot(), preferLeader);
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
									// Otherwise assist the party-resolved current target
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
						const sHELPER_NPC_CONFIG* pCfg = isHelper
							? GetHelperNpcManager()->GetConfigForHelper(GetBot())
							: &GetHelperNpcManager()->GetConfig();
						if (pCfg && pCfg->bProactiveAutoAttack)
						{
							if (GetBot()->GetTargetListManager() && GetBot()->GetTargetListManager()->GetAggroCount() == 0 && GetBot()->GetTargetHandle() == INVALID_HOBJECT)
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

						}
					}
				}
			}
		}

		// Decoupled tank aggro enforcement pulse (runs regardless of proactive attack)
		if (isHelper)
		{
			const sHELPER_NPC_CONFIG* pTankCfg = GetHelperNpcManager()->GetConfigForHelper(GetBot());
			if (pTankCfg && pTankCfg->bEnforceTankAggro)
			{
				m_dwSinceLastTankAggroPulseMs = UnsignedSafeIncrease<DWORD>(m_dwSinceLastTankAggroPulseMs, 1000);
				DWORD pulseMs = pTankCfg->dwTankAggroPulseMs > 0 ? pTankCfg->dwTankAggroPulseMs : 500;
				if (m_dwSinceLastTankAggroPulseMs >= pulseMs)
				{
					m_dwSinceLastTankAggroPulseMs = 0;
					HOBJECT hCurTarget = GetBot()->GetTargetHandle();
					if (hCurTarget != INVALID_HOBJECT && hCurTarget != GetBot()->GetLinkPc())
					{
						CCharacter* pVict = g_pObjectManager->GetChar(hCurTarget);
						if (pVict && pVict->IsInitialized() && (pVict->IsNPC() || pVict->IsMonster()))
						{
							bool bSkip = false;
							HOBJECT hTop = pVict->GetTargetHandle();
							if (hTop == GetBot()->GetID()) bSkip = true;
							BYTE vs = pVict->GetCharStateID();
							if (vs == CHARSTATE_SKILL_AFFECTING || vs == CHARSTATE_CASTING) bSkip = true;
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
					else
					{
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

		// Rebuff controller (full coverage audit with rotation)
		if (isHelper)
		{
			const sHELPER_NPC_CONFIG* pCfg = GetHelperNpcManager()->GetConfigForHelper(GetBot());
			if (pCfg && pCfg->dwRebuffCooldownMs > 0 && (!pCfg->vBuffSkills.empty() || !pCfg->vForcedSkills.empty()))
			{
				m_dwSinceLastRebuffCheckMs = UnsignedSafeIncrease<DWORD>(m_dwSinceLastRebuffCheckMs, dwTickDiff);
				if (m_dwSinceLastRebuffCheckMs >= pCfg->dwRebuffCooldownMs)
				{
					m_dwSinceLastRebuffCheckMs = 0;
					CPlayer* pLeader = (CPlayer*)g_pObjectManager->GetChar(GetBot()->GetLinkPc());
					if (pLeader && pLeader->IsInitialized())
					{
						CSkillManagerBot* pSM = (CSkillManagerBot*)GetBot()->GetSkillManager();
						if (pSM)
						{
							BYTE buffsQueuedThisAudit = 0;
							std::unordered_map<HOBJECT, BYTE> perTargetCount;

							// Build party member list (leader + members in same world)
							std::vector<CPlayer*> partyMembers;
							partyMembers.push_back(pLeader);
							if (pLeader->GetParty())
							{
								BYTE cnt = pLeader->GetParty()->GetPartyMemberCount();
								for (BYTE i = 0; i < cnt; ++i)
								{
									const sPARTY_MEMBER_INFO& mi = pLeader->GetParty()->GetMemberInfo(i);
									CPlayer* pMem = g_pObjectManager->GetPC(mi.hHandle);
									if (pMem && pMem != pLeader && pMem->IsInitialized() && pMem->GetCurWorld() == GetBot()->GetCurWorld())
										partyMembers.push_back(pMem);
								}
							}

							// Build buff candidate list = BuffSkills U ForcedSkills (forced often include speed/buff skills)
							std::vector<TBLIDX> buffCandidates;
							buffCandidates.reserve(pCfg->vBuffSkills.size() + pCfg->vForcedSkills.size());
							for (TBLIDX id : pCfg->vBuffSkills) buffCandidates.push_back(id);
							for (TBLIDX id : pCfg->vForcedSkills) buffCandidates.push_back(id);
							// Deduplicate while preserving order of first appearance
							{
								std::unordered_set<TBLIDX> seen;
								std::vector<TBLIDX> uniq;
								uniq.reserve(buffCandidates.size());
								for (TBLIDX id : buffCandidates) { if (id != INVALID_TBLIDX && seen.insert(id).second) uniq.push_back(id); }
								buffCandidates.swap(uniq);
							}

							// If configured for party-wide, attempt party-capable skills as single casts
							if (pCfg->bBuffPartyWide)
							{
								static std::unordered_map<HOBJECT, size_t> s_lastBuffIdx;
								const HOBJECT hid = SAFE_ID(GetBot());
								size_t start = 0; auto itI = s_lastBuffIdx.find(hid); if (itI != s_lastBuffIdx.end()) start = itI->second % (buffCandidates.empty() ? 1 : buffCandidates.size());
								const size_t nbc = buffCandidates.size();
								for (size_t step = 0; step < nbc; ++step)
								{
									TBLIDX buffId = buffCandidates[(start + step) % nbc];
									if (buffsQueuedThisAudit >= (pCfg->byMaxBuffsPerAudit ? pCfg->byMaxBuffsPerAudit : 1)) break;
									sSKILL_TBLDAT* pTb = (sSKILL_TBLDAT*)g_pTableContainer->GetSkillTable()->FindData(buffId);
									bool partyCapable = false;
									if (pTb)
									{
										BYTE at = pTb->byApply_Target;
										partyCapable = (at == DBO_SKILL_APPLY_TARGET_PARTY || at == DBO_SKILL_APPLY_TARGET_ALLIANCE || at == DBO_SKILL_APPLY_TARGET_MOB_PARTY || at == DBO_SKILL_APPLY_TARGET_ANY || at == DBO_SKILL_APPLY_TARGET_SELF);
									}
									if (!partyCapable) continue;
									bool needed = false;
									for (CPlayer* pM : partyMembers)
									{
										if (!pM || !pM->IsInitialized() || pM->IsFainting() || pM->GetCurWorld() != GetBot()->GetCurWorld()) continue;
										CBuff* pExisting = pM->GetBuffManager()->FindBuff(buffId);
										if (!pExisting || pExisting->GetRemainTime(0) <= pCfg->dwRebuffMinRemainingMs) { needed = true; break; }
									}
									if (!needed) continue;
									CSkillCondition* pCond = pSM->FindSkillCondition(buffId);
									if (pCond && !pSM->IsSkillUseLock())
									{
										CBotAiState* pCurState = GetBot()->GetBotController()->GetCurrentState();
										if (pCurState)
										{
											CBotAiAction_SkillUse* pSkillUse = new CBotAiAction_SkillUse(GetBot(), pCond->GetSkillConditionIdx());
											if (pCurState->AddSubControlQueue(pSkillUse, true))
											{
												GetBot()->SetTargetHandle(GetBot()->GetID());
												++buffsQueuedThisAudit;
												s_lastBuffIdx[hid] = (start + step + 1);
												if (pCfg->bVerboseLogs)
													ERR_LOG(LOG_BOTAI, "HelperNPC: party-wide buff queued skill=%u (audit %u/%u)", buffId,
														(unsigned)buffsQueuedThisAudit, (unsigned)(pCfg->byMaxBuffsPerAudit ? pCfg->byMaxBuffsPerAudit : 1));
											}
										}
									}
								}
								// Remove early return - let helper continue to combat after party-wide buffing
							}

							auto tryBuffOn = [&](CPlayer* pTarget) {
								if (!pTarget || !pTarget->IsInitialized() || pTarget->IsFainting() || pTarget->GetCurWorld() != GetBot()->GetCurWorld()) return false;
								BYTE& targetCount = perTargetCount[pTarget->GetID()];
								if (targetCount >= (pCfg->byMaxBuffsPerTargetPerAudit ? pCfg->byMaxBuffsPerTargetPerAudit : 1)) return false;
								for (TBLIDX buffId : buffCandidates)
								{
									if (buffsQueuedThisAudit >= (pCfg->byMaxBuffsPerAudit ? pCfg->byMaxBuffsPerAudit : 1)) return true;
									CBuff* pExisting = pTarget->GetBuffManager()->FindBuff(buffId);
									if (pExisting && pExisting->GetRemainTime(0) > pCfg->dwRebuffMinRemainingMs) continue;
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
												++buffsQueuedThisAudit;
												++targetCount;
												if (pCfg->bVerboseLogs)
													ERR_LOG(LOG_BOTAI, "HelperNPC: rebuff queued skill=%u on %u (audit %u/%u, per-target %u/%u)", buffId, SAFE_ID(pTarget),
														(unsigned)buffsQueuedThisAudit, (unsigned)(pCfg->byMaxBuffsPerAudit ? pCfg->byMaxBuffsPerAudit : 1), (unsigned)targetCount, (unsigned)(pCfg->byMaxBuffsPerTargetPerAudit ? pCfg->byMaxBuffsPerTargetPerAudit : 1));
												return true;
											}
										}
									}
								}
								return false;
								};

							// Leader first, then entire party
							tryBuffOn(pLeader);
							if (pLeader->GetParty())
							{
								BYTE cnt = pLeader->GetParty()->GetPartyMemberCount();
								for (BYTE i = 0; i < cnt; ++i)
								{
									if (buffsQueuedThisAudit >= (pCfg->byMaxBuffsPerAudit ? pCfg->byMaxBuffsPerAudit : 1)) break;
									const sPARTY_MEMBER_INFO& mi = pLeader->GetParty()->GetMemberInfo(i);
									CPlayer* pMem = g_pObjectManager->GetPC(mi.hHandle);
									if (!pMem || pMem == pLeader) continue;
									tryBuffOn(pMem);
								}
							}
							// Remove early return - let helper continue to combat after per-target buffing
						}
					}
				}
			}
		}

		// Skill queuing (with forced-skill priority)
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
							if (isHelper)
							{
								const sHELPER_NPC_CONFIG* pCfgPrio = GetHelperNpcManager()->GetConfigForHelper(GetBot());
								if (pCfgPrio && pCfgPrio->bPrioritizeForcedSkills && !pCfgPrio->vForcedSkills.empty())
								{
									static std::unordered_map<HOBJECT, size_t> s_lastForcedIdx2;
									const HOBJECT hid2 = SAFE_ID(GetBot());
									size_t start2 = 0; auto it2 = s_lastForcedIdx2.find(hid2); if (it2 != s_lastForcedIdx2.end()) start2 = it2->second % pCfgPrio->vForcedSkills.size();
									const size_t n2 = pCfgPrio->vForcedSkills.size();
									for (size_t step = 0; step < n2; ++step)
									{
										TBLIDX fsId = pCfgPrio->vForcedSkills[(start2 + step) % n2];
										CSkillCondition* pForcedCond = pBotSkillManager->FindSkillCondition(fsId);
										if (pForcedCond && pForcedCond->GetCanUseSkill())
										{
											if (GetBot()->GetCharStateID() == CHARSTATE_FOLLOWING || GetBot()->GetMoveFlag() != NTL_MOVE_FLAG_INVALID)
												GetBot()->SendCharStateStanding(true);
											if (pCfgPrio->bVerboseLogs)
												ERR_LOG(LOG_BOTAI, "HelperNPC: prioritized forced skill tblidx=%u", fsId);
											CBotAiAction_SkillUse* pSkillUse = new CBotAiAction_SkillUse(GetBot(), pForcedCond->GetSkillConditionIdx());
											if (!pCurState->AddSubControlQueue(pSkillUse, true))
											{
												delete pSkillUse;
											}
											else
											{
												s_lastForcedIdx2[hid2] = (start2 + step + 1);
											}
											return m_status;
										}
									}
								}
							}
							DWORD SKILL_TRY_COOLDOWN_MS = 100;
							if (const sHELPER_NPC_CONFIG* pCfgTry = GetHelperNpcManager()->GetConfigForHelper(GetBot()))
								SKILL_TRY_COOLDOWN_MS = (pCfgTry->dwSkillTryCooldownMs > 0 ? pCfgTry->dwSkillTryCooldownMs : 100);
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
						}
					}
				}
			}
		}

		// Fast-path proactive scan (outside 1s gate) - helpers only (redundant gating for safety)
		if (isHelper)
		{
			// If helper has an attack target but no aggro entries for some time, clear it so scans resume
			if (GetBot()->GetTargetHandle() != INVALID_HOBJECT && GetBot()->GetTargetListManager() && GetBot()->GetTargetListManager()->GetAggroCount() == 0)
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
				if ((GetBot()->GetTargetListManager() && GetBot()->GetTargetListManager()->GetAggroCount() != 0) || (GetBot()->GetTargetHandle() != INVALID_HOBJECT && GetBot()->GetTargetHandle() != GetBot()->GetLinkPc()))
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
				if (pCfg->bVerboseLogs) ERR_LOG(LOG_BOTAI, "HelperNPC: fast-scan proactive engage");
				m_dwSinceLastEngageMs = 0; // start stick window
			} while (false);
		}

		// Fast-cadence skill queuing (outside 1s gate) to improve reaction speed (helpers only)
		if (isHelper && (GetBot()->HasNearbyPlayer(false) || GetBot()->GetLinkPc() != INVALID_HOBJECT))
		{

			const sHELPER_NPC_CONFIG* pCfgHeal = GetHelperNpcManager()->GetConfigForHelper(GetBot());
			if (pCfgHeal)
			{
				m_dwSinceLastHealScanMs = UnsignedSafeIncrease<DWORD>(m_dwSinceLastHealScanMs, dwTickDiff);
				DWORD healCadence = (pCfgHeal->dwHealScanCooldownMs > 0) ? pCfgHeal->dwHealScanCooldownMs : 150;
				if (m_dwSinceLastHealScanMs >= healCadence)
				{
					m_dwSinceLastHealScanMs = 0;
					CSkillManagerBot* pSM = (CSkillManagerBot*)GetBot()->GetSkillManager();
					if (pSM && !pSM->IsSkillUseLock())
					{
						// Ask manager for next best skill, but only queue if it's a heal (LP/Give/OnlyLP)
						CSkillCondition* pCond = pSM->GetSkill(dwTickDiff);
						if (pCond)
						{
							BYTE basis = pCond->GetSkillBasis();
							if (!(basis == 3 || basis == 4 || basis == 7))
							{
								pCond = nullptr; // not a heal; skip fast-heal queue this tick
							}
						}
						if (pCond && pCond->GetCanUseSkill())
						{
							CBotAiState* pCurState = GetBot()->GetBotController()->GetCurrentState();
							if (pCurState)
							{
								if (GetBot()->GetCharStateID() == CHARSTATE_FOLLOWING || GetBot()->GetMoveFlag() != NTL_MOVE_FLAG_INVALID)
									GetBot()->SendCharStateStanding(true);
								CBotAiAction_SkillUse* pSkillUse = new CBotAiAction_SkillUse(GetBot(), pCond->GetSkillConditionIdx());
								if (pCurState->AddSubControlQueue(pSkillUse, true))
								{
									// Reset general try cooldown so we don’t double-queue this tick
									m_dwSinceLastSkillTryMs = 0;
									if (pCfgHeal->bVerboseLogs)
										ERR_LOG(LOG_BOTAI, "HelperNPC: fast-heal queued tblidx=%u", pCond->GetSkillTblidx());
								}
								else
								{
									delete pSkillUse;
								}
							}
						}
					}
				}

				// Fast resurrection scan (independent of 1s gate)
				if (isHelper)
				{
					const sHELPER_NPC_CONFIG* pCfg = GetHelperNpcManager()->GetConfigForHelper(GetBot());
					// Try both skill-based resurrection AND fallback manual resurrection
					bool hasResurrectSkill = (pCfg && pCfg->resurrectSkillTblidx != INVALID_TBLIDX);

					if (pCfg) // Allow resurrection attempts even without specific skill
					{
						// Use dwTickDiff via accumulating m_dwSinceLastResurrectScanMs (already exists) here as true fast path
						m_dwSinceLastResurrectScanMs = UnsignedSafeIncrease<DWORD>(m_dwSinceLastResurrectScanMs, dwTickDiff);
						DWORD cadence = 100; // default
						if (pCfg && pCfg->dwResurrectScanCooldownMs > 0) cadence = pCfg->dwResurrectScanCooldownMs;
						if (m_dwSinceLastResurrectScanMs >= cadence)
						{
							m_dwSinceLastResurrectScanMs = 0;
							CPlayer* pLeader = (CPlayer*)g_pObjectManager->GetChar(GetBot()->GetLinkPc());
							if (pLeader && pLeader->IsInitialized())
							{
								auto considerMember = [&](CPlayer* pMem) -> bool {
									if (!pMem || !pMem->IsInitialized() || !pMem->IsFainting()) return false;
									if (pMem->GetCurWorld() != GetBot()->GetCurWorld()) return false;

									// Try skill-based resurrection first if available
									if (hasResurrectSkill)
									{
										CSkillManagerBot* pSM = (CSkillManagerBot*)GetBot()->GetSkillManager(); if (!pSM) return false;
										CSkillCondition* pCond = pSM->FindSkillCondition(pCfg->resurrectSkillTblidx);
										if (!pCond)
										{
											if (pCfg->bVerboseLogs) ERR_LOG(LOG_BOTAI, "HelperNPC: resurrect skill condition missing skill=%u", pCfg->resurrectSkillTblidx);
											// Fall through to manual resurrection
										}
										else if (pSM->IsSkillUseLock())
										{
											return false; // busy, try later
										}
										else if (!pCond->GetCanUseSkill())
										{
											if (pCfg->bVerboseLogs) ERR_LOG(LOG_BOTAI, "HelperNPC: resurrect skill not usable (cd/resources) skill=%u", pCfg->resurrectSkillTblidx);
											// Fall through to manual resurrection
										}
										else
										{
											// Skill-based resurrection available - use it
											float fDist = GetBot()->GetDistance(pMem->GetCurLoc());
											if (fDist > 40.0f)
											{
												if (GetBot()->GetTargetListManager() && GetBot()->GetTargetListManager()->GetAggroCount() == 0)
												{
													CWorld* pWorld = pMem->GetCurWorld();
													if (pWorld)
													{
														CNtlVector vLoc = pMem->GetCurLoc();
														CNtlVector vDir = pMem->GetCurDir();
														if (GetBot()->GetBotController()->ChangeControlState_Teleporting(pWorld->GetID(), pWorld->GetIdx(), vLoc, vDir))
														{
															if (pCfg->bVerboseLogs)
																ERR_LOG(LOG_BOTAI, "HelperNPC: teleport to fainted member %u for resurrect (dist=%.1f)", SAFE_ID(pMem), fDist);
															if (CSkillManagerBot* pSM2 = (CSkillManagerBot*)GetBot()->GetSkillManager()) pSM2->SetSkillUse_Unlock();
															GetBot()->SendCharStateStanding(true);
															return true; // handled this tick; will try to cast on next cadence
														}
													}
												}
												if (pCfg->bVerboseLogs) ERR_LOG(LOG_BOTAI, "HelperNPC: resurrect target out of hard range dist=%.1f target=%u", fDist, SAFE_ID(pMem));
												return false;
											}

											// Skill-based resurrection - queue the skill
											CBotAiState* pCurState = GetBot()->GetBotController()->GetCurrentState(); if (!pCurState) return false;
											CBotAiAction_SkillUse* pSkillUse = new CBotAiAction_SkillUse(GetBot(), pCond->GetSkillConditionIdx());
											if (pCurState->AddSubControlQueue(pSkillUse, true))
											{
												GetBot()->SetTargetHandle(pMem ? pMem->GetID() : INVALID_HOBJECT);
												m_hPendingResurrectTarget = pMem ? pMem->GetID() : INVALID_HOBJECT;
												m_byResurrectAttemptCount = 1;
												m_dwSinceLastResurrectAttemptMs = 0;
												if (pCfg->bVerboseLogs) ERR_LOG(LOG_BOTAI, "HelperNPC: fast resurrect queued skill=%u target=%u dist=%.1f", pCfg->resurrectSkillTblidx, SAFE_ID(pMem), fDist);
												if (pCfg) { ++pCfg->dwMetricResurrectAttempts; }
												return true;
											}
											else if (pCfg->bVerboseLogs)
											{
												ERR_LOG(LOG_BOTAI, "HelperNPC: failed to queue resurrect skill=%u target=%u dist=%.1f", pCfg->resurrectSkillTblidx, SAFE_ID(pMem), fDist);
											}
											delete pSkillUse;
											// Fall through to manual resurrection as backup
										}
									}

									// Manual resurrection fallback (GM-style call/revive)
									float fDist = GetBot()->GetDistance(pMem->GetCurLoc());
									if (fDist <= 10.0f) // Close range for manual resurrection
									{
										// Use proper revival method instead of just setting LP
										pMem->Revival(pMem->GetCurLoc(), pMem->GetWorldID(), REVIVAL_TYPE_RESCUED);

										if (pCfg->bVerboseLogs)
											ERR_LOG(LOG_BOTAI, "HelperNPC: manual resurrection performed on %u (LP restored: %u/%u)",
												SAFE_ID(pMem), pMem->GetCurLP(), pMem->GetMaxLP());

										return true;
									}
									else if (pCfg->bVerboseLogs)
									{
										ERR_LOG(LOG_BOTAI, "HelperNPC: member too far for manual resurrection dist=%.1f target=%u", fDist, SAFE_ID(pMem));
									}

									return false;
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
										considerMember(pMem); // Check all party members for resurrection, don't break early
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
										// Allow healing even while following unless extremely far away and moving (avoid skill fail spam)
										if (fDistL > 40.0f && GetBot()->GetMoveFlag() != NTL_MOVE_FLAG_INVALID)
										{
											return m_status; // too far and moving fast to catch up; try later
										}
									}
								}
								DWORD SKILL_TRY_COOLDOWN_MS = 100;
								if (const sHELPER_NPC_CONFIG* pCfgTryFast = GetHelperNpcManager()->GetConfigForHelper(GetBot()))
									SKILL_TRY_COOLDOWN_MS = (pCfgTryFast->dwSkillTryCooldownMs > 0 ? pCfgTryFast->dwSkillTryCooldownMs : 100);
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
		}
	}

	return m_status;
}
