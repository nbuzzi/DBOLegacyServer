#include "stdafx.h"
#include "WorldIdxList.h"
#include "World.h"


CWorldIdxList::CWorldIdxList()
{
	m_iterator = m_worldMap.end();
	m_iteratorBeg = m_worldMap.end();
	m_iteratorEnd = m_worldMap.end();
}

CWorldIdxList::~CWorldIdxList()
{
}


bool CWorldIdxList::AddWorld(CWorld *pWorld)
{
	if (!Find(pWorld))
	{
		m_worldMap.insert(WORLDVAL(pWorld->GetIdx(), pWorld));
	}

	return true;
}

void CWorldIdxList::RemoveWorld(CWorld *pWorld)
{
	if (!pWorld)
		return;

	TBLIDX key = pWorld->GetIdx();
	auto lower = m_worldMap.lower_bound(key);
	auto upper = m_worldMap.upper_bound(key);
	if (lower == upper)
		return;

	if (m_iterator != m_worldMap.end())
	{
		if (!(m_iterator->first < key) && !(key < m_iterator->first))
			m_iterator = m_worldMap.end();
	}
	if (m_iteratorBeg != m_worldMap.end())
	{
		if (!(m_iteratorBeg->first < key) && !(key < m_iteratorBeg->first))
			m_iteratorBeg = m_worldMap.end();
	}
	if (m_iteratorEnd != m_worldMap.end())
	{
		if (!(m_iteratorEnd->first < key) && !(key < m_iteratorEnd->first))
			m_iteratorEnd = m_worldMap.end();
	}

	m_worldMap.erase(lower, upper);
}



CWorld* CWorldIdxList::GetFirst()
{
	m_iterator = m_worldMap.begin();
	if (m_iterator != m_worldMap.end())
	{
		return m_iterator->second;
	}

	return NULL;
}

CWorld* CWorldIdxList::GetNext()
{
	if (m_iterator == m_worldMap.end())
	{
		ERR_LOG(LOG_SYSTEM, "m_iterator == m_worldMap.end()");
		return NULL;
	}

	WORLDMAP::iterator it = ++m_iterator;

	if (it != m_worldMap.end())
	{
		return it->second;
	}

	return NULL;
}

CWorld* CWorldIdxList::GetFirst(TBLIDX worldIdx)
{
	m_iteratorBeg = m_worldMap.lower_bound(worldIdx);
	m_iteratorEnd = m_worldMap.upper_bound(worldIdx);

	if (m_iteratorBeg == m_iteratorEnd)
		return NULL;

	return m_iteratorBeg->second;
}

CWorld* CWorldIdxList::GetNext(TBLIDX worldIdx)
{
	if (m_iteratorBeg == m_iteratorEnd)
	{
		ERR_LOG(LOG_SYSTEM, "m_iteratorBeg || m_iteratorEnd == NULL");
		return NULL;
	}

	if (worldIdx != m_iteratorBeg->second->GetIdx())
	{
		return NULL;
	}

	if (++m_iteratorBeg == m_iteratorEnd)
	{
		return NULL;
	}
	return m_iteratorBeg->second;
}


bool CWorldIdxList::Find(CWorld *pWorld)
{
	WORLDMAP::iterator beg = m_worldMap.lower_bound(pWorld->GetIdx());
	WORLDMAP::iterator end = m_worldMap.upper_bound(pWorld->GetIdx());

	while (beg != end)
	{
		if (beg->second->GetIdx() == pWorld->GetIdx())
			return true;

		++beg;
	}

	return false;
}


void CWorldIdxList::Init()
{
	m_worldMap.clear();
}