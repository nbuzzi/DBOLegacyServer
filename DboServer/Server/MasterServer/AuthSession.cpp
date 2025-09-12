#include "stdafx.h"
#include "MasterServer.h"
#include "PacketHead.h"
#include "IpGuard.h"
extern IpGuard g_ipGuard;

////////////////////////////////////////////////////////////////////////////////////////////////
//// RECEIVE PACKETS FROM CONNECTED SERVERS
////////////////////////////////////////////////////////////////////////////////////////////////

int CAuthServerPassiveSession::OnAccept()
{
	const char* rip = GetRemoteIP();
	std::string ip = rip ? rip : "";

	std::string reason;
	if (!g_ipGuard.OnAccept(ip, reason)) {
		ERR_LOG(LOG_NETWORK, "DROP %s (Auth): %s", ip.c_str(), reason.c_str());
		Disconnect(false);
		return NTL_SUCCESS;
	}

	NTL_PRINT(PRINT_APP, "AUTH SERVER CONNECTED");
	return CNtlSession::OnAccept();
}


void CAuthServerPassiveSession::OnClose()
{
	const char* rip = GetRemoteIP();
	if (rip) g_ipGuard.OnClose(rip);

	NTL_PRINT(PRINT_APP, "AUTH SERVER %u DISCONNECTED", serverIndex);
	g_pSrvMgr->SetServerOff(NTL_SERVER_TYPE_AUTH, 0, 0, serverIndex);
}


int CAuthServerPassiveSession::OnDispatch(CNtlPacket * pPacket)
{
	CMasterServer * app = (CMasterServer*) NtlSfxGetApp();
	sNTLPACKETHEADER * pHeader = (sNTLPACKETHEADER *)pPacket->GetPacketData();

	//if(pHeader->wOpCode != AM_PING_RES) //PING opcode
	//	printf("Auth Server | pHeader->wOpCode %d\n", pHeader->wOpCode);

	switch( pHeader->wOpCode )
	{
		case AM_NOTIFY_SERVER_BEGIN:	{	Am_NfyServerBegin(pPacket,app);	}	break;
		case AM_ON_PLAYER_CHECK_REQ:	{	Am_CheckPlayerOnline(pPacket,app);	}	break;


		case AM_PING_RES:
		{
			sAM_PING_RES * req = (sAM_PING_RES*)pPacket->GetPacketData();

			ResetAliveTime();

			DWORD dwTick = GetTickCount();
			if (dwTick - req->dwTick > 10)
				ERR_LOG(LOG_NETWORK, "Auth Server Ping: %d ", dwTick - req->dwTick);
		}
		break;

		default: ERR_LOG(LOG_NETWORK, "Auth: Undefined Packet. wOpCode = %u", pHeader->wOpCode); break;
	}
	

	return NTL_SUCCESS;
}