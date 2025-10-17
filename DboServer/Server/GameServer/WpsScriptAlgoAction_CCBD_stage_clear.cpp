#include "stdafx.h"
#include "WpsScriptAlgoAction_CCBD_stage_clear.h"
#include "CPlayer.h"
#include "Party.h" // for party enumeration
#include "NtlPacketGU.h"
#include "BattlePassManager.h" // Battle Pass dungeon stage hook
#include "DungeonConfig.h" // for boss-only mode check


CWpsScriptAlgoAction_CCBD_stage_clear::CWpsScriptAlgoAction_CCBD_stage_clear(CWpsAlgoObject* pObject) :
	CScriptAlgoAction(pObject, SCRIPTCONTROL_ACTION_CCBD_STAGE_CLEAR, "SCRIPTCONTROL_ACTION_CCBD_STAGE_CLEAR")
{
}


CWpsScriptAlgoAction_CCBD_stage_clear::~CWpsScriptAlgoAction_CCBD_stage_clear()
{
}

int CWpsScriptAlgoAction_CCBD_stage_clear::OnUpdate(DWORD dwTickDiff, float fMultiple)
{
//	NTL_PRINT(PRINT_APP, "CWpsScriptAlgoAction_CCBD_stage_clear\n");

	CNtlPacket packet(sizeof(sGU_BATTLE_DUNGEON_STAGE_CLEAR_NFY));
	sGU_BATTLE_DUNGEON_STAGE_CLEAR_NFY* res = (sGU_BATTLE_DUNGEON_STAGE_CLEAR_NFY *)packet.GetPacketData();
	res->wOpCode = GU_BATTLE_DUNGEON_STAGE_CLEAR_NFY;
	GetOwner()->Broadcast(&packet);

	if (GetOwner()->GetCCBDTimeLimit())
	{
		GetOwner()->SetCCBDTimeLimit(false);

		CNtlPacket packet2(sizeof(sGU_BATTLE_DUNGEON_LIMIT_TIME_END_NFY));
		sGU_BATTLE_DUNGEON_LIMIT_TIME_END_NFY* res2 = (sGU_BATTLE_DUNGEON_LIMIT_TIME_END_NFY *)packet2.GetPacketData();
		res2->wOpCode = GU_BATTLE_DUNGEON_LIMIT_TIME_END_NFY;
		GetOwner()->Broadcast(&packet2);
	}

	// CCBD Boss-Only Mode: After clearing a boss floor, save progress
	// Players will continue to next boss floor on re-entry
	if (g_pDungeonConfig && g_pDungeonConfig->IsCCBDBossOnlyModeEnabled())
	{
		BYTE byClearedStage = GetOwner()->GetCCBDStage();
		
		// Check if this is a boss floor (5, 10, 15, 20...)
		if (byClearedStage % 5 == 0 && byClearedStage >= 5)
		{
			// Calculate next boss floor
			BYTE byNextBossFloor = byClearedStage + 5; // 5→10, 10→15, 15→20...
			
			NTL_PRINT(PRINT_APP, "[CCBD_BOSS_MODE] Boss floor %u cleared, next entry will be floor %u\n", 
				byClearedStage, byNextBossFloor);
			
			// Save NEXT boss floor to all players in the dungeon
			CPlayer* pPlayer = GetOwner()->GetPlayersFirst();
			while (pPlayer)
			{
				if (pPlayer->GetWorldID() == GetOwner()->GetWorld()->GetID())
				{
					pPlayer->SetCCBDLastBossStageCleared(byNextBossFloor);
				}
				pPlayer = GetOwner()->GetPlayersNext();
			}
			
			
			// Direct controller to target the next boss floor
			GetOwner()->SetCCBDStage(byNextBossFloor);
			NTL_PRINT(PRINT_APP, "[CCBD_BOSS_MODE] Next boss stage set to %u after clearing %u", byNextBossFloor, byClearedStage);
		}
	}

	m_status = COMPLETED;
	return m_status;
}
