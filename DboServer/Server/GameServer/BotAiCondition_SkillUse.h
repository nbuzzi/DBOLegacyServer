#ifndef __AI_DBOG_BOTCONDITION_SKILLUSE_H__
#define __AI_DBOG_BOTCONDITION_SKILLUSE_H__

#include "BotAiCondition.h"


class CBotAiCondition_SkillUse : public CBotAiCondition
{

public:
	CBotAiCondition_SkillUse(CNpc* pBot);
	virtual	~CBotAiCondition_SkillUse();


public:

	virtual int OnUpdate(DWORD dwTickDiff, float fMultiple);


private:

	DWORD m_dwTime;
	// Tracks how long the helper has been too far or in a different world than its leader
	DWORD m_dwOutOfRangeTimeMs;

	// Tracks time since the last proactive enemy scan while idle
	DWORD m_dwSinceLastProactiveScanMs;

	// Tracks time spent with a target handle but no aggro; used to clear stale targets
	DWORD m_dwChaseNoAggroMs;

	// Helper skill attempt cadence outside the 1s gate
	DWORD m_dwSinceLastSkillTryMs;

	// Cooldown to avoid spamming follow reasserts/logs
	DWORD m_dwSinceLastFollowReassertMs;

	// Track distance to leader to detect lack of progress
	float m_fLastLeaderDist;
	DWORD m_dwNoFollowProgressMs;

	// Rebuff scheduler
	DWORD m_dwSinceLastRebuffCheckMs;

	// Random follow refresh timer
	DWORD m_dwSinceRandomFollowMs;

	// Half-second cadence resurrection scan timer
	DWORD m_dwSinceLastResurrectScanMs = 0;

	// Fast heal scan timer (prioritize heals on its own cadence)
	DWORD m_dwSinceLastHealScanMs = 0;

	// Tank aggro pulse timer
	DWORD m_dwSinceLastTankAggroPulseMs = 0;

	// Time since last engage/follow switch to prevent oscillation
	DWORD m_dwSinceLastEngageMs = 0;

	// Resurrect retry tracking
	HOBJECT m_hPendingResurrectTarget = INVALID_HOBJECT; // currently tracked faint target
	DWORD  m_dwSinceLastResurrectAttemptMs = 0;          // time since last attempt
	BYTE   m_byResurrectAttemptCount = 0;                // number of attempts so far

	// Buff coverage rotation: index offset to start from next cycle so one member isn't always first
	BYTE   m_byBuffCoverageStartIndex = 0;

};

#endif