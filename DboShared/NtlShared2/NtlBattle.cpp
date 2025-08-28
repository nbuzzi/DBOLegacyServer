//***********************************************************************************
//
//	File		:	NtlBattle.cpp
//
//	Begin		:	2006-04-24
//
//	Copyright	:	¨Ï NTL-Inc Co., Ltd
//
//	Author		:	Hyun Woo, Koo   ( zeroera@ntl-inc.com )
//
//	Desc		:	
//
//***********************************************************************************

#include "stdafx.h"
#include "NtlBattle.h"

#include <crtdbg.h>
#include <stdlib.h>

static bool PointInPolygonXZ(const std::vector<std::pair<float, float>>& poly, const CNtlVector& p)
{
	bool inside = false;
	const size_t n = poly.size();
	for (size_t i = 0, j = n - 1; i < n; j = i++)
	{
		const float xi = poly[i].first, zi = poly[i].second;
		const float xj = poly[j].first, zj = poly[j].second;

		const bool intersect = ((zi > p.z) != (zj > p.z)) &&
			(p.x < ((xj - xi) * (p.z - zi) / ((zj - zi) != 0.f ? (zj - zi) : 1e-6f) + xi));

		if (intersect) inside = !inside;
	}
	return inside;
}

static const std::vector<std::pair<float, float>> kTatami = {
	{4521.230f,4061.800f},
	{4517.420f,4071.060f},
	{4515.290f,4075.800f},
	{4511.670f,4084.540f},
	{4506.670f,4096.250f},
	{4506.580f,4096.480f},
	{4499.730f,4093.470f},
	{4491.690f,4090.070f},
	{4483.650f,4086.850f},
	{4475.080f,4083.040f},
	{4472.230f,4081.810f},
	{4474.810f,4075.530f},
	{4477.610f,4068.950f},
	{4481.340f,4060.220f},
	{4483.960f,4054.120f},
	{4485.750f,4049.990f},
	{4486.750f,4047.650f},
	{4486.870f,4047.360f},
	{4490.810f,4049.000f},
	{4498.020f,4052.040f},
	{4503.750f,4054.480f},
	{4509.420f,4056.910f},
	{4515.610f,4059.620f},
	{4518.460f,4060.810f},
	{4520.920f,4061.840f}
};


//-----------------------------------------------------------------------------------
// static variable
//-----------------------------------------------------------------------------------
const char * s_battle_attack_type_string[ BATTLE_ATTACK_TYPE_COUNT ] = 
{
	"BATTLE_ATTACK_TYPE_PHYSICAL",
	"BATTLE_ATTACK_TYPE_ENERGY",
};

const char * s_battle_attack_result_string[ BATTLE_ATTACK_RESULT_COUNT ] = 
{
	"BATTLE_ATTACK_RESULT_HIT", // ÀÏ¹Ý °ø°Ý
	"BATTLE_ATTACK_RESULT_CRITICAL_HIT", // Å©¸®Æ¼Ä® °ø°Ý
	"BATTLE_ATTACK_RESULT_DODGE", // È¸ÇÇ
	"BATTLE_ATTACK_RESULT_RESISTED", // ÀúÇ×
	"BATTLE_ATTACK_RESULT_BLOCK", // ºí¶ô
	"BATTLE_ATTACK_RESULT_KNOCKDOWN", // ³Ë´Ù¿î
	"BATTLE_ATTACK_RESULT_SLIDING", // ½½¶óÀÌµù
};

const char * s_freebattle_result_string[ FREEBATTLE_RESULT_COUNT ] = 
{
	"FREEBATTLE_RESULT_WIN", 
	"FREEBATTLE_RESULT_LOSE",
	"FREEBATTLE_RESULT_DRAW",
};


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
const char * NtlGetBattleAttackTypeString(BYTE byAttackType)
{
	if( byAttackType >= BATTLE_ATTACK_TYPE_COUNT )
	{
		return "NOT DEFINED";
	}

	return s_battle_attack_type_string[ byAttackType ];
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
const char * DboGetFreeBattleResultString(BYTE byBattleResult)
{
	if( byBattleResult >= FREEBATTLE_RESULT_COUNT )
	{
		return "NOT DEFINED";
	}

	return s_freebattle_result_string[ byBattleResult ];
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
const char * NtlGetBattleAttackResultString(BYTE byAttackResult)
{
	if( byAttackResult >= BATTLE_ATTACK_RESULT_COUNT )
	{
		return "NOT DEFINED";
	}

	return s_battle_attack_result_string[ byAttackResult ];
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
float NtlGetBattleChainAttackBounsRate(BYTE byAttackSequence)
{
	// NTL_BATTLE_MAX_CHAIN_ATTACK_COUNT_PLAYER is simply the max possible attack chain number (6).
	static float s_afChainAttackBonusRate[ NTL_BATTLE_MAX_CHAIN_ATTACK_COUNT_PLAYER ] = { 0.0f, 0.5f, 1.0f, 2.0f, 3.0f, 4.0f };

	if( byAttackSequence <= 0 )
	{
		return 0.0f;
	}

	if( byAttackSequence > NTL_BATTLE_MAX_CHAIN_ATTACK_COUNT_PLAYER )
	{
		_ASSERT( 0 );
		return 0.0f;
	}

	return s_afChainAttackBonusRate[ byAttackSequence - 1 ];
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
BYTE NtlGetBattleChainAttackSequence(BYTE byCharLevel)
{
	if ( byCharLevel>=20)
		return 6;
	
	if ( byCharLevel>=16)
		return 5;
	
	if ( byCharLevel>=12)
		return 4;
	
	if ( byCharLevel>=8 )
		return 3;
	
	if ( byCharLevel>=4 )
		return 2;
	
	return 1;
}


////-----------------------------------------------------------------------------------
////		Purpose	:
////		Return	:
////-----------------------------------------------------------------------------------
float NtlGetBattleAttributeBonusRate(BYTE bySubjectAtt, BYTE byTargetAtt)
{
	static float afBattleAttributeBonusRate[BATTLE_ATTRIBUTE_COUNT][BATTLE_ATTRIBUTE_COUNT] = 
	{
		// BATTLE_ATTRIBUTE_NONE
		{ 0.0f, -5.0f, -5.0f, -5.0f, -5.0f, -5.0f },

		// BATTLE_ATTRIBUTE_HONEST
		{ 5.0f, 0.0f, 5.0f, 10.0f, -10.0f, -5.0f },

		// BATTLE_ATTRIBUTE_STRANGE
		{ 5.0f, -5.0f, 0.0f, 5.0f, 10.0f, -10.0f },

		// BATTLE_ATTRIBUTE_WILD
		{ 5.0f, -10.0f, -5.0f, 0.0f, 5.0f, 10.0f },

		// BATTLE_ATTRIBUTE_ELEGANCE
		{ 5.0f, 10.0f, -10.0f, -5.0f, 0.0f, 5.0f },

		// BATTLE_ATTRIBUTE_FUNNY
		{ 5.0f, 5.0f, 10.0f, -10.0f, -5.0f, 0.0f },

	};


	if( bySubjectAtt >= BATTLE_ATTRIBUTE_COUNT || byTargetAtt >= BATTLE_ATTRIBUTE_COUNT )
	{
		_ASSERT( 0 );
		return 0.0f;
	}


	return afBattleAttributeBonusRate[ bySubjectAtt ][byTargetAtt ];
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
eSYSTEM_EFFECT_CODE GetBattleAttributeEffectCodeOfence(BYTE byAtt)
{
	static eSYSTEM_EFFECT_CODE awBattleAttribute[BATTLE_ATTRIBUTE_COUNT] =
	{
		INVALID_SYSTEM_EFFECT_CODE, ACTIVE_HONEST_OFFENCE_UP, ACTIVE_STRANGE_OFFENCE_UP, ACTIVE_WILD_OFFENCE_UP, ACTIVE_ELEGANCE_OFFENCE_UP, ACTIVE_FUNNY_OFFENCE_UP
	};

	if (byAtt >= BATTLE_ATTRIBUTE_COUNT)
	{
		return INVALID_SYSTEM_EFFECT_CODE;
	}

	return awBattleAttribute[byAtt];
}
eSYSTEM_EFFECT_CODE GetBattleAttributeEffectCodeDefence(BYTE byAtt)
{
	static eSYSTEM_EFFECT_CODE awBattleAttribute[BATTLE_ATTRIBUTE_COUNT] =
	{
		INVALID_SYSTEM_EFFECT_CODE, ACTIVE_HONEST_DEFENCE_UP, ACTIVE_STRANGE_DEFENCE_UP, ACTIVE_WILD_DEFENCE_UP, ACTIVE_ELEGANCE_DEFENCE_UP, ACTIVE_FUNNY_DEFENCE_UP
	};

	if (byAtt >= BATTLE_ATTRIBUTE_COUNT)
	{
		return INVALID_SYSTEM_EFFECT_CODE;
	}

	return awBattleAttribute[byAtt];
}
//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
BYTE GetBattleAttributeEffectApplyType(BYTE byAtt)
{
	static BYTE abyBattleAttributeApplyType[BATTLE_ATTRIBUTE_COUNT] =
	{
		SYSTEM_EFFECT_APPLY_TYPE_UNKNOWN, SYSTEM_EFFECT_APPLY_TYPE_VALUE, SYSTEM_EFFECT_APPLY_TYPE_VALUE, SYSTEM_EFFECT_APPLY_TYPE_VALUE, SYSTEM_EFFECT_APPLY_TYPE_VALUE, SYSTEM_EFFECT_APPLY_TYPE_VALUE
	};

	if (byAtt >= BATTLE_ATTRIBUTE_COUNT)
	{
		return SYSTEM_EFFECT_APPLY_TYPE_UNKNOWN;
	}

	return abyBattleAttributeApplyType[byAtt];
}

float GetBattleAttributeEffectApplyValue(BYTE byAtt)
{
	static float afBattleAttributeApplyValue[BATTLE_ATTRIBUTE_COUNT] =
	{
		0.f, 5.f, 5.f, 5.f, 5.f, 5.f
	};

	if (byAtt >= BATTLE_ATTRIBUTE_COUNT)
	{
		return 0.f;
	}

	return afBattleAttributeApplyValue[byAtt];
}


//-----------------------------------------------------------------------------------
//		Purpose	: check if player is in korin arena. This function is no longer required. Use DBO_WORLD_ATTR_BASIC_FREE_PVP_ZONE
//		Return	:
//-----------------------------------------------------------------------------------
bool IsInBattleArena(TBLIDX worldTblidx, CNtlVector& vCurLoc, bool isPowerTournament)
{
	// Korin
	if (worldTblidx == 1 && (vCurLoc.x < 5792 && vCurLoc.z < 788 && vCurLoc.x > 5752 && vCurLoc.z > 748))
		return true;

	// Arena 2 – Tatami (AABB rápido + polígono preciso)
	if (worldTblidx == 1) {
		if (vCurLoc.x > 4472.23f && vCurLoc.x < 4521.23f &&
			vCurLoc.z > 4047.36f && vCurLoc.z < 4096.48f)
		{
			if (PointInPolygonXZ(kTatami, vCurLoc))
				return true;
		}
	}

	if (worldTblidx == 510000 && isPowerTournament == false)
		return true;

	return false;
}
