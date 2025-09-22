#include "stdafx.h"
#include "GameServer.h"
#include "MasterServerSession.h"
#include "ChatServerSession.h"
#include "QueryServerSession.h"
#include "NtlRandom.h"

#include "SubNeighborServerInfoManager.h"
#include "EventMgr.h"

#include "ItemManager.h"
#include "freebattle.h"
#include "trade.h"
#include "Party.h"
#include "DungeonManager.h"
#include "TriggerManager.h"
#include "ObjectManager.h"
#include "CPlayer.h"
#include "privateshop.h"
#include "RankBattle.h"
#include "PartyMatching.h"
#include "ShenronManager.h"
#include "DojoManager.h"
#include "Guild.h"

#include "GameProcessor.h"
#include "GameData.h"
#include "GameMain.h"
#include "ActionPatternSystem.h"
#include "ug_opcodes.h"
#include "qg_opcodes.h"
#include "tg_opcodes.h"
#include "mg_opcodes.h"
#include "ScriptAlgoObjectManager.h"
#include "DynamicFieldSystemEvent.h"
#include "DragonballHunt.h"
#include "DragonballScramble.h"
#include "DojoWar.h"
#include "BudokaiManager.h"
#include "ExpEvent.h"
#include "BusSystem.h"
#include "scsManager.h"
#include "HoneyBeeEvent.h"
#include "StoneDropEvent.h"
#include "Fairy Event.h"
#include "CustomDropEvent.h"
#include "PlayerModifiers.h"
#include "HelperNpcManager.h"
#include "SkillTable.h"
// --- INICIO SOCKET COMANDOS ---
#include <thread>
#include <atomic>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#include <iostream>
#include <sstream>
#include <vector>

std::atomic<bool> g_CommandSocketRunning{ false };

void CommandSocketThread(CGameServer* pServer)

{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr << "WSAStartup failed" << std::endl;
		return;
	}
	SOCKET listenSock = INVALID_SOCKET;
	int startPort = 6666;
	int maxAttempts = 10;
	int usedPort = 0;
	for (int i = 0; i < maxAttempts; ++i) {
		listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (listenSock == INVALID_SOCKET) {
			std::cerr << "Socket creation failed" << std::endl;
			WSACleanup();
			return;
		}
		sockaddr_in serverAddr{};
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
		serverAddr.sin_port = htons(startPort + i);
		if (bind(listenSock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == 0) {
			usedPort = startPort + i;
			break;
		}
		closesocket(listenSock);
		listenSock = INVALID_SOCKET;
	}
	if (listenSock == INVALID_SOCKET) {
		std::cerr << "Bind failed on all ports" << std::endl;
		WSACleanup();
		return;
	}
	if (listen(listenSock, 1) == SOCKET_ERROR) {
		std::cerr << "Listen failed" << std::endl;
		closesocket(listenSock);
		WSACleanup();
		return;
	}
	std::cout << "[GameServer] Command socket listening on 127.0.0.1:" << usedPort << std::endl;
	g_CommandSocketRunning = true;
	while (g_CommandSocketRunning) {
		SOCKET clientSock = accept(listenSock, nullptr, nullptr);
		if (clientSock == INVALID_SOCKET) continue;
		char buffer[256] = { 0 };
		int bytes = recv(clientSock, buffer, sizeof(buffer) - 1, 0);
		if (bytes > 0) {
			buffer[bytes] = '\0';
			std::string cmd(buffer);
			// Elimina saltos de línea
			cmd.erase(std::remove(cmd.begin(), cmd.end(), '\r'), cmd.end());
			cmd.erase(std::remove(cmd.begin(), cmd.end(), '\n'), cmd.end());
			BOOL result = pServer->OnCommandInput(cmd);
			const char* reply = (result == TRUE) ? "OK\n" : "KO\n";
			send(clientSock, reply, (int)strlen(reply), 0);
		}
		closesocket(clientSock);
	}
	closesocket(listenSock);
	WSACleanup();
}
// --- FIN SOCKET COMANDOS ---


CGameServer::CGameServer()
{
	Init();
}


CGameServer::~CGameServer()
{
	Destroy();
}



int CGameServer::OnInitApp()
{
	m_tmCurrentTime = time(NULL);
	NtlRandInit(m_tmCurrentTime);

	m_nMaxSessionCount = m_config.nMaxConnection + 3;

	m_dwCurTickCount = 0;

	NTL_PRINT(PRINT_APP, "Init Session Factory ");
	m_pSessionFactory = new CGameSessionFactory;

	NTL_PRINT(PRINT_APP, "Init Timed-Event Manager");
	EventMgr* m_pEventMgr = new EventMgr;
	UNREFERENCED_PARAMETER(m_pEventMgr);

	NTL_PRINT(PRINT_APP, "Create Neighborserverinfomanager");
	CSubNeighborServerInfoManager* m_pNeighborServerInfoManager = new CSubNeighborServerInfoManager;
	UNREFERENCED_PARAMETER(m_pNeighborServerInfoManager);

	CScriptAlgoObjectManager* pScriptAlgoMgr = new CScriptAlgoObjectManager;
	UNREFERENCED_PARAMETER(pScriptAlgoMgr);

	NTL_PRINT(PRINT_APP, "Create Game Data");
	m_pGameData = new CGameData;
	m_pGameData->Create(0);

	NTL_PRINT(PRINT_APP, "Create Game Main");
	m_pGameMain = new CGameMain;
	m_pGameMain->Create(m_pGameData);

	NTL_PRINT(PRINT_APP, "Create Game Processor");
	m_pGameProcessor = new CGameProcessor;
	m_pGameProcessor->Create(100, m_pGameMain);

	NTL_PRINT(PRINT_APP, "Create Actionpatter System");
	m_pActionPatternSystem = new CActionPatternSystem;
	m_pActionPatternSystem->Create();

	NTL_PRINT(PRINT_APP, "Init Trigger Manager");
	CTriggerManager* trigger_manager = new CTriggerManager;
	trigger_manager->Init(m_config.bLoadTriggersEnc);

	NTL_PRINT(PRINT_APP, "Init Trade Manager ");
	CTradeManager* trade_manager = new CTradeManager;
	UNREFERENCED_PARAMETER(trade_manager);

	NTL_PRINT(PRINT_APP, "Init Dungeon Manager ");
	CDungeonManager* dungeon_manager = new CDungeonManager;
	UNREFERENCED_PARAMETER(dungeon_manager);

	NTL_PRINT(PRINT_APP, "Init Guild System");
	CGuildManager* guild = new CGuildManager;
	UNREFERENCED_PARAMETER(guild);

	NTL_PRINT(PRINT_APP, "Init Party Manager ");
	CPartyManager* party_manager = new CPartyManager;
	UNREFERENCED_PARAMETER(party_manager);

	NTL_PRINT(PRINT_APP, "Init Item Manager ");
	CItemManager* item_manager = new CItemManager;
	UNREFERENCED_PARAMETER(item_manager);

	NTL_PRINT(PRINT_APP, "Init Free Battle Manager ");
	CFreeBattleManager* freebattle_manager = new CFreeBattleManager;
	UNREFERENCED_PARAMETER(freebattle_manager);

	NTL_PRINT(PRINT_APP, "Init Private shop Manager ");
	CShopManager* privateshop_manager = new CShopManager;
	UNREFERENCED_PARAMETER(privateshop_manager);

	NTL_PRINT(PRINT_APP, "Init Rank Battle Manager ");
	CRankbattle* rankbattle_manager = new CRankbattle;
	UNREFERENCED_PARAMETER(rankbattle_manager);

	NTL_PRINT(PRINT_APP, "Init Party Matching");
	CPartyMatching* partymatching_manager = new CPartyMatching;
	UNREFERENCED_PARAMETER(partymatching_manager);

	NTL_PRINT(PRINT_APP, "Init Shenron Manager");
	CShenronManager* shenron_manager = new CShenronManager;
	UNREFERENCED_PARAMETER(shenron_manager);

	NTL_PRINT(PRINT_APP, "Prepare Opcodes System");
	CUG_Opcodes* cUG_opcodes = new CUG_Opcodes;
	UNREFERENCED_PARAMETER(cUG_opcodes);
	CQG_Opcodes* cQG_opcodes = new CQG_Opcodes;
	UNREFERENCED_PARAMETER(cQG_opcodes);
	CTG_Opcodes* cTG_opcodes = new CTG_Opcodes;
	UNREFERENCED_PARAMETER(cTG_opcodes);
	CMG_Opcodes* cMG_opcodes = new CMG_Opcodes;
	UNREFERENCED_PARAMETER(cMG_opcodes);

	CDynamicFieldSystemEvent* pDynEvent = new CDynamicFieldSystemEvent;
	UNREFERENCED_PARAMETER(pDynEvent);

	NTL_PRINT(PRINT_APP, "Prepare DragonballHunt Event System");
	CDragonballHunt* pDragonballHunt = new CDragonballHunt;
	UNREFERENCED_PARAMETER(pDragonballHunt);

	NTL_PRINT(PRINT_APP, "Prepare DragonballScramble Event System");
	CDragonballScramble* pDragonballScramble = new CDragonballScramble;
	UNREFERENCED_PARAMETER(pDragonballScramble);

	NTL_PRINT(PRINT_APP, "Prepare DojoWar System");
	CDojoWar* pDojoWar = new CDojoWar;
	UNREFERENCED_PARAMETER(pDojoWar);

	NTL_PRINT(PRINT_APP, "Exp Event System");
	CExpEvent* pExpEvent = new CExpEvent;
	UNREFERENCED_PARAMETER(pExpEvent);

	NTL_PRINT(PRINT_APP, "Bus System");
	CBusSystem* pBusSystem = new CBusSystem;
	UNREFERENCED_PARAMETER(pBusSystem);

	NTL_PRINT(PRINT_APP, "SCS System");
	CScsManager* pScs = new CScsManager;
	UNREFERENCED_PARAMETER(pScs);

	NTL_PRINT(PRINT_APP, "HoneyBee System");
	CHoneyBeeEvent* pHoneyBee = new CHoneyBeeEvent;
	UNREFERENCED_PARAMETER(pScs);

	NTL_PRINT(PRINT_APP, "StoneDrop System");
	CStoneDropEvent* pStoneDrop = new CStoneDropEvent;
	UNREFERENCED_PARAMETER(pScs);

	NTL_PRINT(PRINT_APP, "Fairy System");
	CFairyEvent* FairyEven = new CFairyEvent;
	UNREFERENCED_PARAMETER(pScs);

	NTL_PRINT(PRINT_APP, "Custom Drop System");
	CCustomDropEvent* pCustomDrop = new CCustomDropEvent;
	UNREFERENCED_PARAMETER(pScs);

	NTL_PRINT(PRINT_APP, "Player Modifiers System");
	CPlayerModifiers* pPlayerMods = new CPlayerModifiers;
	UNREFERENCED_PARAMETER(pPlayerMods);
	if (!m_pGameMain->PrepareWorldAndObject())
	{
		NTL_PRINT(PRINT_APP, "m_pGameMain->PrepareWorldAndObject() == FALSE");
		return NTL_FAIL;
	}

	// This is only to prepare the manager, actual loading is done on demand - see CHelperNpcManager::LoadFromIni
	NTL_PRINT(PRINT_APP, "Init Helper Npc Manager");
	//CHelperNpcManager* pHelperNpcManager = new CHelperNpcManager;
	//UNREFERENCED_PARAMETER(pHelperNpcManager);

	return NTL_SUCCESS;
}

int CGameServer::OnAppStart()
{
	NTL_PRINT(PRINT_APP, "Init Dojo Manager");
	CDojoManager* dojo_manager = new CDojoManager;
	UNREFERENCED_PARAMETER(dojo_manager);

	NTL_PRINT(PRINT_APP, "Prepare Budokai System");
	CBudokaiManager* pBudokaiManager = new CBudokaiManager;
	UNREFERENCED_PARAMETER(pBudokaiManager);

	int rc = NTL_SUCCESS;

	rc = m_clientAcceptor.Create(m_config.strClientAcceptAddr.c_str(), m_config.wClientAcceptPort, 1, m_config.wClientAcceptPort, SESSION_CLIENT, m_config.nMaxConnection, m_config.nMaxConnection, m_config.nMaxConnection, m_config.nMaxConnection);
	if (NTL_SUCCESS != rc)
		return rc;
	rc = m_network.Associate(&m_clientAcceptor, true);
	if (NTL_SUCCESS != rc)
		return rc;

	//Connect chat server
	rc = m_serverChatConnector.Create(m_config.strChatServerConAddr.c_str(), m_config.wChatServerConPort, SESSION_SERVER_CON_CHAT, 1000);
	if (NTL_SUCCESS != rc)
		return rc;
	rc = m_network.Associate(&m_serverChatConnector, true);
	if (NTL_SUCCESS != rc)
		return rc;

	//Query server connector
	rc = m_serverQueryConnector.Create(m_config.strQueryServerIP.c_str(), m_config.wQueryServerPort, SESSION_SERVER_CON_QUERY_GAME, INVALID_DWORD, INVALID_DWORD); //dont try to reconnect. Server will shut down once dc
	if (NTL_SUCCESS != rc)
		return rc;
	rc = m_network.Associate(&m_serverQueryConnector, true);
	if (NTL_SUCCESS != rc)
		return rc;

	//Connect master server
	rc = m_serverMasterConnector.Create(m_config.strMasterServerIP.c_str(), m_config.wMasterServerPort, SESSION_SERVER_CON_GAME_TO_MASTER, INVALID_DWORD, INVALID_DWORD); //dont try to reconnect. Server will shut down once dc
	if (NTL_SUCCESS != rc)
		return rc;
	rc = m_network.Associate(&m_serverMasterConnector, true);
	if (NTL_SUCCESS != rc)
		return rc;


	NTL_PRINT(PRINT_APP, "GAME SERVER READY ");

	// Iniciar el hilo del socket de comandos
	static std::thread commandThread;
	if (!g_CommandSocketRunning) {
		commandThread = std::thread(CommandSocketThread, this);
		commandThread.detach();
	}

	NTL_PRINT(PRINT_APP, "LOCAL PAGE COMMAND HANDLER READY");

	return NTL_SUCCESS;
}


int CGameServer::OnCreate()
{
	return NTL_SUCCESS;
}


void CGameServer::Run()
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
	DWORD dwNow, dwLastLoop = 0;

	while (true)
	{
		LARGE_INTEGER m_freq, rStart, rEnd, rLoadReport, rMemoryUsage;

		QueryPerformanceFrequency(&m_freq);
		QueryPerformanceCounter(&rStart);

		dwNow = GetTickCount();
		m_dwCurTickCount = dwNow;
		m_tmCurrentTime = time(NULL);

		//	DoUpdatePerformanceLog(dwNow); //requires too much time
		DoReportLoad(dwNow);
		QueryPerformanceCounter(&rLoadReport);

		DoUpdateMemoryUseLog(dwNow);
		DoUpdateSessionLog(dwNow);
		QueryPerformanceCounter(&rMemoryUsage);

		if (GetMasterServerSession())		//master server is the last one we connect.. So only loop when we are connected to master server
			m_pGameProcessor->Run(dwNow);

		dwLastLoop = GetTickCount();
		QueryPerformanceCounter(&rEnd);

		// float fDur = ((float)(rEnd.QuadPart - rStart.QuadPart)) * 1000.f / ((float)m_freq.QuadPart);

		// if (fDur > 200.f)
		// {
		// 	NTL_PRINT(PRINT_APP, "dwLastLoop %u - m_dwCurTickCount %u = %u > 200.", m_dwCurTickCount, dwLastLoop, dwLastLoop - m_dwCurTickCount);
		// 	ERR_LOG(LOG_SYSTEM, "MainLoop: Total %f, LoadReport %f, MemoryUsage %f, GameProcess %f",
		// 		fDur,
		// 		((float)(rLoadReport.QuadPart - rStart.QuadPart)) * 1000.f / ((float)m_freq.QuadPart),
		// 		((float)(rMemoryUsage.QuadPart - rLoadReport.QuadPart)) * 1000.f / ((float)m_freq.QuadPart),
		// 		((float)(rEnd.QuadPart - rMemoryUsage.QuadPart)) * 1000.f / ((float)m_freq.QuadPart)
		// 	);
		// }

		Wait(1);
	}

	ERR_LOG(LOG_SYSTEM, "%s", "CGameServer::Run(): IsRunnable() == false");
}


int	CGameServer::OnConfiguration(const char* lpszConfigFile)
{
	CNtlIniFile file;
	int rc = file.Create(lpszConfigFile);
	if (NTL_SUCCESS != rc)
		return rc;

	NTL_PRINT(PRINT_APP, "[CONFIG] Using config file: %s", lpszConfigFile ? lpszConfigFile : "<null>");

	if (!file.Read("Game Server", "Address", m_config.strClientAcceptAddr))
	{
		return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;
	}
	if (!file.Read("Game Server", "PublicAddress", m_config.strPublicClientAcceptAddr))
	{
		return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;
	}
	if (!file.Read("Game Server", "Port", m_config.wClientAcceptPort))
	{
		return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;
	}
	if (!file.Read("Game Server", "ServerID", m_config.byServerID))
	{
		return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;
	}
	if (!file.Read("Game Server", "Channel", m_config.byChannel))
	{
		return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;
	}
	if (!file.Read("Game Server", "Servername", m_config.ServerName))
	{
		return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;
	}
	file.Read("Game Server", "Channelname", m_config.ChannelName);



	//CONNECT CHAT SERVER
	if (!file.Read("Chat Connect", "Address", m_config.strChatServerIP))
	{
		return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;
	}
	if (!file.Read("Chat Connect", "Port", m_config.wChatServerPort))
	{
		return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;
	}
	if (!file.Read("Chat Connect", "ServerConAddr", m_config.strChatServerConAddr))
	{
		return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;
	}
	if (!file.Read("Chat Connect", "ServerConPort", m_config.wChatServerConPort))
	{
		return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;
	}

	//CONNECT MASTER SERVER
	if (!file.Read("Master Connect", "Address", m_config.strMasterServerIP))
	{
		return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;
	}
	if (!file.Read("Master Connect", "Port", m_config.wMasterServerPort))
	{
		return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;
	}


	if (!file.Read("Query Connect", "Address", m_config.strQueryServerIP))
	{
		return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;
	}
	if (!file.Read("Query Connect", "Port", m_config.wQueryServerPort))
	{
		return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;
	}

	//DATABASE
	if (!file.Read("DATABASE_CHARACTER", "Host", m_config.DatabaseHost))
	{
		return NTL_ERR_DBC_HANDLE_ALREADY_ALLOCATED;
	}
	if (!file.Read("DATABASE_CHARACTER", "Port", m_config.DatabasePort))
	{
		return NTL_ERR_DBC_HANDLE_ALREADY_ALLOCATED;
	}
	if (!file.Read("DATABASE_CHARACTER", "User", m_config.DatabaseUser))
	{
		return NTL_ERR_SYS_MEMORY_ALLOC_FAIL;
	}
	if (!file.Read("DATABASE_CHARACTER", "Password", m_config.DatabasePassword))
	{
		return NTL_ERR_SYS_LOG_SYSTEM_INITIALIZE_FAIL;
	}
	if (!file.Read("DATABASE_CHARACTER", "Db", m_config.Database))
	{
		return NTL_ERR_DBC_CONNECTION_CONNECT_FAIL;
	}
	if (!file.Read("DATABASE_ACCOUNT", "Host", m_config.AccDatabaseHost))
	{
		return NTL_ERR_DBC_HANDLE_ALREADY_ALLOCATED;
	}
	if (!file.Read("DATABASE_ACCOUNT", "Port", m_config.AccDatabasePort))
	{
		return NTL_ERR_DBC_HANDLE_ALREADY_ALLOCATED;
	}
	if (!file.Read("DATABASE_ACCOUNT", "User", m_config.AccDatabaseUser))
	{
		return NTL_ERR_SYS_MEMORY_ALLOC_FAIL;
	}
	if (!file.Read("DATABASE_ACCOUNT", "Password", m_config.AccDatabasePassword))
	{
		return NTL_ERR_SYS_LOG_SYSTEM_INITIALIZE_FAIL;
	}
	if (!file.Read("DATABASE_ACCOUNT", "Db", m_config.AccDatabase))
	{
		return NTL_ERR_DBC_CONNECTION_CONNECT_FAIL;
	}
	// CONFIG
	if (!file.Read("CONFIG", "MaxConnection", m_config.nMaxConnection))
	{
		return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;
	}

	// TABLES
	if (!file.Read("TABLE", "LoadTableFormat", m_config.LoadTableFormat))
	{
		return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;
	}
	if (!file.Read("TABLE", "Path", m_config.TablePath))
	{
		return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;
	}

	//SETTINGS
	if (!file.Read("SETTINGS", "TestServer", m_config.bTestServer))
		return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;
	if (!file.Read("SETTINGS", "LoadTriggersEnc", m_config.bLoadTriggersEnc))
		return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;
	if (!file.Read("SETTINGS", "LogPath", m_config.strLogPath))
		return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;
	if (!file.Read("SETTINGS", "TsPath", m_config.strTsPath))
		return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;

	// Optional GM-only mode (defaults to 0/false)
	{
		int gmOnly = 0;
		if (file.Read("SETTINGS", "AllowOnlyGMs", gmOnly))
			m_bGmOnlyMode = (gmOnly != 0);
		else
			m_bGmOnlyMode = false;
	}


	if (!file.Read("Play Script", "DataPath", m_config.strPlayScriptPath))
		return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;

	if (!file.Read("AI Script", "DataPath", m_config.strAIScriptPath))
		return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;

	if (!file.Read("World Play Script", "DataPath", m_config.strWorldPlayScriptPath))
		return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;

	if (!file.Read("Time Quest Script", "DataPath", m_config.strTimeQuestScriptPath))
		return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;


	//GRAPHIC
	if (!file.Read("Graphic Data", "CharacterDataBinaryPath", m_config.m_strGraphicCharacterDataPath))
		return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;
	if (!file.Read("Graphic Data", "ObjectDataBinaryPath", m_config.m_strGraphicObjectDataPath))
		return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;
	if (!file.Read("Graphic Data", "WorldDataPath", m_config.m_strWorldDataPath))
		return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;

	//NAVI
	if (!file.Read("Navigator", "PathEngineDllName", m_config.m_strPathEngineDllName))
		return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;
	if (!file.Read("Navigator", "DataPath", m_config.m_strNavDataPath))
		return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;
	if (!file.Read("Navigator", "EnableNavigator", m_config.m_bEnableNavigator))
		return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;

	// Optional AI verbose guard logging flag (defaults to false if missing)
	{
		m_config.m_bAIVerbose = false; // hard default
		int v = 0;
		if (file.Read("AI", "VerboseGuards", v))
		{
			m_config.m_bAIVerbose = (v != 0);
		}
	}

	//GAME CONFIGS
	if (!file.Read("GAMECONFIG", "MaxLevel", m_config.MaxLevel))
	{
		return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;
	}
	if (!file.Read("GAMECONFIG", "SoloExpRate", m_config.SoloExpRate))
	{
		return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;
	}
	if (!file.Read("GAMECONFIG", "PartyExpRate", m_config.PartyExpRate))
	{
		return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;
	}
	if (!file.Read("GAMECONFIG", "ItemDropRate", m_config.ItemDropRate))
	{
		return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;
	}
	if (!file.Read("GAMECONFIG", "StoneDropRate", m_config.StoneDropRate))
	{
		m_config.StoneDropRate = 100;
	}
	if (!file.Read("GAMECONFIG", "QuestExpRate", m_config.QuestExpRate))
	{
		return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;
	}
	if (!file.Read("GAMECONFIG", "ZeniDropRate", m_config.ZeniDropRate))
	{
		return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;
	}
	if (!file.Read("GAMECONFIG", "ZeniBonusRate", m_config.ZeniBonusRate))
	{
		return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;
	}
	if (!file.Read("GAMECONFIG", "ZeniPartyBonusRate", m_config.ZeniPartyBonusRate))
	{
		return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;
	}
	if (!file.Read("GAMECONFIG", "QuestMoneyRate", m_config.QuestMoneyRate))
	{
		return NTL_ERR_SYS_CONFIG_FILE_READ_FAIL;
	}

	// Optional helper NPC feature
	GetHelperNpcManager()->LoadConfig(file);
	{
		const sHELPER_NPC_CONFIG& cfg = GetHelperNpcManager()->GetConfig();
		NTL_PRINT(PRINT_APP, "[HELPER_NPC] Enable=%d AllowUltimate=%d AllowBattleDungeon=%d AllowTimeQuest=%d MinPartySizeToAvoid=%u",
			(int)cfg.bEnabled, (int)cfg.bAllowUltimate, (int)cfg.bAllowBattleDungeon, (int)cfg.bAllowTimeQuest, cfg.byMinPartySizeToAvoidSpawn);
		NTL_PRINT(PRINT_APP, "[HELPER_NPC] PrimaryNpcId=%u FallbackNpcId=%u UseMobAsHelper=%d MobId=%u",
			cfg.primaryNpcTblidx, cfg.fallbackNpcTblidx, (int)cfg.bUseMobAsHelper, cfg.helperMobTblidx);
		NTL_PRINT(PRINT_APP, "[HELPER_NPC] SpawnOffset=%.2f FollowLeader=%d AssistLeaderTarget=%d HealLpThresholdOverride=%u DamageMultiplier=%.2f HealPowerMultiplier=%.2f MoveSpeedMultiplier=%.2f AttackSpeedPercent=%u EpRegenPercent=%u InvincibleHelper=%d BuffCount=%zu",
			cfg.fSpawnOffset, (int)cfg.bFollowLeader, (int)cfg.bAssistLeaderTarget, cfg.wHealLpThresholdOverride, cfg.fDamageMultiplier, cfg.fHealPowerMultiplier, cfg.fMoveSpeedMultiplier, cfg.wAttackSpeedPercent, cfg.wEpRegenPercent, (int)cfg.bInvincibleHelper, cfg.vBuffSkills.size());
	}

	return NTL_SUCCESS;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------

BOOL CGameServer::OnCommandInput(std::string& sCmd)
{
	// Soporte para comandos con argumentos
	std::istringstream iss(sCmd);
	std::vector<std::string> args;
	std::string token;
	while (iss >> token) args.push_back(token);

	if (args.empty()) return TRUE;

	if (args[0] == "help") {
		printf("shutdown - Shutdown the server after 30 seconds \n");
		printf("playercount - return amount of players online\n");
		printf("sessioninfo - display detailed session and connection information\n");
		printf("startdbhunt - start dragonball hunt event\n");
		printf("stopdbhunt - stop dragonball hunt event\n");
		printf("startbossspawnevent - start boss spawn event\n");
		printf("addtitle <charname> <id> - add title to character\n");
		printf("additem <player> <itemid> <amount> - add item to player\n");
		printf("setzenny <player> <amount> - set zenny of player\n");
		printf("\n");
	}
	else if (args[0] == "additem" && (args.size() == 4 || args.size() == 3)) {
		// additem <player> <itemid> <amount> o additem <player> <itemid>
		std::string playerName = args[1];
		TBLIDX itemId = (TBLIDX)atoi(args[2].c_str());
		BYTE amount = 1;
		if (args.size() == 4) {
			amount = (BYTE)atoi(args[3].c_str());
			if (amount == 0 || amount == INVALID_BYTE)
				amount = 1;
		}
		WCHAR wszCharName[64] = { 0 };
		mbstowcs(wszCharName, playerName.c_str(), 63);
		CPlayer* pTarget = g_pObjectManager->FindByName(wszCharName);
		if (!pTarget || !pTarget->IsInitialized()) {
			printf("Player '%s' not found or not initialized\n", playerName.c_str());
			return FALSE;
		}
		if (pTarget->GetPlayerItemContainer()->CountEmptyInventory() >= 1) {
			sITEM_TBLDAT* pTblData = (sITEM_TBLDAT*)g_pTableContainer->GetItemTable()->FindData(itemId);
			if (pTblData) {
				if (pTblData->bValidity_Able == true && pTblData->byItem_Type != eITEM_TYPE::ITEM_TYPE_RECIPE) {
					if (amount > pTblData->byMax_Stack)
						amount = pTblData->byMax_Stack;
					g_pItemManager->CreateItem(pTarget, itemId, amount, INVALID_BYTE, INVALID_BYTE, pTblData->Item_Option_Tblidx == INVALID_TBLIDX);
					printf("Item %u x%d added to %s\n", itemId, amount, playerName.c_str());
					return TRUE;
				}
				else {
					printf("Item %u is not valid or is a recipe\n", itemId);
					return FALSE;
				}
			}
			else {
				printf("Item %u not found in table\n", itemId);
				return FALSE;
			}
		}
		else {
			printf("Player '%s' has no empty inventory slot\n", playerName.c_str());
			return FALSE;
		}
	}
	else if (args[0] == "setzenny" && args.size() == 3) {
		std::string playerName = args[1];
		DWORD amount = static_cast<DWORD>(std::stoul(args[2]));

		WCHAR wszCharName[64] = { 0 };
		mbstowcs(wszCharName, playerName.c_str(), 63);
		CPlayer* pTarget = g_pObjectManager->FindByName(wszCharName);
		if (!pTarget || !pTarget->IsInitialized()) {
			printf("Player '%s' not found or not initialized\n", playerName.c_str());
			return FALSE;
		}

		pTarget->UpdateZeni(ZENNY_CHANGE_TYPE_CHEAT, amount, true);
		return TRUE;
	}
	else if (args[0] == "addtitle" && args.size() == 3) {
		// addtitle <charname> <id>
		std::string charname = args[1];
		int titleId = atoi(args[2].c_str());
		WCHAR wszCharName[64] = { 0 };
		mbstowcs(wszCharName, charname.c_str(), 63);
		CPlayer* pPlayer = g_pObjectManager->FindByName(wszCharName);
		if (pPlayer) {
			pPlayer->AddCharTitle((TBLIDX)titleId);
			printf("Title %d added to %s\n", titleId, charname.c_str());
			return TRUE;
		}
		else {
			printf("Character '%s' not found\n", charname.c_str());
			return FALSE;
		}
	}
	// ...existing code for other commands...
	else if (sCmd == "shutdown") {
		printf("Server shut down begin\n");
		g_pGameProcessor->StartServerShutdownEvent();
	}
	else if (sCmd == "playercount") {
		printf("Currently %zu players online \n", g_pObjectManager->GetPlayerCount());
	}
	else if (sCmd == "sessioninfo") {
		// Display comprehensive session information with multi-instance awareness
		int currentSessions = GetNetwork()->GetSessionList()->GetCurCount();
		int maxSessions = GetNetwork()->GetSessionList()->GetMaxCount();
		int configMaxSessions = m_config.nMaxConnection;
		
		// Get current player count from ObjectManager
		size_t playerCount = g_pObjectManager->GetPlayerCount();
		
		// Identify server instance by port/config
		WORD serverPort = m_config.wClientAcceptPort;
		const char* instanceType = "Unknown";
		if (serverPort == 30000) instanceType = "Channel 0";
		else if (serverPort == 30001) instanceType = "Channel 1"; 
		else if (serverPort == 30009) instanceType = "Budokai Tournament";
		
		printf("[SESSION INFO - %s (Port: %d)]\n", instanceType, serverPort);
		printf("Current Sessions: %d\n", currentSessions);
		printf("Max Session Capacity: %d\n", maxSessions);
		printf("Config Max Connections: %d\n", configMaxSessions);
		printf("Session Utilization: %.1f%%\n", 
			configMaxSessions > 0 ? (float)currentSessions / configMaxSessions * 100.0f : 0.0f);
		printf("Available Slots: %d\n", configMaxSessions - currentSessions);
		printf("Current Players: %zu\n", playerCount);
		
		// Multi-instance specific warnings
		bool isBudokaiServer = (serverPort == 30009);
		if (isBudokaiServer) {
			printf("\n[BUDOKAI SERVER MONITORING]\n");
			printf("*** This is the Budokai Tournament server instance ***\n");
			if (g_pBudokaiManager) {
				printf("Budokai Manager Status: Active\n");
			} else {
				printf("Budokai Manager Status: NULL\n");
			}
		}
		
		// Basic leak detection warnings
		if (currentSessions > (int)playerCount + 10) { // Allow tolerance for server connections and connecting players
			printf("\n*** POTENTIAL LEAK: %d total sessions vs %zu players ***\n", currentSessions, playerCount);
			printf("*** Sessions may not be cleaned up properly on disconnect ***\n");
			if (isBudokaiServer) {
				printf("*** BUDOKAI SERVER LEAK: Check tournament event cleanup! ***\n");
			}
		}
		
		if (currentSessions >= configMaxSessions) {
			printf("\n*** SESSION LIMIT REACHED: Server may refuse new connections! ***\n");
			if (isBudokaiServer) {
				printf("*** CRITICAL: Budokai server cannot accept tournament participants! ***\n");
			}
		}
		else if (currentSessions >= (configMaxSessions * 0.9)) { // 90% threshold
			printf("\n*** WARNING: Session usage at %.1f%% - approaching limit ***\n", 
				(float)currentSessions / configMaxSessions * 100.0f);
		}
		
		// Session lifecycle monitoring for leak investigation
		printf("\n[SESSION ANALYSIS]\n");
		printf("Session overhead (sessions - players): %d\n", currentSessions - (int)playerCount);
		printf("Server connection slots (estimated): ~3-5\n");
		if ((currentSessions - (int)playerCount) > 15) {
			printf("*** HIGH OVERHEAD: Investigate session cleanup mechanisms ***\n");
		}
	}
	else if (sCmd == "sessioncleanup") {
		// Forces cleanup of dead/invalid sessions
		printf("Starting session cleanup...\n");
		
		// Get session counts before cleanup
		int sessionsBefore = GetNetwork()->GetSessionList()->GetCurCount();
		int maxSessions = GetNetwork()->GetSessionList()->GetMaxCount();
		
		// Force session list validation/cleanup
		DWORD currentTime = GetTickCount();
		GetNetwork()->GetSessionList()->ValidCheck(currentTime);
		
		// Get session counts after cleanup
		int sessionsAfter = GetNetwork()->GetSessionList()->GetCurCount();
		int sessionsRemoved = sessionsBefore - sessionsAfter;
		
		// Get current player count
		size_t playerCount = g_pObjectManager->GetPlayerCount();
		
		// Display results
		printf("[SESSION CLEANUP COMPLETE]\n");
		printf("Sessions before cleanup: %d\n", sessionsBefore);
		printf("Sessions after cleanup: %d\n", sessionsAfter);
		printf("Sessions removed: %d\n", sessionsRemoved);
		printf("Available slots now: %d\n", maxSessions - sessionsAfter);
		printf("Current players: %zu\n", playerCount);
		printf("Session overhead: %d\n", sessionsAfter - (int)playerCount);
		
		if (sessionsRemoved > 0) {
			printf("*** Session cleanup successful - %d dead sessions removed ***\n", sessionsRemoved);
		} else {
			printf("No dead sessions found - session list is clean\n");
		}
	}
	else if (sCmd == "budokaiinfo") {
		// Special command for Budokai server monitoring
		WORD serverPort = m_config.wClientAcceptPort;
		bool isBudokaiServer = (serverPort == 30009);
		
		if (!isBudokaiServer) {
			printf("This is not the Budokai server instance (Port: %d)\n", serverPort);
			printf("Use this command on the Budokai server (typically port 30009)\n");
			return TRUE;
		}
		
		printf("[BUDOKAI SERVER DIAGNOSTICS]\n");
		printf("Server Port: %d (Budokai Tournament Instance)\n", serverPort);
		
		// Session information
		int currentSessions = GetNetwork()->GetSessionList()->GetCurCount();
		int configMaxSessions = m_config.nMaxConnection;
		size_t playerCount = g_pObjectManager->GetPlayerCount();
		
		printf("Current Sessions: %d\n", currentSessions);
		printf("Current Players: %zu\n", playerCount);
		printf("Session Overhead: %d\n", currentSessions - (int)playerCount);
		
		// Budokai-specific checks
		if (g_pBudokaiManager) {
			printf("Budokai Manager: Active\n");
			printf("Check tournament state and participant cleanup\n");
		} else {
			printf("Budokai Manager: NULL (This may be an issue)\n");
		}
		
		// Leak detection specific to tournament server
		if (currentSessions > (int)playerCount + 20) {
			printf("\n*** BUDOKAI SESSION LEAK SUSPECTED ***\n");
			printf("*** Tournament participants may not be cleaning up properly ***\n");
			printf("*** Consider restarting Budokai server if persistent ***\n");
		}
		
		printf("\n[RECOMMENDATIONS]\n");
		printf("- Monitor this instance during tournament events\n");
		printf("- Check session counts before/after tournaments\n");
		printf("- Use @sessioncleanup if session overhead grows\n");
	}
	else if (sCmd == "logwpsscript") {
		g_pScriptAlgoManager->LogAllActiveScripts();
	}
	else if (sCmd == "startdbhunt") {
		g_pDragonballHuntEvent->StartEvent();
		NTL_PRINT(PRINT_APP, "Dragonball Hunt Event Started");
	}
	else if (sCmd == "stopdbhunt") {
		g_pDragonballHuntEvent->EndEvent();
		NTL_PRINT(PRINT_APP, "Dragonball Hunt Event Stopped");
	}
	else if (sCmd == "startdojo") {
		g_pDojoManager->StartDojoEvent();
		NTL_PRINT(PRINT_APP, "Dojo Event Started (manual)");
	}
	else if (sCmd == "stopdojo") {
		g_pDojoManager->StopDojoEvent();
		NTL_PRINT(PRINT_APP, "Dojo Event Stopped (manual)");
	}
	else if (sCmd == "dumpthreads") {
		// tThreadFactory::Instance().AllThreadDump();
		printf("Thread dump functionality is not available\n");
	}
	else if (sCmd == "StartAdultSolo") {
		g_pBudokaiManager->StartSoloAdultBudokai();
	}
	else if (sCmd == "StartAdultTeam") {
		g_pBudokaiManager->StartPartyAdultBudokai();
	}
	else if (sCmd == "StartJuniorSolo") {
		g_pBudokaiManager->StartSoloJuniorBudokai();
	}
	else if (sCmd == "StartJuniorTeam") {
		g_pBudokaiManager->StartPartyJuniorBudokai();
	}
	else if (sCmd == "startdbscramble") {
		g_pDragonballScramble->StartEvent();
		NTL_PRINT(PRINT_APP, "DragonballScramble Started");
	}
	else if (sCmd == "stopdbscramble") {
		g_pDragonballScramble->EndEvent(true);
		NTL_PRINT(PRINT_APP, "DragonballScramble Stopped");
	}
	else if (sCmd == "startbeeevent") {
		g_pHoneyBeeEvent->StartEvent();
		NTL_PRINT(PRINT_APP, "HoneybeeEvent Started");
	}
	else if (sCmd == "stopbeeevent") {
		g_pHoneyBeeEvent->EndEvent();
		NTL_PRINT(PRINT_APP, "HoneybeeEvent Stopped");
	}
	else if (sCmd == "startfairy") {
		g_pFairyEvent->StartEvent();
		NTL_PRINT(PRINT_APP, "Fairy Event Started");
	}
	else if (sCmd == "stopfairy") {
		g_pFairyEvent->EndEvent();
		NTL_PRINT(PRINT_APP, "Fairy Event Stopped");
	}
	else if (sCmd == "startdropstone") {
		g_pStoneDropEvent->StartEvent();
		NTL_PRINT(PRINT_APP, "Drop Stone Event Started");
	}
	else if (sCmd == "stopdropstone") {
		g_pStoneDropEvent->EndEvent();
		NTL_PRINT(PRINT_APP, "Drop Stone Event Stopped");
	}
	else if (args[0] == "dumphelper") {
		const sHELPER_NPC_CONFIG& cfg = GetHelperNpcManager()->GetConfig();
		printf("[HELPER_NPC] Enable=%d AllowUltimate=%d AllowBattleDungeon=%d AllowTimeQuest=%d MinPartySizeToAvoid=%u\n",
			(int)cfg.bEnabled, (int)cfg.bAllowUltimate, (int)cfg.bAllowBattleDungeon, (int)cfg.bAllowTimeQuest, cfg.byMinPartySizeToAvoidSpawn);
		printf("PrimaryNpcId=%u FallbackNpcId=%u UseMobAsHelper=%d MobId=%u\n",
			cfg.primaryNpcTblidx, cfg.fallbackNpcTblidx, (int)cfg.bUseMobAsHelper, cfg.helperMobTblidx);
		printf("SpawnOffset=%.2f FollowLeader=%d AssistLeaderTarget=%d HealLpThresholdOverride=%u DamageMultiplier=%.2f HealPowerMultiplier=%.2f MoveSpeedMultiplier=%.2f AttackSpeedPercent=%u EpRegenPercent=%u InvincibleHelper=%d BuffCount=%zu ForcedSkills=%zu\n",
			cfg.fSpawnOffset, (int)cfg.bFollowLeader, (int)cfg.bAssistLeaderTarget, cfg.wHealLpThresholdOverride, cfg.fDamageMultiplier, cfg.fHealPowerMultiplier, cfg.fMoveSpeedMultiplier, cfg.wAttackSpeedPercent, cfg.wEpRegenPercent, (int)cfg.bInvincibleHelper, cfg.vBuffSkills.size(), cfg.vForcedSkills.size());
		if (!cfg.vBuffSkills.empty())
		{
			printf("BuffSkills: ");
			for (size_t i=0; i<cfg.vBuffSkills.size(); ++i)
			{
				printf("%u%s", cfg.vBuffSkills[i], (i+1<cfg.vBuffSkills.size())?", ":"\n");
			}
			printf("BuffBasis=%u BuffLP=%u BuffTime=%u\n", cfg.buffBasis, cfg.buffLP, cfg.buffTime);
		}
		if (!cfg.vForcedSkills.empty())
		{
			printf("ForcedSkills: ");
			for (size_t i=0; i<cfg.vForcedSkills.size(); ++i)
			{
				printf("%u%s", cfg.vForcedSkills[i], (i+1<cfg.vForcedSkills.size())?", ":"\n");
			}
			printf("ForcedBasis=%u ForcedLP=%u ForcedTime=%u\n", cfg.forcedSkillBasis, cfg.forcedSkillLP, cfg.forcedSkillTime);
		}
	}
	else if (args[0] == "findskill" && args.size() == 2)
	{
		TBLIDX skillId = (TBLIDX)atoi(args[1].c_str());
		sSKILL_TBLDAT* pSkill = (sSKILL_TBLDAT*)g_pTableContainer->GetSkillTable()->FindData(skillId);
		if (!pSkill)
		{
			printf("Skill %u not found in SkillTable\n", skillId);
			return FALSE;
		}
		// Print name if available in table struct (commonly wszNameText)
		// Note: If your sSKILL_TBLDAT does not have wszNameText, this will still compile if defined in SkillTable.h;
		// otherwise, only the fallback line will execute after you add proper name resolution.
		if (pSkill->wszNameText[0] != L'\0')
			wprintf(L"Skill %u name: %ls\n", skillId, pSkill->wszNameText);
		else
			printf("Skill %u found (empty name field)\n", skillId);
		return TRUE;
	}
	return TRUE;
}


void CGameServer::Init()
{
	m_dwCurTickCount = 0;
	m_dwLastTimePerformanceLogged = 0;
	m_dwLastTimeLoadReported = 0;
	m_dwLastTimeMemoryUseLogged = 0;
	m_dwLastTimeSessionLogged = 0;
	m_pChatServerSession = NULL;
	m_pMasterServerSession = NULL;
	m_pQueryServerSession = NULL;
	m_pGameProcessor = NULL;
	m_pGameMain = NULL;
	m_pGameData = NULL;
	m_pActionPatternSystem = NULL;

	// default: GM-only mode disabled
	m_bGmOnlyMode = false;
}


void CGameServer::Destroy()
{
	SAFE_DELETE(m_pSessionFactory);

	m_pChatServerSession = NULL;
	m_pMasterServerSession = NULL;
	m_pQueryServerSession = NULL;

	SAFE_DELETE(m_pGameProcessor);
	SAFE_DELETE(m_pGameMain);
	SAFE_DELETE(m_pGameData);

	// Detener el hilo del socket de comandos
	g_CommandSocketRunning = false;
}


void CGameServer::DoUpdatePerformanceLog(DWORD dwNow)
{
	if (dwNow - m_dwLastTimePerformanceLogged >= 10000)
	{
		//	m_performance.UpdateLog();

		m_dwLastTimePerformanceLogged = dwNow;
	}
}


void CGameServer::DoReportLoad(DWORD dwNow)
{
	if (dwNow - m_dwLastTimeLoadReported >= 10000)
	{
		if (m_dwLastTimeLoadReported)
		{
			//	ERR_LOG(LOG_SYSTEM, "GetProcessProcessorLoad() = %u, GetSystemProcessorLoad() = %u, GetProcessMemoryUsage() = %u", m_performance.GetProcessProcessorLoad(), m_performance.GetSystemProcessorLoad(), m_performance.GetProcessMemoryUsage());
			g_pServerInfoManager->SendMasterServerAlive();
		}

		m_dwLastTimeLoadReported = dwNow;
	}
}


void CGameServer::DoUpdateMemoryUseLog(DWORD dwNow)
{
	if (dwNow - m_dwLastTimeMemoryUseLogged >= 60000)
	{


		m_dwLastTimeMemoryUseLogged = dwNow;
	}
}

void CGameServer::DoUpdateSessionLog(DWORD dwNow)
{
	// Log session information every 5 minutes to help track connection issues
	if (dwNow - m_dwLastTimeSessionLogged >= 300000) // 5 minutes = 300,000 ms
	{
		int currentSessions = GetNetwork()->GetSessionList()->GetCurCount();
		int maxSessions = GetNetwork()->GetSessionList()->GetMaxCount();
		float utilization = maxSessions > 0 ? (float)currentSessions / maxSessions * 100.0f : 0.0f;
		
		// Include acceptor counters for deeper diagnostics
		int accAccepting = m_clientAcceptor.GetAcceptingCount();
		int accAccepted = m_clientAcceptor.GetAcceptedCount();
		// Accepted-client utilization relative to configured client capacity
		int cfgMaxClients = m_config.nMaxConnection;
		float clientUtil = cfgMaxClients > 0 ? (float)accAccepted / (float)cfgMaxClients * 100.0f : 0.0f;
		NTL_PRINT(PRINT_APP, "[SESSION MONITOR] Sessions: %d/%d (%.1f%%) | Clients(accepted): %d/%d (%.1f%%) | Avail(Sessions): %d | Acceptor accepting:%d accepted:%d total:%lu",
			currentSessions, maxSessions, utilization,
			accAccepted, cfgMaxClients, clientUtil,
			maxSessions - currentSessions,
			accAccepting, accAccepted, m_clientAcceptor.GetTotalAcceptCount());
		
		// Warn if accepted-client utilization is getting high
		if (clientUtil >= 80.0f)
		{
			NTL_PRINT(PRINT_APP, "WARNING: High client utilization detected! May start refusing connections soon.");
		}
		
		// Error if we're at capacity
		if (currentSessions >= maxSessions)
		{
			ERR_LOG(LOG_SYSTEM, "CRITICAL: Session capacity reached! Server will refuse new connections!");
		}
		
		m_dwLastTimeSessionLogged = dwNow;
	}
}


//-----------------------------------------------------------------------------------
//		GameServerMain
//-----------------------------------------------------------------------------------
int main(int argc, _TCHAR* argv[])
{
	CGameServer app;
	CNtlFileStream traceFileStream;

	SYSTEMTIME ti;
	GetLocalTime(&ti);

	// CHECK INI FILE AND START PROGRAM
	int rc = app.Create(argc, argv, argv[1]);
	if (NTL_SUCCESS != rc)
		return rc;

	CNtlString consolename;
	consolename.Format("OpenDBO Game Server - Server %d Channel %d", app.GetGsServerId(), app.GetGsChannel());

	SetConsoleTitle(consolename.c_str());

	// LOG FILE
	char m_LogFile[256];
	sprintf(m_LogFile, "%s\\channel%u\\gamelog_%02u-%02u-%02u.txt", app.GetLogPath().c_str(), app.GetGsChannel(), ti.wYear, ti.wMonth, ti.wDay);

	rc = traceFileStream.Create(m_LogFile);
	if (NTL_SUCCESS != rc)
		return rc;

	app.GetLog()->AttachLogStream(traceFileStream.GetFilePtr());


	NtlSetPrintFlag(PRINT_APP | PRINT_SYSTEM);

	app.Start();
	app.WaitCommandInput();
	app.WaitForTerminate();

	return 0;
}
