#include "stdafx.h"
#include "WpsScriptAlgoAction_CCBD_stage.h"
#include "WpsNodeAction_CCBD_stage.h"
#include "CPlayer.h"
#include "NtlPacketGU.h"
#include "TableContainerManager.h"
#include "ServerConfigTable.h"
#include "NtlCCBD.h"
#include "DungeonConfig.h"


CWpsScriptAlgoAction_CCBD_stage::CWpsScriptAlgoAction_CCBD_stage(CWpsAlgoObject* pObject) :
	CScriptAlgoAction(pObject, SCRIPTCONTROL_ACTION_CCBD_STAGE, "SCRIPTCONTROL_ACTION_CCBD_STAGE")
{
	m_byStage = INVALID_BYTE;
	m_bDirectPlay = true;
	m_dwFailTimer = CCBD_FAIL_TIMER_IN_MS;
	m_bIsEveryoneReady = false;
	m_bSkipStage = false;
	m_byBossArenaSlot = INVALID_BYTE;
}


CWpsScriptAlgoAction_CCBD_stage::~CWpsScriptAlgoAction_CCBD_stage()
{
}


bool CWpsScriptAlgoAction_CCBD_stage::AttachControlScriptNode(CControlScriptNode* pControlScriptNode)
{
	CWpsNodeAction_CCBD_stage* pAction = dynamic_cast<CWpsNodeAction_CCBD_stage*>(pControlScriptNode);
	if (pAction)
	{
		m_byStage = pAction->m_byStage;
		m_bDirectPlay = pAction->m_bDirectPlay;
		m_byBossArenaSlot = pAction->m_byBossArenaSlot;

		return true;
	}

	ERR_LOG(LOG_BOTAI, "fail : Can't dynamic_cast from CControlScriptNode[%X] to CWpsNodeAction_CCBD_stage", pControlScriptNode);
	return false;
}

void CWpsScriptAlgoAction_CCBD_stage::OnEnter()
{
//	NTL_PRINT(PRINT_APP, "enter stage %u \n", m_byStage);

	m_bSkipStage = false;
	m_bIsEveryoneReady = false;
	// Keep the arena slot value provided by the control node; reset only if invalid
	if (m_byBossArenaSlot >= ENTER_BOSS_STATE_LOC_COUNT)
		m_byBossArenaSlot = INVALID_BYTE;

	// CCBD Boss-Only Mode: Skip non-boss floors and snap to the targeted boss floor
	if (g_pDungeonConfig && g_pDungeonConfig->IsCCBDBossOnlyModeEnabled())
	{
		BYTE byTargetStage = GetOwner()->GetCCBDStage();
		if (byTargetStage == 0 || byTargetStage == INVALID_BYTE)
			byTargetStage = m_byStage;

		if (m_byStage < byTargetStage)
		{
			NTL_PRINT(PRINT_APP, "[CCBD_BOSS_MODE] Skipping stage %u (target %u)", m_byStage, byTargetStage);
			m_bSkipStage = true;
			return;
		}

		// Align controller stage if script advanced further than target
		if (m_byStage > byTargetStage)
		{
			byTargetStage = m_byStage;
			GetOwner()->SetCCBDStage(byTargetStage);
		}

		// Ensure we land on a boss floor (multiples of 5)
		BYTE byBossStage = ((byTargetStage - 1) / 5 + 1) * 5;
		if (byBossStage != byTargetStage)
		{
			byTargetStage = byBossStage;
			GetOwner()->SetCCBDStage(byTargetStage);
		}
		else
		{
			// Persist the resolved stage even if no adjustment was needed
			GetOwner()->SetCCBDStage(byTargetStage);
		}

		if (m_byStage != byTargetStage)
		{
			NTL_PRINT(PRINT_APP, "[CCBD_BOSS_MODE] Adjusting stage node %u to boss floor %u", m_byStage, byTargetStage);
			m_byStage = byTargetStage;
		}

		ERR_LOG(LOG_GENERAL, "[CCBD_BOSS_MODE] Entering boss floor %u", m_byStage);
		m_bDirectPlay = false;
	}
	else
	{
		// Normal CCBD mode (with all stages)
		GetOwner()->SetCCBDStage(m_byStage);
	}

	if (m_bSkipStage)
		return;

	if (m_bDirectPlay == false) //if boss stage
	{
		TeleportToBoss();
	}
	else //if not boss stage
	{
		if (m_byStage > 1) //if not first stage then spawn at base loc
		{
			if(IsCCBDBossStage(m_byStage - 1) == false) //if previous stage was boss then do not spawn because we use teleport at OnExit
				SpawnNextStage();
		}
	}
}

void CWpsScriptAlgoAction_CCBD_stage::OnExit()
{
//	NTL_PRINT(PRINT_APP, "exit stage %u \n", m_byStage);

	if (m_bSkipStage)
	{
		m_bSkipStage = false;
		return;
	}

	// In boss-only mode, all stages are treated as boss stages
	bool bIsBossStage = (g_pDungeonConfig && g_pDungeonConfig->IsCCBDBossOnlyModeEnabled()) ? true : (m_bDirectPlay == false);

	//leave boss room
	if (bIsBossStage)
	{
		CPlayer* pPlayer = GetOwner()->GetPlayersFirst();
		while (pPlayer)
		{
			if (pPlayer->GetWorldID() == GetOwner()->GetWorld()->GetID())
			{
				if (pPlayer->GetParty())
				{
					pPlayer->GetParty()->DecidePartySelect();
					break;
				}
			}
			else
			{
				ERR_LOG(LOG_GENERAL, "User is registered for ccbd but has different world id (%u != %u) !!!", pPlayer->GetWorldID(), GetOwner()->GetWorld()->GetID());
			}

			pPlayer = GetOwner()->GetPlayersNext();
		}
	}
}

int CWpsScriptAlgoAction_CCBD_stage::OnUpdate(DWORD dwTickDiff, float fMultiple)
{
	if (m_bSkipStage)
	{
		// Skip scripted filler stages when boss-only mode pre-advances the controller
		m_status = COMPLETED;
		return m_status;
	}

	// In boss-only mode, all stages are boss stages
	bool bIsBossStage = (g_pDungeonConfig && g_pDungeonConfig->IsCCBDBossOnlyModeEnabled()) ? true : IsCCBDBossStage(m_byStage);

	if (m_bIsEveryoneReady == false && bIsBossStage) //wait until everyone arrive in boss stage
	{
		if (IsEveryoneReady())
		{
			m_bIsEveryoneReady = true;

			CNtlPacket packet(sizeof(sGU_BATTLE_DUNGEON_STATE_UPATE_NFY));
			sGU_BATTLE_DUNGEON_STATE_UPATE_NFY* res = (sGU_BATTLE_DUNGEON_STATE_UPATE_NFY *)packet.GetPacketData();
			res->wOpCode = GU_BATTLE_DUNGEON_STATE_UPATE_NFY;
			res->byStage = GetOwner()->GetCCBDStage();
			res->dwLimitTime = 0;
			res->titleTblidx = 41;
			res->subTitleTblidx = 42;
			packet.SetPacketLen(sizeof(sGU_BATTLE_DUNGEON_STATE_UPATE_NFY));
			GetOwner()->Broadcast(&packet);
		}
		else
			return m_status;
	}

	//if ccbd failed then teleport everyone out
	if (GetOwner()->GetCCBDFail())
	{
		if (m_dwFailTimer > 0) //check if over 0 to avoid possible double teleport
		{
			m_dwFailTimer = UnsignedSafeDecrease<DWORD>(m_dwFailTimer, dwTickDiff);

			if (m_dwFailTimer == 0)
			{
				TeleportOut();
			}
		}
		return m_status;
	}
	else
	{
		if (CheckPlayersState())
			return m_status;
	}

	m_status = UpdateSubControlQueue(dwTickDiff, fMultiple); //update actions

	return m_status;
}


void CWpsScriptAlgoAction_CCBD_stage::SpawnNextStage()
{
	//reset players loc/dir and send direct play

	CNtlPacket packet(sizeof(sGU_CHAR_DIRECT_PLAY));
	sGU_CHAR_DIRECT_PLAY* res = (sGU_CHAR_DIRECT_PLAY *)packet.GetPacketData();
	res->wOpCode = GU_CHAR_DIRECT_PLAY;
	res->bCanSkip = false;
	res->bSynchronize = true; //not sure
	res->byPlayMode = 1; //eDIRECTION_TYPE
	res->directTblidx = g_pTableContainer->GetServerConfigTable()->GetServerConfigData()->sBattleDungeonData.directPlay_StageChange;
	packet.SetPacketLen(sizeof(sGU_CHAR_DIRECT_PLAY));

	CNtlVector destLoc(g_pTableContainer->GetServerConfigTable()->GetServerConfigData()->sBattleDungeonData.sEnterLoc_NormalStage.sLoc);
	CNtlVector destDir(g_pTableContainer->GetServerConfigTable()->GetServerConfigData()->sBattleDungeonData.sEnterLoc_NormalStage.sDir);

	CPlayer* pPlayer = GetOwner()->GetPlayersFirst();
	while (pPlayer)
	{
		if (pPlayer->GetWorldID() == GetOwner()->GetWorld()->GetID())
		{
			if (pPlayer->IsFainting() == false)
			{
				pPlayer->SetCurLoc(destLoc, GetOwner()->GetWorld());
				pPlayer->SetCurDir(destDir);
				pPlayer->SendCharStateSpawning(TELEPORT_TYPE_DUNGEON);

				pPlayer->SendPacket(&packet);
			}
		}
		else
		{
			ERR_LOG(LOG_GENERAL, "User is registered for ccbd but has different world id (%u != %u) !!!", pPlayer->GetWorldID(), GetOwner()->GetWorld()->GetID());
		}

		pPlayer = GetOwner()->GetPlayersNext();
	}
}

void CWpsScriptAlgoAction_CCBD_stage::TeleportToBoss()
{
	//teleport players to boss and send dungeon state
	//if player is fainting then remove from CCBD

	BYTE byBossStageCount = (m_byStage / 5) - 1;

	// Use override when provided; otherwise keep legacy cycling behaviour
	BYTE byArenaIndex;
	if (m_byBossArenaSlot != INVALID_BYTE)
		byArenaIndex = m_byBossArenaSlot % ENTER_BOSS_STATE_LOC_COUNT;
	else
		byArenaIndex = byBossStageCount % ENTER_BOSS_STATE_LOC_COUNT;

	WORLDID destWorld = GetOwner()->GetWorld()->GetID();
	CNtlVector destLoc(g_pTableContainer->GetServerConfigTable()->GetServerConfigData()->sBattleDungeonData.aEnterLoc_BossStage[byArenaIndex].sLoc);
	CNtlVector destDir(g_pTableContainer->GetServerConfigTable()->GetServerConfigData()->sBattleDungeonData.aEnterLoc_BossStage[byArenaIndex].sDir);

	CPlayer* pPlayer = GetOwner()->GetPlayersFirst();
	while (pPlayer)
	{
		if (pPlayer->GetWorldID() == GetOwner()->GetWorld()->GetID())
		{
			pPlayer->StartTeleport(destLoc, destDir, destWorld, TELEPORT_TYPE_DEFAULT);
		}
		else
		{
			ERR_LOG(LOG_GENERAL, "User is registered for ccbd but has different world id (%u != %u) !!!", pPlayer->GetWorldID(), GetOwner()->GetWorld()->GetID());
		}

		pPlayer = GetOwner()->GetPlayersNext();
	}
}

void CWpsScriptAlgoAction_CCBD_stage::TeleportOut()
{
	CWorld* pWorld = GetOwner()->GetWorld();
	CNtlVector destLoc(pWorld->GetTbldat()->outWorldLoc);
	CNtlVector destDir(pWorld->GetTbldat()->outWorldDir);
	WORLDID destWorldID = pWorld->GetTbldat()->outWorldTblidx;

	CPlayer* pPlayer = GetOwner()->GetPlayersFirst();
	while (pPlayer)
	{
		if (pPlayer->GetWorldID() == GetOwner()->GetWorld()->GetID())
		{
			//teleport out
			pPlayer->StartTeleport(destLoc, destDir, destWorldID, TELEPORT_TYPE_WORLD_MOVE);
		}
		else
		{
			ERR_LOG(LOG_GENERAL, "User is registered for ccbd but has different world id (%u != %u) !!!", pPlayer->GetWorldID(), GetOwner()->GetWorld()->GetID());
		}

		pPlayer = GetOwner()->GetPlayersNext();
	}
}

bool CWpsScriptAlgoAction_CCBD_stage::CheckPlayersState()
{
	//send dungeon fail nfy, set ccbd fail and return true when everyoene is dead

	DWORD dwPlayerFaintCount = 0;
	DWORD dwPlayerCount = 0;
	DWORD dwPlayerDisconnectedCount = 0;

	CPlayer* pPlayer = GetOwner()->GetPlayersFirst();
	while (pPlayer)
	{
		if (pPlayer->GetWorldID() == GetOwner()->GetWorld()->GetID())
		{
			++dwPlayerCount;

			if (pPlayer->IsFainting())
			{
				++dwPlayerFaintCount;
			}
		}
		else
		{
			// Player disconnected or in transition - count them separately
			// Don't fail the dungeon immediately if players are just reconnecting
			++dwPlayerDisconnectedCount;
			ERR_LOG(LOG_GENERAL, "User is registered for ccbd but has different world id (%u != %u) !!!", pPlayer->GetWorldID(), GetOwner()->GetWorld()->GetID());
		}

		pPlayer = GetOwner()->GetPlayersNext();
	}

	// Only fail if all CONNECTED players are fainting AND there's at least one connected player
	// Don't fail if players are just disconnected/reconnecting
	if (dwPlayerCount > 0 && dwPlayerFaintCount >= dwPlayerCount)
	{
		CNtlPacket packet(sizeof(sGU_BATTLE_DUNGEON_FAIL_NFY));
		sGU_BATTLE_DUNGEON_FAIL_NFY* res = (sGU_BATTLE_DUNGEON_FAIL_NFY *)packet.GetPacketData();
		res->wOpCode = GU_BATTLE_DUNGEON_FAIL_NFY;
		GetOwner()->GetWorld()->Broadcast(&packet);

		GetOwner()->SetCCBDFail(true); //block further ccbd process & tp out in 10 seconds
		return true;
	}

	return false;
}

bool CWpsScriptAlgoAction_CCBD_stage::IsEveryoneReady()
{
	bool bFlag = true;

	CPlayer* pPlayer = GetOwner()->GetPlayersFirst();
	while (pPlayer)
	{
		if (pPlayer->GetWorldID() != GetOwner()->GetWorld()->GetID() || pPlayer->GetCharStateID() == CHARSTATE_DIRECT_PLAY || pPlayer->GetCharStateID() == CHARSTATE_SPAWNING || pPlayer->GetCharStateID() == CHARSTATE_DESPAWNING || pPlayer->GetCharStateID() == CHARSTATE_TELEPORTING)
		{
			bFlag = false;
			break;
		}

		pPlayer = GetOwner()->GetPlayersNext();
	}

	return bFlag;
}
