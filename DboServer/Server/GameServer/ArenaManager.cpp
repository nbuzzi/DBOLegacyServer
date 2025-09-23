#include "stdafx.h"
#include "ArenaManager.h"
#include "NtlIniFile.h"
#include "GameServer.h"
#include "ObjectManager.h"
#include "CPlayer.h"
#include "NtlPacketGU.h"
#include "NtlPacketGT.h"
#include "NtlAdmin.h"
#include "GameMain.h"
#include "World.h"
#include "WorldTable.h"
#include "TableContainerManager.h"
#include "ItemManager.h"
#include "NtlLog.h"

static unsigned long ToMs(unsigned int seconds) { return seconds * 1000UL; }

CArenaManager::CArenaManager()
{
	m_state = State::IDLE;
	m_mode = Mode::OPEN;
	m_currentWorldTblidx = 0;
	m_worldIndex = 0;
	m_rotationRemainMs = 0;
	m_roundUiActive = false;
	m_roundRemainMs = 0;
	m_roundWorldId = 0;
}

CArenaManager::~CArenaManager() {}

bool CArenaManager::LoadConfigFromIniPath(const char* iniPath)
{
	CNtlIniFile file;
	if (file.Create(iniPath) != NTL_SUCCESS)
		return false;

	// [Arena]
	int enabled = 0;
	if (file.Read("Arena", "Enabled", enabled)) 
	{
		m_cfg.enabled = (enabled != 0);
	}

	CNtlString worldsCsv = file.Read("Arena", "WorldTblidxList");
	ParseWorldListCsv(worldsCsv);

	unsigned int rotSec = 0;
	if (file.Read("Arena", "RotationSeconds", rotSec)) m_cfg.rotationSeconds = rotSec;
	unsigned int roundSec = 0;
	if (file.Read("Arena", "RoundTimerSeconds", roundSec)) m_cfg.roundTimerSeconds = roundSec;

	// [Spectator]
	int specEnabled = 0;
	if (file.Read("Spectator", "Enabled", specEnabled)) m_cfg.spectatorsEnabled = (specEnabled != 0);
	int specSame = 1;
	if (file.Read("Spectator", "UseSameWorld", specSame)) m_cfg.spectatorsUseSameWorld = (specSame != 0);
	unsigned int specWorld = 0;
	if (file.Read("Spectator", "WorldTblidx", specWorld)) m_cfg.spectatorWorldTblidx = specWorld;
	float fx = 0, fy = 0, fz = 0;
	if (file.Read("Spectator", "PosX", fx)) m_cfg.spectatorPosX = fx;
	if (file.Read("Spectator", "PosY", fy)) m_cfg.spectatorPosY = fy;
	if (file.Read("Spectator", "PosZ", fz)) m_cfg.spectatorPosZ = fz;

	// [Rewards]
	int rewardsEnabled = 0;
	if (file.Read("Rewards", "Enabled", rewardsEnabled)) m_cfg.rewardsEnabled = (rewardsEnabled != 0);
	CNtlString winCsv = file.Read("Rewards", "Winners");
	CNtlString partCsv = file.Read("Rewards", "Participants");
	ParseRewardsCsv(winCsv, m_cfg.winnerRewards);
	ParseRewardsCsv(partCsv, m_cfg.participantRewards);

	// Initialize rotation
	if (!m_cfg.worldTblidxList.empty())
	{
		m_worldIndex = 0;
		m_currentWorldTblidx = m_cfg.worldTblidxList[m_worldIndex];
		m_rotationRemainMs = ToMs(m_cfg.rotationSeconds);
	}

	return true;
}

void CArenaManager::TickProcess(unsigned long dwTickDiff)
{
	if (!m_cfg.enabled)
		return;

	TryRotateByTime(dwTickDiff);

	// Update round timer UI countdown
	if (m_roundUiActive)
	{
		if (m_roundRemainMs > dwTickDiff)
		{
			m_roundRemainMs -= dwTickDiff;
		}
		else
		{
			// time over
			m_roundRemainMs = 0;
			StopRoundTimerUI();
		}
	}
}

void CArenaManager::Start(Mode mode)
{
	m_mode = mode;
	m_state = State::ENROLLMENT; // placeholder entry
	m_participants.clear();
	m_spectators.clear();
	m_winners.clear();
	BroadcastSystem(L"[Arena] Started. Use @arena status for details.");
}

void CArenaManager::Stop(bool abort)
{
	m_state = State::COMPLETE;
	StopRoundTimerUI();
	if (m_cfg.rewardsEnabled && !abort)
	{
		AwardRewards(true);   // winners
		AwardRewards(false);  // all participants
	}
	BroadcastSystem(abort ? L"[Arena] Stopped (abort)." : L"[Arena] Completed.");
	m_state = State::IDLE;
}

void CArenaManager::RotateMapNow()
{
	if (m_cfg.worldTblidxList.empty()) return;
	StopRoundTimerUI();
	m_worldIndex = (m_worldIndex + 1) % m_cfg.worldTblidxList.size();
	m_currentWorldTblidx = m_cfg.worldTblidxList[m_worldIndex];
	m_rotationRemainMs = ToMs(m_cfg.rotationSeconds);
	BroadcastSystem(L"[Arena] Map rotated.");
}
bool CArenaManager::SetCurrentWorld(unsigned int worldTblidx)
{
	if (worldTblidx == 0) return false;
	// validate exists in table
	sWORLD_TBLDAT* pWorldTbldat = (sWORLD_TBLDAT*)g_pTableContainer->GetWorldTable()->FindData((TBLIDX)worldTblidx);
	if (!pWorldTbldat) return false;

	StopRoundTimerUI();
	m_currentWorldTblidx = worldTblidx;
	// update index if present in list
	for (size_t i = 0; i < m_cfg.worldTblidxList.size(); ++i)
		if (m_cfg.worldTblidxList[i] == worldTblidx) { m_worldIndex = i; break; }
	m_rotationRemainMs = ToMs(m_cfg.rotationSeconds);
	BroadcastSystem(L"[Arena] Map set.");
	return true;
}

void CArenaManager::StatusTo(CPlayer* pWho)
{
	if (!pWho) return;
	wchar_t buf[256];
	swprintf_s(buf, L"[Arena] State=%u Mode=%u World=%u NextRotate=%us Enabled=%d Participants=%u Spectators=%u Winners=%u",
		(unsigned)m_state, (unsigned)m_mode, m_currentWorldTblidx, (unsigned)(m_rotationRemainMs / 1000), (int)m_cfg.enabled,
		(unsigned)m_participants.size(), (unsigned)m_spectators.size(), (unsigned)m_winners.size());
	CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
	res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
	res->byDisplayType = SERVER_TEXT_SYSTEM;
	res->wMessageLengthInUnicode = (WORD)wcslen(buf);
	wcscpy_s(res->awchMessage, NTL_MAX_LENGTH_OF_CHAT_MESSAGE + 1, buf);
	packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	pWho->SendPacket(&packet);
}

void CArenaManager::BroadcastSystem(const wchar_t* msg)
{
	CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
	res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
	res->byDisplayType = SERVER_TEXT_SYSTEM;
	res->wMessageLengthInUnicode = (WORD)wcslen(msg);
	wcscpy_s(res->awchMessage, NTL_MAX_LENGTH_OF_CHAT_MESSAGE + 1, msg);
	packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	g_pObjectManager->SendPacketToAll(&packet);
}

void CArenaManager::AwardRewards(bool winnersOnly)
{
	if (!m_cfg.rewardsEnabled)
		return;

	const auto& list = winnersOnly ? m_cfg.winnerRewards : m_cfg.participantRewards;
	if (list.empty()) return;

	auto grant = [&](CPlayer* p, const std::vector<std::pair<unsigned int, unsigned int>>& rewards)
		{
			for (const auto& pr : rewards)
			{
				unsigned int item = pr.first;
				unsigned int cnt = pr.second ? pr.second : 1;
				g_pItemManager->CreateItem(p, (TBLIDX)item, (BYTE)cnt);
			}
		};

	if (winnersOnly)
	{
		for (auto cid : m_winners)
		{
			CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid);
			if (p && p->IsInitialized()) grant(p, list);
		}
	}
	else
	{
		for (auto cid : m_participants)
		{
			CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid);
			if (p && p->IsInitialized()) grant(p, list);
		}
	}
}

void CArenaManager::TryRotateByTime(unsigned long dwTickDiff)
{
	if (m_cfg.rotationSeconds == 0 || m_cfg.worldTblidxList.size() <= 1)
		return;
	if (m_rotationRemainMs > dwTickDiff)
		m_rotationRemainMs -= dwTickDiff;
	else
		RotateMapNow();
}

void CArenaManager::ParseWorldListCsv(const CNtlString& csv)
{
	m_cfg.worldTblidxList.clear();
	std::string s = csv.c_str();
	size_t pos = 0;
	while (pos != std::string::npos)
	{
		size_t comma = s.find(',', pos);
		std::string tok = s.substr(pos, comma == std::string::npos ? std::string::npos : (comma - pos));
		if (!tok.empty())
		{
			unsigned int v = (unsigned int)strtoul(tok.c_str(), nullptr, 10);
			if (v != 0)
				m_cfg.worldTblidxList.push_back(v);
		}
		if (comma == std::string::npos) break;
		pos = comma + 1;
		while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\t')) ++pos; // trim whitespace
	}
}

bool CArenaManager::AddParticipant(CPlayer* pPlayer)
{
	if (!pPlayer || !pPlayer->IsInitialized() || !m_cfg.enabled) return false;
	m_participants.insert(pPlayer->GetCharID());
	return true;
}

bool CArenaManager::AddSpectator(CPlayer* pPlayer)
{
	if (!pPlayer || !pPlayer->IsInitialized() || !m_cfg.enabled || !m_cfg.spectatorsEnabled) return false;
	m_spectators.insert(pPlayer->GetCharID());
	return true;
}

bool CArenaManager::Remove(CPlayer* pPlayer)
{
	if (!pPlayer) return false;
	m_participants.erase(pPlayer->GetCharID());
	m_spectators.erase(pPlayer->GetCharID());
	m_winners.erase(pPlayer->GetCharID());
	return true;
}

void CArenaManager::TeleportParticipants()
{
	if (m_currentWorldTblidx == 0) { NTL_PRINT(PRINT_APP, "[ARENA] TeleportParticipants aborted: currentWorldTblidx=0"); return; }
	for (auto cid : m_participants)
	{
		CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid);
		if (!p || !p->IsInitialized()) { NTL_PRINT(PRINT_APP, "[ARENA] Skip TP: player missing or not initialized: cid=%u", cid); continue; }
		TeleportOneToWorldTblidx(p, m_currentWorldTblidx, 0.f, 0.f, 0.f);
	}
	// Start visible round timer if configured
	if (m_cfg.roundTimerSeconds > 0)
		StartRoundTimerUI(m_cfg.roundTimerSeconds);
}

void CArenaManager::TeleportSpectators()
{
	if (!m_cfg.spectatorsEnabled) return;
	unsigned int worldTblidx = m_cfg.spectatorsUseSameWorld ? m_currentWorldTblidx : m_cfg.spectatorWorldTblidx;
	if (worldTblidx == 0) return;
	for (auto cid : m_spectators)
	{
		CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid);
		if (!p || !p->IsInitialized()) continue;
		TeleportOneToWorldTblidx(p, worldTblidx, m_cfg.spectatorPosX, m_cfg.spectatorPosY, m_cfg.spectatorPosZ);
	}
}

bool CArenaManager::TeleportOneToWorldTblidx(CPlayer* pPlayer, unsigned int worldTblidx, float posX, float posY, float posZ)
{
	CGameServer* app = (CGameServer*)g_pApp;
	sWORLD_TBLDAT* pWorldTbldat = (sWORLD_TBLDAT*)g_pTableContainer->GetWorldTable()->FindData((TBLIDX)worldTblidx);
	if (!pWorldTbldat) return false;

	// Determine destination location
	CNtlVector destLoc = (posX == 0.f && posY == 0.f && posZ == 0.f) ? pWorldTbldat->vStart1Loc : CNtlVector(posX, posY, posZ);

	// Ensure world exists locally on this GameServer and teleport directly (no channel change)
	CWorld* pWorld = app->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)worldTblidx);
	if (!pWorld)
	{
		pWorld = app->GetGameMain()->GetWorldManager()->CreateWorld(pWorldTbldat);
		if (!pWorld) return false;
		NTL_PRINT(PRINT_APP, "[ARENA] Created local world for tblidx=%u id=%u on channel=%u", worldTblidx, (unsigned)pWorld->GetID(), (unsigned)app->GetGsChannel());
	}

	pPlayer->StartTeleport(destLoc, pPlayer->GetCurDir(), pWorld->GetID(), TELEPORT_TYPE_DOJO);
	NTL_PRINT(PRINT_APP, "[ARENA] Local teleport: char=%u worldTblidx=%u worldId=%u channel=%u", (unsigned)pPlayer->GetCharID(), worldTblidx, (unsigned)pWorld->GetID(), (unsigned)app->GetGsChannel());
	return true;
}

void CArenaManager::TeleportParticipantsHere(CPlayer* pGm)
{
	if (!pGm || !pGm->IsInitialized()) return;
	for (auto cid : m_participants)
	{
		if (CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid))
		{
			if (!p->IsInitialized()) continue;
			// Only teleports players connected to this GameServer instance
			p->StartTeleport(pGm->GetCurLoc(), pGm->GetCurDir(), pGm->GetWorldID(), TELEPORT_TYPE_DOJO);
		}
	}
}

void CArenaManager::TeleportSpectatorsHere(CPlayer* pGm)
{
	if (!pGm || !pGm->IsInitialized()) return;
	for (auto cid : m_spectators)
	{
		if (CPlayer* p = g_pObjectManager->FindByChar((CHARACTERID)cid))
		{
			if (!p->IsInitialized()) continue;
			// Only teleports players connected to this GameServer instance
			p->StartTeleport(pGm->GetCurLoc(), pGm->GetCurDir(), pGm->GetWorldID(), TELEPORT_TYPE_DOJO);
		}
	}
}

void CArenaManager::MarkWinner(CPlayer* pPlayer)
{
	if (!pPlayer) return;
	m_winners.insert(pPlayer->GetCharID());
}

void CArenaManager::ClearWinners()
{
	m_winners.clear();
}

void CArenaManager::ParseRewardsCsv(const CNtlString& csv, std::vector<std::pair<unsigned int, unsigned int>>& out)
{
	out.clear();
	std::string s = csv.c_str();
	size_t pos = 0;
	while (pos != std::string::npos)
	{
		size_t sep = s.find(',', pos);
		std::string tok = s.substr(pos, sep == std::string::npos ? std::string::npos : (sep - pos));
		if (!tok.empty())
		{
			size_t colon = tok.find(':');
			if (colon == std::string::npos)
			{
				unsigned int item = (unsigned int)strtoul(tok.c_str(), nullptr, 10);
				if (item) out.push_back({ item, 1 });
			}
			else
			{
				unsigned int item = (unsigned int)strtoul(tok.substr(0, colon).c_str(), nullptr, 10);
				unsigned int cnt = (unsigned int)strtoul(tok.substr(colon + 1).c_str(), nullptr, 10);
				if (item && cnt) out.push_back({ item, cnt });
			}
		}
		if (sep == std::string::npos) break;
		pos = sep + 1;
		while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\t')) ++pos;
	}
}

void CArenaManager::BroadcastRoundTimerStartToWorld(unsigned int worldId, unsigned int seconds)
{
	CGameServer* app = (CGameServer*)g_pApp;
	CWorld* pWorld = app->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)worldId);
	if (!pWorld)
		return;

	CNtlPacket packet(sizeof(sGU_BATTLE_DUNGEON_LIMIT_TIME_START_NFY));
	sGU_BATTLE_DUNGEON_LIMIT_TIME_START_NFY* res = (sGU_BATTLE_DUNGEON_LIMIT_TIME_START_NFY*)packet.GetPacketData();
	res->wOpCode = GU_BATTLE_DUNGEON_LIMIT_TIME_START_NFY;
	res->dwLimitTime = seconds; // client expects seconds
	packet.SetPacketLen(sizeof(sGU_BATTLE_DUNGEON_LIMIT_TIME_START_NFY));
	pWorld->Broadcast(&packet);
}

void CArenaManager::BroadcastRoundTimerEndToWorld(unsigned int worldId)
{
	CGameServer* app = (CGameServer*)g_pApp;
	CWorld* pWorld = app->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)worldId);
	if (!pWorld)
		return;

	CNtlPacket packet(sizeof(sGU_BATTLE_DUNGEON_LIMIT_TIME_END_NFY));
	sGU_BATTLE_DUNGEON_LIMIT_TIME_END_NFY* res = (sGU_BATTLE_DUNGEON_LIMIT_TIME_END_NFY*)packet.GetPacketData();
	res->wOpCode = GU_BATTLE_DUNGEON_LIMIT_TIME_END_NFY;
	packet.SetPacketLen(sizeof(sGU_BATTLE_DUNGEON_LIMIT_TIME_END_NFY));
	pWorld->Broadcast(&packet);
}

void CArenaManager::StartRoundTimerUI(unsigned int seconds)
{
	if (m_currentWorldTblidx == 0)
		return;

	// Ensure world exists and fetch ID
	CGameServer* app = (CGameServer*)g_pApp;
	sWORLD_TBLDAT* pWorldTbldat = (sWORLD_TBLDAT*)g_pTableContainer->GetWorldTable()->FindData((TBLIDX)m_currentWorldTblidx);
	if (!pWorldTbldat)
		return;
	CWorld* pWorld = app->GetGameMain()->GetWorldManager()->FindWorld((WORLDID)m_currentWorldTblidx);
	if (!pWorld)
		pWorld = app->GetGameMain()->GetWorldManager()->CreateWorld(pWorldTbldat);
	if (!pWorld)
		return;

	m_roundUiActive = true;
	m_roundRemainMs = ToMs(seconds);
	m_roundWorldId = (unsigned int)pWorld->GetID();
	BroadcastRoundTimerStartToWorld(m_roundWorldId, seconds);
}

void CArenaManager::StopRoundTimerUI()
{
	if (!m_roundUiActive)
		return;
	BroadcastRoundTimerEndToWorld(m_roundWorldId);
	m_roundUiActive = false;
	m_roundRemainMs = 0;
	m_roundWorldId = 0;
}
