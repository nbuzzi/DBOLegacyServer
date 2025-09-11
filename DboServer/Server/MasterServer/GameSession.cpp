#include "stdafx.h"
#include "PacketHead.h"
#include "MasterServer.h"
#include "IpGuard.h"
extern IpGuard g_ipGuard;

////////////////////////////////////////////////////////////////////////////////////////////////
//// RECEIVE PACKETS FROM CONNECTED SERVERS
////////////////////////////////////////////////////////////////////////////////////////////////

int CGameServerPassiveSession::OnAccept()
{
	const char* rip = GetRemoteIP();
	std::string ip = rip ? rip : "";

	if (ip != "127.0.0.1") {
		ERR_LOG(LOG_NETWORK, "DROP %s (Game): not loopback", ip.c_str());
		Disconnect(false);
		return NTL_SUCCESS;
	}

	std::string reason;
	if (!g_ipGuard.OnAccept(ip, reason)) {
		ERR_LOG(LOG_NETWORK, "DROP %s (Game): %s", ip.c_str(), reason.c_str());
		Disconnect(false);
		return NTL_SUCCESS;
	}

	NTL_PRINT(PRINT_APP, "GAME SERVER CONNECTED");
	return CNtlSession::OnAccept();
}


void CGameServerPassiveSession::OnClose()
{
	const char* rip = GetRemoteIP();
	std::string ip = rip ? rip : "";
	if (rip) g_ipGuard.OnClose(rip);

	NTL_PRINT(PRINT_APP, "GAME SERVER (server %u channel %u) DISCONNECTED, IP: %s", serverFarmID, serverChannelID, ip.c_str());
	g_pSrvMgr->SetServerOff(NTL_SERVER_TYPE_GAME, serverFarmID, serverChannelID, serverChannelID);
}


int CGameServerPassiveSession::OnDispatch(CNtlPacket* pPacket)
{
	CMasterServer* app = (CMasterServer*)NtlSfxGetApp();
	sNTLPACKETHEADER* pHeader = (sNTLPACKETHEADER*)pPacket->GetPacketData();

	switch (pHeader->wOpCode)
	{
	case GM_NOTIFY_SERVER_BEGIN: { Gm_NfyServerBegin(pPacket, app); } break;
	case GM_LOGIN_REQ: { Gm_PlayerEnter(pPacket, app); } break;
	case GM_LOGOUT_REQ: { Gm_PlayerLeave(pPacket, app); } break;
	case GM_MOVE_REQ: { Gm_PlayerMove(pPacket, app); } break;

	case GM_PLAYER_SWITCH_CHANNEL_REQ: { Gm_PlayerSwitchChannel(pPacket, app); } break;
	case GM_CHAR_SERVER_TELEPORT_REQ: { RecvCharServerTeleportReq(pPacket, app); } break;

	case GM_PING_RES:
	{
		sGM_PING_RES* req = (sGM_PING_RES*)pPacket->GetPacketData();

		ResetAliveTime();

		DWORD dwTick = GetTickCount();
		if (dwTick - req->dwTick > 10)
			ERR_LOG(LOG_GENERAL, "Game Server %d Channel %d Ping: %d ", req->serverId, req->serverChannelId, dwTick - req->dwTick);
	}
	break;

	case GM_SERVER_SHUT_DOWN:	Gm_RecvServerShutdownNfy(pPacket, app);	break;

	case GM_DRAGONBALL_SCRAMBLE_SEASON_STATE_NFY: RecvDragonballScrambleSeasonStateNfy(pPacket, app); break;

	default: ERR_LOG(LOG_NETWORK, "Game: Undefined Packet. wOpCode = %u", pHeader->wOpCode); break;
	}


	return NTL_SUCCESS;
}