#include "stdafx.h"
#include "Npc.h"
#include "BuffBot.h"
#include "TableContainerManager.h"
#include "SystemEffectTable.h"
#include "Monster.h"
#include "CustomDropEvent.h"


CBuffManagerBot::CBuffManagerBot()
{
	m_pBotRef = NULL;
}

CBuffManagerBot::~CBuffManagerBot()
{
	m_pBotRef = NULL;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
bool CBuffManagerBot::Create(CNpc *pOwnerRef)
{
	if (CBuffManager::Create(pOwnerRef))
	{
		m_pBotRef = pOwnerRef;
		return true;
	}

	return false;
}


bool CBuffManagerBot::RegisterBuff(DWORD& rdwKeepTime, eSYSTEM_EFFECT_CODE* effectCode, sDBO_BUFF_PARAMETER * paBuffParameter, HOBJECT hCaster, eBUFF_TYPE buffType, sSKILL_TBLDAT* pSkillTbldat, BYTE* prBuffIndex)
{
	// Selective debuff immunity for event-modified monsters - only during custom event drop event
	if (g_pCustomDropEvent->m_bOn == TRUE && buffType == BUFF_TYPE_CURSE && m_pBotRef && m_pBotRef->IsMonster())
	{
		CMonster* pMon = reinterpret_cast<CMonster*>(m_pBotRef);
		if (pMon && pMon->IsEventDebuffImmune())
		{
			// Check global toggle
			if (g_pCustomDropEvent->IsDebuffImmunityEnabled())
			{
				// If no specific debuff effects configured, block all curse-type buffs
				if (g_pCustomDropEvent->GetBlockedDebuffEffectCount() == 0)
					return false;
				// Otherwise, block only if any effect code matches configured block list
				if (effectCode)
				{
					for (int i = 0; i < NTL_MAX_EFFECT_IN_SKILL; ++i)
					{
						if (effectCode[i] == INVALID_SYSTEM_EFFECT_CODE)
							continue;
						if (g_pCustomDropEvent->IsDebuffEffectBlocked((int)effectCode[i]))
							return false;
					}
				}
			}
		}
	}

	if (CBuffManager::RegisterBuff(rdwKeepTime, effectCode, paBuffParameter, hCaster, buffType, pSkillTbldat, prBuffIndex))
	{
		if (rdwKeepTime > 0)
		{
			CBuffBot* pBuff = new CBuffBot;

			sBUFF_INFO buffInfo;
			buffInfo.buffIndex = INVALID_BYTE;
			buffInfo.sourceTblidx = pSkillTbldat->tblidx;
			buffInfo.dwTimeRemaining = rdwKeepTime;
			buffInfo.dwInitialDuration = rdwKeepTime;
			buffInfo.bySourceType = DBO_OBJECT_SOURCE_SKILL;
			memcpy(&buffInfo.aBuffParameter[0], &paBuffParameter[0], sizeof(sDBO_BUFF_PARAMETER));
			memcpy(&buffInfo.aBuffParameter[1], &paBuffParameter[1], sizeof(sDBO_BUFF_PARAMETER));

			if (pBuff->Create(m_pBotRef, &buffInfo, effectCode, hCaster, buffType, pSkillTbldat->byBuff_Group, pSkillTbldat->bySkill_Effect_Type))
			{
				if (!AddBuff(pBuff, true))
				{
					SAFE_DELETE(pBuff);
					return false;
				}

				if (prBuffIndex)
					*prBuffIndex = pBuff->GetBuffIndex();
			}
			else
			{
				SAFE_DELETE(pBuff);
				return false;
			}
		}

		return true;
	}

	return false;
}

