#include "stdafx.h"
#include "gm.h"
#include "GameServer.h"
#include "CPlayer.h"
#include "Monster.h"
#include "ItemManager.h"
#include "NtlPacketUG.h"
#include "NtlPacketGU.h"
#include "NtlPacketGQ.h"
#include "NtlPacketGT.h"
#include "NtlPacketGM.h"
#include "NtlResultCode.h"
#include "NtlMrPoPoMsg.h"
#include "SubNeighborServerInfoManager.h"
#include "NtlTokenizer.h"
#include "TableContainerManager.h"
#include "ItemTable.h"
#include "ExpTable.h"
#include "GameProcessor.h"
#include "HLSItemTable.h"
#include "SystemEffectTable.h"
#include "TriggerObject.h"
#include "GameMain.h"
#include "DragonballScramble.h"
#include "DragonballHunt.h"
#include "HoneyBeeEvent.h"
#include "ItemDrop.h"
#include "PortalTable.h"
#include "NtlPacketTU.h"
#include "NtlStringW.h"
#include "DungeonManager.h"
#include "StoneDropEvent.h"
#include "Fairy Event.h"
#include "CustomDropEvent.h"
#include "HelperNpcManager.h"
#include "Guild.h"
#include "BudokaiManager.h"
#include "PlayerModifiers.h"
#include "FeatureFlags.h"
#include "VirtualTransformationManager.h"
#include "ArenaManager.h"
#include "DojoManager.h"
#include "EventManager.h"
#include "BattlePassManager.h"
#include <algorithm>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

// Helper function to send commands to other GameServer channels via command socket
static bool SendCommandToChannel(BYTE byTargetChannel, const std::string& sCmd, std::string* pResponse = nullptr)
{
	// Calculate target port (6666 + channel number)
	int targetPort = 6666 + byTargetChannel;

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		return false;

	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET)
	{
		WSACleanup();
		return false;
	}

	// Set timeout to avoid hanging
	DWORD timeout = 3000; // 3 seconds
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
	setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));

	sockaddr_in serverAddr{};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	serverAddr.sin_port = htons(targetPort);

	if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		closesocket(sock);
		WSACleanup();
		return false;
	}

	std::string cmdWithNewline = sCmd + "\n";
	send(sock, cmdWithNewline.c_str(), (int)cmdWithNewline.length(), 0);

	char buffer[256] = { 0 };
	int bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
	bool success = (bytes > 0 && strstr(buffer, "OK") != nullptr);

	if (pResponse && bytes > 0)
	{
		buffer[bytes] = '\0';
		*pResponse = std::string(buffer);
	}

	closesocket(sock);
	WSACleanup();

	return success;
}

void gm_read_command(sUG_SERVER_COMMAND* sPacket, CPlayer* pPlayer)
{
	CGameServer* app = (CGameServer*)g_pApp;

	WCHAR wchBuffer[1024];
	wcscpy_s(wchBuffer, sizeof(wchBuffer) / sizeof(wchBuffer[0]), sPacket->awchCommand);

	CNtlTokenizerW lexer(wchBuffer);
	int iLine;
	int icmd;

	std::wstring strCommand = lexer.PeekNextToken(NULL, &iLine);

	for (icmd = 0; icmd < NTL_MAX_LENGTH_OF_CHAT_MESSAGE; icmd++)
	{
		if (!wcscmp(cmd_info[icmd].command, L"@qwasawedsadas"))
			return;
		else if (!wcscmp(cmd_info[icmd].command, strCommand.c_str()))
			break;
	}

	if (cmd_info[icmd].eAdminLevel > pPlayer->GetGMLevel()) //check if player gm level high enough to use command
		return;

	if (cmd_info[icmd].eAdminLevel > ADMIN_LEVEL_EARLY_ACCESS && pPlayer->IsGameMaster() == false)
		return;

	//log
	CNtlPacket packetQry(sizeof(sGQ_GM_LOG));
	sGQ_GM_LOG* resQry = (sGQ_GM_LOG*)packetQry.GetPacketData();
	resQry->wOpCode = GQ_GM_LOG;
	resQry->byLogType = 0;
	resQry->charId = pPlayer->GetCharID();
	wcscpy_s(resQry->wchBuffer, wchBuffer);
	packetQry.SetPacketLen(sizeof(sGQ_GM_LOG)); // to avoid having huge packet
	app->SendTo(app->GetQueryServerSession(), &packetQry);

	((*cmd_info[icmd].command_pointer) (pPlayer, &lexer, iLine));
}




ACMD(do_setspeed);
ACMD(do_addmob);
ACMD(do_addmobgroup);
ACMD(do_addnpc);
ACMD(do_additem);
ACMD(do_additem_group);
ACMD(do_sessioninfo);
ACMD(do_sessioncleanup);
ACMD(do_budokaiinfo);
ACMD(do_addmasteritem); // this function will give
ACMD(do_addskill);
ACMD(do_addskill2); // this function add missing master class passive. Example.. if player is swordsman and dont have swordsman masterclass skill, then this can be used.
ACMD(do_r);
ACMD(do_addhtb);
ACMD(do_setzenny);
ACMD(do_setlevel);
ACMD(do_setlevel2);
ACMD(do_hide);
ACMD(do_notice);
ACMD(do_pm);
ACMD(do_teleport);
ACMD(do_world);
ACMD(do_warp);
ACMD(do_call);
ACMD(do_shutdown);
ACMD(do_setadult);
ACMD(do_setclass);
ACMD(do_changeclass);
ACMD(do_dc);
ACMD(do_kill);
ACMD(do_delallitems);
ACMD(do_god);
ACMD(do_invincible);
ACMD(do_bann);
ACMD(do_dbann);
ACMD(do_purge);
ACMD(do_unstuck);
ACMD(do_warfog);
ACMD(do_upgrade);
ACMD(do_setitemrank);
ACMD(do_mute);
ACMD(do_unmute);
ACMD(do_go);
ACMD(do_addtitle);
ACMD(do_deltitle);
ACMD(do_setitemduration);
ACMD(do_bind);
ACMD(do_exp);
ACMD(do_resetexp);
ACMD(do_startevent);
ACMD(do_stophoneybee);
ACMD(do_deleteguild);
ACMD(do_cancelah);
ACMD(do_addmudosa);
ACMD(do_start);
ACMD(do_createloot);
ACMD(do_test);
ACMD(do_TeleportAll);
ACMD(do_PlayerCount);
ACMD(do_fly);
ACMD(do_BatleEvent);
ACMD(do_notify);
ACMD(do_resetskills);
ACMD(do_setdark);
ACMD(do_buff);
ACMD(do_start_dbhunt);
ACMD(do_stop_dbhunt);
ACMD(do_start_dbscramble);
ACMD(do_stop_dbscramble);
ACMD(do_big);
ACMD(do_start_stonedrop);
ACMD(do_stop_stonedrop);
ACMD(do_start_customdrop);
ACMD(do_stop_customdrop);
ACMD(do_reload_customdrop_cfg);
ACMD(do_customdrop_chainspawns);
ACMD(do_customdrop_healmul);
ACMD(do_customdrop_buffduration);
ACMD(do_reload_playermods_cfg);
ACMD(do_playermods_toggle);
ACMD(do_vtransform);
ACMD(do_vtransform_end);
ACMD(do_reload_helpernpc_cfg);
ACMD(do_helpernpc_metrics);
ACMD(do_helpernpc_resetmetrics);
ACMD(do_helpernpc_refresh);
ACMD(do_arena);
ACMD(do_arena_join_public);
ACMD(do_arena_joinparty_public);
ACMD(do_arena_joinguild_public);
ACMD(do_event);
ACMD(do_event_reload);
ACMD(do_event_participate);
ACMD(do_world_fight);
ACMD(do_budokai);
ACMD(do_dojo);
ACMD(do_budokai_findteam);
ACMD(do_budokai_start);
ACMD(do_battlepass);
ACMD(do_battlepass_setxp);
ACMD(do_battlepass_addxp);
ACMD(do_battlepass_status);
ACMD(do_battlepass_flush);
ACMD(do_battlepass_master);

struct command_info cmd_info[] =
{
	// Everyone
	{ L"@unstuck", do_unstuck, ADMIN_LEVEL_NONE }, // This command is used to teleport you back to your Popo Stone if you are stuck.
	{ L"@online", do_PlayerCount, ADMIN_LEVEL_NONE }, // Shows amount of online players.
	{ L"@exp", do_exp, ADMIN_LEVEL_NONE }, // (on/off) This command toggles whether or not your character continues to gain experience. It is used to continue to farm without gaining any more levels, to gather gear for kid budokai.
	{ L"@resetexp", do_resetexp, ADMIN_LEVEL_NONE }, // This command resets your experience to 0.
	{ L"@addmasteritem", do_addmasteritem, ADMIN_LEVEL_NONE }, // This command adds the item used to complete the master quest for your class, in case you lose it by accident. Can only be used after you accept it from Korin.
	{ L"@addskill2", do_addskill2, ADMIN_LEVEL_NONE }, // This command unlocks the master class passive skill, in case a bug causes you to lose it. Can only be used after you have unlocked your master class.
	{ L"@addhtb", do_addhtb, ADMIN_LEVEL_NONE }, // This command unlocks the main htb and master class htb, if you have already learned them. This is meant to be used after a skill reset. Can only be used after you have unlocked your master class.
	{ L"@arenajoin", do_arena_join_public, ADMIN_LEVEL_NONE }, // Public: self-join Arena (no target parameter)
	{ L"@joinarena", do_arena_join_public, ADMIN_LEVEL_NONE }, // Alias
	{ L"@arenajoinparty", do_arena_joinparty_public, ADMIN_LEVEL_NONE }, // Public: party join (leader only)
	{ L"@arenajoinguild", do_arena_joinguild_public, ADMIN_LEVEL_NONE }, // Public: guild join (guild member only)
	{ L"@participate", do_event_participate, ADMIN_LEVEL_NONE }, // Public: join EVENT channel
	{ L"@findteam", do_budokai_findteam, ADMIN_LEVEL_NONE }, // Public: join Team Budokai matchmaking queue
	{ L"@battlepass", do_battlepass, ADMIN_LEVEL_NONE }, // Show your Battle Pass progress


	// Admin (Full GM access required)
	{ L"@mute", do_mute, ADMIN_LEVEL_ADMIN },
	{ L"@unmute", do_unmute, ADMIN_LEVEL_ADMIN },
	{ L"@teleport", do_teleport, ADMIN_LEVEL_ADMIN },
	{ L"@all", do_TeleportAll, ADMIN_LEVEL_ADMIN },
	{ L"@setlevel", do_setlevel, ADMIN_LEVEL_ADMIN },
	{ L"@setclass", do_setclass, ADMIN_LEVEL_ADMIN },
	{ L"@fly", do_fly, ADMIN_LEVEL_ADMIN },
	{ L"@addtitle", do_addtitle, ADMIN_LEVEL_ADMIN },
	{ L"@deltitle", do_deltitle, ADMIN_LEVEL_ADMIN },
	{ L"@big", do_big, ADMIN_LEVEL_ADMIN },
	{ L"@startevent", do_startevent, ADMIN_LEVEL_ADMIN }, // 0 honey, 1 Fairy
	{ L"@start_customdrop", do_start_customdrop, ADMIN_LEVEL_ADMIN },
	{ L"@start_stonedrop", do_start_stonedrop, ADMIN_LEVEL_ADMIN },
	{ L"@stop_stonedrop", do_stop_stonedrop, ADMIN_LEVEL_ADMIN },
	{ L"@dc", do_dc, ADMIN_LEVEL_ADMIN },
	{ L"@bann", do_bann, ADMIN_LEVEL_ADMIN },
	{ L"@budokai", do_budokai, ADMIN_LEVEL_ADMIN },
	{ L"@dojo", do_dojo, ADMIN_LEVEL_ADMIN },
	{ L"@budokaistart", do_budokai_start, ADMIN_LEVEL_ADMIN }, // Start Budokai on channel 9 remotely
	{ L"@createloot", do_createloot, ADMIN_LEVEL_ADMIN },
	{ L"@additem", do_additem, ADMIN_LEVEL_ADMIN },
	{ L"@event", do_event, ADMIN_LEVEL_ADMIN },
	{ L"@budokaiinfo", do_budokaiinfo, ADMIN_LEVEL_ADMIN },
	{ L"@hide", do_hide, ADMIN_LEVEL_ADMIN },
	{ L"@setadult", do_setadult, ADMIN_LEVEL_ADMIN },
	{ L"@appear", do_warp, ADMIN_LEVEL_ADMIN },
	{ L"@cc", do_notify, ADMIN_LEVEL_ADMIN },
	{ L"@call", do_call, ADMIN_LEVEL_ADMIN },
	{ L"@setspeed", do_setspeed, ADMIN_LEVEL_ADMIN },
	{ L"@changeclass", do_changeclass, ADMIN_LEVEL_GAME_MASTER },
	{ L"@addmob", do_addmob, ADMIN_LEVEL_ADMIN },
	{ L"@addmobgroup", do_addmobgroup, ADMIN_LEVEL_ADMIN },
	{ L"@addnpc", do_addnpc, ADMIN_LEVEL_ADMIN },
	{ L"@additem_group", do_additem_group, ADMIN_LEVEL_ADMIN },
	{ L"@sessioninfo", do_sessioninfo, ADMIN_LEVEL_ADMIN },
	{ L"@sessioncleanup", do_sessioncleanup, ADMIN_LEVEL_ADMIN },
	{ L"@addskill", do_addskill, ADMIN_LEVEL_ADMIN },
	{ L"@heal", do_r, ADMIN_LEVEL_ADMIN },
	{ L"@setzenny", do_setzenny, ADMIN_LEVEL_ADMIN },
	{ L"@delallitems", do_delallitems, ADMIN_LEVEL_ADMIN },
	{ L"@shutdown", do_shutdown, ADMIN_LEVEL_ADMIN },
	{ L"@kill", do_kill, ADMIN_LEVEL_ADMIN },
	{ L"@god", do_god, ADMIN_LEVEL_ADMIN },
	{ L"@invincible", do_invincible, ADMIN_LEVEL_ADMIN },
	{ L"@dbann", do_dbann, ADMIN_LEVEL_ADMIN },
	{ L"@purge", do_purge, ADMIN_LEVEL_ADMIN },
	{ L"@notice", do_notice, ADMIN_LEVEL_ADMIN },
	{ L"@world", do_world, ADMIN_LEVEL_ADMIN },
	{ L"@warfog", do_warfog, ADMIN_LEVEL_ADMIN },
	{ L"@upgrade", do_upgrade, ADMIN_LEVEL_ADMIN },
	{ L"@battlepass_setxp", do_battlepass_setxp, ADMIN_LEVEL_ADMIN }, // Set current level XP (absolute, dev/admin)
	{ L"@battlepass_addxp", do_battlepass_addxp, ADMIN_LEVEL_ADMIN }, // Add raw XP (bypasses action mapping)
	{ L"@battlepass_master", do_battlepass_master, ADMIN_LEVEL_ADMIN }, // Toggle master enable flag
	{ L"@battlepass_status", do_battlepass_status, ADMIN_LEVEL_ADMIN }, // Show extended Battle Pass system status
	{ L"@battlepass_flush", do_battlepass_flush, ADMIN_LEVEL_ADMIN }, // Force immediate persistence flush
	{ L"@setitemrank", do_setitemrank, ADMIN_LEVEL_ADMIN },
	{ L"@go", do_go, ADMIN_LEVEL_ADMIN },
	{ L"@setitemduration", do_setitemduration, ADMIN_LEVEL_ADMIN },
	{ L"@bind", do_bind, ADMIN_LEVEL_ADMIN },
	{ L"@stopevent", do_stophoneybee, ADMIN_LEVEL_ADMIN }, // 0 Honey, 1 Fairy
	{ L"@deleteguild", do_deleteguild, ADMIN_LEVEL_ADMIN },
	{ L"@cancelah", do_cancelah, ADMIN_LEVEL_ADMIN },
	{ L"@addmudosa", do_addmudosa, ADMIN_LEVEL_ADMIN },
	{ L"@startgm", do_start, ADMIN_LEVEL_ADMIN },
	{ L"@test", do_test, ADMIN_LEVEL_ADMIN },
	{ L"@PvpEvent", do_BatleEvent, ADMIN_LEVEL_ADMIN },
	{ L"@level", do_setlevel2, ADMIN_LEVEL_ADMIN },
	{ L"@resetskills", do_resetskills, ADMIN_LEVEL_ADMIN },
	{ L"@setdark", do_setdark, ADMIN_LEVEL_GAME_MASTER},
	{ L"@buff", do_buff, ADMIN_LEVEL_ADMIN },
	{ L"@start_dbhunt",do_start_dbhunt,ADMIN_LEVEL_GAME_MASTER},
	{ L"@stop_dbhunt",do_stop_dbhunt,ADMIN_LEVEL_GAME_MASTER},
	{ L"@start_dbscramble",do_start_dbscramble,ADMIN_LEVEL_GAME_MASTER},
	{ L"@stop_dbscramble",do_stop_dbscramble,ADMIN_LEVEL_GAME_MASTER},
	{ L"@stop_customdrop", do_stop_customdrop, ADMIN_LEVEL_GAME_MASTER },
	{ L"@reload_customdrop", do_reload_customdrop_cfg, ADMIN_LEVEL_GAME_MASTER },
	{ L"@reload_helpernpc", do_reload_helpernpc_cfg, ADMIN_LEVEL_GAME_MASTER },
	{ L"@helpernpc_metrics", do_helpernpc_metrics, ADMIN_LEVEL_GAME_MASTER },
	{ L"@helpernpc_resetmetrics", do_helpernpc_resetmetrics, ADMIN_LEVEL_GAME_MASTER },
	{ L"@helpernpc_refresh", do_helpernpc_refresh, ADMIN_LEVEL_GAME_MASTER },
	{ L"@customdrop_chainspawns", do_customdrop_chainspawns, ADMIN_LEVEL_GAME_MASTER },
	{ L"@customdrop_healmul", do_customdrop_healmul, ADMIN_LEVEL_GAME_MASTER },
	{ L"@customdrop_buffduration", do_customdrop_buffduration, ADMIN_LEVEL_GAME_MASTER },
	{ L"@reload_playermods", do_reload_playermods_cfg, ADMIN_LEVEL_GAME_MASTER },
	{ L"@playermods", do_playermods_toggle, ADMIN_LEVEL_GAME_MASTER },
	{ L"@vtransform", do_vtransform, ADMIN_LEVEL_GAME_MASTER },
	{ L"@vtransform_end", do_vtransform_end, ADMIN_LEVEL_GAME_MASTER },
	{ L"@arena", do_arena, ADMIN_LEVEL_GAME_MASTER },
	{ L"@event_reload", do_event_reload, ADMIN_LEVEL_GAME_MASTER },
	{ L"@world_fight", do_world_fight, ADMIN_LEVEL_GAME_MASTER },
	{ L"@pm", do_pm, ADMIN_LEVEL_GAME_MASTER },

	{ L"@qwasawedsadas", NULL, ADMIN_LEVEL_ADMIN }
};

// ------------------------------------------------------------
// Battle Pass GM / Public helper commands
// ------------------------------------------------------------
ACMD(do_battlepass)
{
	if (!g_pBattlePassManager->IsMasterEnabled())
	{
		CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
		res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
		res->byDisplayType = SERVER_TEXT_SYSNOTICE;
		const wchar_t* txt = L"[BattlePass] System disabled (master switch).";
		res->wMessageLengthInUnicode = (WORD)wcslen(txt);
		wcsncpy_s(res->awchMessage, NTL_MAX_LENGTH_OF_CHAT_MESSAGE + 1, txt, _TRUNCATE);
		packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		pPlayer->SendPacket(&packet);
		return;
	}
	auto* prog = g_pBattlePassManager->GetProgress(pPlayer->GetCharID());
	wchar_t msg[256];
	unsigned need = g_pBattlePassManager->GetXpForLevel(prog->level);
	swprintf_s(msg, L"[BattlePass] Season %u Level %u (%u/%u XP) TotalXP=%u", prog->seasonId, prog->level, prog->xp, need, prog->totalXp);
	CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
	res->wOpCode = GU_SYSTEM_DISPLAY_TEXT; res->byDisplayType = SERVER_TEXT_SYSNOTICE; res->wMessageLengthInUnicode = (WORD)wcslen(msg);
	wcsncpy_s(res->awchMessage, NTL_MAX_LENGTH_OF_CHAT_MESSAGE + 1, msg, _TRUNCATE);
	packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	pPlayer->SendPacket(&packet);
}

// ADMIN: Set current level XP (not total). Does not recalc level.
ACMD(do_battlepass_setxp)
{
	if (!g_pBattlePassManager->IsMasterEnabled()) return;
	std::wstring tok = pToken->PeekNextToken(NULL, &iLine);
	if (tok.empty()) return;
	unsigned val = (unsigned)_wtoi(tok.c_str());
	auto* prog = g_pBattlePassManager->GetProgress(pPlayer->GetCharID());
	prog->xp = val;
	CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
	res->wOpCode = GU_SYSTEM_DISPLAY_TEXT; res->byDisplayType = SERVER_TEXT_SYSNOTICE; const wchar_t* txt = L"[BattlePass] XP set.";
	res->wMessageLengthInUnicode = (WORD)wcslen(txt); wcsncpy_s(res->awchMessage, NTL_MAX_LENGTH_OF_CHAT_MESSAGE + 1, txt, _TRUNCATE);
	packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT)); pPlayer->SendPacket(&packet);
}

// ADMIN: Add raw XP (triggers level ups & mudosa rewards).
ACMD(do_battlepass_addxp)
{
	if (!g_pBattlePassManager->IsMasterEnabled()) return;
	std::wstring tok = pToken->PeekNextToken(NULL, &iLine);
	if (tok.empty()) return;
	unsigned add = (unsigned)_wtoi(tok.c_str());
	if (add == 0) return;
	// Reuse internal Award path by synthesizing a temporary action override
	auto* prog = g_pBattlePassManager->GetProgress(pPlayer->GetCharID());
	unsigned beforeLvl = prog->level;
	unsigned beforeXp = prog->xp;
	g_pBattlePassManager->AddActionXp(pPlayer, CBattlePassManager::Action::MOB_KILL, add); // customOverrideXp path used
	wchar_t msg[256];
	swprintf_s(msg, L"[BattlePass] Added %u XP (L%u %u->%u/%u).", add, prog->level, beforeXp, prog->xp, g_pBattlePassManager->GetXpForLevel(prog->level));
	CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
	res->wOpCode = GU_SYSTEM_DISPLAY_TEXT; res->byDisplayType = SERVER_TEXT_SYSNOTICE; res->wMessageLengthInUnicode = (WORD)wcslen(msg);
	wcsncpy_s(res->awchMessage, NTL_MAX_LENGTH_OF_CHAT_MESSAGE + 1, msg, _TRUNCATE);
	packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	pPlayer->SendPacket(&packet);
}

// ADMIN: Show Battle Pass system status (config + runtime counters summary size)
ACMD(do_battlepass_status)
{
	CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
	res->wOpCode = GU_SYSTEM_DISPLAY_TEXT; res->byDisplayType = SERVER_TEXT_SYSNOTICE;
	if (!g_pBattlePassManager->IsEnabled())
	{
		const wchar_t* txt = L"[BattlePass] Disabled (Enabled=0)";
		res->wMessageLengthInUnicode = (WORD)wcslen(txt);
		wcsncpy_s(res->awchMessage, NTL_MAX_LENGTH_OF_CHAT_MESSAGE + 1, txt, _TRUNCATE);
		packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		pPlayer->SendPacket(&packet);
		return;
	}
	bool master = g_pBattlePassManager->IsMasterEnabled();
	const CBattlePassManager::Config& cfg = g_pBattlePassManager->GetConfig();
	wchar_t msg[256];
	swprintf_s(msg, L"[BattlePass] Master=%s Season=%u DB=%s Flush=%us MinDelta=%u", master?L"ON":L"OFF", cfg.seasonId, cfg.useDatabase?L"ON":L"OFF", cfg.flushSeconds, cfg.minDeltaXp);
	res->wMessageLengthInUnicode = (WORD)wcslen(msg);
	wcsncpy_s(res->awchMessage, NTL_MAX_LENGTH_OF_CHAT_MESSAGE + 1, msg, _TRUNCATE);
	packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	pPlayer->SendPacket(&packet);
}

// ADMIN: Force immediate DB/file flush depending on mode
ACMD(do_battlepass_flush)
{
	if (!g_pBattlePassManager->IsEnabled()) return;
	if (!g_pBattlePassManager->IsMasterEnabled()) return;
	if (g_pBattlePassManager->GetConfig().useDatabase)
		g_pBattlePassManager->ForceFlushToDatabase();
	else
		g_pBattlePassManager->ForceAutosave();
	CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
	res->wOpCode = GU_SYSTEM_DISPLAY_TEXT; res->byDisplayType = SERVER_TEXT_SYSNOTICE;
	const wchar_t* txt = L"[BattlePass] Flush triggered.";
	res->wMessageLengthInUnicode = (WORD)wcslen(txt);
	wcsncpy_s(res->awchMessage, NTL_MAX_LENGTH_OF_CHAT_MESSAGE + 1, txt, _TRUNCATE);
	packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	pPlayer->SendPacket(&packet);
}

// ADMIN: Toggle or show master enable flag
ACMD(do_battlepass_master)
{
	// Usage: @battlepass_master            -> show state
	//        @battlepass_master on|off     -> set state
	pToken->PopToPeek();
	std::wstring sub = pToken->PeekNextToken(NULL, &iLine);
	bool queryOnly = sub.empty();
	bool newState = g_pBattlePassManager->IsMasterEnabled();
	if (!queryOnly)
	{
		std::wstring lower = sub;
		for (auto &ch : lower) ch = (wchar_t)towlower(ch);
		if (lower == L"on" || lower == L"1" || lower == L"true") newState = true;
		else if (lower == L"off" || lower == L"0" || lower == L"false") newState = false;
		// apply: direct access to config (safe—admin only command)
		g_pBattlePassManager->GetConfig().enabled; // no-op read to silence potential unused macro expansions
		// We need a setter; modifying internal config via a helper lambda to keep minimal diff
		g_pBattlePassManager->SetMasterEnable(newState);
	}
	bool finalState = g_pBattlePassManager->IsMasterEnabled();
	wchar_t msg[96];
	swprintf_s(msg, L"[BattlePass] MasterEnable is %s", finalState ? L"ON" : L"OFF");
	CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
	res->wOpCode = GU_SYSTEM_DISPLAY_TEXT; res->byDisplayType = SERVER_TEXT_SYSNOTICE;
	res->wMessageLengthInUnicode = (WORD)wcslen(msg);
	wcsncpy_s(res->awchMessage, NTL_MAX_LENGTH_OF_CHAT_MESSAGE + 1, msg, _TRUNCATE);
	packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	pPlayer->SendPacket(&packet);
}
ACMD(do_arena)
{
	// Syntax:
	// @arena start <mode>
	// @arena stop [abort]
	// @arena rotate
	// @arena status
	// @arena join [name]
	// @arena spectate [name]
	// @arena tp participants|spectators|all|here [participants|spectators|all]
	// @arena win [name]
	// @arena award winners|participants
	// @arena map <worldTblidx>
	// @arena spawn <mobTblidx> [count]
	// @arena mobs on|off | perwave <n> | waveseconds <sec> | preset <name|none>
	// @arena joinparty [name]  -- add entire party of player (or self)
	// @arena joinguild [name]  -- add all online guild members of player (or self)
	// @arena cfg show | rankonly | custom on|off | onlycustom on|off | budokai on|off | randomize on|off | rotsec <n> | worlds <csv>
	pToken->PopToPeek();
	std::wstring sub = pToken->PeekNextToken(NULL, &iLine);
	if (sub.empty()) {
		g_pArenaManager->StatusTo(pPlayer);
		return;
	}

	std::string sc = ws2s(sub);
	for (auto& c : sc) c = (char)tolower(c);

	if (sc == "start") {
		pToken->PopToPeek();
		std::wstring wmode = pToken->PeekNextToken(NULL, &iLine);
		std::string smode = ws2s(wmode);
		for (auto& c : smode) c = (char)tolower(c);
		CArenaManager::Mode mode = CArenaManager::Mode::OPEN;
		if (smode == "gvg" || smode == "guild" || smode == "guild_vs_guild") mode = CArenaManager::Mode::GUILD_VS_GUILD;
		else if (smode == "pvp" || smode == "party" || smode == "party_vs_party") mode = CArenaManager::Mode::PARTY_VS_PARTY;
		else if (smode == "ffa" || smode == "free" || smode == "free_for_all") mode = CArenaManager::Mode::FREE_FOR_ALL;
		else mode = CArenaManager::Mode::OPEN;
		g_pArenaManager->Start(mode);
	}
	else if (sc == "stop") {
		pToken->PopToPeek();
		std::wstring warg = pToken->PeekNextToken(NULL, &iLine);
		std::string sarg = ws2s(warg);
		for (auto& c : sarg) c = (char)tolower(c);
		bool abort = (sarg == "abort");
		g_pArenaManager->Stop(abort);
	}
	else if (sc == "rotate") {
		// support: @arena rotate [in <seconds>]
		pToken->PopToPeek();
		std::wstring warg = pToken->PeekNextToken(NULL, &iLine);
		std::string sarg = ws2s(warg);
		for (auto& c : sarg) c = (char)tolower(c);
		if (sarg == "in")
		{
			pToken->PopToPeek();
			std::wstring wsec = pToken->PeekNextToken(NULL, &iLine);
			unsigned int sec = (unsigned int)atoi(ws2s(wsec).c_str());
			if (sec > 0)
				g_pArenaManager->SetRotationSecondsRemaining(sec);
			else
				g_pArenaManager->StatusTo(pPlayer);
		}
		else if (!sarg.empty())
		{
			// if user provided a number directly after rotate, treat as seconds
			bool isNum = isdigit((unsigned char)sarg[0]);
			if (isNum)
			{
				unsigned int sec = (unsigned int)atoi(sarg.c_str());
				if (sec > 0) {
					g_pArenaManager->SetRotationSecondsRemaining(sec);
					return;
				}
			}
			// fallback to immediate rotate
			g_pArenaManager->RotateMapNow();
		}
		else {
			g_pArenaManager->RotateMapNow();
		}
	}
	else if (sc == "time") {
		// @arena time <seconds>  OR  @arena time +<delta>
		pToken->PopToPeek();
		std::wstring wval = pToken->PeekNextToken(NULL, &iLine);
		std::string sval = ws2s(wval);
		if (sval.empty()) { g_pArenaManager->StatusTo(pPlayer); }
		else if (sval[0] == '+' || sval[0] == '-') {
			int delta = atoi(sval.c_str());
			g_pArenaManager->AddRoundTimeSeconds(delta);
		}
		else {
			unsigned int sec = (unsigned int)atoi(sval.c_str());
			if (sec == 0) sec = 1; // avoid zero which would stop immediately
			g_pArenaManager->SetRoundTimeRemaining(sec, true);
		}
	}
	else if (sc == "status") {
		g_pArenaManager->StatusTo(pPlayer);
	}
	else if (sc == "mobs") {
		pToken->PopToPeek();
		std::wstring wopt = pToken->PeekNextToken(NULL, &iLine);
		std::string opt = ws2s(wopt);
		for (auto& c : opt) c = (char)tolower(c);
		if (opt == "on") { g_pArenaManager->SetRandomMobsSpawn(true); }
		else if (opt == "off") { g_pArenaManager->SetRandomMobsSpawn(false); }
		else if (opt == "perwave") {
			pToken->PopToPeek();
			std::wstring wv = pToken->PeekNextToken(NULL, &iLine);
			unsigned int n = (unsigned int)atoi(ws2s(wv).c_str());
			if (n == 0) n = 1; g_pArenaManager->SetRandomMobsPerWave(n);
		}
		else if (opt == "waveseconds") {
			pToken->PopToPeek();
			std::wstring ws = pToken->PeekNextToken(NULL, &iLine);
			unsigned int sec = (unsigned int)atoi(ws2s(ws).c_str());
			if (sec == 0) sec = 1; g_pArenaManager->SetRandomMobsWaveSeconds(sec);
		}
		else if (opt == "preset") {
			pToken->PopToPeek();
			std::wstring wname = pToken->PeekNextToken(NULL, &iLine);
			std::string name = ws2s(wname);
			if (name == "none" || name == "") { g_pArenaManager->SetRandomMobsPreset(""); }
			else {
				if (!g_pArenaManager->SetRandomMobsPreset(name)) {
					CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
					sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
					res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
					res->byDisplayType = SERVER_TEXT_SYSTEM;
					NTL_SAFE_WCSCPY(res->awchMessage, L"[Arena] Unknown preset name.");
					packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
					pPlayer->SendPacket(&packet);
				}
			}
		}
		else {
			g_pArenaManager->StatusTo(pPlayer);
		}
	}
	else if (sc == "cfg") {
		pToken->PopToPeek();
		std::wstring wopt = pToken->PeekNextToken(NULL, &iLine);
		std::string opt = ws2s(wopt); for (auto& c : opt) c = (char)tolower(c);
		if (opt.empty() || opt == "show") {
			g_pArenaManager->ShowCfgTo(pPlayer);
			return;
		}
		if (opt == "rankonly") {
			g_pArenaManager->SetAllowCustomWorlds(false);
			g_pArenaManager->SetUseOnlyCustomWorlds(false);
			g_pArenaManager->RebuildWorldList_RankOnly();
			g_pArenaManager->ShowCfgTo(pPlayer);
			return;
		}
		if (opt == "custom") {
			pToken->PopToPeek(); std::wstring won = pToken->PeekNextToken(NULL, &iLine);
			std::string son = ws2s(won); for (auto& c : son) c = (char)tolower(c);
			bool on = (son == "on" || son == "1" || son == "true");
			g_pArenaManager->SetAllowCustomWorlds(on);
			g_pArenaManager->ShowCfgTo(pPlayer); return;
		}
		if (opt == "onlycustom") {
			pToken->PopToPeek(); std::wstring won = pToken->PeekNextToken(NULL, &iLine);
			std::string son = ws2s(won); for (auto& c : son) c = (char)tolower(c);
			bool on = (son == "on" || son == "1" || son == "true");
			g_pArenaManager->SetUseOnlyCustomWorlds(on);
			g_pArenaManager->ShowCfgTo(pPlayer); return;
		}
		if (opt == "budokai") {
			pToken->PopToPeek(); std::wstring won = pToken->PeekNextToken(NULL, &iLine);
			std::string son = ws2s(won); for (auto& c : son) c = (char)tolower(c);
			bool on = (son == "on" || son == "1" || son == "true");
			g_pArenaManager->SetAllowBudokaiRuleWorlds(on);
			g_pArenaManager->ShowCfgTo(pPlayer); return;
		}
		if (opt == "randomize") {
			pToken->PopToPeek(); std::wstring won = pToken->PeekNextToken(NULL, &iLine);
			std::string son = ws2s(won); for (auto& c : son) c = (char)tolower(c);
			bool on = (son == "on" || son == "1" || son == "true");
			g_pArenaManager->SetRandomizeMapOnStart(on);
			g_pArenaManager->ShowCfgTo(pPlayer); return;
		}
		if (opt == "rotsec") {
			pToken->PopToPeek(); std::wstring wv = pToken->PeekNextToken(NULL, &iLine);
			unsigned int sec = (unsigned int)atoi(ws2s(wv).c_str());
			g_pArenaManager->SetRotationSeconds(sec);
			g_pArenaManager->ShowCfgTo(pPlayer); return;
		}
		if (opt == "worlds") {
			// worlds <csv>
			pToken->PopToPeek();
			std::wstring wcsv = pToken->PeekNextToken(NULL, &iLine);
			std::string csv = ws2s(wcsv);
			g_pArenaManager->SetWorldListCsv(csv);
			g_pArenaManager->ShowCfgTo(pPlayer); return;
		}
		{
			CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
			sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
			res->wOpCode = GU_SYSTEM_DISPLAY_TEXT; res->byDisplayType = SERVER_TEXT_SYSTEM;
			NTL_SAFE_WCSCPY(res->awchMessage, L"Usage: @arena cfg show | rankonly | custom on|off | onlycustom on|off | budokai on|off | randomize on|off | rotsec <n> | worlds <csv>");
			packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT)); pPlayer->SendPacket(&packet);
		}
	}
	else if (sc == "join") {
		// Individual join - warn if inappropriate for team mode
		if (g_pArenaManager->GetState() == CArenaManager::State::ENROLLMENT &&
			(g_pArenaManager->GetMode() == CArenaManager::Mode::PARTY_VS_PARTY ||
				g_pArenaManager->GetMode() == CArenaManager::Mode::GUILD_VS_GUILD)) {
					{
						CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
						sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
						res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
						res->byDisplayType = SERVER_TEXT_SYSTEM;
						NTL_SAFE_WCSCPY(res->awchMessage, L"[Arena] Consider using 'joinparty' or 'joinguild' for team modes for better team coordination.");
						packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
						pPlayer->SendPacket(&packet);
					}
		}

		pToken->PopToPeek();
		std::wstring wname = pToken->PeekNextToken(NULL, &iLine);
		CPlayer* who = pPlayer;
		if (!wname.empty()) {
			CPlayer* found = g_pObjectManager->FindByName(wname.c_str());
			if (found && found->IsInitialized()) who = found;
		}
		if (who) g_pArenaManager->AddParticipant(who);
	}
	else if (sc == "joinparty") {
		// Validate mode for party joining
		if (g_pArenaManager->GetState() == CArenaManager::State::ENROLLMENT &&
			g_pArenaManager->GetMode() != CArenaManager::Mode::PARTY_VS_PARTY &&
			g_pArenaManager->GetMode() != CArenaManager::Mode::FREE_FOR_ALL &&
			g_pArenaManager->GetMode() != CArenaManager::Mode::OPEN) {
				{
					CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
					sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
					res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
					res->byDisplayType = SERVER_TEXT_SYSTEM;
					NTL_SAFE_WCSCPY(res->awchMessage, L"[Arena] Party joining only available for Party vs Party, Free For All, or Open modes.");
					packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
					pPlayer->SendPacket(&packet);
				}
				return;
		}

		pToken->PopToPeek();
		std::wstring wname = pToken->PeekNextToken(NULL, &iLine);
		CPlayer* base = pPlayer;
		if (!wname.empty()) {
			CPlayer* found = g_pObjectManager->FindByName(wname.c_str());
			if (found && found->IsInitialized()) base = found;
		}
		if (base && base->GetParty() && base->GetPartyID() != INVALID_PARTYID) {
			auto fn = [&](CPlayer* mem) { if (mem) g_pArenaManager->AddParticipant(mem); };
			base->GetParty()->ForEachOnlineMember(fn);
		}
		else if (base) {
			g_pArenaManager->AddParticipant(base);
		}
	}
	else if (sc == "joinguild") {
		// Validate mode for guild joining
		if (g_pArenaManager->GetState() == CArenaManager::State::ENROLLMENT &&
			g_pArenaManager->GetMode() != CArenaManager::Mode::GUILD_VS_GUILD &&
			g_pArenaManager->GetMode() != CArenaManager::Mode::FREE_FOR_ALL &&
			g_pArenaManager->GetMode() != CArenaManager::Mode::OPEN) {
				{
					CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
					sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
					res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
					res->byDisplayType = SERVER_TEXT_SYSTEM;
					NTL_SAFE_WCSCPY(res->awchMessage, L"[Arena] Guild joining only available for Guild vs Guild, Free For All, or Open modes.");
					packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
					pPlayer->SendPacket(&packet);
				}
				return;
		}

		pToken->PopToPeek();
		std::wstring wname = pToken->PeekNextToken(NULL, &iLine);
		CPlayer* base = pPlayer;
		if (!wname.empty()) {
			CPlayer* found = g_pObjectManager->FindByName(wname.c_str());
			if (found && found->IsInitialized()) base = found;
		}
		if (base && base->GetGuildID() != 0) {
			GUILDID gid = base->GetGuildID();
			std::vector<HOBJECT> members;
			g_pGuildManager->ForEachOnlineMember(gid, members);
			for (HOBJECT h : members) {
				CPlayer* p = g_pObjectManager->GetPC(h);
				if (p) g_pArenaManager->AddParticipant(p);
			}
		}
		else {
			{
				CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
				sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
				res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
				res->byDisplayType = SERVER_TEXT_SYSTEM;
				NTL_SAFE_WCSCPY(res->awchMessage, L"[Arena] Target player must be in a guild for guild joining.");
				packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
				pPlayer->SendPacket(&packet);
			}
		}
	}
	else if (sc == "spectate") {
		pToken->PopToPeek();
		std::wstring wname = pToken->PeekNextToken(NULL, &iLine);
		CPlayer* who = pPlayer;
		if (!wname.empty()) {
			CPlayer* found = g_pObjectManager->FindByName(wname.c_str());
			if (found && found->IsInitialized()) who = found;
		}
		if (who) g_pArenaManager->AddSpectator(who);
	}
	else if (sc == "tp") {
		pToken->PopToPeek();
		std::wstring which = pToken->PeekNextToken(NULL, &iLine);
		std::string swhich = ws2s(which);
		for (auto& c : swhich) c = (char)tolower(c);
		if (swhich == "participants") g_pArenaManager->TeleportParticipants(true);
		else if (swhich == "spectators") g_pArenaManager->TeleportSpectators();
		else if (swhich == "all") { g_pArenaManager->TeleportParticipants(true); g_pArenaManager->TeleportSpectators(); }
		else if (swhich == "here") {
			pToken->PopToPeek();
			std::wstring wsub = pToken->PeekNextToken(NULL, &iLine);
			std::string subarg = ws2s(wsub);
			for (auto& c : subarg) c = (char)tolower(c);
			if (subarg == "participants") g_pArenaManager->TeleportParticipantsHere(pPlayer);
			else if (subarg == "spectators") g_pArenaManager->TeleportSpectatorsHere(pPlayer);
			else /* all or empty */ { g_pArenaManager->TeleportParticipantsHere(pPlayer); g_pArenaManager->TeleportSpectatorsHere(pPlayer); }
		}
	}
	else if (sc == "win") {
		pToken->PopToPeek();
		std::wstring wname = pToken->PeekNextToken(NULL, &iLine);
		CPlayer* who = pPlayer;
		if (!wname.empty()) {
			CPlayer* found = g_pObjectManager->FindByName(wname.c_str());
			if (found && found->IsInitialized()) who = found;
		}
		if (who) g_pArenaManager->MarkWinner(who);
	}
	else if (sc == "award") {
		pToken->PopToPeek();
		std::wstring wset = pToken->PeekNextToken(NULL, &iLine);
		std::string sset = ws2s(wset);
		for (auto& c : sset) c = (char)tolower(c);
		if (sset == "winners") g_pArenaManager->AwardRewards(true);
		else if (sset == "participants") g_pArenaManager->AwardRewards(false);
	}
	else if (sc == "map") {
		pToken->PopToPeek();
		std::wstring wnum = pToken->PeekNextToken(NULL, &iLine);
		unsigned int tbl = (unsigned int)atoi(ws2s(wnum).c_str());
		if (tbl) g_pArenaManager->SetCurrentWorld(tbl);
	}
	else if (sc == "spawn") {
		pToken->PopToPeek();
		std::wstring wmob = pToken->PeekNextToken(NULL, &iLine);
		if (wmob.empty()) { g_pArenaManager->StatusTo(pPlayer); return; }
		unsigned int mobId = (unsigned int)atoi(ws2s(wmob).c_str());
		unsigned int count = 1;
		pToken->PopToPeek();
		std::wstring wcnt = pToken->PeekNextToken(NULL, &iLine);
		if (!wcnt.empty()) count = (unsigned int)atoi(ws2s(wcnt).c_str());
		if (count == 0) count = 1;
		unsigned int spawned = 0;
		for (unsigned int i = 0; i < count; ++i)
			if (g_pArenaManager->SpawnMob(mobId)) ++spawned;
		{
			CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
			sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
			res->wOpCode = GU_SYSTEM_DISPLAY_TEXT; res->byDisplayType = SERVER_TEXT_SYSTEM;
			wchar_t msg[128]; swprintf_s(msg, _countof(msg), L"[Arena] Spawned %u/%u mobs (tblidx=%u)", spawned, count, mobId);
			NTL_SAFE_WCSCPY(res->awchMessage, msg);
			packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
			pPlayer->SendPacket(&packet);
		}
	}
	else {
		g_pArenaManager->StatusTo(pPlayer);
	}
}

ACMD(do_event)
{
	// Syntax:
	// @event start
	// @event stop [abort]
	// @event status
	// @event beginnow
	// @event nextround
	// @event restartround
	// @event mobsperwave [amount]
	// @event purgemobs
	pToken->PopToPeek();
	std::wstring wsub = pToken->PeekNextToken(NULL, &iLine);
	if (wsub.empty())
	{
		if (g_pEventManager)
			g_pEventManager->StatusTo(pPlayer);
		return;
	}

	std::string sub = ws2s(wsub);
	for (auto &c : sub) c = (char)tolower(c);

	if (sub == "start")
	{
		if (g_pEventManager)
			g_pEventManager->Start();
	}
	else if (sub == "stop")
	{
		// optional: abort flag
		pToken->PopToPeek();
		std::wstring warg = pToken->PeekNextToken(NULL, &iLine);
		std::string arg = ws2s(warg);
		for (auto &c : arg) c = (char)tolower(c);
		bool abort = (arg == "abort");
		if (g_pEventManager)
			g_pEventManager->Stop(abort);
	}
	else if (sub == "status")
	{
		if (g_pEventManager)
			g_pEventManager->StatusTo(pPlayer);
	}
	else if (sub == "beginnow")
	{
		if (g_pEventManager)
			g_pEventManager->BeginNow();
	}
	else if (sub == "nextround")
	{
		if (g_pEventManager)
		{
			if (g_pEventManager->GetState() == CEventManager::State::IN_ROUND)
			{
				g_pEventManager->CompleteCurrentRound();
				CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
				sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
				res->wOpCode = GU_SYSTEM_DISPLAY_TEXT; res->byDisplayType = SERVER_TEXT_SYSTEM;
				const wchar_t* wmsg = L"[EVENT] Skipping to next round...";
				res->wMessageLengthInUnicode = (WORD)wcslen(wmsg);
				NTL_SAFE_WCSCPY(res->awchMessage, wmsg);
				packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
				pPlayer->SendPacket(&packet);
			}
			else
			{
				CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
				sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
				res->wOpCode = GU_SYSTEM_DISPLAY_TEXT; res->byDisplayType = SERVER_TEXT_SYSTEM;
				const wchar_t* wmsg = L"[EVENT] No active round to skip.";
				res->wMessageLengthInUnicode = (WORD)wcslen(wmsg);
				NTL_SAFE_WCSCPY(res->awchMessage, wmsg);
				packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
				pPlayer->SendPacket(&packet);
			}
		}
	}
	else if (sub == "restartround")
	{
		if (g_pEventManager)
		{
			if (g_pEventManager->GetState() == CEventManager::State::IN_ROUND)
			{
				// Force restart current round by going back one and then completing
				unsigned int currentRound = g_pEventManager->GetCurrentRound();
				if (currentRound > 0)
				{
					g_pEventManager->SetCurrentRound(currentRound - 1);
				}
				g_pEventManager->CompleteCurrentRound();
				CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
				sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
				res->wOpCode = GU_SYSTEM_DISPLAY_TEXT; res->byDisplayType = SERVER_TEXT_SYSTEM;
				const wchar_t* wmsg = L"[EVENT] Restarting current round...";
				res->wMessageLengthInUnicode = (WORD)wcslen(wmsg);
				NTL_SAFE_WCSCPY(res->awchMessage, wmsg);
				packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
				pPlayer->SendPacket(&packet);
			}
			else
			{
				CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
				sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
				res->wOpCode = GU_SYSTEM_DISPLAY_TEXT; res->byDisplayType = SERVER_TEXT_SYSTEM;
				const wchar_t* wmsg = L"[EVENT] No active round to restart.";
				res->wMessageLengthInUnicode = (WORD)wcslen(wmsg);
				NTL_SAFE_WCSCPY(res->awchMessage, wmsg);
				packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
				pPlayer->SendPacket(&packet);
			}
		}
	}
	else if (sub == "mobsperwave")
	{
		// Get the amount parameter
		pToken->PopToPeek();
		std::wstring wamount = pToken->PeekNextToken(NULL, &iLine);

		if (wamount.empty())
		{
			// Show current value
			if (g_pEventManager)
			{
				CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
				sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
				res->wOpCode = GU_SYSTEM_DISPLAY_TEXT; res->byDisplayType = SERVER_TEXT_SYSTEM;
				wchar_t wmsg[256];
				swprintf_s(wmsg, L"[EVENT] Current MobsPerWave: %u. Usage: @event mobsperwave <amount>",
					g_pEventManager->GetMobsPerWave());
				res->wMessageLengthInUnicode = (WORD)wcslen(wmsg);
				NTL_SAFE_WCSCPY(res->awchMessage, wmsg);
				packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
				pPlayer->SendPacket(&packet);
			}
			return;
		}

		int amount = _wtoi(wamount.c_str());
		if (amount <= 0 || amount > 1000)
		{
			CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
			sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
			res->wOpCode = GU_SYSTEM_DISPLAY_TEXT; res->byDisplayType = SERVER_TEXT_SYSTEM;
			const wchar_t* wmsg = L"[EVENT] Invalid amount. Must be between 1 and 1000.";
			res->wMessageLengthInUnicode = (WORD)wcslen(wmsg);
			NTL_SAFE_WCSCPY(res->awchMessage, wmsg);
			packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
			pPlayer->SendPacket(&packet);
			return;
		}

		if (g_pEventManager)
		{
			g_pEventManager->SetMobsPerWave((unsigned int)amount);
			CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
			sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
			res->wOpCode = GU_SYSTEM_DISPLAY_TEXT; res->byDisplayType = SERVER_TEXT_SYSTEM;
			wchar_t wmsg[256];
			swprintf_s(wmsg, L"[EVENT] MobsPerWave set to %d. Change will apply to next wave.", amount);
			res->wMessageLengthInUnicode = (WORD)wcslen(wmsg);
			NTL_SAFE_WCSCPY(res->awchMessage, wmsg);
			packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
			pPlayer->SendPacket(&packet);
		}
	}
	else if (sub == "purgemobs")
	{
		if (g_pEventManager)
		{
			g_pEventManager->DespawnAllEventMobs();
			CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
			sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
			res->wOpCode = GU_SYSTEM_DISPLAY_TEXT; res->byDisplayType = SERVER_TEXT_SYSTEM;
			const wchar_t* wmsg = L"[EVENT] All event mobs have been purged.";
			res->wMessageLengthInUnicode = (WORD)wcslen(wmsg);
			NTL_SAFE_WCSCPY(res->awchMessage, wmsg);
			packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
			pPlayer->SendPacket(&packet);
		}
	}
	else
	{
		CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
		res->wOpCode = GU_SYSTEM_DISPLAY_TEXT; res->byDisplayType = SERVER_TEXT_SYSTEM;
		const wchar_t* wmsg = L"Usage: @event start | stop [abort] | status | beginnow | nextround | restartround | mobsperwave [amount] | purgemobs";
		res->wMessageLengthInUnicode = (WORD)wcslen(wmsg);
		NTL_SAFE_WCSCPY(res->awchMessage, wmsg);
		packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		pPlayer->SendPacket(&packet);
	}
}

ACMD(do_event_reload)
{
	// Reload Events.cfg and reset auto scheduling. If InitialDelaySeconds=0, start immediately.
	bool ok = false;
	if (g_pEventManager)
		ok = g_pEventManager->ReloadConfigFromDefault();

	if (g_pEventManager)
		g_pEventManager->ResetAutomation(true);

	CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
	res->wOpCode = GU_SYSTEM_DISPLAY_TEXT; res->byDisplayType = SERVER_TEXT_SYSTEM;
	const wchar_t* wmsg = ok ? L"[EVENT] Config reloaded; automation reset" : L"[EVENT] Failed to reload config; automation reset";
	res->wMessageLengthInUnicode = (WORD)wcslen(wmsg);
	NTL_SAFE_WCSCPY(res->awchMessage, wmsg);
	packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	pPlayer->SendPacket(&packet);
}

ACMD(do_world_fight)
{
	// Syntax:
	// @world_fight start score [worldTblidx] [ffa|party|guild] [seconds]
	// @world_fight start elimination [worldTblidx] [ffa|party|guild]
	// @world_fight stop
	// Defaults: world=900043 (Arena base), mode=ffa, seconds=900 (15min)
	const unsigned int DEFAULT_WORLD = 900043;
	const unsigned int DEFAULT_SECONDS = 900;

	pToken->PopToPeek();
	std::wstring wsub = pToken->PeekNextToken(NULL, &iLine);
	if (wsub.empty()) { ERR_LOG(LOG_SYSTEM, "@world_fight: missing subcommand"); return; }
	std::string sub = ws2s(wsub); for (auto& c : sub) c = (char)tolower(c);

	if (sub == "stop")
	{
		// Re-enable helpers for the active world and stop arena
		unsigned int wid = g_pArenaManager->GetOrCreateCurrentWorldId();
		if (wid)
		{
			// Unsuppress helpers and flush any lingering ones
			CWorld* pWorld = ((CGameServer*)g_pApp)->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)wid);
			if (pWorld)
			{
				GetHelperNpcManager()->SetWorldSuppressed(pWorld->GetID(), false);
				GetHelperNpcManager()->DespawnAllHelpersInWorld(pWorld);
			}
		}
		g_pArenaManager->Stop(true);
		g_pArenaManager->StatusTo(pPlayer);
		return;
	}

	if (sub != "start") { ERR_LOG(LOG_SYSTEM, "@world_fight: unknown subcommand"); return; }

	// mode token: score | elimination
	pToken->PopToPeek();
	std::wstring wmode = pToken->PeekNextToken(NULL, &iLine);
	if (wmode.empty()) { ERR_LOG(LOG_SYSTEM, "@world_fight: missing mode (score|elimination)"); return; }
	std::string smode = ws2s(wmode); for (auto& c : smode) c = (char)tolower(c);
	bool scoreMode = (smode == "score");
	if (!scoreMode && smode != "elimination") { ERR_LOG(LOG_SYSTEM, "@world_fight: mode must be 'score' or 'elimination'"); return; }

	// optional world tblidx
	unsigned int worldTblidx = DEFAULT_WORLD;
	pToken->PopToPeek();
	std::wstring wworld = pToken->PeekNextToken(NULL, &iLine);
	if (!wworld.empty())
	{
		unsigned int t = (unsigned int)atoi(ws2s(wworld).c_str());
		if (t != 0) worldTblidx = t;
	}

	// optional mode ffa|party|guild
	CArenaManager::Mode arenaMode = CArenaManager::Mode::FREE_FOR_ALL;
	pToken->PopToPeek();
	std::wstring wfight = pToken->PeekNextToken(NULL, &iLine);
	if (!wfight.empty())
	{
		std::string s = ws2s(wfight); for (auto& c : s) c = (char)tolower(c);
		if (s == "party") arenaMode = CArenaManager::Mode::PARTY_VS_PARTY;
		else if (s == "guild" || s == "gvg") arenaMode = CArenaManager::Mode::GUILD_VS_GUILD;
		else if (s == "ffa" || s == "all") arenaMode = CArenaManager::Mode::FREE_FOR_ALL;
		else {
			// push back one token if it's not a mode (so it can be seconds)
		}
	}

	// optional seconds (score mode only)
	unsigned int seconds = DEFAULT_SECONDS;
	if (scoreMode)
	{
		pToken->PopToPeek();
		std::wstring wsec = pToken->PeekNextToken(NULL, &iLine);
		if (!wsec.empty())
		{
			unsigned int t = (unsigned int)atoi(ws2s(wsec).c_str());
			if (t > 0) seconds = t;
		}
	}

	// Force arena world and configure event
	if (!g_pArenaManager->ForceCurrentWorld(worldTblidx))
	{
		CNtlPacket pkt(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)pkt.GetPacketData();
		res->wOpCode = GU_SYSTEM_DISPLAY_TEXT; res->byDisplayType = SERVER_TEXT_SYSTEM;
		wchar_t msg[128]; swprintf_s(msg, _countof(msg), L"[WorldFight] Invalid or unavailable world tblidx %u", worldTblidx);
		NTL_SAFE_WCSCPY(res->awchMessage, msg);
		pkt.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		pPlayer->SendPacket(&pkt);
		return;
	}
	g_pArenaManager->SetupWorldFight(scoreMode, seconds, arenaMode);

	// Prepare world instance and suppress helper NPCs there
	unsigned int wid = g_pArenaManager->GetOrCreateCurrentWorldId();
	if (wid)
	{
		CWorld* pWorld = ((CGameServer*)g_pApp)->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)wid);
		if (pWorld)
		{
			GetHelperNpcManager()->SetWorldSuppressed(pWorld->GetID(), true);
			GetHelperNpcManager()->DespawnAllHelpersInWorld(pWorld);
		}
	}

	// Start arena enrollment immediately for chosen mode
	g_pArenaManager->Start(arenaMode);
	wchar_t msg[256];
	const wchar_t* modeName = (arenaMode == CArenaManager::Mode::PARTY_VS_PARTY) ? L"Party" : (arenaMode == CArenaManager::Mode::GUILD_VS_GUILD) ? L"Guild" : L"FFA";
	if (scoreMode)
		swprintf_s(msg, _countof(msg), L"[WorldFight] Score mode in world %u for %u seconds (%s)", worldTblidx, seconds, modeName);
	else
		swprintf_s(msg, _countof(msg), L"[WorldFight] Elimination mode in world %u (%s)", worldTblidx, modeName);
	CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
	res->wOpCode = GU_SYSTEM_DISPLAY_TEXT; res->byDisplayType = SERVER_TEXT_NOTICE;
	NTL_SAFE_WCSCPY(res->awchMessage, msg);
	packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	pPlayer->SendPacket(&packet);
}

ACMD(do_budokai)
{
	// @budokai show
	// @budokai set time <DojoRecommend|OpenNotice|Register|EndingWait|MinorWait|MajorWait|FinalWait|End> <seconds>
	// @budokai set score <major|final> <n>
	pToken->PopToPeek();
	std::wstring sub = pToken->PeekNextToken(NULL, &iLine);
	if (sub.empty() || !_wcsicmp(sub.c_str(), L"show"))
	{
		char sz[512];
		sprintf_s(sz, sizeof(sz),
			"[BUDOKAI]\nDojoRecommendTime=%u OpenNoticeTime=%u RegisterTime=%u EndingWaitTime=%u\nMinorWait=%u MajorWait=%u FinalWait=%u EndTime=%u\nMajorMaxScore=%u FinalMaxScore=%u",
			g_pBudokaiManager->GetDojoRecommendTime(), g_pBudokaiManager->GetOpenNoticeTime(), g_pBudokaiManager->GetRegisterTime(), g_pBudokaiManager->GetEndingWaitTime(),
			g_pBudokaiManager->GetMinorMatchWaitTime(), g_pBudokaiManager->GetMajorMatchWaitTime(), g_pBudokaiManager->GetFinalMatchWaitTime(), g_pBudokaiManager->GetBudokaiEndTime(),
			g_pBudokaiManager->GetMajorMatchMaxScore(), g_pBudokaiManager->GetFinalMatchMaxScore());
		CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
		res->wOpCode = GU_SYSTEM_DISPLAY_TEXT; res->byDisplayType = SERVER_TEXT_SYSTEM;
		std::string msg(sz);
		std::wstring wmsg(msg.begin(), msg.end());
		wcscpy_s(res->awchMessage, NTL_MAX_LENGTH_OF_CHAT_MESSAGE + 1, wmsg.c_str());
		packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		((CGameServer*)g_pApp)->Send(pPlayer->GetClientSessionID(), &packet);
		return;
	}

	std::string ssub = ws2s(sub); for (auto& c : ssub) c = (char)tolower(c);
	if (ssub == "set")
	{
		pToken->PopToPeek();
		std::wstring wwhat = pToken->PeekNextToken(NULL, &iLine);
		std::string what = ws2s(wwhat); for (auto& c : what) c = (char)tolower(c);

		if (what == "time")
		{
			pToken->PopToPeek(); std::wstring wkey = pToken->PeekNextToken(NULL, &iLine);
			pToken->PopToPeek(); std::wstring wval = pToken->PeekNextToken(NULL, &iLine);
			if (wkey.empty() || wval.empty()) return;
			std::string key = ws2s(wkey); for (auto& c : key) c = (char)tolower(c);
			unsigned int sec = (unsigned int)atoi(ws2s(wval).c_str());
			if (key == "dojorecommend") g_pBudokaiManager->SetDojoRecommendTime(sec);
			else if (key == "opennotice") g_pBudokaiManager->SetOpenNoticeTime(sec);
			else if (key == "register") g_pBudokaiManager->SetRegisterTime(sec);
			else if (key == "endingwait") g_pBudokaiManager->SetEndingWaitTime(sec);
			else if (key == "minorwait") g_pBudokaiManager->SetMinorMatchWaitTime(sec);
			else if (key == "majorwait") g_pBudokaiManager->SetMajorMatchWaitTime(sec);
			else if (key == "finalwait") g_pBudokaiManager->SetFinalMatchWaitTime(sec);
			else if (key == "end") g_pBudokaiManager->SetBudokaiEndTime(sec);
		}
		else if (what == "score")
		{
			pToken->PopToPeek(); std::wstring wwhich = pToken->PeekNextToken(NULL, &iLine);
			pToken->PopToPeek(); std::wstring wnum = pToken->PeekNextToken(NULL, &iLine);
			if (wwhich.empty() || wnum.empty()) return;
			std::string which = ws2s(wwhich); for (auto& c : which) c = (char)tolower(c);
			BYTE v = (BYTE)atoi(ws2s(wnum).c_str()); if (!v) v = 1;
			if (which == "major") g_pBudokaiManager->SetMajorMatchMaxScore(v);
			else if (which == "final") g_pBudokaiManager->SetFinalMatchMaxScore(v);
		}
	}
}

// @budokaistart - Start Budokai tournaments on channel 9 from any channel
// Usage: @budokaistart <adultsolo|adultteam|juniorsolo|juniorteam>
ACMD(do_budokai_start)
{
	pToken->PopToPeek();
	std::wstring wtype = pToken->PeekNextToken(NULL, &iLine);

	if (wtype.empty())
	{
		CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
		res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
		res->byDisplayType = SERVER_TEXT_SYSTEM;
		wcscpy_s(res->awchMessage, NTL_MAX_LENGTH_OF_CHAT_MESSAGE + 1,
			L"Usage: @budokaistart <adultsolo|adultteam|juniorsolo|juniorteam>");
		packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		((CGameServer*)g_pApp)->Send(pPlayer->GetClientSessionID(), &packet);
		return;
	}

	// Convert to lowercase for comparison
	std::string type;
	for (wchar_t wc : wtype)
		type += (char)tolower((char)wc);

	// Map user-friendly command to actual server command
	std::string serverCmd;
	std::string displayName;

	if (type == "adultsolo")
	{
		serverCmd = "StartAdultSolo";
		displayName = "Adult Solo Budokai";
	}
	else if (type == "adultteam" || type == "adultparty")
	{
		serverCmd = "StartAdultTeam";
		displayName = "Adult Team Budokai";
	}
	else if (type == "juniorsolo")
	{
		serverCmd = "StartJuniorSolo";
		displayName = "Junior Solo Budokai";
	}
	else if (type == "juniorteam" || type == "juniorparty")
	{
		serverCmd = "StartJuniorTeam";
		displayName = "Junior Team Budokai";
	}
	else
	{
		CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
		res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
		res->byDisplayType = SERVER_TEXT_SYSTEM;
		wcscpy_s(res->awchMessage, NTL_MAX_LENGTH_OF_CHAT_MESSAGE + 1,
			L"Invalid type! Use: adultsolo, adultteam, juniorsolo, or juniorteam");
		packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		((CGameServer*)g_pApp)->Send(pPlayer->GetClientSessionID(), &packet);
		return;
	}

	// Send command to Budokai channel (channel 9)
	const BYTE BUDOKAI_CHANNEL = 9;
	std::string response;
	bool success = SendCommandToChannel(BUDOKAI_CHANNEL, serverCmd, &response);

	// Prepare response message
	CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
	res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
	res->byDisplayType = SERVER_TEXT_SYSTEM;

	if (success)
	{
		char msg[NTL_MAX_LENGTH_OF_CHAT_MESSAGE + 1];
		sprintf_s(msg, "[BUDOKAI] %s has been started on channel 9!", displayName.c_str());
		std::wstring wmsg;
		for (char c : std::string(msg))
			wmsg += (wchar_t)c;
		wcscpy_s(res->awchMessage, NTL_MAX_LENGTH_OF_CHAT_MESSAGE + 1, wmsg.c_str());
	}
	else
	{
		wcscpy_s(res->awchMessage, NTL_MAX_LENGTH_OF_CHAT_MESSAGE + 1,
			L"[BUDOKAI] Failed to start - Is channel 9 (Budokai server) running?");
	}

	packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	((CGameServer*)g_pApp)->Send(pPlayer->GetClientSessionID(), &packet);
}

// @dojo control for starting/stopping dojo wars from in-game GM
ACMD(do_dojo)
{
	// Syntax:
	// @dojo on                      -> enable manual mode (GS ticks outside Sunday)
	// @dojo off                     -> disable manual mode
	// @dojo start [dojoTblidx]      -> open RECEIVE on all or a specific dojo (via ChatServer)
	// @dojo clear <dojoTblidx>      -> reset specific dojo to NORMAL (via ChatServer)
	// @dojo next <dojoTblidx>       -> ask ChatServer to re-broadcast current state
	// @dojo setatt <dojoTblidx> <attGuildId> -> set attacker guild locally (prep for war)
	// @dojo war <dojoTblidx>        -> force state progression to READY/START locally
	// @dojo status                  -> show usage
	// Note: When possible, we drive ChatServer via GT_* opcodes to mirror Sunday flow.
	pToken->PopToPeek();
	std::wstring sub = pToken->PeekNextToken(NULL, &iLine);
	if (sub.empty()) sub = L"status";

	std::wstring lower = sub;
	for (auto& ch : lower) ch = towlower(ch);

	if (lower == L"on")
	{
		// Enable manual dojo mode so GS ticks regardless of Sunday window
		g_pDojoManager->SetManualMode(true);

		// Prefer additionally syncing ChatServer-driven RECEIVE window for all dojos
		CGameServer* app = (CGameServer*)g_pApp;
		if (app->GetChatServerSession())
		{
			DBOTIME until = app->GetTime() + 7200; // 2 hours receive window
			for (auto it = g_pDojoManager->GetDojoSetBegin(); it != g_pDojoManager->GetDojoSetEnd(); ++it)
			{
				CDojo* d = it->second; if (!d) continue;
				CNtlPacket pk(sizeof(sGT_DOJO_SCRAMBLE_STATE_CHANGE));
				sGT_DOJO_SCRAMBLE_STATE_CHANGE* rq = (sGT_DOJO_SCRAMBLE_STATE_CHANGE*)pk.GetPacketData();
				rq->wOpCode = GT_DOJO_SCRAMBLE_STATE_CHANGE;
				rq->byState = eDBO_DOJO_STATUS_RECEIVE;
				rq->dojoTblidx = d->GetDojoTblidx();
				rq->tmNextStepTime = until;
				pk.SetPacketLen(sizeof(sGT_DOJO_SCRAMBLE_STATE_CHANGE));
				app->SendTo(app->GetChatServerSession(), &pk);
			}
			CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
			sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
			res->wOpCode = GU_SYSTEM_DISPLAY_TEXT; res->byDisplayType = SERVER_TEXT_SYSTEM;
			NTL_SAFE_WCSCPY(res->awchMessage, L"[Dojo] Manual mode ON. Opened RECEIVE on all dojos for 2 hours (synced with ChatServer).");
			packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
			pPlayer->SendPacket(&packet);
		}
		else
		{
			CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
			sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
			res->wOpCode = GU_SYSTEM_DISPLAY_TEXT; res->byDisplayType = SERVER_TEXT_SYSTEM;
			NTL_SAFE_WCSCPY(res->awchMessage, L"[Dojo] Manual mode ON. ChatServer not connected; GS will drive dojo flow.");
			packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
			pPlayer->SendPacket(&packet);
		}
	}
	else if (lower == L"off")
	{
		CGameServer* app = (CGameServer*)g_pApp;
		g_pDojoManager->SetManualMode(false);
		CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
		res->wOpCode = GU_SYSTEM_DISPLAY_TEXT; res->byDisplayType = SERVER_TEXT_SYSTEM;
		NTL_SAFE_WCSCPY(res->awchMessage, L"[Dojo] Manual mode OFF.");
		packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		pPlayer->SendPacket(&packet);
	}
	else if (lower == L"start")
	{
		// Optional dojoTblidx argument
		pToken->PopToPeek();
		std::wstring widx = pToken->PeekNextToken(NULL, &iLine);
		CGameServer* app = (CGameServer*)g_pApp;
		if (!app->GetChatServerSession())
		{
			CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
			sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
			res->wOpCode = GU_SYSTEM_DISPLAY_TEXT; res->byDisplayType = SERVER_TEXT_SYSTEM;
			NTL_SAFE_WCSCPY(res->awchMessage, L"[Dojo] ChatServer not connected. Cannot open RECEIVE.");
			packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
			pPlayer->SendPacket(&packet);
			return;
		}
		else if (lower == L"setatt")
		{
			// setatt <dojoTblidx> <attGuildId>
			pToken->PopToPeek();
			std::wstring widx = pToken->PeekNextToken(NULL, &iLine);
			pToken->PopToPeek();
			std::wstring wgid = pToken->PeekNextToken(NULL, &iLine);
			if (widx.empty() || wgid.empty())
			{
				CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
				sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
				res->wOpCode = GU_SYSTEM_DISPLAY_TEXT; res->byDisplayType = SERVER_TEXT_SYSTEM;
				NTL_SAFE_WCSCPY(res->awchMessage, L"Usage: @dojo setatt <dojoTblidx> <attGuildId>");
				packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
				pPlayer->SendPacket(&packet);
				return;
			}
			TBLIDX dojoTblidx = (TBLIDX)atoi(ws2s(widx).c_str());
			GUILDID gid = (GUILDID)atoi(ws2s(wgid).c_str());
			CDojo* d = g_pDojoManager->GetDojoWithTblidx(dojoTblidx);
			if (!d)
			{
				CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
				sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
				res->wOpCode = GU_SYSTEM_DISPLAY_TEXT; res->byDisplayType = SERVER_TEXT_SYSTEM;
				NTL_SAFE_WCSCPY(res->awchMessage, L"[Dojo] Dojo not found.");
				packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
				pPlayer->SendPacket(&packet);
				return;
			}
			d->SetAttGuild(gid);
			CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
			sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
			res->wOpCode = GU_SYSTEM_DISPLAY_TEXT; res->byDisplayType = SERVER_TEXT_SYSTEM;
			NTL_SAFE_WCSCPY(res->awchMessage, L"[Dojo] Attacker guild set.");
			packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
			pPlayer->SendPacket(&packet);
		}
		else if (lower == L"war")
		{
			// war <dojoTblidx>
			pToken->PopToPeek();
			std::wstring widx = pToken->PeekNextToken(NULL, &iLine);
			if (widx.empty())
			{
				CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
				sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
				res->wOpCode = GU_SYSTEM_DISPLAY_TEXT; res->byDisplayType = SERVER_TEXT_SYSTEM;
				NTL_SAFE_WCSCPY(res->awchMessage, L"Usage: @dojo war <dojoTblidx>");
				packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
				pPlayer->SendPacket(&packet);
				return;
			}
			TBLIDX dojoTblidx = (TBLIDX)atoi(ws2s(widx).c_str());
			CDojo* d = g_pDojoManager->GetDojoWithTblidx(dojoTblidx);
			if (!d)
			{
				CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
				sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
				res->wOpCode = GU_SYSTEM_DISPLAY_TEXT; res->byDisplayType = SERVER_TEXT_SYSTEM;
				NTL_SAFE_WCSCPY(res->awchMessage, L"[Dojo] Dojo not found.");
				packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
				pPlayer->SendPacket(&packet);
				return;
			}
			// Advance to STANDBY soon; TickProcess will handle spawn/teleport and auto-advance to READY
			// Ensure manual mode is on so ticks occur outside Sunday window
			g_pDojoManager->SetManualMode(true);
			DBOTIME now = ((CGameServer*)g_pApp)->GetTime();
			d->SetState(eDBO_DOJO_STATUS_STANDBY, now + 1);
			CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
			sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
			res->wOpCode = GU_SYSTEM_DISPLAY_TEXT; res->byDisplayType = SERVER_TEXT_SYSTEM;
			NTL_SAFE_WCSCPY(res->awchMessage, L"[Dojo] War sequence initiated: STANDBY scheduled in 1s (manual mode enabled).");
			packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
			pPlayer->SendPacket(&packet);
		}
		DBOTIME until = app->GetTime() + 7200;
		if (widx.empty())
		{
			for (auto it = g_pDojoManager->GetDojoSetBegin(); it != g_pDojoManager->GetDojoSetEnd(); ++it)
			{
				CDojo* d = it->second; if (!d) continue;
				CNtlPacket pk(sizeof(sGT_DOJO_SCRAMBLE_STATE_CHANGE));
				sGT_DOJO_SCRAMBLE_STATE_CHANGE* rq = (sGT_DOJO_SCRAMBLE_STATE_CHANGE*)pk.GetPacketData();
				rq->wOpCode = GT_DOJO_SCRAMBLE_STATE_CHANGE;
				rq->byState = eDBO_DOJO_STATUS_RECEIVE;
				rq->dojoTblidx = d->GetDojoTblidx();
				rq->tmNextStepTime = until;
				pk.SetPacketLen(sizeof(sGT_DOJO_SCRAMBLE_STATE_CHANGE));
				app->SendTo(app->GetChatServerSession(), &pk);
			}
			CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
			sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
			res->wOpCode = GU_SYSTEM_DISPLAY_TEXT; res->byDisplayType = SERVER_TEXT_SYSTEM;
			NTL_SAFE_WCSCPY(res->awchMessage, L"[Dojo] Opened RECEIVE on all dojos for 2 hours.");
			packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
			pPlayer->SendPacket(&packet);
		}
		else
		{
			TBLIDX dojoTblidx = (TBLIDX)atoi(ws2s(widx).c_str());
			CNtlPacket pk(sizeof(sGT_DOJO_SCRAMBLE_STATE_CHANGE));
			sGT_DOJO_SCRAMBLE_STATE_CHANGE* rq = (sGT_DOJO_SCRAMBLE_STATE_CHANGE*)pk.GetPacketData();
			rq->wOpCode = GT_DOJO_SCRAMBLE_STATE_CHANGE;
			rq->byState = eDBO_DOJO_STATUS_RECEIVE;
			rq->dojoTblidx = dojoTblidx;
			rq->tmNextStepTime = until;
			pk.SetPacketLen(sizeof(sGT_DOJO_SCRAMBLE_STATE_CHANGE));
			app->SendTo(app->GetChatServerSession(), &pk);
			CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
			sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
			res->wOpCode = GU_SYSTEM_DISPLAY_TEXT; res->byDisplayType = SERVER_TEXT_SYSTEM;
			NTL_SAFE_WCSCPY(res->awchMessage, L"[Dojo] Opened RECEIVE on selected dojo for 2 hours.");
			packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
			pPlayer->SendPacket(&packet);
		}
	}
	else if (lower == L"clear")
	{
		// clear <dojoTblidx>
		pToken->PopToPeek();
		std::wstring widx = pToken->PeekNextToken(NULL, &iLine);
		if (widx.empty())
		{
			CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
			sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
			res->wOpCode = GU_SYSTEM_DISPLAY_TEXT; res->byDisplayType = SERVER_TEXT_SYSTEM;
			NTL_SAFE_WCSCPY(res->awchMessage, L"Usage: @dojo clear <dojoTblidx>");
			packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
			pPlayer->SendPacket(&packet);
			return;
		}
		CGameServer* app = (CGameServer*)g_pApp;
		if (!app->GetChatServerSession())
		{
			CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
			sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
			res->wOpCode = GU_SYSTEM_DISPLAY_TEXT; res->byDisplayType = SERVER_TEXT_SYSTEM;
			NTL_SAFE_WCSCPY(res->awchMessage, L"[Dojo] ChatServer not connected. Cannot clear.");
			packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
			pPlayer->SendPacket(&packet);
			return;
		}
		TBLIDX dojoTblidx = (TBLIDX)atoi(ws2s(widx).c_str());
		CNtlPacket pk(sizeof(sGT_DOJO_SCRAMBLE_RESET));
		sGT_DOJO_SCRAMBLE_RESET* rq = (sGT_DOJO_SCRAMBLE_RESET*)pk.GetPacketData();
		rq->wOpCode = GT_DOJO_SCRAMBLE_RESET;
		rq->byState = eDBO_DOJO_STATUS_NORMAL;
		rq->dojoTblidx = dojoTblidx;
		rq->tmNextStepTime = 0;
		pk.SetPacketLen(sizeof(sGT_DOJO_SCRAMBLE_RESET));
		app->SendTo(app->GetChatServerSession(), &pk);
		CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
		res->wOpCode = GU_SYSTEM_DISPLAY_TEXT; res->byDisplayType = SERVER_TEXT_SYSTEM;
		NTL_SAFE_WCSCPY(res->awchMessage, L"[Dojo] Cleared dojo to NORMAL.");
		packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		pPlayer->SendPacket(&packet);
	}
	else if (lower == L"next")
	{
		// next <dojoTblidx>
		pToken->PopToPeek();
		std::wstring widx = pToken->PeekNextToken(NULL, &iLine);
		if (widx.empty())
		{
			CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
			sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
			res->wOpCode = GU_SYSTEM_DISPLAY_TEXT; res->byDisplayType = SERVER_TEXT_SYSTEM;
			NTL_SAFE_WCSCPY(res->awchMessage, L"Usage: @dojo next <dojoTblidx>");
			packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
			pPlayer->SendPacket(&packet);
			return;
		}
		CGameServer* app = (CGameServer*)g_pApp;
		if (!app->GetChatServerSession())
		{
			CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
			sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
			res->wOpCode = GU_SYSTEM_DISPLAY_TEXT; res->byDisplayType = SERVER_TEXT_SYSTEM;
			NTL_SAFE_WCSCPY(res->awchMessage, L"[Dojo] ChatServer not connected. Cannot nudge.");
			packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
			pPlayer->SendPacket(&packet);
			return;
		}
		TBLIDX dojoTblidx = (TBLIDX)atoi(ws2s(widx).c_str());
		CNtlPacket pkt(sizeof(sGT_DOJO_COMMAND));
		sGT_DOJO_COMMAND* req = (sGT_DOJO_COMMAND*)pkt.GetPacketData();
		req->wOpCode = GT_DOJO_COMMAND;
		req->byCommand = eDBO_DOJO_COMMAND_TYPE_NEXT;
		req->dojoTblidx = dojoTblidx;
		pkt.SetPacketLen(sizeof(sGT_DOJO_COMMAND));
		((CGameServer*)g_pApp)->SendTo(((CGameServer*)g_pApp)->GetChatServerSession(), &pkt);
		CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
		res->wOpCode = GU_SYSTEM_DISPLAY_TEXT; res->byDisplayType = SERVER_TEXT_SYSTEM;
		NTL_SAFE_WCSCPY(res->awchMessage, L"[Dojo] Requested state re-broadcast.");
		packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		pPlayer->SendPacket(&packet);
	}
	else // status or unknown -> print usage
	{
		CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
		res->wOpCode = GU_SYSTEM_DISPLAY_TEXT; res->byDisplayType = SERVER_TEXT_SYSTEM;
		NTL_SAFE_WCSCPY(res->awchMessage, L"Usage: @dojo on | off | start [dojoTblidx] | clear <dojoTblidx> | next <dojoTblidx> | setatt <dojoTblidx> <attGuildId> | war <dojoTblidx> | status");
		packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		pPlayer->SendPacket(&packet);
	}
}


ACMD(do_arena_join_public)
{
	// Players can only join themselves; ignore any parameters to prevent impersonation
	UNREFERENCED_PARAMETER(iLine);
	// consume any leftover tokens if present
	pToken->PopToPeek();

	// Channel gating: only allow joining on ARENA channels and never on Dojo channel
	{
		CGameServer* app = (CGameServer*)g_pApp;
		bool isDojo = app && app->IsDojoChannel();
		bool nameOk = true;
		if (app)
		{
			std::string got = app->m_config.ChannelName.c_str();
			for (auto& c : got) c = (char)tolower(c);
			nameOk = (got.find("arena") != std::string::npos);
		}
		if (isDojo || !nameOk)
		{
			CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
			sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
			res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
			res->byDisplayType = SERVER_TEXT_SYSTEM;
			NTL_SAFE_WCSCPY(res->awchMessage, L"[Arena] Joining is only available on ARENA channels (not Dojo).");
			packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
			pPlayer->SendPacket(&packet);
			return;
		}
	}
	// Add the caller as a participant if ArenaManager exists
	if (g_pArenaManager && pPlayer && pPlayer->IsInitialized())
	{
		// Individual join - warn if inappropriate for team mode
		if (g_pArenaManager->GetState() == CArenaManager::State::ENROLLMENT &&
			(g_pArenaManager->GetMode() == CArenaManager::Mode::PARTY_VS_PARTY ||
				g_pArenaManager->GetMode() == CArenaManager::Mode::GUILD_VS_GUILD)) {

			CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
			sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
			res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
			res->byDisplayType = SERVER_TEXT_SYSTEM;
			NTL_SAFE_WCSCPY(res->awchMessage, L"Consider using @arenajoinparty for team modes for better coordination.");
			packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
			pPlayer->SendPacket(&packet);
		}

		g_pArenaManager->AddParticipant(pPlayer);

		// Optional: feedback to the player (kept lightweight)
		CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
		res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
		res->byDisplayType = SERVER_TEXT_SYSTEM;
		NTL_SAFE_WCSCPY(res->awchMessage, L"You have been added to the Arena.");
		packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		pPlayer->SendPacket(&packet);
	}
}

ACMD(do_arena_joinparty_public)
{
	UNREFERENCED_PARAMETER(iLine);
	// consume any leftover tokens if present (we ignore parameters)
	pToken->PopToPeek();

	// Channel gating: only allow joining on ARENA channels and never on Dojo channel
	{
		CGameServer* app = (CGameServer*)g_pApp;
		bool isDojo = app && app->IsDojoChannel();
		bool nameOk = true;
		if (app)
		{
			std::string got = app->m_config.ChannelName.c_str();
			for (auto& c : got) c = (char)tolower(c);
			nameOk = (got.find("arena") != std::string::npos);
		}
		if (isDojo || !nameOk)
		{
			CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
			sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
			res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
			res->byDisplayType = SERVER_TEXT_SYSTEM;
			NTL_SAFE_WCSCPY(res->awchMessage, L"[Arena] Party joining is only available on ARENA channels (not Dojo).");
			packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
			pPlayer->SendPacket(&packet);
			return;
		}
	}

	if (!pPlayer || !pPlayer->IsInitialized())
		return;

	// Validate arena mode for party joining
	if (g_pArenaManager && g_pArenaManager->GetState() == CArenaManager::State::ENROLLMENT &&
		g_pArenaManager->GetMode() != CArenaManager::Mode::PARTY_VS_PARTY &&
		g_pArenaManager->GetMode() != CArenaManager::Mode::FREE_FOR_ALL &&
		g_pArenaManager->GetMode() != CArenaManager::Mode::OPEN)
	{
		CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
		res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
		res->byDisplayType = SERVER_TEXT_SYSTEM;
		NTL_SAFE_WCSCPY(res->awchMessage, L"Party joining only available for Party vs Party, Free For All, or Open modes.");
		packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		pPlayer->SendPacket(&packet);
		return;
	}

	if (!pPlayer->GetParty() || pPlayer->GetPartyID() == INVALID_PARTYID)
	{
		CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
		res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
		res->byDisplayType = SERVER_TEXT_SYSTEM;
		NTL_SAFE_WCSCPY(res->awchMessage, L"Need to be in a party to register it.");
		packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		pPlayer->SendPacket(&packet);
		return;
	}

	if (pPlayer->GetParty()->GetPartyLeaderID() != pPlayer->GetID())
	{
		CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
		res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
		res->byDisplayType = SERVER_TEXT_SYSTEM;
		NTL_SAFE_WCSCPY(res->awchMessage, L"Only the party leader can register it in the Arena.");
		packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		pPlayer->SendPacket(&packet);
		return;
	}

	auto addFn = [&](CPlayer* mem) { if (mem) g_pArenaManager->AddParticipant(mem); };
	pPlayer->GetParty()->ForEachOnlineMember(addFn);

	CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
	res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
	res->byDisplayType = SERVER_TEXT_SYSTEM;
	NTL_SAFE_WCSCPY(res->awchMessage, L"Your party has been registered in the Arena.");
	packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	pPlayer->SendPacket(&packet);
}

ACMD(do_arena_joinguild_public)
{
	UNREFERENCED_PARAMETER(iLine);
	// consume any leftover tokens if present (we ignore parameters)
	pToken->PopToPeek();

	// Channel gating: only allow joining on ARENA channels and never on Dojo channel
	{
		CGameServer* app = (CGameServer*)g_pApp;
		bool isDojo = app && app->IsDojoChannel();
		bool nameOk = true;
		if (app)
		{
			std::string got = app->m_config.ChannelName.c_str();
			for (auto& c : got) c = (char)tolower(c);
			nameOk = (got.find("arena") != std::string::npos);
		}
		if (isDojo || !nameOk)
		{
			CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
			sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
			res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
			res->byDisplayType = SERVER_TEXT_SYSTEM;
			NTL_SAFE_WCSCPY(res->awchMessage, L"[Arena] Guild joining is only available on ARENA channels (not Dojo).");
			packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
			pPlayer->SendPacket(&packet);
			return;
		}
	}

	if (!pPlayer || !pPlayer->IsInitialized())
		return;

	// Validate arena mode for guild joining
	if (g_pArenaManager && g_pArenaManager->GetState() == CArenaManager::State::ENROLLMENT &&
		g_pArenaManager->GetMode() != CArenaManager::Mode::GUILD_VS_GUILD &&
		g_pArenaManager->GetMode() != CArenaManager::Mode::FREE_FOR_ALL &&
		g_pArenaManager->GetMode() != CArenaManager::Mode::OPEN)
	{
		CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
		res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
		res->byDisplayType = SERVER_TEXT_SYSTEM;
		NTL_SAFE_WCSCPY(res->awchMessage, L"Guild joining only available for Guild vs Guild, Free For All, or Open modes.");
		packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		pPlayer->SendPacket(&packet);
		return;
	}

	if (pPlayer->GetGuildID() == 0)
	{
		CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
		res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
		res->byDisplayType = SERVER_TEXT_SYSTEM;
		NTL_SAFE_WCSCPY(res->awchMessage, L"Need to be in a guild to register it.");
		packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		pPlayer->SendPacket(&packet);
		return;
	}

	GUILDID guildId = pPlayer->GetGuildID();
	std::vector<HOBJECT> members;
	g_pGuildManager->ForEachOnlineMember(guildId, members);
	for (HOBJECT h : members) {
		CPlayer* p = g_pObjectManager->GetPC(h);
		if (p) g_pArenaManager->AddParticipant(p);
	}

	CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
	res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
	res->byDisplayType = SERVER_TEXT_SYSTEM;
	NTL_SAFE_WCSCPY(res->awchMessage, L"Your guild has been registered in the Arena.");
	packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	pPlayer->SendPacket(&packet);
}

ACMD(do_event_participate)
{
	UNREFERENCED_PARAMETER(iLine);
	pToken->PopToPeek();

	// Channel gating: only allow on EVENTS channels
	{
		CGameServer* app = (CGameServer*)g_pApp;
		bool isDojo = app && app->IsDojoChannel();
		bool nameOk = true;
		if (app)
		{
			std::string got = app->m_config.ChannelName.c_str();
			for (auto& c : got) c = (char)tolower(c);
			nameOk = (got.find("events") != std::string::npos);
		}
		if (isDojo || !nameOk)
		{
			CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
			sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
			res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
			res->byDisplayType = SERVER_TEXT_SYSTEM;
			NTL_SAFE_WCSCPY(res->awchMessage, L"[EVENT] Participation is only available on EVENTS channels.");
			packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
			pPlayer->SendPacket(&packet);
			return;
		}
	}

	// Add the caller as a participant if EventManager exists
	if (g_pEventManager && pPlayer && pPlayer->IsInitialized())
	{
		g_pEventManager->AddParticipant(pPlayer);
	}
}

ACMD(do_big)
{
	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	BYTE fsize = (BYTE)atof(ws2s(strToken).c_str());

	pToken->PopToPeek();
	strToken = pToken->PeekNextToken(NULL, &iLine);

	std::wstring name = std::wstring(strToken.begin(), strToken.end());
	const wchar_t* wname = name.c_str();

	CPlayer* cTarget = g_pObjectManager->FindByName(wname);
	if (!cTarget || !cTarget->IsInitialized())
	{
		cTarget = pPlayer;
	}

	cTarget->UpdateSizeRate(fsize);
}

ACMD(do_start_dbhunt)
{
	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	BYTE byHours = (BYTE)atof(ws2s(strToken).c_str());

	if (byHours > 24)
		byHours = 24;

	if (byHours == 0)
		g_pDragonballHuntEvent->StartEvent(true);
	else
		g_pDragonballHuntEvent->StartEvent(true, byHours);

	NTL_PRINT(PRINT_APP, _T("Dragonball Hunt Event Started"));
}

ACMD(do_stop_dbhunt)
{
	g_pDragonballHuntEvent->EndEvent();
	NTL_PRINT(PRINT_APP, _T("Dragonball Hunt Event Stopped"));
}

ACMD(do_start_dbscramble)
{
	g_pDragonballScramble->StartEvent();
	NTL_PRINT(PRINT_APP, _T("Dragonball Scramble Event Started"));
}

ACMD(do_stop_dbscramble)
{
	g_pDragonballScramble->EndEvent(true);
	NTL_PRINT(PRINT_APP, _T("Dragonball Scramble Event Stopped"));
}

ACMD(do_start_stonedrop)
{
	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	BYTE byHours = (BYTE)atof(ws2s(strToken).c_str());

	if (byHours > 24)
		byHours = 24;

	if (byHours == 0)
		g_pStoneDropEvent->StartEvent();
	else
		g_pStoneDropEvent->StartEvent(byHours);

	NTL_PRINT(PRINT_APP, _T("Double Stone Drop Event Started"));
}

ACMD(do_stop_stonedrop)
{
	g_pStoneDropEvent->EndEvent();
	NTL_PRINT(PRINT_APP, _T("Double Stone Drop Event Stopped"));
}

ACMD(do_start_customdrop)
{
	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	BYTE byHours = (BYTE)atof(ws2s(strToken).c_str());

	if (byHours > 24)
		byHours = 24;

	if (byHours == 0)
		g_pCustomDropEvent->StartEvent();
	else
		g_pCustomDropEvent->StartEvent(byHours);

	NTL_PRINT(PRINT_APP, _T("Custom Drop Event Started"));
}

ACMD(do_stop_customdrop)
{
	g_pCustomDropEvent->EndEvent();
	NTL_PRINT(PRINT_APP, _T("Custom Drop Event Stopped"));
}

ACMD(do_reload_customdrop_cfg)
{
	// optional path parameter
	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	std::string arg = ws2s(strToken);
	std::string path;
	if (arg.empty())
	{
		path = ".\\config\\CustomDropEvent.cfg";
	}
	else
	{
		// If user passed a bare name (e.g., Config1), build the full path
		bool hasBackslash = arg.find('\\') != std::string::npos || arg.find('/') != std::string::npos;
		bool hasExt = arg.rfind('.') != std::string::npos;
		if (!hasBackslash && !hasExt)
		{
			path = ".\\config\\" + arg + ".cfg";
		}
		else
		{
			path = arg; // treat as explicit path
		}
	}

	if (g_pCustomDropEvent->ReloadConfig(path.c_str()))
		NTL_PRINT(PRINT_APP, _T("CustomDropEvent: config reloaded from %s"), s2ws(path).c_str());
	else
		NTL_PRINT(PRINT_APP, _T("CustomDropEvent: failed to reload config from %s"), s2ws(path).c_str());
}

// Reload helper NPC configuration at runtime
ACMD(do_reload_helpernpc_cfg)
{
	// usage: @reload_helpernpc [GameServer.ini]
	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	std::string arg = ws2s(strToken);
	std::string path;
	if (arg.empty())
		path = ".\\config\\GameServer.ini"; // default
	else
	{
		bool hasBackslash = arg.find('\\') != std::string::npos || arg.find('/') != std::string::npos;
		bool hasExt = arg.rfind('.') != std::string::npos;
		if (!hasBackslash && !hasExt)
			path = ".\\config\\" + arg + ".ini";
		else
			path = arg;
	}

	CNtlIniFile ini;
	if (!ini.Create(path.c_str()))
	{
		NTL_PRINT(PRINT_APP, _T("HelperNPC: failed to open %s"), s2ws(path).c_str());
		return;
	}
	if (GetHelperNpcManager()->LoadConfig(ini))
	{
		NTL_PRINT(PRINT_APP, _T("HelperNPC: config reloaded from %s"), s2ws(path).c_str());
	}
	else
	{
		NTL_PRINT(PRINT_APP, _T("HelperNPC: failed to reload config from %s"), s2ws(path).c_str());
	}
}

ACMD(do_helpernpc_metrics)
{
	// @helpernpc_metrics -> dump all metrics
	GetHelperNpcManager()->DumpMetrics();
}

ACMD(do_helpernpc_resetmetrics)
{
	// @helpernpc_resetmetrics -> zero counters
	GetHelperNpcManager()->ResetMetrics();
}

ACMD(do_helpernpc_refresh)
{
	// @helpernpc_refresh [respawn]
	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	std::string arg = ws2s(strToken);
	bool respawn = true;
	if (!arg.empty())
	{
		if (_stricmp(arg.c_str(), "norespawn") == 0 || _stricmp(arg.c_str(), "nr") == 0 || _stricmp(arg.c_str(), "off") == 0)
			respawn = false;
	}
	GetHelperNpcManager()->RefreshAllHelpers(respawn);
}

ACMD(do_customdrop_chainspawns)
{
	// usage: @customdrop_chainspawns on|off (no arg prints state)
	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	std::string arg = ws2s(strToken);
	if (!arg.empty())
	{
		bool on = (_stricmp(arg.c_str(), "on") == 0 || _stricmp(arg.c_str(), "1") == 0 || _stricmp(arg.c_str(), "true") == 0);
		g_pCustomDropEvent->SetAllowChainSpawns(on);
		NTL_PRINT(PRINT_APP, _T("CustomDropEvent: chain spawns %s"), (on ? L"ENABLED" : L"DISABLED"));
	}
	else
	{
		bool on = g_pCustomDropEvent->IsAllowChainSpawns();
		NTL_PRINT(PRINT_APP, _T("CustomDropEvent: chain spawns currently %s"), (on ? L"ENABLED" : L"DISABLED"));
	}
}

ACMD(do_customdrop_healmul)
{
	// usage: @customdrop_healmul <float>; no arg prints current
	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	std::string arg = ws2s(strToken);
	if (!arg.empty())
	{
		float mul = (float)atof(arg.c_str());
		if (mul < 0.0f) mul = 0.0f;
		g_pCustomDropEvent->SetTotemHealMultiplier(mul);
		NTL_PRINT(PRINT_APP, _T("CustomDropEvent: totem heal multiplier set to %.2f"), mul);
	}
	else
	{
		NTL_PRINT(PRINT_APP, _T("CustomDropEvent: totem heal multiplier = %.2f"), g_pCustomDropEvent->GetTotemHealMultiplier());
	}
}

ACMD(do_customdrop_buffduration)
{
	// usage: @customdrop_buffduration <ms>; 0 resets to skill/default. No arg prints current.
	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	std::string arg = ws2s(strToken);
	if (!arg.empty())
	{
		DWORD ms = (DWORD)strtoul(arg.c_str(), nullptr, 10);
		g_pCustomDropEvent->SetTotemBuffDurationOverrideMs(ms);
		NTL_PRINT(PRINT_APP, _T("CustomDropEvent: totem buff duration override set to %u ms"), ms);
	}
	else
	{
		NTL_PRINT(PRINT_APP, _T("CustomDropEvent: totem buff duration override = %u ms"), g_pCustomDropEvent->GetTotemBuffDurationOverrideMs());
	}
}

ACMD(do_reload_playermods_cfg)
{
	// optional path parameter
	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	std::string arg = ws2s(strToken);
	std::string path;
	if (arg.empty())
		path = ".\\config\\PlayerModifiers.cfg";
	else
	{
		bool hasBackslash = arg.find('\\') != std::string::npos || arg.find('/') != std::string::npos;
		bool hasExt = arg.rfind('.') != std::string::npos;
		if (!hasBackslash && !hasExt)
			path = ".\\config\\" + arg + ".cfg";
		else
			path = arg;
	}
	if (g_pPlayerModifiers->ReloadConfig(path.c_str()))
		NTL_PRINT(PRINT_APP, _T("PlayerModifiers: config reloaded from %s"), s2ws(path).c_str());
	else
		NTL_PRINT(PRINT_APP, _T("PlayerModifiers: failed to reload config from %s"), s2ws(path).c_str());
}

ACMD(do_playermods_toggle)
{
	// usage: @playermods on|off (no arg prints state)
	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	std::string arg = ws2s(strToken);
	if (!arg.empty())
	{
		bool on = (_stricmp(arg.c_str(), "on") == 0 || _stricmp(arg.c_str(), "1") == 0 || _stricmp(arg.c_str(), "true") == 0);
		g_pPlayerModifiers->SetEnabled(on);
		size_t n = g_pObjectManager->RecalculateAllPlayers();
		NTL_PRINT(PRINT_APP, _T("PlayerModifiers: %s; recalculated %zu players"), (on ? L"ENABLED" : L"DISABLED"), n);
	}
	else
	{
		NTL_PRINT(PRINT_APP, _T("PlayerModifiers: currently %s (cfg=%s)"), (g_pPlayerModifiers->IsEnabled() ? L"ENABLED" : L"DISABLED"), s2ws(g_pPlayerModifiers->GetCfgPath()).c_str());
	}
}

ACMD(do_vtransform)
{
	// Check if virtual transformations are enabled
	if (!g_pFeatureFlags->IsVirtualTransformationsEnabled())
	{
		NTL_PRINT(PRINT_APP, _T("Virtual transformations are currently disabled by feature flag"));
		return;
	}

	// usage: @vtransform <id> [target]
	// Example: @vtransform 200 (transforms self to Human->Namek test)
	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	if (strToken.empty())
	{
		NTL_PRINT(PRINT_APP, _T("Usage: @vtransform <id> [target]"));
		NTL_PRINT(PRINT_APP, _T("Available transforms:"));
		NTL_PRINT(PRINT_APP, _T("  100 - Super Saiyan Blue"));
		NTL_PRINT(PRINT_APP, _T("  101 - Ultra Instinct"));
		NTL_PRINT(PRINT_APP, _T("  200 - Test: Human to Namek"));
		NTL_PRINT(PRINT_APP, _T("  201 - Test: Human to Majin"));
		return;
	}

	DWORD virtualId = (DWORD)_wtoi(strToken.c_str());
	if (virtualId < 100)
	{
		NTL_PRINT(PRINT_APP, _T("Virtual transform IDs must be 100 or higher"));
		return;
	}

	// Check for target parameter
	pToken->PopToPeek();
	std::wstring strTarget = pToken->PeekNextToken(NULL, &iLine);
	CPlayer* pTargetPlayer = pPlayer;

	if (!strTarget.empty())
	{
		// Find target player by name
		pTargetPlayer = g_pObjectManager->FindByName((WCHAR*)strTarget.c_str());
		if (!pTargetPlayer)
		{
			NTL_PRINT(PRINT_APP, _T("Player '%s' not found"), strTarget.c_str());
			return;
		}
	}

	if (!pTargetPlayer || !pTargetPlayer->IsInitialized())
	{
		NTL_PRINT(PRINT_APP, _T("Invalid target player"));
		return;
	}

	// Debug: Check if transform exists before activating
	const auto* checkTransform = g_pVirtualTransformManager->GetVirtualTransform(virtualId);
	if (checkTransform)
	{
		NTL_PRINT(PRINT_APP, _T("DEBUG: Found transform %d: %s"), virtualId, checkTransform->name.c_str());
	}
	else
	{
		NTL_PRINT(PRINT_APP, _T("DEBUG: Transform %d NOT found in loaded transforms!"), virtualId);
	}

	if (g_pVirtualTransformManager->ActivateVirtualTransform(pTargetPlayer, virtualId))
	{
		const auto* transform = g_pVirtualTransformManager->GetVirtualTransform(virtualId);
		if (transform)
		{
			NTL_PRINT(PRINT_APP, _T("Activated virtual transform %d (%s) for %s"),
				virtualId, transform->name.c_str(), pTargetPlayer->GetCharName());
		}
		else
		{
			NTL_PRINT(PRINT_APP, _T("Activated virtual transform %d for %s"),
				virtualId, pTargetPlayer->GetCharName());
		}
	}
	else
	{
		NTL_PRINT(PRINT_APP, _T("Failed to activate virtual transform %d (not found in config)"), virtualId);
	}
}

ACMD(do_vtransform_end)
{
	// Check if virtual transformations are enabled
	if (!g_pFeatureFlags->IsVirtualTransformationsEnabled())
	{
		NTL_PRINT(PRINT_APP, _T("Virtual transformations are currently disabled by feature flag"));
		return;
	}

	// usage: @vtransform_end [target]
	pToken->PopToPeek();
	std::wstring strTarget = pToken->PeekNextToken(NULL, &iLine);
	CPlayer* pTargetPlayer = pPlayer;

	if (!strTarget.empty())
	{
		pTargetPlayer = g_pObjectManager->FindByName((WCHAR*)strTarget.c_str());
		if (!pTargetPlayer)
		{
			NTL_PRINT(PRINT_APP, _T("Player '%s' not found"), strTarget.c_str());
			return;
		}
	}

	if (!pTargetPlayer || !pTargetPlayer->IsInitialized())
	{
		NTL_PRINT(PRINT_APP, _T("Invalid target player"));
		return;
	}

	if (g_pVirtualTransformManager->DeactivateVirtualTransform(pTargetPlayer))
	{
		NTL_PRINT(PRINT_APP, _T("Deactivated virtual transform for %s"), pTargetPlayer->GetCharName());
	}
	else
	{
		NTL_PRINT(PRINT_APP, _T("No virtual transform active for %s"), pTargetPlayer->GetCharName());
	}
}

ACMD(do_buff)
{
	/*
		@buff BUFF_ID DURATION(SECONDS) [RADIUS_METERS] [TARGET_NAME]
		- If RADIUS_METERS > 0, applies to all PCs within radius of target (or caster if no target).
		- If RADIUS_METERS omitted or 0, applies only to target (or caster if target missing).
	*/
	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	int buffindex = (int)atof(ws2s(strToken).c_str());

	pToken->PopToPeek();
	std::wstring strToken1 = pToken->PeekNextToken(NULL, &iLine);
	int nSeconds = (int)atof(ws2s(strToken1).c_str());

	// optional radius
	pToken->PopToPeek();
	std::wstring strToken2 = pToken->PeekNextToken(NULL, &iLine);
	std::string opt2 = ws2s(strToken2);
	float fRadius = 0.0f;
	const wchar_t* wname = L"";
	if (!opt2.empty())
	{
		// decide if this token is a number (radius) or start of name
		bool isNum = !opt2.empty() && (isdigit((unsigned char)opt2[0]) || opt2[0] == '.' || opt2[0] == '-');
		if (isNum)
		{
			fRadius = (float)atof(opt2.c_str());
			// optional 4th token: name
			pToken->PopToPeek();
			std::wstring strToken3 = pToken->PeekNextToken(NULL, &iLine);
			if (!strToken3.empty())
			{
				static std::wstring name; name = strToken3; wname = name.c_str();
			}
		}
		else
		{
			static std::wstring name; name = std::wstring(strToken2.begin(), strToken2.end()); wname = name.c_str();
		}
	}

	CPlayer* cTarget = nullptr;
	if (wname && *wname)
		cTarget = g_pObjectManager->FindByName(wname);
	if (!cTarget || !cTarget->IsInitialized())
		cTarget = pPlayer;

	if (nSeconds <= 0 || nSeconds > 3600)
		nSeconds = 3600;

	sSKILL_TBLDAT* pSkillTbldat = (sSKILL_TBLDAT*)g_pTableContainer->GetSkillTable()->FindData(buffindex);
	if (!pSkillTbldat)
		return;

	// helper lambda to apply to one player
	auto applyTo = [&](CPlayer* tgt)
		{
			if (!tgt || !tgt->IsInitialized()) return;
			sDBO_BUFF_PARAMETER aBuffParameter[NTL_MAX_EFFECT_IN_SKILL];
			eSYSTEM_EFFECT_CODE aeEffectCode[NTL_MAX_EFFECT_IN_SKILL];
			for (int i = 0; i < NTL_MAX_EFFECT_IN_SKILL; i++)
			{
				aBuffParameter[i].byBuffParameterType = DBO_BUFF_PARAMETER_TYPE_DEFAULT;
				aBuffParameter[i].buffParameter.fParameter = (float)(pSkillTbldat->aSkill_Effect_Value[i]);
				aBuffParameter[i].buffParameter.dwRemainValue = (DWORD)pSkillTbldat->aSkill_Effect_Value[i];
				aeEffectCode[i] = g_pTableContainer->GetSystemEffectTable()->GetEffectCodeWithTblidx(pSkillTbldat->skill_Effect[i]);
				if (aeEffectCode[i] == ACTIVE_HEAL_OVER_TIME || aeEffectCode[i] == ACTIVE_EP_OVER_TIME)
				{
					aBuffParameter[i].byBuffParameterType = DBO_BUFF_PARAMETER_TYPE_HOT;
					aBuffParameter[i].buffParameter.dwRemainTime = pSkillTbldat->dwKeepTimeInMilliSecs;
				}
				else if (aeEffectCode[i] == ACTIVE_BLEED || aeEffectCode[i] == ACTIVE_POISON || aeEffectCode[i] == ACTIVE_STOMACHACHE || aeEffectCode[i] == ACTIVE_BURN)
				{
					aBuffParameter[i].byBuffParameterType = DBO_BUFF_PARAMETER_TYPE_DOT;
					aBuffParameter[i].buffParameter.dwRemainTime = pSkillTbldat->dwKeepTimeInMilliSecs;
				}
			}
			// Handle direct heal instantly and do not register as a buff
			bool hasBuffable = false;
			for (int i = 0; i < NTL_MAX_EFFECT_IN_SKILL; ++i)
			{
				//if (aeEffectCode[i] == ACTIVE_DIRECT_HEAL)
				//{
				//	float amt = 0.0f;
				//	CalcDirectHeal(pPlayer, pSkillTbldat, (BYTE)i, amt);
				//	if (amt != 0.0f)
				//	{
				//		tgt->UpdateCurLP((int)amt, true, false);
				//		tgt->SendEffectAffected(g_pTableContainer->GetSystemEffectTable()->GetEffectTblidx(aeEffectCode[i]), DBO_OBJECT_SOURCE_SKILL, pSkillTbldat->tblidx, amt, 0.0f, pPlayer->GetID());
				//	}
				//	aeEffectCode[i] = INVALID_SYSTEM_EFFECT_CODE;
				//}
				if (aeEffectCode[i] != INVALID_SYSTEM_EFFECT_CODE)
					hasBuffable = true;
			}
			DWORD dwDurationInMs = (DWORD)(nSeconds * 1000);
			if (hasBuffable)
				tgt->GetBuffManager()->RegisterBuff(dwDurationInMs, aeEffectCode, aBuffParameter, INVALID_HOBJECT, BUFF_TYPE_BLESS, pSkillTbldat);
		};

	if (fRadius > 0.0f && cTarget->GetCurWorldCell())
	{
		// iterate nearby PCs within radius
		CWorldCell* pCell = cTarget->GetCurWorldCell();
		CWorldCell::QUADPAGE page = pCell->GetCellQuadPage(cTarget->GetCurLoc());
		for (int dir = CWorldCell::QUADDIR_SELF; dir <= CWorldCell::QUADDIR_VERTICAL; dir++)
		{
			CWorldCell* pSibling = pCell->GetQuadSibling(page, (CWorldCell::QUADDIR)dir);
			if (!pSibling) continue;
			CPlayer* pPlr = (CPlayer*)pSibling->GetObjectList()->GetFirst(OBJTYPE_PC);
			while (pPlr && pPlr->IsInitialized())
			{
				if (cTarget->IsInRange(pPlr, fRadius))
					applyTo(pPlr);
				pPlr = (CPlayer*)pSibling->GetObjectList()->GetNext(pPlr->GetWorldCellObjectLinker());
			}
		}
	}
	else
	{
		applyTo(cTarget);
	}
}

ACMD(do_setdark)
{
	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	BYTE dark = (BYTE)atof(ws2s(strToken).c_str());

	if (dark == 1) {
		CWorldZone* pWorldZone = pPlayer->GetCurWorldZone();
		pWorldZone->UpdateZoneInfo(true);
	}
	else {
		CWorldZone* pWorldZone = pPlayer->GetCurWorldZone();
		pWorldZone->UpdateZoneInfo(false);
	}
}

ACMD(do_resetskills)
{
	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);

	std::wstring name = std::wstring(strToken.begin(), strToken.end());
	const wchar_t* wname = name.c_str();

	CPlayer* target = g_pObjectManager->FindByName(wname);
	if (!target || !target->IsInitialized())
	{
		return;
	}

	CGameServer* app = (CGameServer*)g_pApp;

	if (target->GetAspectStateId() == ASPECTSTATE_INVALID)
	{
		CNtlPacket pQry(sizeof(sGQ_SKILL_INIT_REQ));
		sGQ_SKILL_INIT_REQ* rQry = (sGQ_SKILL_INIT_REQ*)pQry.GetPacketData();
		rQry->wOpCode = GQ_SKILL_INIT_REQ;
		rQry->handle = target->GetID();
		rQry->charId = target->GetCharID();
		rQry->dwSP = (DWORD)target->GetLevel() - 1;
		rQry->bySkillResetMethod = 0;
		pQry.SetPacketLen(sizeof(sGQ_SKILL_INIT_REQ));
		app->SendTo(app->GetQueryServerSession(), &pQry);
	}
}

ACMD(do_PlayerCount)
{

	CNtlStringW msg;

	CNtlPacket packetMsg(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	sGU_SYSTEM_DISPLAY_TEXT* resMsg = (sGU_SYSTEM_DISPLAY_TEXT*)packetMsg.GetPacketData();
	resMsg->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
	resMsg->byDisplayType = SERVER_TEXT_SYSTEM;
	resMsg->wMessageLengthInUnicode = (WORD)msg.Format(L"%d Players Online", g_pObjectManager->GetPlayerCount());
	NTL_SAFE_WCSCPY(resMsg->awchMessage, msg.c_str());
	pPlayer->SendPacket(&packetMsg);
}
ACMD(do_setspeed)
{
	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	float fSpeed = (float)atof(ws2s(strToken).c_str());

	pPlayer->UpdateMoveSpeed(fSpeed, fSpeed);
	pPlayer->UpdateAttackSpeed(300);
}

ACMD(do_addmob)
{
	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	TBLIDX MobId = INVALID_TBLIDX;
	// Try numeric first
	{
		std::string as = ws2s(strToken);
		bool allDigits = !as.empty() && std::all_of(as.begin(), as.end(), [](unsigned char ch) { return isdigit(ch); });
		if (allDigits) MobId = (TBLIDX)atoi(as.c_str());
	}
	// If not numeric, try resolve by name via ArenaManager
	if (MobId == INVALID_TBLIDX)
	{
		unsigned int id = g_pArenaManager->ResolveMobIdByName(strToken);
		if (id != 0) MobId = (TBLIDX)id;
	}

	sMOB_TBLDAT* pMOBTblData = (sMOB_TBLDAT*)g_pTableContainer->GetMobTable()->FindData(MobId);

	if (pMOBTblData)
	{
		sVECTOR3 spawnloc;
		spawnloc.x = pPlayer->GetCurLoc().x + rand() % 5;
		spawnloc.y = pPlayer->GetCurLoc().y;
		spawnloc.z = pPlayer->GetCurLoc().z + rand() % 5;

		sVECTOR3 spawndir;
		spawndir.x = pPlayer->GetCurDir().x + rand() % 5;
		spawndir.y = pPlayer->GetCurDir().y;
		spawndir.z = pPlayer->GetCurDir().z + rand() % 5;

		sSPAWN_TBLDAT sMobSpawn;
		sMobSpawn.vSpawn_Dir.CopyFrom(spawndir);
		sMobSpawn.vSpawn_Loc.CopyFrom(spawnloc);
		sMobSpawn.dwParty_Index = INVALID_DWORD;
		sMobSpawn.byMove_Range = 30;
		sMobSpawn.bySpawn_Move_Type = SPAWN_MOVE_WANDER;
		sMobSpawn.bySpawn_Loc_Range = 30;
		sMobSpawn.byWander_Range = 30;
		sMobSpawn.path_Table_Index = INVALID_TBLIDX;
		sMobSpawn.playScript = INVALID_TBLIDX;
		sMobSpawn.playScriptScene = INVALID_TBLIDX;
		sMobSpawn.aiScript = INVALID_TBLIDX;
		sMobSpawn.aiScriptScene = INVALID_TBLIDX;
		sMobSpawn.actionPatternTblidx = 1;

		printf("dwFormulaOffset: %u, wUseRace: %u, wMonsterClass: %u, LP:%u, fSettingRate_LP:%f, EP:%u, Str:%u, Con:%u, Foc:%u, Dex:%u, Sol:%u, Eng:%u\n",
			pMOBTblData->dwFormulaOffset, pMOBTblData->dwMobGroup, pMOBTblData->wMonsterClass, pMOBTblData->dwBasic_LP, pMOBTblData->fSettingRate_LP,
			pMOBTblData->wBasic_EP,
			pMOBTblData->wBasicStr, pMOBTblData->wBasicCon, pMOBTblData->wBasicFoc, pMOBTblData->wBasicDex, pMOBTblData->wBasicSol, pMOBTblData->wBasicEng);

		CMonster* pMob = (CMonster*)g_pObjectManager->CreateCharacter(OBJTYPE_MOB);
		pMob->CreateDataAndSpawn(pPlayer->GetWorldID(), pMOBTblData, &sMobSpawn, false, 0);
	}
}

ACMD(do_addmobgroup)
{
	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	TBLIDX groupId = (TBLIDX)atof(ws2s(strToken).c_str());

	DWORD spawnCount = g_pTableContainer->GetMobSpawnTable(pPlayer->GetCurWorld()->GetIdx())->GetSpawnGroupCount(groupId);
	sSPAWN_TBLDAT* spawnTbldat = g_pTableContainer->GetMobSpawnTable(pPlayer->GetCurWorld()->GetIdx())->GetSpawnGroupFirst(groupId);

	for (DWORD i = 0; i < spawnCount; i++)
	{
		if (spawnTbldat)
		{
			for (int ii = 0; ii < spawnTbldat->bySpawn_Quantity; ii++)
			{
				sMOB_DATA data;
				InitMobData(data);

				data.spawnGroupId = groupId;
				data.worldID = pPlayer->GetCurWorld()->GetID();
				data.worldtblidx = pPlayer->GetCurWorld()->GetIdx();
				data.tblidx = spawnTbldat->mob_Tblidx;
				spawnTbldat->vSpawn_Loc.CopyTo(data.vCurLoc);
				spawnTbldat->vSpawn_Loc.CopyTo(data.vSpawnLoc);
				spawnTbldat->vSpawn_Dir.CopyTo(data.vCurDir);
				spawnTbldat->vSpawn_Dir.CopyTo(data.vSpawnDir);
				data.bySpawnFuncFlag = 0;
				data.sScriptData.playScript = spawnTbldat->playScript;
				data.sScriptData.playScriptScene = spawnTbldat->playScriptScene;
				data.sScriptData.tblidxAiScript = spawnTbldat->aiScript;
				data.sScriptData.tblidxAiScriptScene = spawnTbldat->aiScriptScene;
				data.qwCharConditionFlag = 0;
				data.partyID = spawnTbldat->dwParty_Index;
				data.bPartyLeader = spawnTbldat->bParty_Leader;
				data.byImmortalMode = eIMMORTAL_MODE_OFF;
				data.actionpatternTblIdx = spawnTbldat->actionPatternTblidx;
				data.bySpawnRange = spawnTbldat->bySpawn_Loc_Range;
				data.wSpawnTime = spawnTbldat->wSpawn_Cool_Time;
				data.byMoveType = spawnTbldat->bySpawn_Move_Type;
				data.byWanderRange = spawnTbldat->byWander_Range;
				data.byMoveRange = spawnTbldat->byMove_Range;
				data.pathTblidx = spawnTbldat->path_Table_Index;
				data.hTargetFixedExecuter = INVALID_HOBJECT;
				data.sBotSubData.byNestRange = 20;
				data.sBotSubData.byNestType = NPC_NEST_TYPE_DEFAULT;

				sMOB_TBLDAT* pTbldat = (sMOB_TBLDAT*)g_pTableContainer->GetMobTable()->FindData(data.tblidx);
				if (pTbldat)
				{
					CMonster* pMob = (CMonster*)g_pObjectManager->CreateCharacter(OBJTYPE_MOB);
					if (pMob)
					{
						if (pMob->CreateDataAndSpawn(data, pTbldat))
						{
							pMob->SetStandAlone(false);
						}
						else
						{
							break;
						}
					}
					else
					{
						break;
					}
				}
				else
				{
					ERR_LOG(LOG_SCRIPT, "Could not find MOB-TBLDAT. Tblidx %u Grouptblidx %u", data.tblidx, groupId);
					break;
				}
			}
		}

		spawnTbldat = g_pTableContainer->GetMobSpawnTable(pPlayer->GetCurWorld()->GetIdx())->GetSpawnGroupNext(groupId);
	}
}

ACMD(do_addnpc)
{
	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	TBLIDX npcid = (TBLIDX)atof(ws2s(strToken).c_str());

	sNPC_TBLDAT* pTblData = (sNPC_TBLDAT*)g_pTableContainer->GetNpcTable()->FindData(npcid);

	if (pTblData)
	{
		sVECTOR3 spawnloc;
		spawnloc.x = pPlayer->GetCurLoc().x + rand() % 5;
		spawnloc.y = pPlayer->GetCurLoc().y;
		spawnloc.z = pPlayer->GetCurLoc().z + rand() % 5;

		sVECTOR3 spawndir;
		spawndir.x = pPlayer->GetCurDir().x + rand() % 5;
		spawndir.y = pPlayer->GetCurDir().y;
		spawndir.z = pPlayer->GetCurDir().z + rand() % 5;

		sSPAWN_TBLDAT sSpawn;
		sSpawn.vSpawn_Dir.CopyFrom(spawndir);
		sSpawn.vSpawn_Loc.CopyFrom(spawnloc);
		sSpawn.dwParty_Index = INVALID_DWORD;
		sSpawn.byMove_Range = INVALID_BYTE;
		sSpawn.bySpawn_Move_Type = SPAWN_MOVE_UNKNOWN;
		sSpawn.bySpawn_Loc_Range = 10;
		sSpawn.byWander_Range = INVALID_BYTE;
		sSpawn.path_Table_Index = INVALID_TBLIDX;
		sSpawn.playScript = INVALID_TBLIDX;
		sSpawn.playScriptScene = INVALID_TBLIDX;
		sSpawn.aiScript = INVALID_TBLIDX;
		sSpawn.aiScriptScene = INVALID_TBLIDX;
		sSpawn.actionPatternTblidx = 1;

		CNpc* pNpc = (CNpc*)g_pObjectManager->CreateCharacter(OBJTYPE_NPC);
		pNpc->CreateDataAndSpawn(pPlayer->GetWorldID(), pTblData, &sSpawn, false, 0);
		pNpc->SetStandAlone(false);
		pNpc = pPlayer->GetCurWorld()->FindNpc(npcid);

		if (pNpc == NULL)
		{
			printf("npc not found in world\n");
		}
	}
	else { ERR_LOG(LOG_SYSTEM, _T("[GENERAL] npc not found %u. GM %u"), npcid, pPlayer->GetCharID()); }
}

ACMD(do_additem)
{
	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	TBLIDX ItemId = (TBLIDX)atof(ws2s(strToken).c_str());

	pToken->PopToPeek();
	strToken = pToken->PeekNextToken(NULL, &iLine);
	BYTE amount = (BYTE)atof(ws2s(strToken).c_str());

	if (amount == 0 || amount == INVALID_BYTE)
		amount = 1;

	pToken->PopToPeek();
	strToken = pToken->PeekNextToken(NULL, &iLine);

	CPlayer* pTarget = pPlayer;

	if (!strToken.empty())
	{
		std::wstring name = std::wstring(strToken.begin(), strToken.end());
		const wchar_t* wname = name.c_str();

		pTarget = g_pObjectManager->FindByName(wname);
		if (!pTarget || !pTarget->IsInitialized())
		{
			pTarget = pPlayer; // if target not found, use self
		}
	}

	if (pTarget->GetPlayerItemContainer()->CountEmptyInventory() >= 1)
	{
		sITEM_TBLDAT* pTblData = (sITEM_TBLDAT*)g_pTableContainer->GetItemTable()->FindData(ItemId);
		if (pTblData)
		{
			if (pTblData->bValidity_Able == true && pTblData->byItem_Type != eITEM_TYPE::ITEM_TYPE_RECIPE)
			{
				if (amount > pTblData->byMax_Stack)
					amount = pTblData->byMax_Stack;

				g_pItemManager->CreateItem(pTarget, ItemId, amount, INVALID_BYTE, INVALID_BYTE, pTblData->Item_Option_Tblidx == INVALID_TBLIDX);
			}
			//else NTL_PRINT(PRINT_APP, "GmAddItem(TBLIDX itemid) item not bValidity_Able true ud", ItemId);
		}
		//else NTL_PRINT(PRINT_APP, "GmAddItem(TBLIDX itemid) item not found %u", ItemId);
	}
}

ACMD(do_additem_group)
{
	/*
		@additem_group ITEM_ID AMOUNT RANGE [TARGET_NAME]
		- Gives the specified item to all players within the specified range (in meters)
		- If TARGET_NAME is specified, uses that player as the center point
		- If TARGET_NAME is omitted, uses the command issuer as the center point
		- RANGE can be 0 to give only to the target/issuer
	*/
	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	TBLIDX ItemId = (TBLIDX)atof(ws2s(strToken).c_str());

	pToken->PopToPeek();
	strToken = pToken->PeekNextToken(NULL, &iLine);
	BYTE amount = (BYTE)atof(ws2s(strToken).c_str());

	if (amount == 0 || amount == INVALID_BYTE)
		amount = 1;

	pToken->PopToPeek();
	strToken = pToken->PeekNextToken(NULL, &iLine);
	float fRange = (float)atof(ws2s(strToken).c_str());

	if (fRange < 0.0f)
		fRange = 0.0f;

	pToken->PopToPeek();
	strToken = pToken->PeekNextToken(NULL, &iLine);

	CPlayer* pTarget = pPlayer;

	if (!strToken.empty())
	{
		std::wstring name = std::wstring(strToken.begin(), strToken.end());
		const wchar_t* wname = name.c_str();

		pTarget = g_pObjectManager->FindByName(wname);
		if (!pTarget || !pTarget->IsInitialized())
		{
			pTarget = pPlayer; // if target not found, use self
		}
	}

	// Validate item exists and is valid
	sITEM_TBLDAT* pTblData = (sITEM_TBLDAT*)g_pTableContainer->GetItemTable()->FindData(ItemId);
	if (!pTblData)
		return; // invalid item ID

	if (pTblData->bValidity_Able == false || pTblData->byItem_Type == eITEM_TYPE::ITEM_TYPE_RECIPE)
		return; // item not valid or is a recipe

	// Ensure amount doesn't exceed max stack
	if (amount > pTblData->byMax_Stack)
		amount = pTblData->byMax_Stack;

	// Helper lambda to give item to one player
	auto giveItemTo = [&](CPlayer* target)
		{
			if (!target || !target->IsInitialized()) return;
			if (target->GetPlayerItemContainer()->CountEmptyInventory() >= 1)
			{
				g_pItemManager->CreateItem(target, ItemId, amount, INVALID_BYTE, INVALID_BYTE, pTblData->Item_Option_Tblidx == INVALID_TBLIDX);
			}
		};

	if (fRange > 0.0f && pTarget->GetCurWorldCell())
	{
		// Give to all players within range
		CWorldCell* pCell = pTarget->GetCurWorldCell();
		CWorldCell::QUADPAGE page = pCell->GetCellQuadPage(pTarget->GetCurLoc());
		for (int dir = CWorldCell::QUADDIR_SELF; dir <= CWorldCell::QUADDIR_VERTICAL; dir++)
		{
			CWorldCell* pSibling = pCell->GetQuadSibling(page, (CWorldCell::QUADDIR)dir);
			if (!pSibling) continue;
			CPlayer* pPlr = (CPlayer*)pSibling->GetObjectList()->GetFirst(OBJTYPE_PC);
			while (pPlr && pPlr->IsInitialized())
			{
				if (pTarget->IsInRange(pPlr, fRange))
					giveItemTo(pPlr);
				pPlr = (CPlayer*)pSibling->GetObjectList()->GetNext(pPlr->GetWorldCellObjectLinker());
			}
		}
	}
	else
	{
		// Give only to the target player
		giveItemTo(pTarget);
	}
}

ACMD(do_sessioninfo)
{
	/*
		@sessioninfo - Shows current session statistics for connection debugging
		Displays current vs max sessions, memory usage, and connection statistics
	*/
	CGameServer* app = (CGameServer*)g_pApp;

	// Get current session counts from the network
	int currentSessions = app->GetNetwork()->GetSessionList()->GetCurCount();
	int maxSessions = app->GetNetwork()->GetSessionList()->GetMaxCount();
	int configMaxSessions = app->m_config.nMaxConnection;

	// Get current player count from ObjectManager
	size_t playerCount = g_pObjectManager->GetPlayerCount();

	// Identify server instance by port/config
	WORD serverPort = app->m_config.wClientAcceptPort;
	const char* instanceType = "Unknown";
	if (serverPort == 30000) instanceType = "Channel 0";
	else if (serverPort == 30001) instanceType = "Channel 1";
	else if (serverPort == 30009) instanceType = "Budokai Tournament";

	// Format the response message with multi-instance awareness
	char szMessage[1500];
	sprintf_s(szMessage, sizeof(szMessage),
		"[SESSION INFO - %s (Port: %d)]\n"
		"Current Sessions: %d\n"
		"Max Session Capacity: %d\n"
		"Config Max Connections: %d\n"
		"Session Utilization: %.1f%%\n"
		"Available Slots: %d\n"
		"Player Objects: %zu\n"
		"Session Overhead: %d\n"
		"\n[INSTANCE ANALYSIS]\n"
		"%s"
		"\n[LEAK DETECTION]\n"
		"%s",
		instanceType, serverPort,
		currentSessions,
		maxSessions,
		configMaxSessions,
		configMaxSessions > 0 ? (float)currentSessions / configMaxSessions * 100.0f : 0.0f,
		configMaxSessions - currentSessions,
		playerCount,
		currentSessions - (int)playerCount,
		(serverPort == 30009) ? "BUDOKAI SERVER - Monitor tournament session cleanup!" : "Regular game channel",
		(currentSessions > (int)playerCount + 15) ? "HIGH SESSION OVERHEAD - Investigate cleanup!" :
		(currentSessions > (int)playerCount + 10) ? "MODERATE SESSION OVERHEAD - Monitor closely" :
		(currentSessions >= configMaxSessions) ? "CRITICAL: Session limit reached!" :
		"Session levels appear normal"
	);

	// Send system message to the GM
	CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
	res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
	res->byDisplayType = SERVER_TEXT_SYSTEM;

	// Convert to wide string
	std::string message(szMessage);
	std::wstring wideMessage(message.begin(), message.end());
	wcscpy_s(res->awchMessage, NTL_MAX_LENGTH_OF_CHAT_MESSAGE + 1, wideMessage.c_str());

	packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	app->Send(pPlayer->GetClientSessionID(), &packet);

	// Also log to console for server admin
	NTL_PRINT(PRINT_APP, _T("GM %u requested session info: %d/%d sessions active (%.1f%% utilization)"),
		pPlayer->GetCharID(), currentSessions, configMaxSessions,
		configMaxSessions > 0 ? (float)currentSessions / configMaxSessions * 100.0f : 0.0f);
}

ACMD(do_sessioncleanup)
{
	/*
		@sessioncleanup - Forces cleanup of dead/invalid sessions
		This command can help resolve connection issues by forcing cleanup
		of sessions that may be stuck or not properly removed
	*/
	CGameServer* app = (CGameServer*)g_pApp;

	// Get session counts before cleanup
	int sessionsBefore = app->GetNetwork()->GetSessionList()->GetCurCount();

	// Force session list validation/cleanup
	DWORD currentTime = GetTickCount();
	app->GetNetwork()->GetSessionList()->ValidCheck(currentTime);

	// Get session counts after cleanup
	int sessionsAfter = app->GetNetwork()->GetSessionList()->GetCurCount();
	int sessionsRemoved = sessionsBefore - sessionsAfter;

	// Format the response message
	char szMessage[512];
	sprintf_s(szMessage, sizeof(szMessage),
		"[SESSION CLEANUP COMPLETE]\n"
		"Sessions before cleanup: %d\n"
		"Sessions after cleanup: %d\n"
		"Sessions removed: %d\n"
		"Available slots now: %d",
		sessionsBefore,
		sessionsAfter,
		sessionsRemoved,
		app->GetNetwork()->GetSessionList()->GetMaxCount() - sessionsAfter
	);

	// Send system message to the GM
	CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
	res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
	res->byDisplayType = SERVER_TEXT_SYSTEM;

	// Convert to wide string
	std::string message(szMessage);
	std::wstring wideMessage(message.begin(), message.end());
	wcscpy_s(res->awchMessage, NTL_MAX_LENGTH_OF_CHAT_MESSAGE + 1, wideMessage.c_str());

	packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	app->Send(pPlayer->GetClientSessionID(), &packet);

	// Also log to console for server admin
	NTL_PRINT(PRINT_APP, _T("GM %u forced session cleanup: %d sessions removed (%d -> %d)"),
		pPlayer->GetCharID(), sessionsRemoved, sessionsBefore, sessionsAfter);
}

ACMD(do_budokaiinfo)
{
	/*
		@budokaiinfo - Specialized monitoring for Budokai tournament server
		Provides detailed analysis specifically for the tournament server instance
		including session leak detection related to tournament events
	*/
	CGameServer* app = (CGameServer*)g_pApp;

	// Identify server instance
	WORD serverPort = app->m_config.wClientAcceptPort;
	bool isBudokaiServer = (serverPort == 30009);

	char szMessage[1500];
	if (!isBudokaiServer) {
		sprintf_s(szMessage, sizeof(szMessage),
			"[BUDOKAI INFO]\n"
			"Current server port: %d\n"
			"This is NOT the Budokai server instance\n"
			"Expected Budokai port: 30009\n"
			"Use this command on the tournament server",
			serverPort
		);
	}
	else {
		// Get session information
		int currentSessions = app->GetNetwork()->GetSessionList()->GetCurCount();
		int maxSessions = app->GetNetwork()->GetSessionList()->GetMaxCount();
		int configMaxSessions = app->m_config.nMaxConnection;
		size_t playerCount = g_pObjectManager->GetPlayerCount();
		int sessionOverhead = currentSessions - (int)playerCount;

		sprintf_s(szMessage, sizeof(szMessage),
			"[BUDOKAI TOURNAMENT SERVER]\n"
			"Server Port: %d (Confirmed)\n"
			"Current Sessions: %d\n"
			"Current Players: %zu\n"
			"Session Overhead: %d\n"
			"Max Capacity: %d\n"
			"Available Slots: %d\n"
			"\n[BUDOKAI STATUS]\n"
			"%s"
			"\n[LEAK ANALYSIS]\n"
			"%s"
			"\n[MONITORING TIPS]\n"
			"- Use before/after tournaments\n"
			"- Watch for session buildup\n"
			"- Use @sessioncleanup if needed",
			serverPort,
			currentSessions,
			playerCount,
			sessionOverhead,
			configMaxSessions,
			configMaxSessions - currentSessions,
			(g_pBudokaiManager) ? "Budokai Manager: ACTIVE" : "Budokai Manager: NULL (Issue?)",
			(sessionOverhead > 20) ? "CRITICAL: High session overhead!" :
			(sessionOverhead > 15) ? "WARNING: Elevated session overhead" :
			(sessionOverhead > 10) ? "MODERATE: Monitor session cleanup" :
			"NORMAL: Session levels appear healthy"
		);
	}

	// Send system message to the GM
	CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
	res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
	res->byDisplayType = SERVER_TEXT_SYSTEM;

	// Convert to wide string
	std::string message(szMessage);
	std::wstring wideMessage(message.begin(), message.end());
	wcscpy_s(res->awchMessage, NTL_MAX_LENGTH_OF_CHAT_MESSAGE + 1, wideMessage.c_str());

	packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	app->Send(pPlayer->GetClientSessionID(), &packet);

	// Also log to console
	NTL_PRINT(PRINT_APP, _T("GM %u requested Budokai info on port %d"), pPlayer->GetCharID(), serverPort);
}

ACMD(do_addmasteritem)
{
	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	BYTE bySecondClass = (BYTE)atof(ws2s(strToken).c_str());

	TBLIDX itemTblidx = INVALID_TBLIDX;

	pToken->PopToPeek();
	strToken = pToken->PeekNextToken(NULL, &iLine);

	CPlayer* pTarget = pPlayer;

	if (!strToken.empty())
	{
		std::wstring name = std::wstring(strToken.begin(), strToken.end());
		const wchar_t* wname = name.c_str();

		pTarget = g_pObjectManager->FindByName(wname);
		if (!pTarget || !pTarget->IsInitialized())
		{
			pTarget = pPlayer; // if target not found, use self
		}
	}


	if (pTarget->GetClass() > PC_CLASS_1_LAST)
		return;

	switch (pTarget->GetClass())
	{
	case PC_CLASS_HUMAN_FIGHTER:
	{
		switch (bySecondClass)
		{
		case PC_CLASS_STREET_FIGHTER: itemTblidx = 99078; break;
		case PC_CLASS_SWORD_MASTER: itemTblidx = 99079; break;

		default: return; break;
		}
	}
	break;
	case PC_CLASS_HUMAN_MYSTIC:
	{
		switch (bySecondClass)
		{
		case PC_CLASS_CRANE_ROSHI: itemTblidx = 99081; break;
		case PC_CLASS_TURTLE_ROSHI: itemTblidx = 99080; break;

		default: return; break;
		}
	}
	break;
	case PC_CLASS_NAMEK_FIGHTER:
	{
		switch (bySecondClass)
		{
		case PC_CLASS_DARK_WARRIOR: itemTblidx = 99082; break;
		case PC_CLASS_SHADOW_KNIGHT: itemTblidx = 99083; break;

		default: return; break;
		}
	}
	break;
	case PC_CLASS_NAMEK_MYSTIC:
	{
		switch (bySecondClass)
		{
		case PC_CLASS_DENDEN_HEALER: itemTblidx = 99084; break;
		case PC_CLASS_POCO_SUMMONER: itemTblidx = 99085; break;

		default: return; break;
		}
	}
	break;
	case PC_CLASS_MIGHTY_MAJIN:
	{
		switch (bySecondClass)
		{
		case PC_CLASS_ULTI_MA: itemTblidx = 99086; break;
		case PC_CLASS_GRAND_MA: itemTblidx = 99087; break;

		default: return; break;
		}
	}
	break;
	case PC_CLASS_WONDER_MAJIN:
	{
		switch (bySecondClass)
		{
		case PC_CLASS_PLAS_MA: itemTblidx = 99088; break;
		case PC_CLASS_KAR_MA: itemTblidx = 99089; break;

		default: return; break;
		}
	}
	break;

	default: return; break;
	}

	sITEM_TBLDAT* pTblData = (sITEM_TBLDAT*)g_pTableContainer->GetItemTable()->FindData(itemTblidx);
	if (pTblData)
	{
		if (pTarget->GetPlayerItemContainer()->GetItemByIdx(itemTblidx) == NULL)
		{
			std::pair<BYTE, BYTE> inv = pTarget->GetPlayerItemContainer()->GetEmptyInventory();
			if (inv.first != INVALID_BYTE && inv.second != INVALID_BYTE)
				g_pItemManager->CreateItem(pTarget, itemTblidx, 1, inv.first, inv.second);
		}
	}
}

ACMD(do_addskill)
{
	CGameServer* app = (CGameServer*)g_pApp;

	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	TBLIDX SkillId = (TBLIDX)atof(ws2s(strToken).c_str());

	WORD wTemp;

	pToken->PopToPeek();
	strToken = pToken->PeekNextToken(NULL, &iLine);

	CPlayer* pTarget = pPlayer;

	if (!strToken.empty())
	{
		std::wstring name = std::wstring(strToken.begin(), strToken.end());
		const wchar_t* wname = name.c_str();

		pTarget = g_pObjectManager->FindByName(wname);
		if (!pTarget || !pTarget->IsInitialized())
		{
			pTarget = pPlayer; // if target not found, use self
		}
	}

	if (pTarget->GetSkillManager())
		pTarget->GetSkillManager()->LearnSkill(SkillId, wTemp, false);
}
ACMD(do_fly)
{
	CGameServer* app = (CGameServer*)g_pApp;
	WORD wTemp;

	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);

	CPlayer* pTarget = pPlayer;

	if (!strToken.empty())
	{
		std::wstring name = std::wstring(strToken.begin(), strToken.end());
		const wchar_t* wname = name.c_str();

		pTarget = g_pObjectManager->FindByName(wname);
		if (!pTarget || !pTarget->IsInitialized())
		{
			pTarget = pPlayer; // if target not found, use self
		}
	}

	if (pTarget->GetLevel() < 30)
		return;

	switch (pTarget->GetClass())
	{
	case PC_CLASS_HUMAN_FIGHTER:
	case PC_CLASS_STREET_FIGHTER:
	case PC_CLASS_SWORD_MASTER:
	{
		if (pTarget->GetSkillManager())
			pTarget->GetSkillManager()->LearnSkill(20911, wTemp, false);
		break;
	}
	case PC_CLASS_HUMAN_MYSTIC:
	case PC_CLASS_CRANE_ROSHI:
	case PC_CLASS_TURTLE_ROSHI:
	{
		if (pTarget->GetSkillManager())
			pTarget->GetSkillManager()->LearnSkill(120911, wTemp, false);
		break;
	}
	case PC_CLASS_NAMEK_FIGHTER:
	case PC_CLASS_DARK_WARRIOR:
	case PC_CLASS_SHADOW_KNIGHT:
	{
		if (pTarget->GetSkillManager())
			pTarget->GetSkillManager()->LearnSkill(320811, wTemp, false);
		break;
	}
	case PC_CLASS_NAMEK_MYSTIC:
	case PC_CLASS_DENDEN_HEALER:
	case PC_CLASS_POCO_SUMMONER:
	{
		if (pTarget->GetSkillManager())
			pTarget->GetSkillManager()->LearnSkill(421111, wTemp, false);
		break;
	}
	case PC_CLASS_MIGHTY_MAJIN:
	case PC_CLASS_ULTI_MA:
	case PC_CLASS_GRAND_MA:
	{
		if (pTarget->GetSkillManager())
			pTarget->GetSkillManager()->LearnSkill(520241, wTemp, false);
		break;
	}
	case PC_CLASS_WONDER_MAJIN:
	case PC_CLASS_PLAS_MA:
	case PC_CLASS_KAR_MA:
	{
		if (pTarget->GetSkillManager())
			pTarget->GetSkillManager()->LearnSkill(620231, wTemp, false);
		break;
	}

	default: return; break;
	}
}
ACMD(do_addskill2)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();

	TBLIDX SkillId = INVALID_TBLIDX;

	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);

	CPlayer* pTarget = pPlayer;

	if (!strToken.empty())
	{
		std::wstring name = std::wstring(strToken.begin(), strToken.end());
		const wchar_t* wname = name.c_str();

		pTarget = g_pObjectManager->FindByName(wname);
		if (!pTarget || !pTarget->IsInitialized())
		{
			pTarget = pPlayer; // if target not found, use self
		}
	}

	switch (pTarget->GetClass())
	{
	case PC_CLASS_STREET_FIGHTER: SkillId = 729991; break;
	case PC_CLASS_SWORD_MASTER: SkillId = 829991; break;
	case PC_CLASS_CRANE_ROSHI: SkillId = 929991; break;
	case PC_CLASS_TURTLE_ROSHI: SkillId = 1029991; break;
	case PC_CLASS_DARK_WARRIOR: SkillId = 1329991; break;
	case PC_CLASS_SHADOW_KNIGHT: SkillId = 1429991; break;
	case PC_CLASS_DENDEN_HEALER: SkillId = 1529991; break;
	case PC_CLASS_POCO_SUMMONER: SkillId = 1629991; break;
	case PC_CLASS_ULTI_MA: SkillId = 1729991; break;
	case PC_CLASS_GRAND_MA: SkillId = 1829991; break;
	case PC_CLASS_PLAS_MA: SkillId = 1929991; break;
	case PC_CLASS_KAR_MA: SkillId = 2029991; break;

	default: return; break;
	}

	WORD wTemp;


	if (pTarget->GetSkillManager())
		pTarget->GetSkillManager()->LearnSkill(SkillId, wTemp, false);
}

ACMD(do_r)
{
	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);

	CPlayer* pTarget = pPlayer;

	if (!strToken.empty())
	{
		std::wstring name = std::wstring(strToken.begin(), strToken.end());
		const wchar_t* wname = name.c_str();

		pTarget = g_pObjectManager->FindByName(wname);
		if (!pTarget || !pTarget->IsInitialized())
		{
			pTarget = pPlayer; // if target not found, use self
		}
	}

	pTarget->UpdateCurLpEp(pPlayer->GetMaxLP(), pPlayer->GetMaxEP(), true, false);
}

ACMD(do_addhtb)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();

	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	TBLIDX id = (TBLIDX)atof(ws2s(strToken).c_str());
	WORD wTemp;

	pToken->PopToPeek();
	strToken = pToken->PeekNextToken(NULL, &iLine);

	CPlayer* pTarget = pPlayer;

	if (!strToken.empty())
	{
		std::wstring name = std::wstring(strToken.begin(), strToken.end());
		const wchar_t* wname = name.c_str();

		pTarget = g_pObjectManager->FindByName(wname);
		if (!pTarget || !pTarget->IsInitialized())
		{
			pTarget = pPlayer; // if target not found, use self
		}
	}

	if (pTarget->IsGameMaster() == false)
	{
		switch (pTarget->GetClass())
		{
		case PC_CLASS_STREET_FIGHTER:
		case PC_CLASS_SWORD_MASTER:
			id = 30611;
			break;

		case PC_CLASS_CRANE_ROSHI:
		case PC_CLASS_TURTLE_ROSHI:
			id = 130411;
			break;

		case PC_CLASS_DARK_WARRIOR:
		case PC_CLASS_SHADOW_KNIGHT:
			id = 330611;
			break;

		case PC_CLASS_DENDEN_HEALER:
		case PC_CLASS_POCO_SUMMONER:
			id = 430411;
			break;

		case PC_CLASS_ULTI_MA:
		case PC_CLASS_GRAND_MA:
			id = 532011;
			break;

		case PC_CLASS_PLAS_MA:
		case PC_CLASS_KAR_MA:
			id = 632011;
			break;
		}
	}

	pTarget->GetHtbSkillManager()->LearnHtbSkill(id, wTemp);
}

ACMD(do_setzenny)
{
	CPlayer* pCurrentPlayer = pPlayer;

	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	DWORD zeni = (DWORD)atof(ws2s(strToken).c_str());

	pToken->PopToPeek();
	strToken = pToken->PeekNextToken(NULL, &iLine);
	std::wstring name = std::wstring(strToken.begin(), strToken.end());
	const wchar_t* wname = name.c_str();

	if (!name.empty())
	{
		CPlayer* target = g_pObjectManager->FindByName(wname);
		if (target && target->IsInitialized()) {
			pCurrentPlayer = target;
		}
	}

	ERR_LOG(LOG_USER, "Player: %u receive %u zeni from gm command", pPlayer->GetCharID(), zeni);

	pCurrentPlayer->UpdateZeni(ZENNY_CHANGE_TYPE_CHEAT, zeni, true);
}

ACMD(do_setlevel)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();

	CPlayer* pTarget = pPlayer;

	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	BYTE level = (BYTE)atof(ws2s(strToken).c_str());

	pToken->PopToPeek();
	strToken = pToken->PeekNextToken(NULL, &iLine);

	if (!strToken.empty())
	{
		std::wstring name = std::wstring(strToken.begin(), strToken.end());
		const wchar_t* wname = name.c_str();

		pTarget = g_pObjectManager->FindByName(wname);
		if (!pTarget || !pTarget->IsInitialized())
		{
			pTarget = pPlayer; // if target not found, use self
		}
	}

	RwUInt32 curlv = pTarget->GetLevel();
	sEXP_TBLDAT* ExpData = (sEXP_TBLDAT*)g_pTableContainer->GetExpTable()->FindData(level);
	if (!ExpData)
		return;


	CNtlPacket packet(sizeof(sGU_UPDATE_CHAR_LEVEL));
	sGU_UPDATE_CHAR_LEVEL* res = (sGU_UPDATE_CHAR_LEVEL*)packet.GetPacketData();
	res->wOpCode = GU_UPDATE_CHAR_LEVEL;
	res->byCurLevel = level;
	res->byPrevLevel = curlv;
	res->dwMaxExpInThisLevel = ExpData->dwNeed_Exp;
	res->handle = pTarget->GetID();
	packet.SetPacketLen(sizeof(sGU_UPDATE_CHAR_LEVEL));
	pTarget->Broadcast(&packet);

	DWORD getsp = Dbo_GetLevelUpGainSP(level, level - curlv);
	pTarget->UpdateCharSP(pTarget->GetSkillPoints() + getsp);
	pTarget->SetLevel(level);
	pTarget->GetCharAtt()->CalculateAll();

	pTarget->UpdateCurLpEp(pTarget->GetMaxLP(), pTarget->GetMaxEP(), true, false);

	pTarget->UpdateMaxRpBalls();

	//send to chat server
	app->GetChatServerSession()->SendUpdatePcLevel(pTarget);

	//update party
	if (pTarget->GetPartyID() != INVALID_PARTYID && pTarget->GetParty())
		pTarget->GetParty()->UpdateMemberLevel(pTarget);

	//send to query server
	CNtlPacket packetQry(sizeof(sGQ_PC_UPDATE_LEVEL_REQ));
	sGQ_PC_UPDATE_LEVEL_REQ* resQry = (sGQ_PC_UPDATE_LEVEL_REQ*)packetQry.GetPacketData();
	resQry->wOpCode = GQ_PC_UPDATE_LEVEL_REQ;
	resQry->handle = pTarget->GetID();
	resQry->charId = pTarget->GetCharID();
	resQry->dwEXP = 0;
	resQry->byLevel = pTarget->GetLevel();
	resQry->dwSP = pTarget->GetSkillPoints();
	packetQry.SetPacketLen(sizeof(sGQ_PC_UPDATE_LEVEL_REQ));
	app->SendTo(app->GetQueryServerSession(), &packetQry);
}
ACMD(do_setlevel2)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();

	CPlayer* pTarget = pPlayer;

	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	BYTE level = (BYTE)atof(ws2s(strToken).c_str());


	RwUInt32 curlv = pTarget->GetLevel();
	if (level <= curlv || level > 70)
		return;
	sEXP_TBLDAT* ExpData = (sEXP_TBLDAT*)g_pTableContainer->GetExpTable()->FindData(level);
	if (!ExpData)
		return;


	CNtlPacket packet(sizeof(sGU_UPDATE_CHAR_LEVEL));
	sGU_UPDATE_CHAR_LEVEL* res = (sGU_UPDATE_CHAR_LEVEL*)packet.GetPacketData();
	res->wOpCode = GU_UPDATE_CHAR_LEVEL;
	res->byCurLevel = level;
	res->byPrevLevel = curlv;
	res->dwMaxExpInThisLevel = ExpData->dwNeed_Exp;
	res->handle = pTarget->GetID();
	packet.SetPacketLen(sizeof(sGU_UPDATE_CHAR_LEVEL));
	pTarget->Broadcast(&packet);

	DWORD getsp = Dbo_GetLevelUpGainSP(level, level - curlv);
	pTarget->UpdateCharSP(pTarget->GetSkillPoints() + getsp);
	pTarget->SetLevel(level);
	pTarget->GetCharAtt()->CalculateAll();

	pTarget->UpdateCurLpEp(pTarget->GetMaxLP(), pTarget->GetMaxEP(), true, false);

	pTarget->UpdateMaxRpBalls();

	//send to chat server
	app->GetChatServerSession()->SendUpdatePcLevel(pTarget);

	//update party
	if (pTarget->GetPartyID() != INVALID_PARTYID && pTarget->GetParty())
		pTarget->GetParty()->UpdateMemberLevel(pTarget);

	//send to query server
	CNtlPacket packetQry(sizeof(sGQ_PC_UPDATE_LEVEL_REQ));
	sGQ_PC_UPDATE_LEVEL_REQ* resQry = (sGQ_PC_UPDATE_LEVEL_REQ*)packetQry.GetPacketData();
	resQry->wOpCode = GQ_PC_UPDATE_LEVEL_REQ;
	resQry->handle = pTarget->GetID();
	resQry->charId = pTarget->GetCharID();
	resQry->dwEXP = 0;
	resQry->byLevel = pTarget->GetLevel();
	resQry->dwSP = pTarget->GetSkillPoints();
	packetQry.SetPacketLen(sizeof(sGQ_PC_UPDATE_LEVEL_REQ));
	app->SendTo(app->GetQueryServerSession(), &packetQry);
}
ACMD(do_hide)
{
	if (pPlayer->GetStateManager()->IsCharCondition(CHARCOND_TRANSPARENT))
		pPlayer->GetStateManager()->RemoveConditionState(CHARCOND_TRANSPARENT, NULL, true);
	else
		pPlayer->GetStateManager()->AddConditionState(CHARCOND_TRANSPARENT, NULL, true);
}

ACMD(do_notice)
{
	/*
		@notice TYPE(0-6) TEXT(MAX 256 CHARACTERS)
	*/

	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	BYTE byDisType = (BYTE)atof(ws2s(strToken).c_str());

	pToken->PopToPeek();
	std::wstring text = L"";

	while (text.length() < NTL_MAX_LENGTH_OF_CHAT_MESSAGE)
	{
		std::wstring mtext = pToken->PeekNextToken(NULL, &iLine);
		if (mtext == L";" || mtext == L"")
		{
			break;
		}
		else
		{
			text += mtext; text += L" ";
		}
	}


	CGameServer* app = (CGameServer*)g_pApp;

	CNtlPacket packet(sizeof(sGT_SYSTEM_DISPLAY_TEXT));
	sGT_SYSTEM_DISPLAY_TEXT* res = (sGT_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
	res->wOpCode = GT_SYSTEM_DISPLAY_TEXT;
	res->serverChannelId = INVALID_SERVERCHANNELID;
	res->byDisplayType = byDisType;
	wcscpy_s(res->wszMessage, NTL_MAX_LENGTH_OF_CHAT_MESSAGE + 1, text.c_str());
	packet.SetPacketLen(sizeof(sGT_SYSTEM_DISPLAY_TEXT));
	app->SendTo(app->GetChatServerSession(), &packet);
}

ACMD(do_pm)
{
	pToken->PopToPeek();
	std::wstring strName = pToken->PeekNextToken(NULL, &iLine);

	pToken->PopToPeek();
	std::wstring text = L"";

	while (text.length() < NTL_MAX_LENGTH_OF_CHAT_MESSAGE)
	{
		std::wstring mtext = pToken->PeekNextToken(NULL, &iLine);
		if (mtext == L";" || mtext == L"")
		{
			break;
		}
		else
		{
			text += mtext; text += L" ";
		}
	}

	if (CPlayer* pTarget = g_pObjectManager->FindByName(strName.c_str()))
	{
		CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
		res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
		res->wMessageLengthInUnicode = (WORD)text.length();
		res->byDisplayType = SERVER_TEXT_EMERGENCY;
		wcscpy_s(res->awchMessage, NTL_MAX_LENGTH_OF_CHAT_MESSAGE + 1, text.c_str());
		packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		pTarget->SendPacket(&packet);
	}
}

ACMD(do_teleport)
{
	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	int Portal = (BYTE)atof(ws2s(strToken).c_str());
	sPORTAL_TBLDAT* pPortalTblData = (sPORTAL_TBLDAT*)g_pTableContainer->GetPortalTable()->FindData(Portal);
	if (pPortalTblData == NULL)
	{
		return;
	}

	pPlayer->StartTeleport(pPortalTblData->vLoc, pPortalTblData->vDir, pPortalTblData->worldId, TELEPORT_TYPE_COMMAND);
}

ACMD(do_world)
{
	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	WORLDID worldId = (WORLDID)atof(ws2s(strToken).c_str());

	CGameServer* app = (CGameServer*)g_pApp;

	pToken->PopToPeek();
	strToken = pToken->PeekNextToken(NULL, &iLine);

	CPlayer* pTarget = pPlayer;

	if (!strToken.empty())
	{
		std::wstring name = std::wstring(strToken.begin(), strToken.end());
		const wchar_t* wname = name.c_str();

		pTarget = g_pObjectManager->FindByName(wname);
		if (!pTarget || !pTarget->IsInitialized())
		{
			pTarget = pPlayer; // if target not found, use self
		}
	}

	sWORLD_TBLDAT* pWorldTbldat = (sWORLD_TBLDAT*)g_pTableContainer->GetWorldTable()->FindData(worldId);
	if (pWorldTbldat == NULL)
	{
		return;
	}

	if (CWorld* pWorld = app->GetGameMain()->GetWorldManager()->FindWorld(worldId))
	{
		pTarget->StartTeleport(pWorld->GetTbldat()->vDefaultLoc, pTarget->GetCurDir(), worldId, TELEPORT_TYPE_COMMAND);
	}
	else
	{
		CWorld* pWorld2 = app->GetGameMain()->GetWorldManager()->CreateWorld(pWorldTbldat);
		if (pWorld2 == NULL)
			return;

		pTarget->StartTeleport(pWorld2->GetTbldat()->vStart1Loc, pTarget->GetCurDir(), pWorld2->GetID(), TELEPORT_TYPE_COMMAND);
	}
}


ACMD(do_warp)
{
	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);

	std::wstring name = std::wstring(strToken.begin(), strToken.end());
	const wchar_t* wname = name.c_str();

	CCharacter* target = g_pObjectManager->FindByName(wname);
	if (target && target->IsInitialized())
	{
		pPlayer->StartTeleport(target->GetCurLoc(), target->GetCurDir(), target->GetWorldID(), TELEPORT_TYPE_COMMAND);
	}
}

ACMD(do_call)
{
	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);

	std::wstring name = std::wstring(strToken.begin(), strToken.end());
	const wchar_t* wname = name.c_str();

	CCharacter* target = g_pObjectManager->FindByName(wname);

	pToken->PopToPeek();
	strToken = pToken->PeekNextToken(NULL, &iLine);

	CCharacter* pPlayerTarget = pPlayer;
	if (!strToken.empty())
	{
		std::wstring name = std::wstring(strToken.begin(), strToken.end());
		const wchar_t* wname = name.c_str();

		CCharacter* pFoundTarget = g_pObjectManager->FindByName(wname);
		if (pFoundTarget && pFoundTarget->IsInitialized())
		{
			pPlayerTarget = pFoundTarget;
		}
	}

	//if (target && target->GetCurWorld()) //avoid teleporting by gm code into dungeon
	if (target && pPlayerTarget && pPlayerTarget->IsInitialized())
	{
		target->StartTeleport(pPlayerTarget->GetCurLoc(), pPlayerTarget->GetCurDir(), pPlayerTarget->GetWorldID(), TELEPORT_TYPE_COMMAND);
	}
}
ACMD(do_TeleportAll)
{
	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);

	std::wstring name = std::wstring(strToken.begin(), strToken.end());
	const wchar_t* wname = name.c_str();
	//printf("Name %s \n", name.c_str());

	CNtlVector vPos(pPlayer->GetCurLoc());

	g_pObjectManager->FindAll(vPos, pPlayer->GetCurDir(), pPlayer->GetWorldID());

	//if (target) //avoid teleporting by gm code into dungeon
	//	target->StartTeleport(vPos, pPlayer->GetCurDir(), pPlayer->GetWorldID(), TELEPORT_TYPE_COMMAND);
}
ACMD(do_shutdown)
{
	CGameServer* app = (CGameServer*)g_pApp;

	app->GetGameProcessor()->StartServerShutdownEvent();
}

ACMD(do_setadult)
{
	CPlayer* pTarget = pPlayer;

	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	int n = (int)atof(ws2s(strToken).c_str());
	bool bAdultSet = n != 0;

	pToken->PopToPeek();
	strToken = pToken->PeekNextToken(NULL, &iLine);

	if (!strToken.empty())
	{
		std::wstring name = std::wstring(strToken.begin(), strToken.end());
		const wchar_t* wname = name.c_str();

		pTarget = g_pObjectManager->FindByName(wname);
		if (!pTarget || !pTarget->IsInitialized())
		{
			pTarget = pPlayer; // if target not found, use self
		}
	}

	pTarget->UpdateAdult(bAdultSet);
}

ACMD(do_setclass)
{
	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	BYTE byClass = (BYTE)atof(ws2s(strToken).c_str());

	pToken->PopToPeek();
	strToken = pToken->PeekNextToken(NULL, &iLine);

	CPlayer* target = pPlayer;

	if (!strToken.empty())
	{
		std::wstring name = std::wstring(strToken.begin(), strToken.end());
		const wchar_t* wname = name.c_str();

		target = g_pObjectManager->FindByName(wname);
		if (target && target->IsInitialized()) {
			pPlayer = target;
		}
		else
		{
			ERR_LOG(LOG_GENERAL, "Player %s not found", ws2s(name).c_str());
			return;
		}
	}

	pPlayer->UpdateClass(byClass);
}

ACMD(do_changeclass)
{
	// Syntax: @changeclass <class> [name]
	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	BYTE byClass = (BYTE)atof(ws2s(strToken).c_str());

	pToken->PopToPeek();
	strToken = pToken->PeekNextToken(NULL, &iLine);

	CPlayer* target = pPlayer;

	if (!strToken.empty())
	{
		std::wstring name = std::wstring(strToken.begin(), strToken.end());
		const wchar_t* wname = name.c_str();

		CPlayer* found = g_pObjectManager->FindByName(wname);
		if (found && found->IsInitialized()) {
			target = found;
		}
		else
		{
			ERR_LOG(LOG_GENERAL, "Player %s not found", ws2s(name).c_str());
			return;
		}
	}

	if (!target || !target->IsInitialized())
		return;

	// Prevent during transformations
	if (target->GetAspectStateId() != ASPECTSTATE_INVALID)
		return;

	// Validate class for target race/gender
	sPC_TBLDAT* pcdata = (sPC_TBLDAT*)g_pTableContainer->GetPcTable()->GetPcTbldat(target->GetTbldat()->byRace, byClass, target->GetTbldat()->byGender);
	if (!pcdata)
	{
		// notify GM that class is invalid for target
		CNtlStringW msg;
		CNtlPacket packetMsg(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		sGU_SYSTEM_DISPLAY_TEXT* resMsg = (sGU_SYSTEM_DISPLAY_TEXT*)packetMsg.GetPacketData();
		resMsg->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
		resMsg->byDisplayType = SERVER_TEXT_SYSTEM;
		resMsg->wMessageLengthInUnicode = (WORD)msg.Format(L"Invalid class %u for %s (race=%u gender=%u)", byClass, target->GetCharName(), target->GetTbldat()->byRace, target->GetTbldat()->byGender);
		NTL_SAFE_WCSCPY(resMsg->awchMessage, msg.c_str());
		pPlayer->SendPacket(&packetMsg);
		return;
	}

	// Set pending class to be applied after skill reset completes
	target->SetPendingClassChange(byClass);
	// Ensure no cost charged for this GM-driven reset
	target->SetSkipNextSkillResetCost(true);

	// Trigger a skill reset using the same flow as @resetskills
	CGameServer* app = (CGameServer*)g_pApp;

	CNtlPacket pQry(sizeof(sGQ_SKILL_INIT_REQ));
	sGQ_SKILL_INIT_REQ* rQry = (sGQ_SKILL_INIT_REQ*)pQry.GetPacketData();
	rQry->wOpCode = GQ_SKILL_INIT_REQ;
	rQry->handle = target->GetID();
	rQry->charId = target->GetCharID();
	rQry->dwSP = (DWORD)target->GetLevel() - 1; // base SP after full reset
	rQry->bySkillResetMethod = 0; // normal reset path to receive RES
	pQry.SetPacketLen(sizeof(sGQ_SKILL_INIT_REQ));
	app->SendTo(app->GetQueryServerSession(), &pQry);
}

ACMD(do_dc)
{
	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	std::wstring name = std::wstring(strToken.begin(), strToken.end());
	const wchar_t* wname = name.c_str();

	CPlayer* target = g_pObjectManager->FindByName(wname);
	if (target && target->IsInitialized())
		target->GetClientSession()->Disconnect(false);
}

ACMD(do_kill)
{
	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	std::wstring name = std::wstring(strToken.begin(), strToken.end());
	const wchar_t* wname = name.c_str();

	CPlayer* target = g_pObjectManager->FindByName(wname);
	if (target && target->IsInitialized())
		target->Faint(pPlayer, FAINT_REASON_COMMAND);
}

ACMD(do_delallitems)
{
	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);

	CPlayer* pTarget = pPlayer;

	if (!strToken.empty())
	{
		std::wstring name = std::wstring(strToken.begin(), strToken.end());
		const wchar_t* wname = name.c_str();

		pTarget = g_pObjectManager->FindByName(wname);
		if (!pTarget || !pTarget->IsInitialized())
		{
			pTarget = pPlayer; // if target not found, use self
		}
	}

	pTarget->GetPlayerItemContainer()->DeleteAllItems();
}

ACMD(do_god)
{
	CPlayer* pCurrentPlayer = pPlayer;

	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	std::wstring name = std::wstring(strToken.begin(), strToken.end());
	const wchar_t* wname = name.c_str();

	if (!name.empty())
	{
		CPlayer* target = g_pObjectManager->FindByName(wname);
		if (target && target->IsInitialized()) {
			pCurrentPlayer = target;
		}
	}

	pCurrentPlayer->GetCharAtt()->SetPhysicalOffence(INVALID_WORD);
	pCurrentPlayer->GetCharAtt()->SetPhysicalDefence(INVALID_WORD);
	pCurrentPlayer->GetCharAtt()->SetEnergyOffence(INVALID_WORD);

	pCurrentPlayer->UpdateAttackSpeed(300);

	if (pCurrentPlayer->GetImmortalMode() == eIMMORTAL_MODE_OFF)
		pCurrentPlayer->SetImmortalMode(eIMMORTAL_MODE_NORMAL);
	else
		pCurrentPlayer->SetImmortalMode(eIMMORTAL_MODE_OFF);

	pCurrentPlayer->UpdateMoveSpeed(35.f, 35.f);
}

ACMD(do_invincible)
{
	/*
		@invincible DURATION(SECONDS)
	*/

	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	int nSeconds = (int)atof(ws2s(strToken).c_str());

	if (nSeconds == 0 || nSeconds > 3600)
		nSeconds = 3600;

	pToken->PopToPeek();
	strToken = pToken->PeekNextToken(NULL, &iLine);

	CPlayer* pTarget = pPlayer;

	if (!strToken.empty())
	{
		std::wstring name = std::wstring(strToken.begin(), strToken.end());
		const wchar_t* wname = name.c_str();

		pTarget = g_pObjectManager->FindByName(wname);
		if (!pTarget || !pTarget->IsInitialized())
		{
			pTarget = pPlayer; // if target not found, use self
		}
	}

	sDBO_BUFF_PARAMETER aBuffParameter[NTL_MAX_EFFECT_IN_SKILL];
	sSKILL_TBLDAT* pTempSkillTbldat = (sSKILL_TBLDAT*)g_pTableContainer->GetSkillTable()->FindData(2385);

	eSYSTEM_EFFECT_CODE aeEffectCode[NTL_MAX_EFFECT_IN_SKILL];
	aeEffectCode[0] = g_pTableContainer->GetSystemEffectTable()->GetEffectCodeWithTblidx(pTempSkillTbldat->skill_Effect[0]);
	aeEffectCode[1] = INVALID_SYSTEM_EFFECT_CODE;

	aBuffParameter[0].byBuffParameterType = DBO_BUFF_PARAMETER_TYPE_DEFAULT;
	aBuffParameter[0].buffParameter.fParameter = 0;
	aBuffParameter[0].buffParameter.dwRemainValue = 0;

	DWORD dwDurationInMs = nSeconds * 1000;

	pTarget->GetBuffManager()->RegisterBuff(dwDurationInMs, aeEffectCode, aBuffParameter, INVALID_HOBJECT, BUFF_TYPE_BLESS, pTempSkillTbldat);
}

ACMD(do_bann)
{
	CGameServer* app = (CGameServer*)g_pApp;

	/*
		@bann USERNAME DURATION(DAYS. 255 = PERMA) REASON(MAX 256 CHARACTERS)
	*/
	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	std::wstring name = std::wstring(strToken.begin(), strToken.end());

	pToken->PopToPeek();
	strToken = pToken->PeekNextToken(NULL, &iLine);
	BYTE byDuration = (BYTE)atof(ws2s(strToken).c_str());

	pToken->PopToPeek();
	std::wstring text = L"";

	while (text.length() < NTL_MAX_LENGTH_OF_CHAT_MESSAGE)
	{
		std::wstring mtext = pToken->PeekNextToken(NULL, &iLine);
		if (mtext == L";" || mtext == L"")
		{
			break;
		}
		else
		{
			text += mtext;
			text += L" ";
		}
	}

	CPlayer* target = g_pObjectManager->FindByName(name.c_str());
	if (target && target->IsInitialized())
	{
		target->Bann(ws2s(text), byDuration, pPlayer->GetAccountID());
	}
}


ACMD(do_dbann)
{
	CGameServer* app = (CGameServer*)g_pApp;

	/*
		@dbann ACCOUNT_ID DURATION(DAYS. 255 = PERMA) REASON(MAX 256 CHARACTERS)
	*/
	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	ACCOUNTID accid = (ACCOUNTID)atof(ws2s(strToken).c_str());

	pToken->PopToPeek();
	strToken = pToken->PeekNextToken(NULL, &iLine);
	BYTE byDuration = (BYTE)atof(ws2s(strToken).c_str());

	pToken->PopToPeek();
	std::wstring text = L"";

	while (text.length() < NTL_MAX_LENGTH_OF_CHAT_MESSAGE)
	{
		std::wstring mtext = pToken->PeekNextToken(NULL, &iLine);
		if (mtext == L";" || mtext == L"")
		{
			break;
		}
		else
		{
			text += mtext;
			text += L" ";
		}
	}

	CNtlPacket pQry(sizeof(sGQ_ACCOUNT_BANN));
	sGQ_ACCOUNT_BANN* qRes = (sGQ_ACCOUNT_BANN*)pQry.GetPacketData();
	qRes->wOpCode = GQ_ACCOUNT_BANN;
	qRes->gmAccountID = pPlayer->GetAccountID();
	qRes->targetAccountID = accid;
	strcpy_s(qRes->szReason, NTL_MAX_LENGTH_OF_CHAT_MESSAGE + 1, ws2s(text).c_str());
	qRes->byDuration = byDuration;
	pQry.SetPacketLen(sizeof(sGQ_ACCOUNT_BANN));
	app->SendTo(app->GetQueryServerSession(), &pQry);
}


ACMD(do_purge) //despawn all monster around player
{
	CWorldCell* pWorldCell = pPlayer->GetCurWorldCell();
	if (!pWorldCell)
		return;

	CMonster* pNextMob = NULL;
	CNpc* pNextNpc = NULL;

	CWorldCell::QUADPAGE page = pWorldCell->GetCellQuadPage(pPlayer->GetCurLoc());
	for (int dir = CWorldCell::QUADPAGE_FIRST; dir < CWorldCell::QUADPAGE_COUNT; dir++)
	{
		CWorldCell* pWorldCellSibling = pWorldCell->GetQuadSibling(page, (CWorldCell::QUADDIR)dir);
		if (pWorldCellSibling)
		{
			CMonster* pMobTarget = (CMonster*)pWorldCellSibling->GetObjectList()->GetFirst(OBJTYPE_MOB);
			while (pMobTarget)
			{
				pNextMob = (CMonster*)pWorldCellSibling->GetObjectList()->GetNext(pMobTarget->GetWorldCellObjectLinker());

				if (pMobTarget->GetCurWorld() && !pMobTarget->IsFainting())
				{
					if (GetHelperNpcManager()->IsRegisteredHelper(pMobTarget) || pMobTarget->GetStandAlone())
					{
						// Skip helpers (registered) and standalone MOBs (spawned as helpers)
					}
					else
					{
						pMobTarget->Faint(pPlayer);
					}
				}

				pMobTarget = pNextMob;
			}

			CNpc* pNpcTarget = (CMonster*)pWorldCellSibling->GetObjectList()->GetFirst(OBJTYPE_NPC);
			while (pNpcTarget)
			{
				pNextNpc = (CMonster*)pWorldCellSibling->GetObjectList()->GetNext(pNpcTarget->GetWorldCellObjectLinker());

				if (pNpcTarget->GetCurWorld() && !pNpcTarget->IsFainting())
				{
					if (GetHelperNpcManager()->IsRegisteredHelper(pNpcTarget) || pNpcTarget->GetStandAlone())
					{
						// Skip helpers and standalone NPCs
					}
					else
					{
						pNpcTarget->Faint(pPlayer);
					}
				}

				pNpcTarget = pNextNpc;
			}
		}
	}
}
ACMD(do_BatleEvent) //despawn all monster around player
{
	CGameServer* app = (CGameServer*)g_pApp;
	CWorldCell* pWorldCell = pPlayer->GetCurWorldCell();
	if (!pWorldCell)
		return;

	WORLDID worldId = 510000;

	sWORLD_TBLDAT* pWorldTbldat = (sWORLD_TBLDAT*)g_pTableContainer->GetWorldTable()->FindData(worldId);
	if (pWorldTbldat == NULL)
	{
		return;
	}

	CNtlVector vPos;

	vPos.x = -953.615;
	vPos.z = -72.067;
	vPos.z = -92.004;

	CPlayer* pNextPlayer = NULL;

	CWorldCell::QUADPAGE page = pWorldCell->GetCellQuadPage(pPlayer->GetCurLoc());
	for (int dir = CWorldCell::QUADPAGE_FIRST; dir < CWorldCell::QUADPAGE_COUNT; dir++)
	{
		CWorldCell* pWorldCellSibling = pWorldCell->GetQuadSibling(page, (CWorldCell::QUADDIR)dir);
		if (pWorldCellSibling)
		{
			CPlayer* pMobTarget = (CPlayer*)pWorldCellSibling->GetObjectList()->GetFirst(OBJTYPE_PC);
			while (pMobTarget)
			{
				pNextPlayer = (CPlayer*)pWorldCellSibling->GetObjectList()->GetNext(pMobTarget->GetWorldCellObjectLinker());

				if (pMobTarget->GetCurWorld() && !pMobTarget->IsFainting())
				{
					vPos.x += RandomRangeF(-10.0f, 10.0f);
					vPos.z += RandomRangeF(-10.0f, 10.0f);

					if (CWorld* pWorld = app->GetGameMain()->GetWorldManager()->FindWorld(worldId))
					{
						pMobTarget->StartTeleport(vPos, pMobTarget->GetCurDir(), worldId, TELEPORT_TYPE_COMMAND);
					}
					else
					{
						CWorld* pWorld2 = app->GetGameMain()->GetWorldManager()->CreateWorld(pWorldTbldat);
						if (pWorld2 == NULL)
							return;

						pMobTarget->StartTeleport(vPos, pMobTarget->GetCurDir(), pWorld2->GetID(), TELEPORT_TYPE_COMMAND);
					}

				}

				pMobTarget = pNextPlayer;
			}
		}
	}
}

ACMD(do_unstuck)
{
	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);

	if (!pPlayer->GetClientSession())
		return;

	if (pPlayer->IsFainting())
		return;
	if (pPlayer->GetFreeBattleID() != INVALID_DWORD)
		return;
	if (pPlayer->IsPvpZone() || pPlayer->GetFightMode())
		return;
	if (pPlayer->GetCurWorld() && pPlayer->GetCurWorld()->GetTbldat()->bDynamic)
		return;
	if (pPlayer->GetDragonballScrambleBallFlag() > 0)
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	if (app->GetGsServerId() == DOJO_CHANNEL_INDEX)
		return;

	if (CSkill* pSkill = pPlayer->GetSkillManager()->FindSkillWithSystemEffectCode(ACTIVE_TELEPORT_BIND)) //dont allow to use unstuck while call back skill is on cooldown
	{
		if (pSkill->GetCoolTimeRemaining() > 0)
			return;
	}

	if (pPlayer->GetCanUnstack() == true)
	{
		pPlayer->SetCanUnstack(false);
		CNtlVector vBindLoc(pPlayer->GetBindLoc());
		pPlayer->StartTeleport(vBindLoc, pPlayer->GetCurDir(), pPlayer->GetBindWorldID(), TELEPORT_TYPE_COMMAND);
	}
}

ACMD(do_warfog)
{
	CGameServer* app = (CGameServer*)g_pApp;
	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);

	CTriggerObject* pNextObj = NULL;
	CTriggerObject* pObj = (CTriggerObject*)pPlayer->GetCurWorld()->GetObjectList()->GetFirst(OBJTYPE_TOBJECT);
	while (pObj)
	{
		pNextObj = (CTriggerObject*)pPlayer->GetCurWorld()->GetObjectList()->GetNext(pObj->GetWorldObjectLinker());

		if (pObj->GetFunc() == eDBO_TRIGGER_OBJECT_FUNC_SELECTION + eDBO_TRIGGER_OBJECT_FUNC_NAMEKAN_SIGN)
		{
			if (pPlayer->CheckWarFog(pObj->GetContent()) == false)
			{
				if (pPlayer->AddWarFogFlag(pObj->GetContent()))
				{
					CNtlPacket packetQry(sizeof(sGQ_WAR_FOG_UPDATE_REQ));
					sGQ_WAR_FOG_UPDATE_REQ* resQry = (sGQ_WAR_FOG_UPDATE_REQ*)packetQry.GetPacketData();
					resQry->wOpCode = GQ_WAR_FOG_UPDATE_REQ;
					resQry->charID = pPlayer->GetCharID();
					resQry->contentsTblidx = pObj->GetContent();
					memcpy(resQry->sInfo.achWarFogFlag, pPlayer->GetWarFogFlag(), sizeof(resQry->sInfo.achWarFogFlag));
					packetQry.SetPacketLen(sizeof(sGQ_WAR_FOG_UPDATE_REQ));
					app->SendTo(app->GetQueryServerSession(), &packetQry);
				}
			}
		}

		pObj = pNextObj;
	}
}

ACMD(do_upgrade)
{
	CPlayer* pTarget = pPlayer;

	CGameServer* app = (CGameServer*)g_pApp;
	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	int nGrade = (int)atof(ws2s(strToken).c_str());

	nGrade = (nGrade > NTL_ITEM_MAX_GRADE) ? NTL_ITEM_MAX_GRADE : nGrade;

	pToken->PopToPeek();
	strToken = pToken->PeekNextToken(NULL, &iLine);

	if (!strToken.empty())
	{
		std::wstring name = std::wstring(strToken.begin(), strToken.end());
		const wchar_t* wname = name.c_str();

		pTarget = g_pObjectManager->FindByName(wname);
		if (!pTarget || !pTarget->IsInitialized())
		{
			pTarget = pPlayer; // if target not found, use self
		}
	}

	int nCount = 0;

	for (int i = 0; i < EQUIP_SLOT_TYPE_SCOUTER; i++)
	{
		CItem* pItem = pTarget->GetPlayerItemContainer()->GetItem(CONTAINER_TYPE_EQUIP, i);
		if (pItem)
		{
			if (pItem->GetGrade() != nGrade)
			{
				pItem->SetGrade(nGrade);

				CNtlPacket packet(sizeof(sGU_ITEM_UPDATE));
				sGU_ITEM_UPDATE* res = (sGU_ITEM_UPDATE*)packet.GetPacketData();
				res->wOpCode = GU_ITEM_UPDATE;
				res->handle = pItem->GetID();
				memcpy(&res->sItemData, &pItem->GetItemData(), sizeof(sITEM_DATA));
				packet.SetPacketLen(sizeof(sGU_ITEM_UPDATE));
				pTarget->SendPacket(&packet);

				CNtlPacket pQry(sizeof(sGQ_ITEM_UPDATE_REQ));
				sGQ_ITEM_UPDATE_REQ* rQry = (sGQ_ITEM_UPDATE_REQ*)pQry.GetPacketData();
				rQry->wOpCode = GQ_ITEM_UPDATE_REQ;
				rQry->handle = pTarget->GetID();
				rQry->charId = pTarget->GetCharID();
				memcpy(&rQry->sItem, &pItem->GetItemData(), sizeof(sITEM_DATA));
				pQry.SetPacketLen(sizeof(sGQ_ITEM_UPDATE_REQ));
				app->SendTo(app->GetQueryServerSession(), &pQry);

				++nCount;
			}
		}
	}

	if (nCount > 0)
		pTarget->GetCharAtt()->CalculateAll();
}

ACMD(do_setitemrank)
{
	CGameServer* app = (CGameServer*)g_pApp;
	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	int nRank = (int)atof(ws2s(strToken).c_str());

	nRank = (nRank > ITEM_RANK_LAST) ? ITEM_RANK_LAST : nRank;

	int nCount = 0;

	for (int i = 0; i < EQUIP_SLOT_TYPE_SCOUTER; i++)
	{
		CItem* pItem = pPlayer->GetPlayerItemContainer()->GetItem(CONTAINER_TYPE_EQUIP, i);
		if (pItem)
		{
			if (pItem->GetRank() != nRank)
			{
				pItem->SetRank(nRank);

				CNtlPacket packet(sizeof(sGU_ITEM_UPDATE));
				sGU_ITEM_UPDATE* res = (sGU_ITEM_UPDATE*)packet.GetPacketData();
				res->wOpCode = GU_ITEM_UPDATE;
				res->handle = pItem->GetID();
				memcpy(&res->sItemData, &pItem->GetItemData(), sizeof(sITEM_DATA));
				packet.SetPacketLen(sizeof(sGU_ITEM_UPDATE));
				pPlayer->SendPacket(&packet);

				CNtlPacket pQry(sizeof(sGQ_ITEM_UPDATE_REQ));
				sGQ_ITEM_UPDATE_REQ* rQry = (sGQ_ITEM_UPDATE_REQ*)pQry.GetPacketData();
				rQry->wOpCode = GQ_ITEM_UPDATE_REQ;
				rQry->handle = pPlayer->GetID();
				rQry->charId = pPlayer->GetCharID();
				memcpy(&rQry->sItem, &pItem->GetItemData(), sizeof(sITEM_DATA));
				pQry.SetPacketLen(sizeof(sGQ_ITEM_UPDATE_REQ));
				app->SendTo(app->GetQueryServerSession(), &pQry);

				++nCount;
			}
		}
	}

	if (nCount > 0)
		pPlayer->GetCharAtt()->CalculateAll();
}

ACMD(do_mute)
{
	CGameServer* app = (CGameServer*)g_pApp;

	/*
		@mute CHARNAME DURATION(MINUTES) REASON(MAX 128 CHARACTERS)
	*/

	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	std::wstring name = std::wstring(strToken.begin(), strToken.end());

	pToken->PopToPeek();
	strToken = pToken->PeekNextToken(NULL, &iLine);
	DWORD dwDuration = (DWORD)atof(ws2s(strToken).c_str());

	pToken->PopToPeek();
	std::wstring text = L"";

	while (text.length() < NTL_MAX_LENGTH_OF_MAIL_MESSAGE - 10)
	{
		std::wstring mtext = pToken->PeekNextToken(NULL, &iLine);
		if (mtext == L";" || mtext == L"")
		{
			break;
		}
		else
		{
			text += mtext;
			text += L" ";
		}
	}

	CNtlPacket packet(sizeof(sGT_UPDATE_PUNISH));
	sGT_UPDATE_PUNISH* res = (sGT_UPDATE_PUNISH*)packet.GetPacketData();
	res->wOpCode = GT_UPDATE_PUNISH;
	res->accountId = pPlayer->GetAccountID();
	res->dwDurationInMinute = dwDuration;
	NTL_SAFE_WCSCPY(res->awchGmCharName, pPlayer->GetCharName());
	NTL_SAFE_WCSCPY(res->awchCharName, name.c_str());
	wcscpy_s(res->wchReason, NTL_MAX_LENGTH_OF_MAIL_MESSAGE + 1, text.c_str());
	packet.SetPacketLen(sizeof(sGT_UPDATE_PUNISH));
	app->SendTo(app->GetChatServerSession(), &packet);
}

ACMD(do_unmute)
{
	CGameServer* app = (CGameServer*)g_pApp;

	/*
		@unmute CHARNAME
	*/

	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	std::wstring name = std::wstring(strToken.begin(), strToken.end());

	CNtlPacket packet(sizeof(sGT_UPDATE_PUNISH));
	sGT_UPDATE_PUNISH* res = (sGT_UPDATE_PUNISH*)packet.GetPacketData();
	res->wOpCode = GT_UPDATE_PUNISH;
	res->accountId = pPlayer->GetAccountID();
	res->dwDurationInMinute = 0;
	NTL_SAFE_WCSCPY(res->awchCharName, name.c_str());
	packet.SetPacketLen(sizeof(sGT_UPDATE_PUNISH));
	app->SendTo(app->GetChatServerSession(), &packet);
}

ACMD(do_go)
{
	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	BYTE byMap = (BYTE)atof(ws2s(strToken).c_str());
	WORLDID worldid = 1;

	CNtlVector destLoc;
	switch (byMap)
	{
	case MPP_TELE_YAHOI: {
		destLoc.x = 4493; destLoc.z = 4032;
	}break;
	case MPP_TELE_YUREKA: {
		destLoc.x = 6586; destLoc.z = 3254;
	}break;
	case MPP_TELE_DALPANG: {
		destLoc.x = 3246; destLoc.z = -2749;
	}break;
	case MPP_TELE_DRAGON: {
		destLoc.x = 4620; destLoc.z = -1729;
	}break;
	case MPP_TELE_BAEE: {
		destLoc.x = 4509; destLoc.z = 4031;
	}break;
	case MPP_TELE_AJIRANG: {
		destLoc.x = 4509; destLoc.z = 4031;
	}break;
	case MPP_TELE_KARINGA_1: {
		destLoc.x = 5928; destLoc.z = 658;
	}break;
	case MPP_TELE_KARINGA_2: {
		destLoc.x = 5884; destLoc.z = 827;
	}break;
	case MPP_TELE_GREAT_TREE: {
		destLoc.x = 5903; destLoc.z = 1711;
	}break;
	case MPP_TELE_KARINGA_3: {
		destLoc.x = 7527; destLoc.z = -518;
	}break;
	case MPP_TELE_MERMAID: {
		destLoc.x = 4651; destLoc.z = -206;
	}break;
	case MPP_TELE_GANNET: {
		destLoc.x = 3690; destLoc.z = 1396;
	}break;
	case MPP_TELE_EMERALD: {
		destLoc.x = 3390; destLoc.z = -564;
	}break;
	case MPP_TELE_TEMBARIN: {
		destLoc.x = 1964; destLoc.z = 1072;
	}break;
	case MPP_TELE_CELL: {
		destLoc.x = -480; destLoc.z = 1646;
	}break;
	case MPP_TELE_BUU: {
		destLoc.x = 2481; destLoc.z = 3384;
	}break;
	case MPP_TELE_CC: {
		destLoc.x = -1796; destLoc.z = 1106;
	}break;
	case MPP_TELE_MUSHROOM: {
		destLoc.x = -1408; destLoc.z = -1408;
	}break;

	case MPP_TELE_PAPAYA: {
		destLoc.x = -5336; destLoc.y = -66; destLoc.z = -6658;
		worldid = 15;
	}break;
	default:
		destLoc = pPlayer->GetCurLoc();
		worldid = pPlayer->GetWorldTblidx();
		break;
	}

	pPlayer->StartTeleport(destLoc, pPlayer->GetCurDir(), worldid, TELEPORT_TYPE_COMMAND);
}


ACMD(do_addtitle)
{
	CGameServer* app = (CGameServer*)g_pApp;

	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	TBLIDX titleIdx = (TBLIDX)atof(ws2s(strToken).c_str());

	CPlayer* pTarget = pPlayer;

	strToken = pToken->PeekNextToken(NULL, &iLine);
	std::wstring name = std::wstring(strToken.begin(), strToken.end());
	const wchar_t* wname = name.c_str();

	CPlayer* destPc = g_pObjectManager->FindByName(wname);


	if (destPc != NULL)
	{
		pTarget = g_pObjectManager->FindByChar(destPc->GetCharID());
		if (pTarget == NULL || pTarget->IsInitialized() == false)
			return;
	}

	/*
		@addtitle ID [CHARID]
	*/

	if ((titleIdx - 1) / NTL_MAX_CHAR_TITLE_FLAG_COUNT >= NTL_MAX_CHAR_TITLE_COUNT_IN_FLAG)
		return;

	if (pTarget->CheckCharTitle(titleIdx - 1) == false) // 303 is gm title
	{
		pTarget->AddCharTitle(titleIdx - 1);
	}
}


ACMD(do_deltitle)
{
	CGameServer* app = (CGameServer*)g_pApp;

	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	TBLIDX titleIdx = (TBLIDX)atof(ws2s(strToken).c_str());

	CPlayer* pTarget = pPlayer;

	strToken = pToken->PeekNextToken(NULL, &iLine);
	std::wstring name = std::wstring(strToken.begin(), strToken.end());
	const wchar_t* wname = name.c_str();

	CPlayer* destPc = g_pObjectManager->FindByName(wname);


	if (destPc != NULL)
	{
		pTarget = g_pObjectManager->FindByChar(destPc->GetCharID());
		if (pTarget == NULL || pTarget->IsInitialized() == false)
			return;
	}

	/*
		@addtitle ID [CHARID]
	*/

	if ((titleIdx - 1) / NTL_MAX_CHAR_TITLE_FLAG_COUNT >= NTL_MAX_CHAR_TITLE_COUNT_IN_FLAG)
		return;

	if (pTarget->CheckCharTitle(titleIdx - 1) == true) // 303 is gm title
	{
		pTarget->DelCharTitle(titleIdx - 1);
	}
}


ACMD(do_setitemduration)
{
	CGameServer* app = (CGameServer*)g_pApp;

	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	DWORD dwSeconds = (DWORD)atof(ws2s(strToken).c_str());

	/*
		@setitemduration SECONDS
	*/

	CItem* pItem = pPlayer->GetPlayerItemContainer()->GetItem(1, 0); //get first item from inventory
	if (pItem && pItem->GetDurationtype() == eDURATIONTYPE_FLATSUM)
	{
		pItem->SetUseEndTime(time(0) + dwSeconds);

		CNtlPacket pQry(sizeof(sGQ_ITEM_CHANGE_DURATIONTIME_REQ));
		sGQ_ITEM_CHANGE_DURATIONTIME_REQ* rQry = (sGQ_ITEM_CHANGE_DURATIONTIME_REQ*)pQry.GetPacketData();
		rQry->wOpCode = GQ_ITEM_CHANGE_DURATIONTIME_REQ;
		rQry->charId = pPlayer->GetCharID();
		rQry->handle = pPlayer->GetID();
		memcpy(&rQry->sItem, &pItem->GetItemData(), sizeof(sITEM_DATA));
		pQry.SetPacketLen(sizeof(sGQ_ITEM_CHANGE_DURATIONTIME_REQ));
		app->SendTo(app->GetQueryServerSession(), &pQry);
	}
}


ACMD(do_bind)
{
	CGameServer* app = (CGameServer*)g_pApp;

	pToken->PopToPeek();

	/*
		@bind
	*/

	if (pPlayer->GetCurWorld() && pPlayer->GetCurWorld()->GetTbldat()->bDynamic == false)
	{
		CNtlPacket packetQry(sizeof(sGQ_PC_UPDATE_BIND_REQ));
		sGQ_PC_UPDATE_BIND_REQ* resQry = (sGQ_PC_UPDATE_BIND_REQ*)packetQry.GetPacketData();
		resQry->wOpCode = GQ_PC_UPDATE_BIND_REQ;
		resQry->charId = pPlayer->GetCharID();
		resQry->handle = pPlayer->GetID();
		resQry->byBindType = DBO_BIND_TYPE_GM_TOOL;
		resQry->bindObjectTblidx = INVALID_TBLIDX;
		resQry->bindWorldId = pPlayer->GetWorldID();
		pPlayer->GetCurLoc().CopyTo(resQry->vBindLoc);
		pPlayer->GetCurDir().CopyTo(resQry->vBindDir);
		packetQry.SetPacketLen(sizeof(sGQ_PC_UPDATE_BIND_REQ));
		app->SendTo(app->GetQueryServerSession(), &packetQry);

		pPlayer->SetBindLoc(resQry->vBindLoc);
		pPlayer->SetBindDir(resQry->vBindDir);
		pPlayer->SetBindObjectTblidx(resQry->bindObjectTblidx);
		pPlayer->SetBindWorldID(resQry->bindWorldId);
		pPlayer->SetBindType(DBO_BIND_TYPE_GM_TOOL);
	}
}

ACMD(do_exp)
{
	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);

	if (pPlayer->IsReceiveExpDisabled() == false && strToken.compare(L"off") == 0)
	{
		pPlayer->SetExpReceiveDisabled(true);

		WCHAR* msg = L"Receive EXP has been disabled";

		CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
		res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
		res->wMessageLengthInUnicode = (WORD)wcslen(msg);
		res->byDisplayType = SERVER_TEXT_SYSTEM;
		NTL_SAFE_WCSCPY(res->awchMessage, msg);
		packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		pPlayer->SendPacket(&packet);
	}
	else if (pPlayer->IsReceiveExpDisabled() == true && strToken.compare(L"on") == 0)
	{
		pPlayer->SetExpReceiveDisabled(false);

		WCHAR* msg = L"Receive EXP has been enabled";

		CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
		res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
		res->wMessageLengthInUnicode = (WORD)wcslen(msg);
		res->byDisplayType = SERVER_TEXT_SYSTEM;
		NTL_SAFE_WCSCPY(res->awchMessage, msg);
		packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		pPlayer->SendPacket(&packet);
	}
}

ACMD(do_resetexp)
{
	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);

	if (pPlayer->GetExp() > 0)
	{
		CNtlPacket packet(sizeof(sGU_UPDATE_CHAR_EXP));
		sGU_UPDATE_CHAR_EXP* res = (sGU_UPDATE_CHAR_EXP*)packet.GetPacketData();
		res->handle = pPlayer->GetID();
		res->wOpCode = GU_UPDATE_CHAR_EXP;
		res->dwCurExp = 0;
		res->dwAcquisitionExp = 0;
		res->dwIncreasedExp = 0;
		res->dwBonusExp = 0;
		packet.SetPacketLen(sizeof(sGU_UPDATE_CHAR_EXP));
		pPlayer->SendPacket(&packet);

		pPlayer->SetExp(0);
	}
}

ACMD(do_startevent)
{
	CGameServer* app = (CGameServer*)g_pApp;

	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	BYTE Type = (BYTE)atof(ws2s(strToken).c_str());

	strToken = pToken->PeekNextToken(NULL, &iLine);
	BYTE Hours = (BYTE)atof(ws2s(strToken).c_str());
	if (Hours == 0) Hours = 3;

	if (Type == 0)
	{
		g_pHoneyBeeEvent->StartEvent(Hours);
		NTL_PRINT(PRINT_APP, _T("Honey Bee Event Started"));
	}
	if (Type == 1)
	{
		g_pFairyEvent->StartEvent(Hours);
		NTL_PRINT(PRINT_APP, _T("Fairy Event Started"));
	}
}

ACMD(do_stophoneybee)
{
	CGameServer* app = (CGameServer*)g_pApp;

	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	BYTE Type = (BYTE)atof(ws2s(strToken).c_str());

	if (Type == 0)
	{
		g_pHoneyBeeEvent->EndEvent();
		NTL_PRINT(PRINT_APP, _T("Honey Bee Event End"));
	}
	if (Type == 1)
	{
		g_pFairyEvent->EndEvent();
		NTL_PRINT(PRINT_APP, _T("Fairy Event End"));
	}
}

ACMD(do_deleteguild)
{
	CGameServer* app = (CGameServer*)g_pApp;

	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	const char* chName = ws2s(strToken).c_str();

	/*
		@deleteguild GUILDNAME
	*/

	WCHAR* wchName = Ntl_MB2WC((char*)chName);

	CNtlPacket cPacket(sizeof(sGT_GUILD_DELETE));
	sGT_GUILD_DELETE* cRes = (sGT_GUILD_DELETE*)cPacket.GetPacketData();
	cRes->wOpCode = GT_GUILD_DELETE;
	cRes->gmCharId = pPlayer->GetCharID();
	NTL_SAFE_WCSCPY(cRes->wszGuildName, wchName);
	cPacket.SetPacketLen(sizeof(sGT_GUILD_DELETE));
	app->SendTo(app->GetChatServerSession(), &cPacket); //Send to chat server

	//clean memory
	Ntl_CleanUpHeapStringW(wchName);

	ERR_LOG(LOG_USER, "GM %u deleted Guild %s", pPlayer->GetCharID(), chName);
}

ACMD(do_cancelah)
{
	CGameServer* app = (CGameServer*)g_pApp;

	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	CHARACTERID charid = (CHARACTERID)atof(ws2s(strToken).c_str());

	/*
		@cancelah charid
	*/

	CNtlPacket packet(sizeof(sGT_TENKAICHIDAISIJYOU_SELL_CANCEL_REQ));
	sGT_TENKAICHIDAISIJYOU_SELL_CANCEL_REQ* res = (sGT_TENKAICHIDAISIJYOU_SELL_CANCEL_REQ*)packet.GetPacketData();
	res->wOpCode = GT_TENKAICHIDAISIJYOU_SELL_CANCEL_REQ;
	res->charId = charid;
	res->nItem = INVALID_ITEMID;
	packet.SetPacketLen(sizeof(sGT_TENKAICHIDAISIJYOU_SELL_CANCEL_REQ));
	app->SendTo(app->GetChatServerSession(), &packet);
}

ACMD(do_addmudosa)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();

	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	DWORD dwMudosa = (DWORD)atof(ws2s(strToken).c_str());

	pToken->PopToPeek();
	strToken = pToken->PeekNextToken(NULL, &iLine);
	std::wstring wstrName = std::wstring(strToken.begin(), strToken.end());

	CPlayer* pTarget = pPlayer;

	if (wcslen(wstrName.c_str()) > 0)
	{
		pTarget = g_pObjectManager->FindByName(wstrName.c_str());
		if (pTarget == NULL || pTarget->IsInitialized() == false)
			return;
	}

	DWORD dwFinalMudosa = pTarget->GetMudosaPoints() + dwMudosa;

	pTarget->UpdateMudosaPoints(dwFinalMudosa);
}

ACMD(do_start)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();

	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	BYTE byClass = (BYTE)atof(ws2s(strToken).c_str());

	strToken = pToken->PeekNextToken(NULL, &iLine);
	int nlv = (int)atof(ws2s(strToken).c_str());

	if (nlv != 60 && nlv != 70)
		return;

	if (pPlayer->GetClass() > PC_CLASS_1_LAST)
		return;

	switch (pPlayer->GetClass())
	{
	case PC_CLASS_HUMAN_FIGHTER:
	{
		if (byClass != PC_CLASS_STREET_FIGHTER && byClass != PC_CLASS_SWORD_MASTER)
			return;
	}
	break;
	case PC_CLASS_HUMAN_MYSTIC:
	{
		if (byClass != PC_CLASS_CRANE_ROSHI && byClass != PC_CLASS_TURTLE_ROSHI)
			return;
	}
	break;
	case PC_CLASS_NAMEK_FIGHTER:
	{
		if (byClass != PC_CLASS_DARK_WARRIOR && byClass != PC_CLASS_SHADOW_KNIGHT)
			return;
	}
	break;
	case PC_CLASS_NAMEK_MYSTIC:
	{
		if (byClass != PC_CLASS_DENDEN_HEALER && byClass != PC_CLASS_POCO_SUMMONER)
			return;
	}
	break;
	case PC_CLASS_MIGHTY_MAJIN:
	{
		if (byClass != PC_CLASS_ULTI_MA && byClass != PC_CLASS_GRAND_MA)
			return;
	}
	break;
	case PC_CLASS_WONDER_MAJIN:
	{
		if (byClass != PC_CLASS_PLAS_MA && byClass != PC_CLASS_KAR_MA)
			return;
	}
	break;

	default: return;
	}

	if (pPlayer->GetLevel() >= nlv)
		return;

	// update level
	pPlayer->LevelUp(0, nlv - pPlayer->GetLevel());

	// update class
	pPlayer->UpdateClass(byClass);

	// learn passives
	TBLIDX masterPassive = INVALID_TBLIDX;
	TBLIDX itemPassive = INVALID_TBLIDX;
	TBLIDX flightPassive = INVALID_TBLIDX;

	switch (pPlayer->GetClass())
	{
	case PC_CLASS_STREET_FIGHTER:
	{
		masterPassive = 729991;
		itemPassive = 11140026;
		flightPassive = 11120142;
	}
	break;
	case PC_CLASS_SWORD_MASTER:
	{
		masterPassive = 829991;
		itemPassive = 11140026;
		flightPassive = 11120142;
	}
	break;
	case PC_CLASS_CRANE_ROSHI:
	{
		masterPassive = 929991;
		itemPassive = 11140027;
		flightPassive = 11120143;
	}
	break;
	case PC_CLASS_TURTLE_ROSHI:
	{
		masterPassive = 1029991;
		itemPassive = 11140027;
		flightPassive = 11120143;
	}
	break;
	case PC_CLASS_DARK_WARRIOR:
	{
		masterPassive = 1329991;
		itemPassive = 11140028;
		flightPassive = 11120144;
	}
	break;
	case PC_CLASS_SHADOW_KNIGHT:
	{
		masterPassive = 1429991;
		itemPassive = 11140028;
		flightPassive = 11120144;
	}
	break;
	case PC_CLASS_DENDEN_HEALER:
	{
		masterPassive = 1529991;
		itemPassive = 11140029;
		flightPassive = 11120145;
	}
	break;
	case PC_CLASS_POCO_SUMMONER:
	{
		masterPassive = 1629991;
		itemPassive = 11140029;
		flightPassive = 11120145;
	}
	break;
	case PC_CLASS_ULTI_MA:
	{
		masterPassive = 1729991;
		itemPassive = 11140030;
		flightPassive = 11120146;
	}
	break;
	case PC_CLASS_GRAND_MA:
	{
		masterPassive = 1829991;
		itemPassive = 11140030;
		flightPassive = 11120146;
	}
	break;
	case PC_CLASS_PLAS_MA:
	{
		masterPassive = 1929991;
		itemPassive = 11140031;
		flightPassive = 11120147;
	}
	break;
	case PC_CLASS_KAR_MA:
	{
		masterPassive = 2029991;
		itemPassive = 11140031;
		flightPassive = 11120147;
	}
	break;

	default: break;
	}

	WORD wTemp;

	pPlayer->GetSkillManager()->LearnSkill(masterPassive, wTemp, false);

	g_pItemManager->CreateItem(pPlayer, itemPassive, 1);
	g_pItemManager->CreateItem(pPlayer, flightPassive, 1);
}

ACMD(do_createloot)
{
	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	TBLIDX ItemId = (TBLIDX)atof(ws2s(strToken).c_str());

	strToken = pToken->PeekNextToken(NULL, &iLine);
	int nCount = (int)atof(ws2s(strToken).c_str());

	if (nCount > 100)
		nCount = 100;

	for (int i = 0; i < nCount; i++)
	{
		CNtlVector vPos(pPlayer->GetCurLoc());

		sVECTOR3 vec;
		vPos.CopyTo(vec.x, vec.y, vec.z);

		vec.x += RandomRangeF(-10.0f, 10.0f);
		vec.z += RandomRangeF(-10.0f, 10.0f);

		CItemDrop* pBall = NULL;
		if (g_pItemManager->IsValidSingleDropIdx(ItemId))
		{
			ERR_LOG(LOG_GENERAL, "[DropTrace] GM CreateSingleDrop char=%u item=%u", pPlayer->GetCharID(), ItemId);
			pBall = g_pItemManager->CreateSingleDrop(100.f, ItemId);
		}
		if (pBall)
		{
			pBall->AddToGround(pPlayer->GetWorldID(), vec);
		}
	}
}

ACMD(do_test)
{
	CGameServer* app = (CGameServer*)g_pApp;

	pToken->PopToPeek();
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	int charid = (int)atof(ws2s(strToken).c_str());
	WORD wResultcode = GAME_SUCCESS;
	if (pPlayer->GetParty() && pPlayer->GetPartyID() != INVALID_PARTYID)
	{
		if (pPlayer->GetParty()->GetPartyLeaderID() == pPlayer->GetID())
		{
			if (pPlayer->GetParty()->IsEveryoneInLeaderRange(pPlayer, NTL_MAX_RADIUS_OF_VISIBLE_AREA))
			{
				BYTE byBeginStage = charid;
				CItem* pItem = NULL;

				CBattleDungeon* pDungeon = g_pDungeonManager->CreateBattleDungeon(pPlayer, wResultcode, byBeginStage);
				if (pDungeon == NULL)
					wResultcode = GAME_PARTY_DUNGEON_IS_NOT_CREATED;
				else
				{
					if (pItem && byBeginStage > 1)
						pItem->SetCount(pItem->GetCount() - 1, false, true);
				}
			}
			else wResultcode = GAME_PARTY_MEMBER_IS_TOO_FAR;
		}
		else wResultcode = GAME_COMMON_YOU_ARE_NOT_A_PARTY_LEADER;
	}
}
ACMD(do_notify)
{
	CGameServer* app = (CGameServer*)g_pApp;
	/*CNtlPacket pChat(sizeof(sGU_EVENT_SCHEDULING_START));
	sGU_EVENT_SCHEDULING_START* rChat = (sGU_EVENT_SCHEDULING_START*)pChat.GetPacketData();
	rChat->wOpCode = GU_EVENT_SCHEDULING_START;
	rChat->dwTest1[0] = 1;
	pChat.SetPacketLen(sizeof(sGU_EVENT_SCHEDULING_START));
	pPlayer->SendPacket(&pChat);*/
	std::wstring strToken = pToken->PeekNextToken(NULL, &iLine);
	DWORD Zenny = (DWORD)atof(ws2s(strToken).c_str());

	CNtlPacket cPacket(sizeof(sGT_GUILD_GIVE_ZENNY_REQ));
	sGT_GUILD_GIVE_ZENNY_REQ* cRes = (sGT_GUILD_GIVE_ZENNY_REQ*)cPacket.GetPacketData();
	cRes->wOpCode = GT_GUILD_GIVE_ZENNY_REQ;
	cRes->charId = pPlayer->GetCharID();
	cRes->dwZenny = Zenny;
	cPacket.SetPacketLen(sizeof(sGT_GUILD_GIVE_ZENNY_REQ));
	app->SendTo(app->GetChatServerSession(), &cPacket); //Send to chat server

	CNtlPacket packet(sizeof(sGU_GUILD_GIVE_ZENNY_RES));
	sGU_GUILD_GIVE_ZENNY_RES* res = (sGU_GUILD_GIVE_ZENNY_RES*)packet.GetPacketData();
	res->wOpCode = GU_GUILD_GIVE_ZENNY_RES;
	res->wResultCode = 500;
	packet.SetPacketLen(sizeof(sGU_GUILD_GIVE_ZENNY_RES));
	//app->Send(pPlayer->GetHandle(), &packet);
	pPlayer->SendPacket(&packet);
}

//========================================
// @findteam - Join Team Budokai matchmaking queue
//========================================
ACMD(do_budokai_findteam)
{
	UNREFERENCED_PARAMETER(iLine);
	UNREFERENCED_PARAMETER(pToken);

	if (!pPlayer || !pPlayer->IsInitialized())
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	// Helper function to send system message
	auto SendMessage = [&](const WCHAR* msg) {
		CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
		res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
		res->byDisplayType = SERVER_TEXT_SYSTEM;
		NTL_SAFE_WCSCPY(res->awchMessage, msg);
		packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		pPlayer->SendPacket(&packet);
	};

	// Check if player already has a party
	if (pPlayer->GetParty() != NULL)
	{
		SendMessage(L"[Budokai Matchmaking] You are already in a party! Leave your party first to use matchmaking.");
		return;
	}

	// Check if player is in a valid world (not in dungeon/instance)
	CWorld* pWorld = pPlayer->GetCurWorld();
	if (pWorld && pWorld->GetRuleType() != GAMERULE_NORMAL)
	{
		SendMessage(L"[Budokai Matchmaking] You can only use matchmaking from normal world zones.");
		return;
	}

	// Join the matchmaking queue
	g_pBudokaiManager->JoinMatchmakingQueue(pPlayer);

	// Send confirmation message
	SendMessage(L"[Budokai Matchmaking] You have joined the queue! You will be notified when a team of 5 players is formed.");

	ERR_LOG(LOG_USER, "[MATCHMAKING] Player %u used @findteam command", (unsigned)pPlayer->GetCharID());
}
