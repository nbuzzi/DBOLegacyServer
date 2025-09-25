//#include "..\QueryServer\Dojo.h"
#include "Stdafx.h"
#include "DojoManager.h"
#include "ChatServer.h"
#include "NtlResultCode.h"
#include "NtlPacketTG.h"
#include "NtlPacketTU.h"
#include "GuildManager.h"
#include "DojoWar.h"
#include "NtlPacketTQ.h"

#define DBO_ENABLE_DECLINE_DOJO_SCRAMBLE 0


CDojo::CDojo(sDBO_DOJO_DATA* dojo)
{
	memcpy(&dojoData, dojo, sizeof(sDBO_DOJO_DATA));
	m_pkGuild = NULL;
	m_lastDojoWar = INVALID_DOJOTIME;
	m_lastChallengerRequest = INVALID_GUILDID;
	m_lastChallengerCharID = INVALID_CHARACTERID;
	m_byRejectCount = 0;
	m_bHasWarRequest = false;
	m_bWarInProgress = false;

	m_eDojoState = eDBO_DOJO_STATUS_NORMAL;
	m_tmNextStepTime = 0;

	memset(&m_warPoint, 0, sizeof(sDOJO_SCRAMBLE_POINT));
	m_warPoint.dojoTblidx = dojoData.dojoTblidx;
}

CDojo::~CDojo()
{
}



void CDojo::ChangeDojoOwner(CGuild* pNewGuild)
{
	CChatServer* app = (CChatServer*)g_pApp;

	dojoData.guildId = pNewGuild->GetInfo()->guildId;
	NTL_SAFE_WCSCPY(dojoData.wszName, pNewGuild->GetInfo()->wszName);
	ZeroMemory(dojoData.wchLeaderName, NTL_MAX_SIZE_CHAR_NAME);
	ZeroMemory(dojoData.wchNotice, NTL_MAX_LENGTH_OF_DOJO_NOTICE);
	ZeroMemory(dojoData.wszSeedCharName, NTL_MAX_SIZE_CHAR_NAME);

	pNewGuild->SetDojo(this);

	CNtlPacket packet(sizeof(sTQ_DOJO_UPDATE_REQ));
	sTQ_DOJO_UPDATE_REQ * res = (sTQ_DOJO_UPDATE_REQ *)packet.GetPacketData();
	res->wOpCode = TQ_DOJO_UPDATE_REQ;
	res->byServerChannelId = 0;
	memcpy(&res->sDojoData, &dojoData, sizeof(sDBO_DOJO_DATA));
	packet.SetPacketLen(sizeof(sTQ_DOJO_UPDATE_REQ));
	app->SendTo(app->GetQueryServerSession(), &packet);

}


void CDojo::RequestWar(GUILDID guildChallengerID, CHARACTERID requestorCharID, WORD& rwResultcode)
{
	//check if dojo has an owner
	if (m_pkGuild == NULL)
	{
		rwResultcode = GAME_GUILD_DOJO_NOT_FOUND;
		return;
	}

	//check if this guild already has been rejected
	if(m_lastChallengerRequest == guildChallengerID)
	{
		rwResultcode = GAME_GUILD_DOJO_YOU_ARE_ARLREADY_REJECT;
		return;
	}
	else if (dojoData.challengeGuildId == guildChallengerID)
	{
		rwResultcode = GAME_GUILD_DOJO_SCRAMBLE_CHALLENGE_HAVE;
		return;
	}
	else if (m_eDojoState != eDBO_DOJO_STATUS_RECEIVE)
	{
		rwResultcode = GAME_GUILD_DOJO_SCRAMBLE_CHALLENGE_STATUS_FAIL;
		return;
	}


	dojoData.challengeGuildId = guildChallengerID;
	m_lastChallengerCharID = requestorCharID;
	rwResultcode = GAME_SUCCESS;
}


void CDojo::OnEvent_WarRequestTimer()
{
	//if timer ends, then start dojo war !
	if (CanReceiveWarRequest() == false)
		return;

	CChatServer* appCfg = (CChatServer*)g_pApp;
	if (appCfg && appCfg->m_config.bDisableDojoWar)
	{
		NTL_PRINT(PRINT_APP, "[Dojo] WarRequestTimer suppressed by DisableDojoWar config");
		m_bHasWarRequest = false;
		dojoData.challengeGuildId = INVALID_GUILDID;
		return;
	}

	BeginWarPreparation();
}


void CDojo::WarRequestReponse(bool& rbIsAccept, WORD& rwResultcode, bool& rbIsRetry)
{
	rwResultcode = GAME_SUCCESS;
	rbIsRetry = false;

	if (CanReceiveWarRequest() == false)
	{
		rwResultcode = GAME_FAIL;
		g_pEventMgr->RemoveEvents(this, EVENT_DOJO_WAR_REQUEST);

		return;
	}

#if DBO_ENABLE_DECLINE_DOJO_SCRAMBLE
	if (bIsAccept == false)
	{
		if (m_byRejectCount < DBO_DOJO_SCRAMBLE_MAX_REJECT_COUNT) //check if reject limit not reached
		{
			m_bHasWarRequest = false;
			m_lastChallengerRequest = GetDojoData()->challengeGuildId;

			//cancel request time
			g_pEventMgr->RemoveEvents(this, EVENT_DOJO_WAR_REQUEST);

			CPlayer* pRequestor = g_pPlayerManager->FindPlayerWithCharID(GetChallengerCharID());
			if (pRequestor && pRequestor->GetPcInitState() && pRequestor->GetGuild())
			{
				pRequestor->GetGuild()->SetDojoTblidxWarRequest(INVALID_TBLIDX);

				//send nfy to the requestor that war declined
				CNtlPacket packet2(sizeof(sTU_DOJO_SCRAMBLE_REJECT_NFY));
				sTU_DOJO_SCRAMBLE_REJECT_NFY * res2 = (sTU_DOJO_SCRAMBLE_REJECT_NFY *)packet2.GetPacketData();
				res2->wOpCode = TU_DOJO_SCRAMBLE_REJECT_NFY;
				packet2.SetPacketLen(sizeof(sTU_DOJO_SCRAMBLE_REJECT_NFY));
				pRequestor->SendPacket(&packet2);
			}
			else
			{
				CGuild* pGuild = g_pGuildManager->GetGuild(GetDojoData()->challengeGuildId);
				if (pGuild)
				{
					pGuild->SetDojoTblidxWarRequest(INVALID_TBLIDX);
				}
			}

			dojoData.challengeGuildId = INVALID_GUILDID;
			m_byRejectCount = DBO_DOJO_SCRAMBLE_MAX_REJECT_COUNT;
			return;
		}
		else if(m_byRejectCount == DBO_DOJO_SCRAMBLE_MAX_REJECT_COUNT)
		{
			rwResultcode = GAME_GUILD_DOJO_YOU_ARE_ARLREADY_REJECT_ONE_TIME; //begin war here 
			rbIsRetry = true;
			++m_byRejectCount;
			return;
		}
	}
#else
	rbIsAccept = true;
#endif

	g_pEventMgr->RemoveEvents(this, EVENT_DOJO_WAR_REQUEST);
	BeginWarPreparation();
}


void CDojo::BeginWarPreparation()
{
	CChatServer* appCfg = (CChatServer*)g_pApp;
	if (appCfg && appCfg->m_config.bDisableDojoWar)
	{
		NTL_PRINT(PRINT_APP, "[Dojo] BeginWarPreparation suppressed by DisableDojoWar config");
		// reset pending request flags
		m_bHasWarRequest = false;
		dojoData.challengeGuildId = INVALID_GUILDID;
		return;
	}

	m_lastDojoWar = std::time(NULL);
	m_byRejectCount = 0;
	m_bHasWarRequest = false;

	m_bWarInProgress = true;

	m_eDojoState = eDBO_DOJO_STATUS_STANDBY;

	m_tmNextStepTime = m_lastDojoWar + 10; // 1800 seconds, 30 minutes preparation

	NTL_PRINT(PRINT_APP, "Guild %u has now war with guild %u and fight for the dojo id %u", dojoData.guildId, dojoData.challengeGuildId, dojoData.dojoTblidx);


	//send this packet to requestor that player has accepted the war
	CPlayer* pRequestor = g_pPlayerManager->FindPlayerWithCharID(GetChallengerCharID());
	if (pRequestor && pRequestor->GetPcInitState() && m_pkGuild)
	{
		CNtlPacket packet2(sizeof(sTU_DOJO_SCRAMBLE_RECEIVE_NFY));
		sTU_DOJO_SCRAMBLE_RECEIVE_NFY * res2 = (sTU_DOJO_SCRAMBLE_RECEIVE_NFY *)packet2.GetPacketData();
		res2->wOpCode = TU_DOJO_SCRAMBLE_RECEIVE_NFY;
		res2->dojoTblidx = dojoData.dojoTblidx;
		res2->guildId = m_pkGuild->GetInfo()->guildId;
		packet2.SetPacketLen(sizeof(sTU_DOJO_SCRAMBLE_RECEIVE_NFY));
		pRequestor->SendPacket(&packet2);
	}

	//send nfy to dojo-channel
	CNtlPacket packet(sizeof(sTU_DOJO_SCRAMBLE_INFORM_NFY));
	sTU_DOJO_SCRAMBLE_INFORM_NFY * res = (sTU_DOJO_SCRAMBLE_INFORM_NFY *)packet.GetPacketData();
	res->wOpCode = TU_DOJO_SCRAMBLE_INFORM_NFY;
	res->dojoTblidx = dojoData.dojoTblidx;
	res->byState = m_eDojoState;
	res->tmNextStepTime = m_tmNextStepTime;
	packet.SetPacketLen(sizeof(sTU_DOJO_SCRAMBLE_INFORM_NFY));
	if (GetGuild())
		GetGuild()->Broadcast(&packet);

	if (CGuild* pOpp0 = g_pGuildManager->GetGuild(dojoData.challengeGuildId))
		pOpp0->Broadcast(&packet);


	//
	CNtlPacket packetSrv(sizeof(sTG_DOJO_SEND_ATT_GUILD_ID));
	sTG_DOJO_SEND_ATT_GUILD_ID * resSrv = (sTG_DOJO_SEND_ATT_GUILD_ID *)packetSrv.GetPacketData();
	resSrv->wOpCode = TG_DOJO_SEND_ATT_GUILD_ID;
	resSrv->dojoTblidx = dojoData.dojoTblidx;
	resSrv->defGuildId = dojoData.guildId;
	resSrv->attGuildId = dojoData.challengeGuildId;
	packetSrv.SetPacketLen(sizeof(sTG_DOJO_SEND_ATT_GUILD_ID));
	g_pServerInfoManager->Broadcast(&packetSrv);

	//
	CNtlPacket packetSrv2(sizeof(sTG_DOJO_SCRAMBLE_STATE_CHANGE));
	sTG_DOJO_SCRAMBLE_STATE_CHANGE * resSrv2 = (sTG_DOJO_SCRAMBLE_STATE_CHANGE *)packetSrv2.GetPacketData();
	resSrv2->wOpCode = TG_DOJO_SCRAMBLE_STATE_CHANGE;
	resSrv2->dojoTblidx = dojoData.dojoTblidx;
	resSrv2->byState = m_eDojoState;
	resSrv2->tmNextStepTime = m_tmNextStepTime;
	packetSrv2.SetPacketLen(sizeof(sTG_DOJO_SCRAMBLE_STATE_CHANGE));
	g_pServerInfoManager->Broadcast(&packetSrv2);
}

void CDojo::BeginWarPreparationManual()
{
	// Manual version: identical to BeginWarPreparation but without DisableDojoWar guard
	m_lastDojoWar = std::time(NULL);
	m_byRejectCount = 0;
	m_bHasWarRequest = false;

	m_bWarInProgress = true;

	m_eDojoState = eDBO_DOJO_STATUS_STANDBY;

	m_tmNextStepTime = m_lastDojoWar + 10; // 10 seconds preparation for manual start

	// notify requestor if present
	CPlayer* pRequestor = g_pPlayerManager->FindPlayerWithCharID(GetChallengerCharID());
	if (pRequestor && pRequestor->GetPcInitState() && m_pkGuild)
	{
		CNtlPacket packet2(sizeof(sTU_DOJO_SCRAMBLE_RECEIVE_NFY));
		sTU_DOJO_SCRAMBLE_RECEIVE_NFY * res2 = (sTU_DOJO_SCRAMBLE_RECEIVE_NFY *)packet2.GetPacketData();
		res2->wOpCode = TU_DOJO_SCRAMBLE_RECEIVE_NFY;
		res2->dojoTblidx = dojoData.dojoTblidx;
		res2->guildId = m_pkGuild->GetInfo()->guildId;
		packet2.SetPacketLen(sizeof(sTU_DOJO_SCRAMBLE_RECEIVE_NFY));
		pRequestor->SendPacket(&packet2);
	}

	//send nfy to dojo-channel
	CNtlPacket packet(sizeof(sTU_DOJO_SCRAMBLE_INFORM_NFY));
	sTU_DOJO_SCRAMBLE_INFORM_NFY * res = (sTU_DOJO_SCRAMBLE_INFORM_NFY *)packet.GetPacketData();
	res->wOpCode = TU_DOJO_SCRAMBLE_INFORM_NFY;
	res->dojoTblidx = dojoData.dojoTblidx;
	res->byState = m_eDojoState;
	res->tmNextStepTime = m_tmNextStepTime;
	packet.SetPacketLen(sizeof(sTU_DOJO_SCRAMBLE_INFORM_NFY));
	if (GetGuild())
		GetGuild()->Broadcast(&packet);
	{
		CGuild* pOpp = g_pGuildManager->GetGuild(dojoData.challengeGuildId);
		if (pOpp)
			pOpp->Broadcast(&packet);
	}

	// server broadcasts
	{
		CNtlPacket packetSrv(sizeof(sTG_DOJO_SEND_ATT_GUILD_ID));
		sTG_DOJO_SEND_ATT_GUILD_ID * resSrv = (sTG_DOJO_SEND_ATT_GUILD_ID *)packetSrv.GetPacketData();
		resSrv->wOpCode = TG_DOJO_SEND_ATT_GUILD_ID;
		resSrv->dojoTblidx = dojoData.dojoTblidx;
		resSrv->defGuildId = dojoData.guildId;
		resSrv->attGuildId = dojoData.challengeGuildId;
		packetSrv.SetPacketLen(sizeof(sTG_DOJO_SEND_ATT_GUILD_ID));
		g_pServerInfoManager->Broadcast(&packetSrv);
	}
	{
		CNtlPacket packetSrv2(sizeof(sTG_DOJO_SCRAMBLE_STATE_CHANGE));
		sTG_DOJO_SCRAMBLE_STATE_CHANGE * resSrv2 = (sTG_DOJO_SCRAMBLE_STATE_CHANGE *)packetSrv2.GetPacketData();
		resSrv2->wOpCode = TG_DOJO_SCRAMBLE_STATE_CHANGE;
		resSrv2->dojoTblidx = dojoData.dojoTblidx;
		resSrv2->byState = m_eDojoState;
		resSrv2->tmNextStepTime = m_tmNextStepTime;
		packetSrv2.SetPacketLen(sizeof(sTG_DOJO_SCRAMBLE_STATE_CHANGE));
		g_pServerInfoManager->Broadcast(&packetSrv2);
	}
}

void CDojo::UpdateState(BYTE byState, DOJOTIME tmNextStepTime)
{
	m_eDojoState = (eDBO_DOJO_STATUS)byState;
	m_tmNextStepTime = tmNextStepTime;
	//printf("update dojo %u state %u\n", dojoData.dojoTblidx, byState);

	if (m_eDojoState == eDBO_DOJO_STATUS_READY)
	{
		CGuild* pOpponent = g_pGuildManager->GetGuild(dojoData.challengeGuildId);
		if (m_pkGuild && pOpponent)
		{
			CNtlPacket packet2(sizeof(sTU_DOJO_SCRAMBLE_RECEIVE_INFO_NFY));
			sTU_DOJO_SCRAMBLE_RECEIVE_INFO_NFY * res2 = (sTU_DOJO_SCRAMBLE_RECEIVE_INFO_NFY *)packet2.GetPacketData();
			res2->wOpCode = TU_DOJO_SCRAMBLE_RECEIVE_INFO_NFY;
			res2->dojoTblidx = dojoData.dojoTblidx;
			res2->defGuildId = dojoData.guildId;
			res2->attGuildId = dojoData.challengeGuildId;
			NTL_SAFE_WCSCPY(res2->wszDefGuildName, m_pkGuild->GetInfo()->wszName);
			NTL_SAFE_WCSCPY(res2->wszAttGuildName, pOpponent->GetInfo()->wszName);
			memcpy(&res2->sDefMark, &m_pkGuild->GetInfo()->sMark, sizeof(sDBO_GUILD_MARK));
			memcpy(&res2->sAttMark, &pOpponent->GetInfo()->sMark, sizeof(sDBO_GUILD_MARK));
			packet2.SetPacketLen(sizeof(sTU_DOJO_SCRAMBLE_RECEIVE_INFO_NFY));
			m_pkGuild->Broadcast(&packet2);
			pOpponent->Broadcast(&packet2);
		}
	}

	CNtlPacket packet(sizeof(sTU_DOJO_SCRAMBLE_INFORM_NFY));
	sTU_DOJO_SCRAMBLE_INFORM_NFY * res = (sTU_DOJO_SCRAMBLE_INFORM_NFY *)packet.GetPacketData();
	res->wOpCode = TU_DOJO_SCRAMBLE_INFORM_NFY;
	res->dojoTblidx = dojoData.dojoTblidx;
	res->byState = byState;
	res->tmNextStepTime = tmNextStepTime;
	packet.SetPacketLen(sizeof(sTU_DOJO_SCRAMBLE_INFORM_NFY));

	// Safety: if guild pointers are missing, prefer global notify to avoid crashes
	if (m_eDojoState != eDBO_DOJO_STATUS_RECEIVE)
	{
		CGuild* pDef = GetGuild();
		CGuild* pOpp = g_pGuildManager->GetGuild(dojoData.challengeGuildId);
		if (!pDef || !pOpp)
		{
			g_pPlayerManager->SendMsgToAll(&packet);
			// proceed with state-specific cleanup below without guild broadcasts
		}
	}

	switch (m_eDojoState)
	{
		case eDBO_DOJO_STATUS_RECEIVE:
		{
			g_pDojoWarManager->AddDojo(this);
			g_pPlayerManager->SendMsgToAll(&packet);
		}
		break;

		case eDBO_DOJO_STATUS_END:
		case eDBO_DOJO_STATUS_NORMAL:
		{
			g_pPlayerManager->SendMsgToAll(&packet);
		}
		break;

		default:
		{
			CGuild* pDef = GetGuild();
			if (pDef)
				pDef->Broadcast(&packet);

			CGuild* pOpponent = g_pGuildManager->GetGuild(dojoData.challengeGuildId);
			if (pOpponent)
				pOpponent->Broadcast(&packet);
		}
		break;
	}

	if (m_eDojoState == eDBO_DOJO_STATUS_END)
	{
		CGuild* pGuild = g_pGuildManager->GetGuild(GetDojoData()->challengeGuildId);
		if (pGuild)
		{
			pGuild->SetDojoTblidxWarRequest(INVALID_TBLIDX);
		}

		if (GetGuild())
			GetGuild()->SetDojoTblidxWarRequest(INVALID_TBLIDX);

		m_bHasWarRequest = false;
		m_bWarInProgress = false;

		m_setDefender.clear();
		m_setAttacker.clear();
	}
	else if (m_eDojoState == eDBO_DOJO_STATUS_NORMAL)
	{
		SetGuild(g_pGuildManager->GetGuild(GetDojoData()->guildId));

		dojoData.challengeGuildId = INVALID_GUILDID;
		m_lastChallengerRequest = INVALID_GUILDID;
		m_lastChallengerCharID = INVALID_CHARACTERID;
		m_byRejectCount = 0;
		m_bHasWarRequest = false;
		m_bWarInProgress = false;
		memset(&m_warPoint, NULL, sizeof(m_warPoint));
		m_warPoint.dojoTblidx = dojoData.dojoTblidx;

		g_pDojoWarManager->EraseDojo(dojoData.dojoTblidx);
	}

	CNtlPacket packetSrv(sizeof(sTG_DOJO_SCRAMBLE_STATE_CHANGE));
	sTG_DOJO_SCRAMBLE_STATE_CHANGE * resSrv = (sTG_DOJO_SCRAMBLE_STATE_CHANGE *)packetSrv.GetPacketData();
	resSrv->wOpCode = TG_DOJO_SCRAMBLE_STATE_CHANGE;
	resSrv->dojoTblidx = dojoData.dojoTblidx;
	resSrv->byState = byState;
	resSrv->tmNextStepTime = tmNextStepTime;
	packetSrv.SetPacketLen(sizeof(sTG_DOJO_SCRAMBLE_STATE_CHANGE));
	g_pServerInfoManager->Broadcast(&packetSrv);
}


void CDojo::Reset(BYTE byState, DOJOTIME tmNextStepTime)
{
	m_eDojoState = (eDBO_DOJO_STATUS)byState;
	m_tmNextStepTime = tmNextStepTime;

	CNtlPacket packet(sizeof(sTU_DOJO_SCRAMBLE_INFORM_NFY));
	sTU_DOJO_SCRAMBLE_INFORM_NFY * res = (sTU_DOJO_SCRAMBLE_INFORM_NFY *)packet.GetPacketData();
	res->wOpCode = TU_DOJO_SCRAMBLE_INFORM_NFY;
	res->dojoTblidx = dojoData.dojoTblidx;
	res->byState = byState;
	res->tmNextStepTime = tmNextStepTime;
	packet.SetPacketLen(sizeof(sTU_DOJO_SCRAMBLE_INFORM_NFY));
	if (GetGuild())
		GetGuild()->Broadcast(&packet);

	CNtlPacket packetSrv(sizeof(sTG_DOJO_SCRAMBLE_STATE_CHANGE));
	sTG_DOJO_SCRAMBLE_STATE_CHANGE * resSrv = (sTG_DOJO_SCRAMBLE_STATE_CHANGE *)packetSrv.GetPacketData();
	resSrv->wOpCode = TG_DOJO_SCRAMBLE_STATE_CHANGE;
	resSrv->dojoTblidx = dojoData.dojoTblidx;
	resSrv->byState = byState; // propagate the requested state (e.g., END)
	resSrv->tmNextStepTime = tmNextStepTime;
	packetSrv.SetPacketLen(sizeof(sTG_DOJO_SCRAMBLE_STATE_CHANGE));
	g_pServerInfoManager->Broadcast(&packetSrv);


	CGuild* pGuild = g_pGuildManager->GetGuild(GetDojoData()->challengeGuildId);
	if (pGuild)
	{
		pGuild->SetDojoTblidxWarRequest(INVALID_TBLIDX);
	}

	GetGuild()->SetDojoTblidxWarRequest(INVALID_TBLIDX);

	m_setDefender.clear();
	m_setAttacker.clear();

	dojoData.challengeGuildId = INVALID_GUILDID;
	m_lastChallengerRequest = INVALID_GUILDID;
	m_lastChallengerCharID = INVALID_CHARACTERID;
	m_byRejectCount = 0;
	m_bHasWarRequest = false;
	m_bWarInProgress = false;
	memset(&m_warPoint, NULL, sizeof(m_warPoint));
	m_warPoint.dojoTblidx = dojoData.dojoTblidx;

	g_pDojoWarManager->EraseDojo(dojoData.dojoTblidx);
}


void CDojo::SendDojoState(HSESSION hSession)
{
	//printf("send dojo shit. GuildID %u, tblidx %u, state %u, nextStep %I64u, hSession %u\n", dojoData.guildId, dojoData.dojoTblidx, m_eDojoState, m_tmNextStepTime, hSession);

	if (m_eDojoState == eDBO_DOJO_STATUS_NORMAL)
		return;

	//sTU_DOJO_SCRAMBLE_RECEIVE_INFO_NFY must be send before sTU_DOJO_SCRAMBLE_INFORM_NFY

	if (m_eDojoState == eDBO_DOJO_STATUS_READY || m_eDojoState == eDBO_DOJO_STATUS_START)
	{
		CGuild* pOpponent = g_pGuildManager->GetGuild(dojoData.challengeGuildId);
		if (m_pkGuild && pOpponent)
		{
			CNtlPacket packet2(sizeof(sTU_DOJO_SCRAMBLE_RECEIVE_INFO_NFY));
			sTU_DOJO_SCRAMBLE_RECEIVE_INFO_NFY * res2 = (sTU_DOJO_SCRAMBLE_RECEIVE_INFO_NFY *)packet2.GetPacketData();
			res2->wOpCode = TU_DOJO_SCRAMBLE_RECEIVE_INFO_NFY;
			res2->dojoTblidx = dojoData.dojoTblidx;
			res2->defGuildId = dojoData.guildId;
			res2->attGuildId = dojoData.challengeGuildId;
			NTL_SAFE_WCSCPY(res2->wszDefGuildName, m_pkGuild->GetInfo()->wszName);
			NTL_SAFE_WCSCPY(res2->wszAttGuildName, pOpponent->GetInfo()->wszName);
			memcpy(&res2->sDefMark, &m_pkGuild->GetInfo()->sMark, sizeof(sDBO_GUILD_MARK));
			memcpy(&res2->sAttMark, &pOpponent->GetInfo()->sMark, sizeof(sDBO_GUILD_MARK));
			packet2.SetPacketLen(sizeof(sTU_DOJO_SCRAMBLE_RECEIVE_INFO_NFY));
			g_pApp->Send(hSession, &packet2);
		}
	}

	CNtlPacket packet(sizeof(sTU_DOJO_SCRAMBLE_INFORM_NFY));
	sTU_DOJO_SCRAMBLE_INFORM_NFY * res = (sTU_DOJO_SCRAMBLE_INFORM_NFY *)packet.GetPacketData();
	res->wOpCode = TU_DOJO_SCRAMBLE_INFORM_NFY;
	res->dojoTblidx = dojoData.dojoTblidx;
	res->byState = m_eDojoState;
	res->tmNextStepTime = m_tmNextStepTime;
	packet.SetPacketLen(sizeof(sTU_DOJO_SCRAMBLE_INFORM_NFY));
	g_pApp->Send(hSession, &packet);
}

void CDojo::UpdateWarPoints(sDOJO_SCRAMBLE_POINT & warPoint)
{
	m_warPoint = warPoint;
	//printf("UpdateWarPoints: dojoTblidx %u, byDefCount %u, byAttCount %u, byDefSealCount %u, byAttSealCount %u, dwDefPoint %u, dwAttPoint %u, dwDefCharge %u, dwAttCharge %u\n", 
	//	warPoint.dojoTblidx, warPoint.byDefCount, warPoint.byAttCount, warPoint.byDefSealCount, warPoint.byAttSealCount, warPoint.dwDefPoint, warPoint.dwAttPoint, warPoint.dwDefCharge, warPoint.dwAttCharge);
	CNtlPacket packet(sizeof(sTU_DOJO_SCRAMBLE_POINT_NFY));
	sTU_DOJO_SCRAMBLE_POINT_NFY * res = (sTU_DOJO_SCRAMBLE_POINT_NFY *)packet.GetPacketData();
	res->wOpCode = TU_DOJO_SCRAMBLE_POINT_NFY;
	res->sData = warPoint;
	packet.SetPacketLen(sizeof(sTU_DOJO_SCRAMBLE_POINT_NFY));

	if (GetGuild())
		GetGuild()->Broadcast(&packet);

	CGuild* pOpponent = g_pGuildManager->GetGuild(dojoData.challengeGuildId);
	if (pOpponent)
		pOpponent->Broadcast(&packet);
}

void CDojo::UpdatePlayerCount(GUILDID guildId, CHARACTERID charId, bool bPlus)
{
	if (m_eDojoState == eDBO_DOJO_STATUS_READY || m_eDojoState == eDBO_DOJO_STATUS_START)
	{
		if (dojoData.guildId == guildId)
		{
			if (bPlus)
				m_setDefender.insert(charId);
			else
			{
				if (m_setDefender.find(charId) != m_setDefender.end())
					m_setDefender.erase(charId);
				else
					return;
			}

			m_warPoint.byDefCount = (BYTE)m_setDefender.size();
		}
		else
		{
			if (bPlus)
				m_setAttacker.insert(charId);
			else
			{
				if (m_setAttacker.find(charId) != m_setAttacker.end())
					m_setAttacker.erase(charId);
				else
					return;
			}

			m_warPoint.byAttCount = (BYTE)m_setAttacker.size();
		}

		CNtlPacket packet(sizeof(sTG_DOJO_USERCOUNT_CHANGE));
		sTG_DOJO_USERCOUNT_CHANGE * res = (sTG_DOJO_USERCOUNT_CHANGE *)packet.GetPacketData();
		res->wOpCode = TG_DOJO_USERCOUNT_CHANGE;
		res->dojoTblidx = dojoData.dojoTblidx;
		res->byDefCount = m_warPoint.byDefCount;
		res->byAttCount = m_warPoint.byAttCount;
		packet.SetPacketLen(sizeof(sTG_DOJO_USERCOUNT_CHANGE));
		g_pServerInfoManager->Broadcast(&packet);
	}
}

void CDojo::UpdateSeal(GUILDID guildId, TBLIDX objectTblidx)
{
	//printf("UpdateSeal: guildId %u, objectTblidx %u \n", guildId, objectTblidx);

	CNtlPacket packet(sizeof(sTU_DOJO_SCRAMBLE_SHIEFT_SEAL_STATE_NFY));
	sTU_DOJO_SCRAMBLE_SHIEFT_SEAL_STATE_NFY * res = (sTU_DOJO_SCRAMBLE_SHIEFT_SEAL_STATE_NFY *)packet.GetPacketData();
	res->wOpCode = TU_DOJO_SCRAMBLE_SHIEFT_SEAL_STATE_NFY;
	res->dojoTblidx = dojoData.dojoTblidx;
	res->guildId = guildId;
	res->objectTblidx = objectTblidx;
	packet.SetPacketLen(sizeof(sTU_DOJO_SCRAMBLE_SHIEFT_SEAL_STATE_NFY));

	if (GetGuild())
		GetGuild()->Broadcast(&packet);

	CGuild* pOpponent = g_pGuildManager->GetGuild(dojoData.challengeGuildId);
	if (pOpponent)
		pOpponent->Broadcast(&packet);
}

bool CDojo::CanEnterDojo(GUILDID guildId)
{
	//printf("byDefCount %u, byAttCount %u \n", m_warPoint.byDefCount, m_warPoint.byAttCount);
	if (m_eDojoState == eDBO_DOJO_STATUS_READY || m_eDojoState == eDBO_DOJO_STATUS_START)
	{
		if (dojoData.guildId == guildId)
		{
			if (m_warPoint.byDefCount >= DBO_DOJO_SCRAMBLE_MAX_MEMBER) // DBO_DOJO_SCRAMBLE_MAX_MEMBER
				return false;
		}
		else
		{
			if (m_warPoint.byAttCount >= DBO_DOJO_SCRAMBLE_MAX_MEMBER) // DBO_DOJO_SCRAMBLE_MAX_MEMBER
				return false;
		}

		return true;
	}

	return false;
}


