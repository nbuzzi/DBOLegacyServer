#include "stdafx.h"
#include "MasterServer.h"
#include "PacketHead.h"
#include "IpGuard.h"
extern IpGuard g_ipGuard;

////////////////////////////////////////////////////////////////////////////////////////////////
//// RECEIVE PACKETS FROM CONNECTED SERVERS
////////////////////////////////////////////////////////////////////////////////////////////////

int CCharServerPassiveSession::OnAccept()
{
	const char* rip = GetRemoteIP();
	std::string ip = rip ? rip : "";

	std::string reason;
	if (!g_ipGuard.OnAccept(ip, reason)) {
		ERR_LOG(LOG_NETWORK, "DROP %s (Char): %s", ip.c_str(), reason.c_str());
		Disconnect(false);
		return NTL_SUCCESS;
	}

	NTL_PRINT(PRINT_APP, "CHAR SERVER CONNECTED (%s)", ip.c_str());
	return CNtlSession::OnAccept();
}


void CCharServerPassiveSession::OnClose()
{
	const char* rip = GetRemoteIP();
	if (rip) g_ipGuard.OnClose(rip);

	NTL_PRINT(PRINT_APP, "CHAR SERVER %u DISCONNECTED", serverIndex);
	g_pSrvMgr->SetServerOff(NTL_SERVER_TYPE_CHARACTER, 0, 0, serverIndex);
}


int CCharServerPassiveSession::OnDispatch(CNtlPacket * pPacket)
{
	CMasterServer * app = (CMasterServer*) NtlSfxGetApp();
	sNTLPACKETHEADER * pHeader = (sNTLPACKETHEADER *)pPacket->GetPacketData();

//	if(pHeader->wOpCode != CM_PING_REQ) //PING opcode
//		printf("Char Server | pHeader->wOpCode %d \n", pHeader->wOpCode);
	switch( pHeader->wOpCode )
	{
		case CM_NOTIFY_SERVER_BEGIN: {	Cm_NfyServerBegin(pPacket,app);	} break;
		case CM_LOGIN_REQ:  {	Cm_UserLogin(pPacket,app);	} break;
		case CM_LOGOUT_REQ:  {	Cm_UserLeave(pPacket,app);	} break;
		case CM_CHARACTER_EXIT_REQ:{ Cm_CharExit(pPacket,app);	} break;
		case CM_MOVE_REQ:  {	Cm_UserMove(pPacket,app);	} break;

		case CM_PING_RES:
		{
			sCM_PING_RES * req = (sCM_PING_RES*)pPacket->GetPacketData();

			ResetAliveTime();
			
			DWORD dwTick = GetTickCount();
			if (dwTick - req->dwTick > 10)
				ERR_LOG(LOG_NETWORK, "Char Server Ping: %d ", dwTick - req->dwTick);
		}
		break;

		default: ERR_LOG(LOG_NETWORK, "Char: Undefined Packet. wOpCode = %u", pHeader->wOpCode); break;
	}
	

	return NTL_SUCCESS;
}