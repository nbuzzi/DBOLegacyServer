#include "stdafx.h"
#include "CharacterManager.h"
#include "QueryServer.h"


CCharacterManager::CCharacterManager()
{
	Init();
}

CCharacterManager::~CCharacterManager()
{
}


void CCharacterManager::Init()
{
	m_uiLastCharacterId = 0;

	smart_ptr<QueryResult> result = GetCharDB.Query("SELECT MAX(CharID) FROM characters");
	if (result)
	{
		Field* f = result->Fetch();

		m_uiLastCharacterId = f[0].GetUInt32();
	}

	ERR_LOG(LOG_GENERAL, "Last Character-ID %u", m_uiLastCharacterId);
}


void CCharacterManager::CreateCharacter(ACCOUNTID accountId, sPC_SUMMARY& sSum, SERVERFARMID serverFarmId, bool isGM)
{
	// Convert wide name to UTF-8 and escape for safe SQL insertion
	std::string utf8Name = ws2s(sSum.awchName);
	std::string escName = GetCharDB.EscapeString(utf8Name);

	// Persist SrvFarmID so subsequent reloads (which filter by SrvFarmID) include this character
	GetCharDB.Execute(
		"INSERT INTO characters (CharID,CharName,AccountID,Race,Class,Gender,Face,Hair,HairColor,SkinColor,CurLocX,CurLocY,CurLocZ,WorldID,WorldTable,MapInfoIndex,CreateTime,GameMaster,SrvFarmID)"
		" VALUES (%u,'%s',%u,%u,%u,%u,%u,%u,%u,%u,%f,%f,%f,%u,%u,%u,%I64u,%u,%u)",
		sSum.charId, escName.c_str(), accountId, sSum.byRace, sSum.byClass, sSum.byGender, sSum.sPcShape.byFace, sSum.sPcShape.byHair, sSum.sPcShape.byHairColor, sSum.sPcShape.bySkinColor,
		sSum.fPositionX, sSum.fPositionY, sSum.fPositionZ,
		sSum.worldId, sSum.worldTblidx, sSum.dwMapInfoIndex, time(0), isGM, serverFarmId);
}