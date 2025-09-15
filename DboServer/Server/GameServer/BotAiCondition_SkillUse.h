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

};

#endif