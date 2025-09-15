#pragma once

#include "NtlItem.h"
#include "NtlSharedDef.h"
#include "NtlCharacterState.h"
#include "NtlAvatar.h"
#include "NtlBitFlag.h"
#include "NtlGuild.h"
#include "NtlActionPattern.h"
#include "NtlCharTitle.h"

#include "NtlNetP.h"

#define DBO_RP_CHARGE_INTERVAL_IN_MILLISECS			(Dbo_GetRpChargeIntervalInMilliSecs(CFormulaTable::m_afRate[71][4]))
#define DBO_RP_DIMIMUTION_INTERVAL_IN_MILLISECS		(Dbo_GetRpDiminutionIntervalInMilliSecs(CFormulaTable::m_afRate[72][4]))
#define DBO_BATTLE_REGEN_DURATION_IN_MILLISECS		(Dbo_GetRpDiminutionIntervalInMilliSecs(CFormulaTable::m_afRate[603][1]))

enum eRETURNMODE
{
	eNORMAL_WALK,
	eNORMAL_RUN,
	eCOMEBACK_RUN,
	eINVALID_RETURNMODE,
};


enum eRACE
{
	RACE_HUMAN,
	RACE_NAMEK,
	RACE_MAJIN,

	RACE_COUNT,
	RACE_UNKNOWN = 0xFF,

	RACE_FIRST = RACE_HUMAN,
	RACE_LAST = RACE_MAJIN,
};


enum eRACE_FLAG
{
	RACE_HUMAN_FLAG = MAKE_BIT_FLAG(RACE_HUMAN),
	RACE_NAMEK_FLAG = MAKE_BIT_FLAG(RACE_NAMEK),
	RACE_MAJIN_FLAG = MAKE_BIT_FLAG(RACE_MAJIN),
};

enum eGENDER
{
	GENDER_MALE,
	GENDER_FEMALE,
	GENDER_ONE_SEX,

	GENDER_COUNT,
	GENDER_UNKNOWN = 0xFF,

	GENDER_FIRST = GENDER_MALE,
	GENDER_LAST = GENDER_ONE_SEX,
};

enum eGENDER_FLAG
{
	GENDER_MALE_FLAG = MAKE_BIT_FLAG(GENDER_MALE),
	GENDER_FEMALE_FLAG = MAKE_BIT_FLAG(GENDER_FEMALE),
	GENDER_ONE_SEX_FLAG = MAKE_BIT_FLAG(GENDER_ONE_SEX),
	GENDER_ALL_FLAG = GENDER_MALE_FLAG + GENDER_FEMALE_FLAG + GENDER_ONE_SEX_FLAG,
};

enum eZENNY_CHANGE_TYPE
{
	ZENNY_CHANGE_TYPE_ITEM_BUY = 0,
	ZENNY_CHANGE_TYPE_ITEM_SELL,
	ZENNY_CHANGE_TYPE_PICK,
	ZENNY_CHANGE_TYPE_TRADE,
	ZENNY_CHANGE_TYPE_REPAIR,
	ZENNY_CHANGE_TYPE_REWARD,
	ZENNY_CHANGE_TYPE_SKILL,
	ZENNY_CHANGE_TYPE_PARTY_VICTIM_ADD,
	ZENNY_CHANGE_TYPE_PARTY_VICTIM_DEL,
	ZENNY_CHANGE_TYPE_PARTY_VICTIM_RETURN,
	ZENNY_CHANGE_TYPE_CHEAT,
	ZENNY_CHANGE_TYPE_BANK_BUY,
	ZENNY_CHANGE_TYPE_PARTY_PICK,
	ZENNY_CHANGE_TYPE_GUILD_CREATE,
	ZENNY_CHANGE_TYPE_DB_REWARD,
	ZENNY_CHANGE_TYPE_PRIVATESHOP_ITEM_BUY,
	ZENNY_CHANGE_TYPE_MAIL_SEND,
	ZENNY_CHANGE_TYPE_MAIL_RECV,
	ZENNY_CHANGE_TYPE_RANKBATTLE,
	ZENNY_CHANGE_TYPE_PORTAL_ADD,

	ZENNY_CHANGE_TYPE_GUILD_FUNCTION_ADD,
	ZENNY_CHANGE_TYPE_GUILD_GIVE_ZENNY,
	ZENNY_CHANGE_TYPE_GUILD_MARK_CHANGE,
	ZENNY_CHANGE_TYPE_GUILD_NAME_CHANGE,
	ZENNY_CHANGE_TYPE_RIDE_ON_BUS,
	ZENNY_CHANGE_TYPE_ITEM_IDENTIFY,
	ZENNY_CHANGE_TYPE_SCOUTER_ITEM_SELL,
	ZENNY_CHANGE_TYPE_PARTY_ITEM_INVEST,	// ��Ƽ�κ� ������ ���
	ZENNY_CHANGE_TYPE_ITEM_MIX_MAKE,		// ������ ����µ� �Ҹ�
	ZENNY_CHANGE_TYPE_ITEM_MIX_FARE,		// ������ ������ ȹ��
	ZENNY_CHANGE_TYPE_DOJO_CHANGE,
	ZENNY_CHANGE_TYPE_BANK,					// â�� �����
	ZENNY_CHANGE_TYPE_GUILD_BANK,			// ��� â�� �����
	ZENNY_CHANGE_TYPE_DOJO_SCRAMBLE_REQ,	// Dojo war contest registration fee
	ZENNY_CHANGE_TYPE_DOJO_ANTI_SCRAMBLE_ADD,	// Receive refusal to contest
	ZENNY_CHANGE_TYPE_DOJO_ANTI_SCRAMBLE_DEL,	// Rejection of the dojo war contest
	ZENNY_CHANGE_TYPE_ITEM_RECIPE_JOB_SET,  // Consumed to learn manufacturing skills

	//new
	ZENNY_CHANGE_TYPE_TMP_SELL_FEE,
	ZENNY_CHANGE_TYPE_TMP_BUY,
	ZENNY_CHANGE_TYPE_ITEM_EXCHANGE,
	ZENNY_CHANGE_TYPE_TRADE_INC,
	ZENNY_CHANGE_TYPE_TRADE_DEC,
	ZENNY_CHANGE_TYPE_CHEAT_INC,
	ZENNY_CHANGE_TYPE_CHEAT_DEC,
	ZENNY_CHANGE_TYPE_PRIVATESHOP_ITEM_SELL,
	ZENNY_CHANGE_TYPE_MAIL_RECV_ATTACH_ZENNY,
	ZENNY_CHANGE_TYPE_PARTY_ITEM_INVEST_INC,
	ZENNY_CHANGE_TYPE_PARTY_ITEM_INVEST_DEC,
	ZENNY_CHANGE_TYPE_BANK_ADD,
	ZENNY_CHANGE_TYPE_BANK_WITHRAW,
	ZENNY_CHANGE_TYPE_GUILD_BANK_ADD,
	ZENNY_CHANGE_TYPE_GUILD_BANK_WITHRAW,
	ZENNY_CHANGE_TYPE_SHOP_DURATION_ITEM_BUY,
	ZENNY_CHANGE_TYPE_SHOP_DURATION_RENEW,
	ZENNY_CHANGE_TYPE_SHOP_ZENNY_GAMBLE_BUY,
	ZENNY_CHANGE_TYPE_REMOTE_SHOP_ITEM_BUY,
	ZENNY_CHANGE_TYPE_REMOTE_SHOP_ITEM_SELL,
	ZENNY_CHANGE_TYPE_REPAIR_ONE,
	ZENNY_CHANGE_TYPE_REMOTE_SHOP_REPAIR,
	ZENNY_CHANGE_TYPE_REMOTE_SHOP_REPAIR_ONE,
	ZENNY_CHANGE_TYPE_HTB_SKILL,
	ZENNY_CHANGE_TYPE_LEARN_SKILL,
	ZENNY_CHANGE_TYPE_INIT_SKILL,
	ZENNY_CHANGE_TYPE_INIT_SKILL_PLUS,
	ZENNY_CHANGE_TYPE_REMOTE_SHOP_ITEM_IDENTIFY,
	ZENNY_CHANGE_TYPE_DOJO_FUNCTION_ADD,
	ZENNY_CHANGE_TYPE_ITEM_ATTRIBUTE_CHANGE,

	ZENNY_CHANGE_TYPE_COUNT,
	ZENNY_CHANGE_TYPE_INVALID = 0xFF,

	ZENNY_CHANGE_TYPE_FIRST = ZENNY_CHANGE_TYPE_ITEM_BUY,
	ZENNY_CHANGE_TYPE_LAST = ZENNY_CHANGE_TYPE_COUNT - 1,
};

enum ePC_CLASS
{
	PC_CLASS_HUMAN_FIGHTER,  //(������)->(������,�˼���)
	PC_CLASS_HUMAN_MYSTIC,   //(�����)->(�м���,�ͼ���)
	PC_CLASS_HUMAN_ENGINEER, //(�����Ͼ�)->(�ǸŴϾ�,��ī�޴Ͼ�)
	PC_CLASS_NAMEK_FIGHTER,  //(����)->(��������,��������)
	PC_CLASS_NAMEK_MYSTIC,   //(����)->(��������,���ڵ���)
	PC_CLASS_MIGHTY_MAJIN,   //(�븶��)->(�׷���,��Ƽ��)
	PC_CLASS_WONDER_MAJIN,   //(�Ǹ���)->(�ö��,ī����)
	PC_CLASS_STREET_FIGHTER, //(������)
	PC_CLASS_SWORD_MASTER,   //(�˼���)
	PC_CLASS_CRANE_ROSHI,    //(�м���)
	PC_CLASS_TURTLE_ROSHI,   //(�ͼ���)
	PC_CLASS_GUN_MANIA,      //(�ǸŴϾ�)
	PC_CLASS_MECH_MANIA,     //(��ī�ŴϾ�)
	PC_CLASS_DARK_WARRIOR,   //(��������)
	PC_CLASS_SHADOW_KNIGHT,  //(��������)
	PC_CLASS_DENDEN_HEALER,  //(��������)
	PC_CLASS_POCO_SUMMONER,  //(���ڵ���)
	PC_CLASS_ULTI_MA,        //(��Ƽ��)
	PC_CLASS_GRAND_MA,       //(�׷���)
	PC_CLASS_PLAS_MA,        //(�ö��)
	PC_CLASS_KAR_MA,         //(ī����)

	PC_CLASS_COUNT,
	PC_CLASS_UNKNOWN = 0xFF,

	PC_CLASS_1_FIRST = PC_CLASS_HUMAN_FIGHTER,
	PC_CLASS_1_LAST = PC_CLASS_WONDER_MAJIN,
	PC_CLASS_2_FIRST = PC_CLASS_STREET_FIGHTER,
	PC_CLASS_2_LAST = PC_CLASS_KAR_MA,

	PC_CLASS_FIRST = PC_CLASS_HUMAN_FIGHTER,
	PC_CLASS_LAST = PC_CLASS_COUNT - 1,
};

// Class �˻�� ��Ʈ �÷���
enum ePC_CLASS_FLAG
{
	PC_CLASS_FLAG_HUMAN_FIGHTER = (0x01 << PC_CLASS_HUMAN_FIGHTER),
	PC_CLASS_FLAG_HUMAN_MYSTIC = (0x01 << PC_CLASS_HUMAN_MYSTIC),
	PC_CLASS_FLAG_HUMAN_ENGINEER = (0x01 << PC_CLASS_HUMAN_ENGINEER),								//(�����Ͼ�)->(�ǸŴϾ�,��ī�޴Ͼ�)
	PC_CLASS_FLAG_NAMEK_FIGHTER = (0x01 << PC_CLASS_NAMEK_FIGHTER),								//(����)->(��������,��������)
	PC_CLASS_FLAG_NAMEK_MYSTIC = (0x01 << PC_CLASS_NAMEK_MYSTIC),									//(����)->(��������,���ڵ���)
	PC_CLASS_FLAG_MIGHTY_MAJIN = (0x01 << PC_CLASS_MIGHTY_MAJIN),									//(�븶��)->(�׷���,��Ƽ��)
	PC_CLASS_FLAG_WONDER_MAJIN = (0x01 << PC_CLASS_WONDER_MAJIN),									//(�Ǹ���)->(�ö��,ī����)
	PC_CLASS_FLAG_STREET_FIGHTER = (0x01 << PC_CLASS_STREET_FIGHTER) | PC_CLASS_FLAG_HUMAN_FIGHTER,	//(������)
	PC_CLASS_FLAG_SWORD_MASTER = (0x01 << PC_CLASS_SWORD_MASTER) | PC_CLASS_FLAG_HUMAN_FIGHTER,	//(�˼���)
	PC_CLASS_FLAG_CRANE_ROSHI = (0x01 << PC_CLASS_CRANE_ROSHI) | PC_CLASS_FLAG_HUMAN_MYSTIC,	//(�м���)
	PC_CLASS_FLAG_TURTLE_ROSHI = (0x01 << PC_CLASS_TURTLE_ROSHI) | PC_CLASS_FLAG_HUMAN_MYSTIC,	//(�źϼ���)
	PC_CLASS_FLAG_GUN_MANIA = (0x01 << PC_CLASS_GUN_MANIA) | PC_CLASS_FLAG_HUMAN_ENGINEER,//(�ǸŴϾ�)
	PC_CLASS_FLAG_MECH_MANIA = (0x01 << PC_CLASS_MECH_MANIA) | PC_CLASS_FLAG_HUMAN_ENGINEER,//(��ī�ŴϾ�)
	PC_CLASS_FLAG_DARK_WARRIOR = (0x01 << PC_CLASS_DARK_WARRIOR) | PC_CLASS_FLAG_NAMEK_FIGHTER,	//(��������)
	PC_CLASS_FLAG_SHADOW_KNIGHT = (0x01 << PC_CLASS_SHADOW_KNIGHT) | PC_CLASS_FLAG_NAMEK_FIGHTER,	//(��������)
	PC_CLASS_FLAG_DENDEN_HEALER = (0x01 << PC_CLASS_DENDEN_HEALER) | PC_CLASS_FLAG_NAMEK_MYSTIC,	//(��������)
	PC_CLASS_FLAG_POCO_SUMMONER = (0x01 << PC_CLASS_POCO_SUMMONER) | PC_CLASS_FLAG_NAMEK_MYSTIC,	//(���ڵ���)
	PC_CLASS_FLAG_GRAND_MA = (0x01 << PC_CLASS_GRAND_MA) | PC_CLASS_FLAG_MIGHTY_MAJIN,	//(�׷���)
	PC_CLASS_FLAG_ULTI_MA = (0x01 << PC_CLASS_ULTI_MA) | PC_CLASS_FLAG_MIGHTY_MAJIN,	//(��Ƽ��)
	PC_CLASS_FLAG_PLAS_MA = (0x01 << PC_CLASS_PLAS_MA) | PC_CLASS_FLAG_WONDER_MAJIN,	//(�ö��)
	PC_CLASS_FLAG_KAR_MA = (0x01 << PC_CLASS_KAR_MA) | PC_CLASS_FLAG_WONDER_MAJIN,	//(ī����)
};


// Spawn character movement type
enum eSPAWN_MOVE_TYPE
{
	SPAWN_MOVE_FIX, // fix
	SPAWN_MOVE_WANDER, // Simple prowl
	SPAWN_MOVE_PATROL, // Repeat along the path to the top
	SPAWN_MOVE_FOLLOW, // Target Follow
	SPAWN_MOVE_PATH_ONCE, // Follow the path once

	SPAWN_MOVE_COUNT,

	SPAWN_MOVE_UNKNOWN = 0xFF,
	SPAWN_MOVE_FIRST = SPAWN_MOVE_FIX,
	SPAWN_MOVE_LAST = SPAWN_MOVE_PATH_ONCE,
};


// It can spawn flag
enum eSPAWN_FUNC_FLAG
{
	SPAWN_FUNC_FLAG_RESPAWN = 0x01, //Respawn
	SPAWN_FUNC_FLAG_NO_SPAWN_WAIT = 0x02, // No Spawn Wait Time
	SPAWN_FUNC_FLAG_INCORRECT_HEIGHT = 0x04, // The exact value of the height adjustment needed
};


// Remove spawn type
enum eSPAWN_REMOVE_TYPE
{
	SPAWN_REMOVE_TYPE_CLEAR, // Just delete
	SPAWN_REMOVE_TYPE_DESPAWN, // despawn and delete
	SPAWN_REMOVE_TYPE_FAINT, // Deleted after death

	INVALID_SPAWN_REMOVE_TYPE = 0xFF,
};


// �� ���
enum eMOB_GRADE
{
	MOB_GRADE_NORMAL,// (�Ϲ�)
	MOB_GRADE_SUPER, // (����)
	MOB_GRADE_ULTRA, // (��Ʈ��)
	MOB_GRADE_BOSS,  // (����)
	MOB_GRADE_HERO,  // (�����)

	MOB_GRADE_UNKNOWN = 0xFF,

	MOB_GRADE_FIRST = MOB_GRADE_NORMAL,
	MOB_GRADE_LAST = MOB_GRADE_HERO,

	MOB_GRADE_MAX_COUNT,
};


// �� Ÿ��
enum eMOB_TYPE
{
	MOB_TYPE_ANIMAL,
	MOB_TYPE_HUMAN_ANIMAL,
	MOB_TYPE_DINOSAUR,
	MOB_TYPE_ALIEN,
	MOB_TYPE_ANDROID,
	MOB_TYPE_ROBOT,
	MOB_TYPE_DRAGON,
	MOB_TYPE_DEVIL,
	MOB_TYPE_UNDEAD,
	MOB_TYPE_PLANT,
	MOB_TYPE_INSECT,
	MOB_TYPE_HUMAN,
	MOB_TYPE_NAMEC,
	MOB_TYPE_MAJIN,
	MOB_TYPE_BUILDING,
	MOB_TYPE_ITEM_BOX,

	MOB_TYPE_COUNT,
	MOB_TYPE_UNKNOWN = 0xFF,

	MOB_TYPE_FIRST = MOB_TYPE_ANIMAL,
	MOB_TYPE_LAST = MOB_TYPE_COUNT - 1,
};

enum eUPDATE_AGGRO_TYPE
{
	UPDATE_AGGRO_NORMAL_ATTACK,
	UPDATE_AGGRO_DIRECT_DAMAGE,
	UPDATE_AGGRO_DIRECT_LP_HEAL,
	UPDATE_AGGRO_DIRECT_EP_HEAL,
	UPDATE_AGGRO_DAMAGE_OVER_TIME,
	UPDATE_AGGRO_LP_HEAL_OVER_TIME,
	UPDATE_AGGRO_EP_HEAL_OVER_TIME,
	UPDATE_AGGRO_OTHER,
};

struct sCHAR_AGGRO_INFO
{
	HOBJECT	handle;
	DWORD	dwAggoPoint;
};

// Avatar Type
//
// Summon Pet�� Item Pet�� Ŭ���̾�Ʈ���� �����ϱ� ������ ���� avatar�� �����Ѵ�.
// Summon Pet and Item Pet are regarded as 'avatar' because they are controlled by Client.
// by YOSHIKI(2006-12-26)
enum eDBO_AVATAR_TYPE
{
	DBO_AVATAR_TYPE_AVATAR = 0,
	DBO_AVATAR_TYPE_SUMMON_PET_1,
	DBO_AVATAR_TYPE_ITEM_PET_1,

	DBO_AVATAR_TYPE_COUNT,

	DBO_AVATAR_TYPE_INVALID = 0xFF,

	DBO_AVATAR_TYPE_PET_FIRST = DBO_AVATAR_TYPE_SUMMON_PET_1,
	DBO_AVATAR_TYPE_PET_LAST = DBO_AVATAR_TYPE_ITEM_PET_1,

	DBO_AVATAR_TYPE_SUMMON_PET_FIRST = DBO_AVATAR_TYPE_SUMMON_PET_1,
	DBO_AVATAR_TYPE_SUMMON_PET_LAST = DBO_AVATAR_TYPE_SUMMON_PET_1,

	DBO_AVATAR_TYPE_ITEM_PET_FIRST = DBO_AVATAR_TYPE_ITEM_PET_1,
	DBO_AVATAR_TYPE_ITEM_PET_LAST = DBO_AVATAR_TYPE_ITEM_PET_1,

	DBO_AVATAR_TYPE_COUNT_SUMMON_PET = DBO_AVATAR_TYPE_SUMMON_PET_LAST - DBO_AVATAR_TYPE_SUMMON_PET_FIRST + 1,
	DBO_AVATAR_TYPE_COUNT_ITEM_PET = DBO_AVATAR_TYPE_ITEM_PET_LAST - DBO_AVATAR_TYPE_ITEM_PET_FIRST + 1,
};


enum eAI_FUNCTION
{
	AI_FUNC_DEFENSIVE = 0,			//�İ�
	AI_FUNC_TIMID,					//coward
	AI_FUNC_OFFENSIVE,				//aggresive
	AI_FUNC_SMART_OFFENSIVE,		//smart aggresive
	AI_FUNC_ONLYSKILL,				//Only use skills

	AI_FUNC_DESPERATION = 5,		//Transcription
	AI_FUNC_FLEE,					//Escape
	AI_FUNC_RETREAT,				//Retreat

	AI_FUNC_REVENGE = 9,			//Revenge
	AI_FUNC_FOCUS,					//Concentration
	AI_FUNC_MEAN,					//Specific heat
	AI_FUNC_BRAVE,					//Bravery

	AI_FUNC_ALLIANCE_HELP = 15,
	AI_FUNC_DEFEND,

	AI_FUNC_NOT_CHASE = 20,
	AI_FUNC_NOT_MOVE,

	AI_FUNC_COUNT,

	AI_FUNC_FIRST = AI_FUNC_DEFENSIVE,
	AI_FUNC_LAST = AI_FUNC_ALLIANCE_HELP,
};


enum eAI_FUNCTION_FLAG
{
	AI_FUNC_FLAG_DEFENSIVE = 0x01 << AI_FUNC_DEFENSIVE,
	AI_FUNC_FLAG_TIMID = 0x01 << AI_FUNC_TIMID,// [3/12/2008]
	AI_FUNC_FLAG_OFFENSIVE = 0x01 << AI_FUNC_OFFENSIVE,
	AI_FUNC_FLAG_SMART_OFFENSIVE = 0x01 << AI_FUNC_SMART_OFFENSIVE, // [3/11/2008]
	AI_FUNC_FLAG_ONLYSKILL = 0x01 << AI_FUNC_ONLYSKILL, // [11/10/2008]

	AI_FUNC_FLAG_DESPERATION = 0x01 << AI_FUNC_DESPERATION,
	AI_FUNC_FLAG_FLEE = 0x01 << AI_FUNC_FLEE,
	AI_FUNC_FLAG_RETREAT = 0x01 << AI_FUNC_RETREAT,// [3/12/2008]

	AI_FUNC_FLAG_REVENGE = 0x01 << AI_FUNC_REVENGE,
	AI_FUNC_FLAG_FOCUS = 0x01 << AI_FUNC_FOCUS,
	AI_FUNC_FLAG_MEAN = 0x01 << AI_FUNC_MEAN,
	AI_FUNC_FLAG_BRAVE = 0x01 << AI_FUNC_BRAVE,

	AI_FUNC_FLAG_ALLIANCE_HELP = 0x01 << AI_FUNC_ALLIANCE_HELP,// [3/11/2008]
	AI_FUNC_FLAG_DEFEND = 0x01 << AI_FUNC_DEFEND,// ��� [3/12/2008]

	AI_FUNC_FLAG_NOT_CHASE = 0x01 << AI_FUNC_NOT_CHASE, // [11/10/2008]
	AI_FUNC_FLAG_NOT_MOVE = 0x01 << AI_FUNC_NOT_MOVE, // [11/10/2008]
};


// NPC JOB
enum eNPC_JOB
{
	NPC_JOB_WEAPON_MERCHANT,				// Sell weapon for zeni
	NPC_JOB_ARMOR_MERCHANT,					// Sell armor for zeni
	NPC_JOB_GOODS_MERCHANT,					// Sell pots etc for zeni
	NPC_JOB_SCOUTER_MERCHANT,				// Sell scouter for zeni
	NPC_JOB_GUARD,							// Does not do shit
	NPC_JOB_SKILL_TRAINER_HFI,				// skill trainer
	NPC_JOB_SKILL_TRAINER_HMY,				// skill trainer
	NPC_JOB_SKILL_TRAINER_HEN,				// skill trainer
	NPC_JOB_SKILL_TRAINER_NFI,				// skill trainer
	NPC_JOB_SKILL_TRAINER_NMY,				// skill trainer
	NPC_JOB_SKILL_TRAINER_MMI,				// skill trainer
	NPC_JOB_SKILL_TRAINER_MWO,				// skill trainer
	NPC_JOB_BANKER,							// banker
	NPC_JOB_TALKER,							// does not do shit like guard
	NPC_JOB_GUILD_MANAGER,					// guild manager
	NPC_JOB_SUMMON_PET,						// summoned pets
	NPC_JOB_DOGI_MERCHANT,					// sell dogi for zeni
	NPC_JOB_SPECIAL_WEAPON_MERCHANT,		// sell high level weapon for zeni
	NPC_JOB_SPECIAL_ARMOR_MERCHANT,			// sell high level armor for zeni
	NPC_JOB_SPECIAL_GOODS_MERCHANT,			// sell necklace, pots, etc for zeni
	NPC_JOB_SPECIAL_FOODS_MERCHANT,			// sell pots for zeni
	NPC_JOB_SPECIAL_SCOUTER_MERCHANT,		// sell scouter for zeni
	NPC_JOB_GRAND_SKILL_TRAINER_HFI,		// skill trainer
	NPC_JOB_GRAND_SKILL_TRAINER_HMY,		// skill trainer
	NPC_JOB_GRAND_SKILL_TRAINER_HEN,		// skill trainer
	NPC_JOB_GRAND_SKILL_TRAINER_NFI,		// skill trainer
	NPC_JOB_GRAND_SKILL_TRAINER_NMY,		// skill trainer
	NPC_JOB_GRAND_SKILL_TRAINER_MMI,		// skill trainer
	NPC_JOB_GRAND_SKILL_TRAINER_MWO,		// skill trainer
	NPC_JOB_SUB_WEAPON_MERCHANT,			// sell sub weapon for zeni
	NPC_JOB_GATE_KEEPER,					// gate keeper / teleporter
	NPC_JOB_VENDING_MACHINE,				// sell items for zeni. Also used for seal bind items.
	NPC_JOB_TIMEMACHINE_MERCHANT,			// TMQ teleporter 
	NPC_JOB_PORTAL_MAN,						// ���� �̵� ���񽺸�			
	NPC_JOB_BUS,							// ����
	NPC_JOB_RECEPTION,						// 
	NPC_JOB_BUDOHSI_MERCHANT,				// npc exchange items for item and/or mudosa points
	NPC_JOB_REFEREE,						// ����
	NPC_JOB_GAMBLE_MERCHANT,				// mudosa gambler npc
	NPC_JOB_CHAMPION_MERCHANT,				// unknown
	NPC_JOB_DOJO_MANAGER,					// ���� ������
	NPC_JOB_DOJO_MERCHANT,					// sell shit for zeni
	NPC_JOB_DOJO_SEAL,						// ���� ����
	NPC_JOB_DOJO_BANKER,					// ���� â��
	NPC_JOB_MIX_MASTER,						// sell items for zeni

	//new
	NPC_JOB_COMIC_NPC,
	NPC_JOB_QUEST_MERCHANT,					// sell quest items for zeni
	NPC_JOB_MASCOT_MERCHANT,				// sell mascot items for zeni
	NPC_JOB_EVENT_NPC,						// event manager npc
	NPC_JOB_DWC_TELEPORT,
	NPC_JOB_MASCOT_GAMBLE_MERCHANT,			//mascot gamble npc
	NPC_JOB_BATTLE_DUNGEON_MANAGER,			//ccbd entrance npc
	NPC_JOB_ITEM_UPGRADE,					//upgrade merchant. DOES NOT SELL ITEMS. ONLY POSSIBLE TO UPGRADE
	NPC_JOB_VEHICLE_MERCHANT,
	NPC_JOB_ALIEN_ENGINEER,
	NPC_JOB_MASCOTEX_MERCHANT,
	NPC_JOB_MASCOT_GAMBLE_MERCHANT_2,	//mascot gamble npc.. get random mascot egg for zeni
	NPC_JOB_MIX_MASTER2,
	NPC_JOB_SCOUTER_MERCHANT2,				//sell scouter for zeni
	NPC_JOB_SPECIAL_SCOUTER_MERCHANT2,
	NPC_JOB_BUDOHSI_MERCHANT2,
	NPC_JOB_BUDOHSI_MERCHANT3,			//sell scouter for mudosa points
	NPC_JOB_AIR_GAMBLE_MERCHANT,		//air capsule machine gamble using mudosa points

	NPC_JOB_COUNT,
	NPC_JOB_UNKNOWN = 0xFF,

	NPC_JOB_FIRST = NPC_JOB_WEAPON_MERCHANT,
	NPC_JOB_LAST = NPC_JOB_COUNT - 1
};


// NPC Function Flag
enum eNPC_FUNCTION
{
	NPC_FUNC_MERCHANT,			// ���α��
	NPC_FUNC_GUARD,				// Ultimate Dungeon (UD) NPC
	NPC_FUNC_SKILL_TRAINER,		// �������
	NPC_FUNC_BANKER,			// ��������
	NPC_FUNC_TALKER,			// �̾߱���
	NPC_FUNC_QUEST_GRANTER,		// ����Ʈ �ο�
	NPC_FUNC_GUILD_MANAGER,		// ���Ŵ���
	NPC_FUNC_SUMMON_PET,		// ��ȯ��
	NPC_FUNC_GATE_KEEPER,		// ������
	NPC_FUNC_TIME_QUEST,		// Ÿ�Ӹӽ� ����Ʈ ���
	NPC_FUNC_PORTAL,			// Teleport Portal
	NPC_FUNC_SCAN_BY_MOB,		// ������ ������ ���Ҽ� ����
	NPC_FUNC_BUS,				// ����
	NPC_FUNC_RECEPTION,			// õ������ ����ȸ ������
	NPC_FUNC_BUDOHSI_MERCHANT,	// ������ ����
	NPC_FUNC_REFEREE,			// ����
	NPC_FUNC_BUILDING,			// ������
	NPC_FUNC_FACING,			// ��� : ��Ʈ�� BOT�� Ŭ���ϸ� ���߰��ϴ� ���
	NPC_FUNC_TURN_OFF,			// ������ ���ص� �� ������ �Ⱥ���
	NPC_FUNC_DISCLOSE_LP,		// LPǥ�ÿ���
	NPC_FUNC_GAMBLE_MERCHANT,	// �̱� ����
	NPC_FUNC_MOVING_NPC,		// �����̴� NPC (�о� ������)
	NPC_FUNC_SPAWN_NPC,			//�߰� ���� NPC
	NPC_FUNC_DOJO_MANAGER,		// ���� ������
	NPC_FUNC_DOJO_MERCHANT,		// ���� ����
	NPC_FUNC_DOJO_SEAL,			// ���� ����
	NPC_FUNC_DOJO_BANKER,		// ���� â��

	//new
	NPC_FUNC_DWC_TELEPORT,
	NPC_FUNC_BATTLE_DUNGEON_MANAGER,
	NPC_FUNC_ITEM_UPGRADE,
	NPC_FUNC_EVENT_NPC,
	NPC_FUNC_ITEM_EXCHANGE,

	NPC_FUNC_COUNT,
	NPC_FUNC_FIRST = NPC_FUNC_MERCHANT,
	NPC_FUNC_LAST = NPC_FUNC_COUNT - 1,
};


// NPC Function Flag
enum eNPC_FUNCTION_FLAG
{
	NPC_FUNC_FLAG_MERCHANT = MAKE_BIT_FLAG(NPC_FUNC_MERCHANT),
	NPC_FUNC_FLAG_GUARD = MAKE_BIT_FLAG(NPC_FUNC_GUARD),
	NPC_FUNC_FLAG_SKILL_TRAINER = MAKE_BIT_FLAG(NPC_FUNC_SKILL_TRAINER),
	NPC_FUNC_FLAG_BANKER = MAKE_BIT_FLAG(NPC_FUNC_BANKER),
	NPC_FUNC_FLAG_TALKER = MAKE_BIT_FLAG(NPC_FUNC_TALKER),
	NPC_FUNC_FLAG_QUEST_GRANTER = MAKE_BIT_FLAG(NPC_FUNC_QUEST_GRANTER),
	NPC_FUNC_FLAG_GUILD_MANAGER = MAKE_BIT_FLAG(NPC_FUNC_GUILD_MANAGER),
	NPC_FUNC_FLAG_SUMMON_PET = MAKE_BIT_FLAG(NPC_FUNC_SUMMON_PET),
	NPC_FUNC_FLAG_GATE_KEEPER = MAKE_BIT_FLAG(NPC_FUNC_GATE_KEEPER),
	NPC_FUNC_FLAG_TIME_QUEST = MAKE_BIT_FLAG(NPC_FUNC_TIME_QUEST),
	NPC_FUNC_FLAG_PORTAL = MAKE_BIT_FLAG(NPC_FUNC_PORTAL),
	NPC_FUNC_FLAG_SCAN_BY_MOB = MAKE_BIT_FLAG(NPC_FUNC_SCAN_BY_MOB),
	NPC_FUNC_FLAG_BUS = MAKE_BIT_FLAG(NPC_FUNC_BUS),
	NPC_FUNC_FLAG_RECEPTION = MAKE_BIT_FLAG(NPC_FUNC_RECEPTION),
	NPC_FUNC_FLAG_BUDOHSI_MERCHANT = MAKE_BIT_FLAG(NPC_FUNC_BUDOHSI_MERCHANT),
	NPC_FUNC_FLAG_REFEREE = MAKE_BIT_FLAG(NPC_FUNC_REFEREE),
	NPC_FUNC_FLAG_BUILDING = MAKE_BIT_FLAG(NPC_FUNC_BUILDING),
	NPC_FUNC_FLAG_FACING = MAKE_BIT_FLAG(NPC_FUNC_FACING),	// ���� ( NPC_FUNC_FLAG_FACING = NPC_FUNC_FLAG_MERCHANT | NPC_FUNC_FLAG_QUEST_GRANTER, // �̵��߿� �޴��� ���� �Ǵ� NPC ���� ) [5/19/2008]	
																		// �̷����ϸ� �����Ǵ� ���� �ϳ� �ִµ�, ���� NPC�� �ٸ� ������ ���� �� �װ������� ���� �ɸ� ������ ���ƾ� �ɶ����� NPC�� ���� ������ �Ѵ�
																		NPC_FUNC_FLAG_TURN_OFF = MAKE_BIT_FLAG(NPC_FUNC_TURN_OFF), // [6/2/2008]
																		NPC_FUNC_FLAG_DISCLOSE_LP = MAKE_BIT_FLAG(NPC_FUNC_DISCLOSE_LP), // [8/21/2008 Peessi]
																		NPC_FUNC_FLAG_GAMBLE_MERCHANT = MAKE_BIT_FLAG(NPC_FUNC_GAMBLE_MERCHANT),// [7/21/2008]
																		NPC_FUNC_FLAG_MOVING_NPC = MAKE_BIT_FLAG(NPC_FUNC_MOVING_NPC), // [8/27/2008]
																		NPC_FUNC_FLAG_SPAWN_NPC = MAKE_BIT_FLAG(NPC_FUNC_SPAWN_NPC),
																		NPC_FUNC_FLAG_DOJO_MANAGER = MAKE_BIT_FLAG(NPC_FUNC_DOJO_MANAGER),
																		NPC_FUNC_FLAG_DOJO_MERCHANT = MAKE_BIT_FLAG(NPC_FUNC_DOJO_MERCHANT),
																		NPC_FUNC_FLAG_DOJO_SEAL = MAKE_BIT_FLAG(NPC_FUNC_DOJO_SEAL),
																		NPC_FUNC_FLAG_DOJO_BANKER = MAKE_BIT_FLAG(NPC_FUNC_DOJO_BANKER),

																		NPC_FUNC_FLAG_DWC_TELEPORT = MAKE_BIT_FLAG(NPC_FUNC_DWC_TELEPORT),
																		NPC_FUNC_FLAG_BATTLE_DUNGEON_MANAGER = MAKE_BIT_FLAG(NPC_FUNC_BATTLE_DUNGEON_MANAGER),
																		NPC_FUNC_FLAG_ITEM_UPGRADE = MAKE_BIT_FLAG(NPC_FUNC_ITEM_UPGRADE),
																		NPC_FUNC_FLAG_EVENT_NPC = MAKE_BIT_FLAG64(NPC_FUNC_EVENT_NPC),
																		NPC_FUNC_FLAG_ITEM_EXCHANGE = MAKE_BIT_FLAG64(NPC_FUNC_ITEM_EXCHANGE),
};

enum eNPC_SHOP_TYPE
{
	NPC_SHOP_TYPE_DEFAULT,
	NPC_SHOP_TYPE_EXCHANGE = 9,
	NPC_SHOP_TYPE_TENKAICHI, // like exchange

	NPC_SHOP_TYPE_COUNT,
	NPC_SHOP_TYPE_INVALID = 0xFF,
};

enum eNPC_ATTRIBUTE
{
	NPC_ATTRIBUTE_DEFY = 1,
	NPC_ATTRIBUTE_NOT_PUSH,

	NPC_ATTRIBUTE_COUNT,

	NPC_ATTRIBUTE_FIRST = NPC_ATTRIBUTE_DEFY,
	NPC_ATTRIBUTE_LAST = NPC_ATTRIBUTE_COUNT - 1,
};

enum eNPC_ATTRIBUTE_FLAG
{
	NPC_ATTRIBUTE_FLAG_DEFY = MAKE_BIT_FLAG(NPC_ATTRIBUTE_DEFY),
	NPC_ATTRIBUTE_FLAG_NOT_PUSH = MAKE_BIT_FLAG(NPC_ATTRIBUTE_NOT_PUSH),
};


// MERCHANT SELL TYPE
enum eMERCHANT_SELL_TYPE
{
	MERCHANT_SELL_TYPE_ITEM,
	MERCHANT_SELL_TYPE_SKILL,
	MERCHANT_SELL_TYPE_HTB_SKILL,
	MERCHANT_SELL_TYPE_BANK,
	MERCHANT_SELL_TYPE_TIMEMACHINE,
	MERCHANT_SELL_TYPE_BUDOKAI,
	MERCHANT_SELL_TYPE_GAMBLE,
	MERCHANT_SELL_TYPE_NETPY,
	//new
	MERCHANT_SELL_TYPE_GAMBLE_ZENNY,
	MERCHANT_SELL_TYPE_ITEM_EXCHANGE,

	MERCHANT_SELL_TYPE_COUNT,
	MERCHANT_SELL_TYPE_FIRST = MERCHANT_SELL_TYPE_ITEM,
	MERCHANT_SELL_TYPE_LAST = MERCHANT_SELL_TYPE_NETPY,
};

// QuickSlotType Define
enum eQUICK_SLOT_TYPE
{
	QUICK_SLOT_TYPE_ITEM,
	QUICK_SLOT_TYPE_SKILL,
	QUICK_SLOT_TYPE_HTB_SKILL,
	QUICK_SLOT_TYPE_SOCIALACTION,

	QUICK_SLOT_TYPE_COUNT,
	QUICK_SLOT_TYPE_FIRST = QUICK_SLOT_TYPE_ITEM,
	QUICK_SLOT_TYPE_LAST = QUICK_SLOT_TYPE_SOCIALACTION,
};

enum eLocalizeType : BYTE {
	//LOCALIZE_TYPE_NONE = 0x0,
	LOCALIZE_TYPE_DEV = 0x00,
	LOCALIZE_TYPE_KOREA = 0x01,
	LOCALIZE_TYPE_JAPAN = 0x02,
	LOCALIZE_TYPE_TAIWAN = 0x03,
	LOCALIZE_TYPE_HONGKONG = 0x04,
	LOCALIZE_TYPE_CHINA = 0x05,

	LOCALIZE_TYPE_TEST = 0x0A,

	LOCALIZE_TYPE_COUNT,

	LOCALIZE_TYPE_FIRST = LOCALIZE_TYPE_KOREA,
	LOCALIZE_TYPE_LAST = LOCALIZE_TYPE_COUNT - 1,
	INVALID_LOCALIZE_TYPE = 0xFF,
};

//-----------------------------------------------------------------------------------
// ĳ���� ���� ��� ���� : [4/25/2006 zeroera] : �����ʿ� : lua�� �ű� ��
//-----------------------------------------------------------------------------------
const DWORD			NTL_CHAR_RP_REGEN_WAIT_TIME = 15000; // Start decreasing after 15 seconds

const DWORD			NTL_CHAR_RP_BALL_UPDATE_INTERVAL = 30000; // ��� �� RP ���� ���� ���� (30)

const BYTE			NTL_CHAR_RP_BALL_MAX = 7; // Max 7 rp balls

const int			NTL_CHAR_RP_BALL_INCREASE_LEVEL = 10; // RP ball increases every 10 level
const int			NTL_CHAR_BP_INCREASE_CHAR_LEVEL_UP = 300; //new
const int			NTL_CHAR_BP_INCREASE_ARMOR_LEVEL_UP = 200;//new
const DWORD			NTL_CHAR_BP_RESCUE_RECOVER_RATE_OF_BP = INVALID_DWORD;
const DWORD			NTL_CHAR_BP_YOU_KILL_RECOVER_RATE_OF_BP = INVALID_DWORD;

const int			DBO_CHAR_DEFAULT_AP = 450000;

const int			DBO_CHAR_FACE_SHAPE_COUNT = 10;

const int			DBO_CHAR_HAIR_SHAPE_COUNT = 10;

const int			DBO_CHAR_HAIR_COLOR_COUNT = 13;

const int			DBO_CHAR_SKIN_COLOR_COUNT = 5;

const int			NTL_CHAR_CONVERT1_NEED_LEVEL = 30;	// 1Required level in order ex

const int           NTL_CHAR_QUICK_SLOT_MAX_COUNT = 48;   // 12 * 4 ĭ

const int           NTL_GM_USE_LEVEL_NONE = 0;

const int           NTL_GM_USE_LEVEL1 = 1;

const int           NTL_GM_USE_LEVEL2 = 2;

extern const char* DBO_PURE_MAJIN_MODEL_RESOURCE_NAME;

const float			DBO_GREAT_NAMEK_ATTACK_RANGE = 4.0f;

const float			DBO_DEFAULT_SKILL_ANIMATION_SPEED_MODIFIER = 100.0f;

const BYTE			DBO_DEFAULT_CHARACTER_SIZE_RATE = 10;

const DWORD			DBO_NPC_MAX_PARTY_NUMBER = 100000;

const DWORD			DBO_MAX_PLAY_TIME_TERM = 300000;	// Real-time position and play time up to save cycle time to save time

const DWORD			NTL_CHAR_MAX_SAVE_ZENNY = 4000000000;

const DWORD			NTL_CHAR_MAX_BANK_SAVE_ZENNY = 2000000000;

const DWORD			NTL_MAX_USE_ZENI = 1000000000; //max amount of zeni player can use to trade/sell etc

const int			NTL_MAX_NEWBIE_QUICKSLOT_COUNT = 5;  // ������ �ο����� ������ 

const DWORD			NTL_DELETE_CHAR_CHECK_TICK = 1000; //Check Delete waiting time

const DWORD			NTL_MAX_WAGU_WAGU_SHOPPOINTS = 1000000000; //maximal wagu shop points

const DWORD			NTL_MAX_NETPY_SHOPPOINTS = 50000; //maximal NetPy shop points

const DWORD			NTL_INVINCIBLE_EVENT_TIME = 5000;

//-----------------------------------------------------------------------------------
const BYTE			GMT_MAX_EFFECT_LIST_SIZE = 5;		// GMT ���� ȿ���� �ִ� ����
const BYTE			GMT_MAX_TIME_SLOT_SIZE = 3;		// GMT ȿ���� �ð� ���� ����

//new
const int 			NTL_MAX_CHAR_HISTORY_DATA_REQUEST = 20;
const DWORD 		NTL_ACHIEVEMENTS_BINARY_BLOCK_MAX_ACHIVEMENS_COUNT = 1600;

const DWORD			CHAR_POINT_MAX_NUM = 255;

const DWORD			ZENI_GAMBLE_FEE = 10000; //ZENI FEE TO USE GAMBLE NPC

#pragma pack(1)


// NewbieQuickData
struct sNEWBIE_QUICKSLOT_DATA
{
	TBLIDX			tbilidx;
	BYTE			byType;
	BYTE			byQuickSlot;
};

struct sDELETE_WAIT_CHARACTER
{
	CHARACTERID		charId;
	DWORD			dwRemainTick;
};

struct sDELETE_WAIT_CHARACTER_INFO
{
	CHARACTERID		charId;
	DWORD			dwRemainTick;// Elapsed ticks
};

struct sMARKING
{
	unsigned long int byCode;			// eMARKING_TYPE (BYTE)

};//end of sMARKING

// ĳ���� �����
struct sPC_SHAPE
{
	BYTE			byFace;
	BYTE			byHair;
	BYTE			byHairColor;
	BYTE			bySkinColor;
};

struct sASPECTSTATE_BASE
{
	BYTE							byAspectStateId;
};

union sASPECTSTATE_DETAIL
{
	sASPECTSTATE_SUPER_SAIYAN		sSuperSaiyan;
	sASPECTSTATE_PURE_MAJIN			sPureMajin;
	sASPECTSTATE_GREAT_NAMEK		sGreatNamek;
	sASPECTSTATE_KAIOKEN			sKaioken;
	sASPECTSTATE_SPINNING_ATTACK	sSpinningAttack;
	sASPECTSTATE_VEHICLE			sVehicle;
	sASPECTSTATE_ROLLING_ATTACK		sRollingAttack;

};

struct sASPECTSTATE
{
	sASPECTSTATE_BASE				sAspectStateBase;
	sASPECTSTATE_DETAIL				sAspectStateDetail;
};

// Character state
struct sCHARSTATE_BASE
{
	BYTE			byStateID;
	DWORD			dwStateTime;
	QWORD			dwConditionFlag;
	sASPECTSTATE	aspectState;

	BOOLEAN			bFightMode;	

	sVECTOR3		vCurLoc;
	sVECTOR3		vCurDir;
	BYTE			eAirState; //eAIR_STATE
};


// Structure according to state
union sCHARSTATE_DETAIL
{
	sCHARSTATE_SPAWNING				sCharStateSpawning;
	sCHARSTATE_DESPAWNING			sCharStateDespawning;
	sCHARSTATE_STANDING				sCharStateStanding;
	sCHARSTATE_SITTING				sCharStateSitting;
	sCHARSTATE_FAINTING				sCharStateFainting;
	sCHARSTATE_CAMPING				sCharStateCamping;
	sCHARSTATE_LEAVING				sCharStateLeaving;

	sCHARSTATE_MOVING				sCharStateMoving;
	sCHARSTATE_DESTMOVE				sCharStateDestMove;
	sCHARSTATE_FOLLOWING			sCharStateFollwing;
	sCHARSTATE_FALLING				sCharStateFalling;
	sCHARSTATE_DASH_PASSIVE			sCharStateDashPassive;
	sCHARSTATE_TELEPORTING			sCharStateTeleporting;
	sCHARSTATE_SLIDING				sCharStateSliding;
	sCHARSTATE_KNOCKDOWN			sCharStateKnockdown;

	sCHARSTATE_FOCUSING				sCharStateFocusing;
	sCHARSTATE_CASTING				sCharStateCasting;
	sCHARSTATE_SKILL_AFFECTING		sCharStateSkillAffecting;
	sCHARSTATE_KEEPING_EFFECT		sCharStateKeepingEffect;
	sCHARSTATE_CASTING_ITEM			sCharStateCastingItem;

	sCHARSTATE_STUNNED				sCharStateStunned;
	sCHARSTATE_SLEEPING				sCharStateSleeping;
	sCHARSTATE_PARALYZED			sCharStateParalyzed;

	sCHARSTATE_HTB					sCharStateHTB;
	sCHARSTATE_SANDBAG				sCharStateSandBag;
	sCHARSTATE_CHARGING				sCharStateCharging;

	sCHARSTATE_GUARD				sCharStateGuard;

	sCHARSTATE_PRIVATESHOP			sCharStatePrivateShop;//  [7/16/2007]
	sCHARSTATE_DIRECT_PLAY			sCharStateDirectPlay;
	sCHARSTATE_OPERATING			sCharStateOperating;
	sCHARSTATE_RIDEON				sCharStateRideOn;
	sCHARSTATE_TURNING				sCharStateTurning;

	sCHARSTATE_AIRJUMPING			sCharStateAirJumping;
	sCHARSTATE_AIRACCEL				sCharStateAirAccel;

};

struct sCHARSTATE
{
	sCHARSTATE_BASE				sCharStateBase;
	sCHARSTATE_DETAIL			sCharStateDetail;
};


//	STRUCT FOR QUERY SERVER <-> GAME SERVER DATA
struct sPC_DATA
{
	CHARACTERID		charId;
	WCHAR			awchName[NTL_MAX_SIZE_CHAR_NAME + 1];

	BYTE			byRace;
	BYTE			byClass;
	bool			bIsAdult;
	bool			bChangeClass;
	BYTE			byGender;

	float			fPositionX;
	float			fPositionY;
	float			fPositionZ;
	float			fDirX;
	float			fDirY;
	float			fDirZ;

	BYTE			byBindType;
	WORLDID			bindWorldId;
	TBLIDX			bindObjectTblidx;
	sVECTOR3		vBindLoc;
	sVECTOR3		vBindDir;

	TBLIDX			worldTblidx;
	WORLDID			worldId;
	DWORD			dwEXP;
	BYTE			byLevel;
	DWORD			charLp;
	WORD			wEP;
	WORD			wRP;
	BYTE			byCurRPBall;
	DWORD			charAP;
	BYTE			byFace;
	BYTE			byHair;
	BYTE			byHairColor;
	BYTE			bySkinColor;

	DWORD			dwMoney;

	GUILDID			guildId;
	WCHAR			awchGuildName[NTL_MAX_SIZE_GUILD_NAME + 1];

	DWORD			dwTutorialHint;
	DWORD			dwMapInfoIndex;		// ĳ���� ���ý� ����� �� ���� ����� Text Tblidx

	DWORD			dwReputation;
	DWORD			dwMudosaPoint;
	DWORD			dwSpPoint;

	TBLIDX			charTitle; //new

	sHOIPOIMIX_DATA sMixData;
	DWORD			dwWaguWaguPoints;

	BYTE			byRankBattleRemainCount;
	BYTE			byTmqLimitCount;
	BYTE			byTmqLimitPlusCount;
	BYTE			byDWCLimitCount;
	BYTE			byDWCLimitPlusCount;
	bool			bIsScrambleJoinFlag;
	bool			bIsScrambleRewardedFlag;
	TBLIDX			mascotTblidx;
	TBLIDX			revivalAftereffectToGet;
	BYTE			eAirState;
	bool			bInvisibleCostume;
	bool			bInvisibleTitle;
	bool			bIsGameMaster;
	BYTE			byAdminLevel;
	bool			bIsMailAway;
	JOINID			wJoinID; // budokai join id
	DWORD			NetPyPoit;
};

// Character Updated 30.11.2014
struct sPC_SUMMARY
{
	CHARACTERID		charId;
	WCHAR			awchName[NTL_MAX_SIZE_CHAR_NAME + 1];
	BYTE			byRace;
	BYTE			byClass;
	bool			bIsAdult;
	BYTE			byGender;
	sPC_SHAPE		sPcShape;
	BYTE			byLevel;
	TBLIDX			worldTblidx;
	WORLDID			worldId;
	float			fPositionX;
	float			fPositionY;
	float			fPositionZ;
	DWORD			dwMoney;
	DWORD			dwMoneyBank;
	sITEM_SUMMARY	sItem[EQUIP_SLOT_TYPE_COUNT];
	DWORD			dwMapInfoIndex;
	bool			bTutorialFlag;
	TBLIDX			charTitle; //new
	bool			bNeedNameChange;
	sDBO_DOGI_DATA	sDogi;
	BYTE			bySuperiorEffectType; //new
	sDBO_GUILD_MARK sMark;
	bool			bInvisibleCostume; //new

};

struct sPC_BRIEF
{
	CHARACTERID		charId;
	TBLIDX			tblidx; // pc ���̺� �ε���
	bool			bIsAdult;
	WCHAR			awchName[NTL_MAX_SIZE_CHAR_NAME + 1];
	WCHAR			wszGuildName[NTL_MAX_SIZE_GUILD_NAME + 1];

	sPC_SHAPE		sPcShape; // pc �ܾ� ( ��/�Ӹ�/�Ӹ��� )

	int				curLp;
	int				maxLp;
	WORD			wCurEP;
	WORD			wMaxEP;

	DWORD			dwCurAP; // NOTE: Not really needed here
	DWORD			dwMaxAP; // NOTE: Not really needed here
	BYTE			byLevel;

	FLOAT			fBaseRunSpeed;
	FLOAT			fLastRunSpeed;
	FLOAT			fBaseAirSpeed;
	FLOAT			fLastAirSpeed;
	FLOAT			fBaseAirDashSpeed;
	FLOAT			fLastAirDashSpeed;
	FLOAT			fBaseAirDashBoostSpeed;
	FLOAT			fLastAirDashBoostSpeed;

	sITEM_BRIEF		sItemBrief[EQUIP_SLOT_TYPE_COUNT]; // ���� ������ ����

	WORD			wAttackSpeedRate;
	float			fSkillAnimationSpeedModifier;

	sDBO_GUILD_MARK	sMark;

	TBLIDX			charTitle;
	
	sDBO_DOGI_DATA	sDogi;

	bool			bEmergency;
	bool			bIsInFreePvpZone;
	bool			bIsScrambleJoinFlag;
	BYTE			byHasBattleDragonBallBitFlag;

	TBLIDX			mascotTblidx;

	//WCHAR	awcMascotName[16 + 1];

	BYTE			bySizeRate;
	PARTYID			partyId;
	bool			bInvisibleCostume;
	//bool			bInvisibleTitle; // NOTE: Temporary disabled because latest TW not support it ~Nady
};


struct sPcProfileLocalize {
	eLocalizeType eLocalizeType;

	union {
		struct sPC_PROFILE_LOCALIZE_DEV { /* Nothing :( */ } sDev;

		struct sPC_PROFILE_LOCALIZE_CJIKOR {
			DWORD dwNeptyPoints;
			DWORD dwRecvGiftCount;
		} sCJIKOR;

		struct sPC_PROFILE_LOCALIZE_CT {
			DWORD dwNeptyPoints;
			DWORD dwRecvGiftCount;
		} sCT;

		struct sPC_PROFILE_LOCALIZE_SD_CH {
			DWORD dwNeptyPoints;
			DWORD dwRecvGiftCount;
			BYTE byPlayRestrict;
		} sSD_CH;
	};
};

// PC Characters Full information (used for loading avatar or character lookup)
struct sPC_PROFILE
{
	TBLIDX				tblidx; // pc ���̺� �ε���
	bool				bIsAdult;
	bool				bChangeClass;		// Whether or not have rights to change class
	CHARACTERID			charId;		// PC ĳ������ ���� ID(DB index)
	WCHAR				awchName[NTL_MAX_SIZE_CHAR_NAME + 1];

	sPC_SHAPE			sPcShape; // pc �ܾ� ( ��/�Ӹ�/�Ӹ��� )

	sAVATAR_ATTRIBUTE	avatarAttribute;

	DWORD				curLp;
	WORD				wCurEP;
	WORD				wCurRP;
	BYTE				byCurRPBall;
	DWORD				curAP;

	BYTE				byLevel;
	DWORD				dwCurExp;
	DWORD				dwMaxExpInThisLevel;

	DWORD				dwZenny;
	DWORD				dwTutorialHint;

	BYTE				byBindType;
	WORLDID				bindWorldId;
	TBLIDX				bindObjectTblidx;

	DWORD				dwReputation;
	DWORD				dwMudosaPoint;
	DWORD				dwSpPoint;

	TBLIDX				charTitle;

	//bool				bInvisibleTitle; // TODO: Uncomment it?

	sHOIPOIMIX_DATA		sMixData;

	bool				bIsGameMaster;		// true : ��� character

	GUILDID				guildId;

	sPcProfileLocalize	sPcProfileLocalize; // NOTE: To keep compatibility with latest client ~Nady

	 // NOTE: 3x To keep compatibility with latest client ~Nady
	BYTE				bySuperiorEffect;
	BYTE				bySuperiorType;
	BYTE				bySuperiorValue;


	BYTE				byRankBattleRemainCount;
	bool				bIsInFreePvpZone;

	bool				bIsScrambleJoinFlag;
	bool				bIsScrambleRewardedFlag;

	BYTE				byHasBattleDragonBallBitFlag;

	TBLIDX				mascotTblidx;
	BYTE				byMascotIdx; // NOTE: To keep compatibility with latest client ~Nady

	DWORD				dwWaguWaguPoints;
	bool				bInvisibleCostume;
};

//-----------------------------------------------------------------------------------
// BOT ( NPC/MOB/PET ���� ������ ��Ʈ�� �ϴ� ĳ���͵��� ��Ī )
//-----------------------------------------------------------------------------------

struct sBOT_SERVER_SCRIPT_DATA
{
	void Init()
	{
		byCreatorType = INVALID_BYTE;
		tblidxCreator = INVALID_TBLIDX;
		playScript = INVALID_TBLIDX;
		playScriptScene = INVALID_TBLIDX;
		tblidxAiScript = INVALID_TBLIDX;
		tblidxAiScriptScene = INVALID_TBLIDX;
		tblidxBattleScript = INVALID_TBLIDX;
		dwBattleScriptScene = INVALID_DWORD;
		sBotAiIndex.Init();
		byEffectiveLevel = 0;
		byActualLevel = 0;
		wMobClass = INVALID_BYTE;
		byMobGrade = INVALID_BYTE;
	}

	BYTE byCreatorType;
	TBLIDX tblidxCreator;
	TBLIDX playScript;
	TBLIDX playScriptScene;
	TBLIDX tblidxAiScript;
	TBLIDX tblidxAiScriptScene;
	TBLIDX tblidxBattleScript;
	TBLIDX dwBattleScriptScene;

	struct sBOTAI_DATA_INDEX
	{
		void Init()
		{
			tblidx = INVALID_TBLIDX;
			bySubIndex = INVALID_BYTE;
		}

		TBLIDX tblidx;
		BYTE bySubIndex;
	}sBotAiIndex;

	BYTE byEffectiveLevel;
	BYTE byActualLevel;
	WORD wMobClass;
	BYTE byMobGrade;
};

struct sBOT_SUB_DATA
{
	void Init()
	{
		tblidxOnlyOneSkillUse = INVALID_TBLIDX;
		byNestRange = 80;
		byNestType = 0;
	}
	TBLIDX	tblidxOnlyOneSkillUse;
	BYTE	byNestType;
	BYTE	byNestRange;
};

// bot data
struct sBOT_DATA
{
	TBLIDX			tblidx;

	WORLDID			worldID; // world id
	TBLIDX			worldtblidx;
	sVECTOR3		vCurLoc; // ������ġ
	sVECTOR3		vCurDir; // �������

	sVECTOR3		vSpawnLoc; // ������ġ
	sVECTOR3		vSpawnDir; // ���� ����
	BYTE			bySpawnRange; // ���� ���� �Ÿ�
	WORD			wSpawnTime; // ���� �ð� (��)
	BYTE			bySpawnFuncFlag; // ���� ��� �÷���

	BYTE			byMoveType; // eSPAWN_MOVE_TYPE
	BYTE			byWanderRange; // ��ȸ �Ÿ� (m)
	BYTE			byMoveRange; // �ѹ��� �����̴� �Ÿ� (m)

	ACTIONPATTERNTBLIDX actionpatternTblIdx; //������ �ൿ ���� �¿� ���̺� ID

	TBLIDX			pathTblidx; // ��� ���̺� ��ȣ

	sBOT_SERVER_SCRIPT_DATA		sScriptData;

	PARTYID			partyID; // ��Ƽ ���̵�
	bool			bPartyLeader; // ��Ƽ ����
	sVECTOR3		vPartyLeaderDistance; // ��Ƽ �������� ���� ����
	QWORD			qwCharConditionFlag;
	HOBJECT			hTargetFixedExecuter;
	BYTE			byImmortalMode;
	sBOT_SUB_DATA	sBotSubData;
};


// bot brief
struct sBOT_BRIEF
{
	TBLIDX			tblidx;

	int				curLp;
	WORD			wCurEP;
	int				maxLp;
	WORD			wMaxEP;

	float			fLastWalkingSpeed;
	float			fLastRunningSpeed;
	float			fLastFlyingSpeed;
	float			fLastFlyingDashSpeed;
	float			fLastFlyingAccelSpeed;
	WORD			wAttackSpeedRate;

	float			fSkillAnimationSpeedModifier;

	ACTIONPATTERNTBLIDX actionpatternTblIdx; //Combat table in behavior pattern set ID

	TBLIDX nicknameTblidx;
	BYTE bySizeRate;
};


// bot profile
struct sBOT_PROFILE
{
	CNtlVector		vSpawnLoc; // ������ġ
	CNtlVector		vSpawnDir; // ���� ����
	BYTE			bySpawnRange; // ���� ���� �Ÿ�
	WORD			wSpawnTime; // ���� �ð� (��)
	BYTE			bySpawnFuncFlag; // ������ ����

	BYTE			byMoveType; // eSPAWN_MOVE_TYPE
	BYTE			byWanderRange; // ��ȸ �Ÿ�
	BYTE			byMoveRange; // �ѹ��� �����̴� �Ÿ�

//	BYTE			byMoveDelayTime; // �����̴� �ð� ����
	TBLIDX			pathTblidx; // ��� ���̺� ��ȣ

	sBOT_SERVER_SCRIPT_DATA		sScriptData;

	bool			bPartyLeader; // ��Ƽ ����
	sVECTOR3		vPartyLeaderDistance; // �������� ���� ����
	DWORD			dwParty_Index;

	sBOT_SUB_DATA	sBotSubData;

	TBLIDX			nicknameTblidx;
};



//-----------------------------------------------------------------------------------
// Non Player Character ( NPC )
//-----------------------------------------------------------------------------------

// npc data
struct sNPC_DATA : public sBOT_DATA
{
};


// npc brief
struct sNPC_BRIEF : public sBOT_BRIEF
{
	bool			bIsEnemyPc;
};


// npc profile
struct sNPC_PROFILE : public sBOT_PROFILE
{
};


//-----------------------------------------------------------------------------------
// Monster Character ( MOB )
//-----------------------------------------------------------------------------------

// mob data
struct sMOB_DATA : public sBOT_DATA
{
	SPAWNGROUPID	spawnGroupId;
};


// mob brief
struct sMOB_BRIEF : public sBOT_BRIEF
{
	BYTE		byActualLevel;
	BYTE		byBallType; // Whether the mob is not created at the pleasure gulsu Dragon Ball if NtlItem.h cf) eDRAGON_BALL_TYPE
};

// mob profile
struct sMOB_PROFILE : public sBOT_PROFILE
{
	SPAWNGROUPID	spawnGroupId;
};



//-----------------------------------------------------------------------------------
// Pet Character ( PET )
//-----------------------------------------------------------------------------------

// pet data
struct sPET_DATA
{
	BYTE			byPetIndex;

	BYTE			bySourceType;		// eDBO_OBJECT_SOURCE
	TBLIDX			sourceTblidx;
};


// pet brief
struct sPET_BRIEF
{
	BYTE			bySourceType;		// eDBO_OBJECT_SOURCE
	TBLIDX			sourceTblidx;
	TBLIDX			npcTblidx;

	BYTE			bySizeRate;//new
	HOBJECT			hOwner;
	float			fLastRunningSpeed;
	float			fLastAttackRange;
	float			fSkillAnimationSpeedModifier;

};

// pet profile
struct sPET_PROFILE
{
	BYTE					bySourceType;		// eDBO_OBJECT_SOURCE
	TBLIDX					sourceTblidx;
	TBLIDX					npcTblidx;

	HOBJECT					hOwner;
	BYTE					byAvatarType;		// eDBO_AVATAR_TYPE

	sAVATAR_ATTRIBUTE		attribute;
};


//-----------------------------------------------------------------------------------
// Summon Pet Character ( SUMMON PET )
//-----------------------------------------------------------------------------------

// summon pet data
struct sSUMMON_PET_DATA : public sPET_DATA
{
	int			curLP;
	WORD		wCurEP;
};

// summon pet brief
struct sSUMMON_PET_BRIEF : public sPET_BRIEF
{
	int				curLp;
	WORD			wCurEP;
	int				maxLp;
	WORD			wMaxEP;

	WORD			wAttackSpeedRate;
	bool			bIsInFreePvpZone;
};

// summon pet profile
struct sSUMMON_PET_PROFILE : public sPET_PROFILE
{
	int				curLp;
	WORD			wCurEP;
};


//-----------------------------------------------------------------------------------
// Summon Pet Character ( ITEM PET )
//-----------------------------------------------------------------------------------

// item pet data
struct sITEM_PET_DATA : public sPET_DATA
{
	ITEMID		itemId;
};

// item pet brief
struct sITEM_PET_BRIEF : public sPET_BRIEF
{
};

// item pet profile
struct sITEM_PET_PROFILE : public sPET_PROFILE
{
};


//-----------------------------------------------------------------------------------
// CHARACTER DATA ( PC ONLY )
//-----------------------------------------------------------------------------------
struct sPLAYER_DATA
{
	CHARACTERID				charId;
	WCHAR					awchName[NTL_MAX_SIZE_CHAR_NAME + 1];
	sPC_SHAPE				sPcShape;
	bool					bIsAdult;
	bool					bChangeClass;

	sDBO_GUILD_MARK			sMark;
	TBLIDX					charTitle;
	sDBO_DOGI_DATA			sDogi;
	bool					bEmergency;

	bool					bIsInFreePvpZone;
	bool					bIsScrambleJoinFlag;
	bool					bIsScrambleRewardedFlag;
	BYTE					byHasBattleDragonBallBitFlag;
	TBLIDX					mascotTblidx;
	bool					bInvisibleTitle;
	bool					bInvisibleCostume;
	sHOIPOIMIX_DATA			sMixData;
	DWORD					dwMaxExpInThisLevel;

	DWORD					dwTutorialHint;

	BYTE					byBindType;
	WORLDID					bindWorldId;
	TBLIDX					bindObjectTblidx;
	sVECTOR3				vBindLoc;
	sVECTOR3				vBindDir;

	DWORD					dwMapInfoIndex;

	DWORD					dwReputation;
	DWORD					dwMudosaPoint;
	DWORD					dwSpPoint;
	DWORD					dwWaguWaguPoints;	//points used for the wagu wagu shop. Max 2000

	DWORD					dwZeniBank;
	bool					bIsMailAway;

	bool					bCombatMode;
};

//-----------------------------------------------------------------------------------
// QUICKSLOT
//-----------------------------------------------------------------------------------
struct sQUICK_SLOT_DATA
{
	TBLIDX	tblidx;
	BYTE	bySlot;
	BYTE	byType;
	ITEMID  itemID;
	HOBJECT	hItem;
};

struct sQUICK_SLOT_PROFILE
{
	TBLIDX	tblidx;
	BYTE	bySlot;
	BYTE	byType;
	HOBJECT	hItem;
};

enum eACCEPT_RES_TYPE
{
	ACCEPT_RES_TYPE_DENY = 0,
	ACCEPT_RES_TYPE_OK,
	ACCEPT_RES_TYPE_CANCEL,
};


//-----------------------------------------------------------------------------------
// ����Ű
//-----------------------------------------------------------------------------------
struct sSHORTCUT_UPDATE_DATA
{
	BYTE    byType;		// eSHORTCUT_CHANGE_TYPE 
	WORD	wActionID;
	WORD	wKey;
};

struct sSHORTCUT_DATA
{
	WORD	wActionID;
	WORD	wKey;
};

enum eSHORTCUT_CHANGE_TYPE
{
	eSHORTCUT_CHANGE_TYPE_ADD = 0,
	eSHORTCUT_CHANGE_TYPE_DEL,
	eSHORTCUT_CHANGE_TYPE_UPDATE,
};

const int		 NTL_SHORTCUT_MAX_COUNT = 100;
//-----------------------------------------------------------------------------------
// WARFOG
//-----------------------------------------------------------------------------------
// WARFOG FLAG
struct sCHAR_WAR_FOG_FLAG
{
	BYTE achWarFogFlag[NTL_MAX_SIZE_WAR_FOG];
};


//-----------------------------------------------------------------------------------
// SERVER CHANGE
//-----------------------------------------------------------------------------------
struct sDBO_SERVER_CHANGE_INFO
{
	sDBO_SERVER_CHANGE_INFO()
	{
		Init();
	}

	void Init()
	{
		prevServerChannelId = INVALID_SERVERCHANNELID;
		destWorldId = INVALID_WORLDID;
		destWorldTblidx = INVALID_TBLIDX;
	}

	SERVERCHANNELID		prevServerChannelId;
	WORLDID				destWorldId;
	TBLIDX				destWorldTblidx;
};


//-----------------------------------------------------------------------------------
// Game Mania Time .by Ju-hyeung Lee
//-----------------------------------------------------------------------------------

// from QueryServer
struct sGAME_MANIA_TIME_DATA
{
	DWORD dwSetTime;
	BYTE abyTimeSlot[GMT_MAX_EFFECT_LIST_SIZE][GMT_MAX_TIME_SLOT_SIZE];
};

// to client
struct sGAME_MANIA_TIME
{
	BYTE abyTimeSlot[GMT_MAX_EFFECT_LIST_SIZE][GMT_MAX_TIME_SLOT_SIZE];
};



////NEW

//-----------------------------------------------------------------------------------
// TARGET
//-----------------------------------------------------------------------------------
struct sTARGET_INFO
{
	BYTE		byDBTblIdx;
	BYTE		byArrayIdx;
	bool		bAtOnceUpdate;
};

struct sTARGET_INFO_ANSWER
{
	DWORD		dwIdx;
	BYTE		byArrayIdx;
	__int64		nData;
};

struct sTARGET_INFO_REQUEST
{
	BYTE		byDBTblIdx;
	BYTE		byArrayIdx;
	DWORD		dwAchieTblIdx;
};


#pragma pack()

enum eTIMID_TYPE
{
	TIMID_NORMAL,
	TIMID_SPAWN_AREA,
	TIMID_FIGHT_AREA,
	INVALID_TIMID_TYPE = 0xFF,
};

//-----------------------------------------------------------------------------------
// Initialize function for union data
//-----------------------------------------------------------------------------------
void InitBotData(sBOT_DATA& rsBotData);

void InitNpcData(sNPC_DATA& rsNpcData);

void InitMobData(sMOB_DATA& rsMobData);

//-----------------------------------------------------------------------------------
// Copy character state data
//-----------------------------------------------------------------------------------

void CopyCharState(sCHARSTATE* pDest, const sCHARSTATE* pSrc);

//-----------------------------------------------------------------------------------
// Scouter
//-----------------------------------------------------------------------------------
DWORD Dbo_CalculatePowerLevel(
	WORD wLastPhysicalOffence, WORD wLastPhysicalDefence, WORD wLastEnergyOffence, WORD wLastEnergyDefence,
	WORD wLastAttackRate, WORD wLastDodgeRate, WORD wLastCurseSuccessRate, WORD wLastCurseTolerance,
	WORD wLastPhysicalCriticalRate, WORD wLastEnergyCriticalRate,
	WORD wAttackSpeedRate,
	WORD wLastMaxLp, WORD wLastMaxEp,
	BYTE byLevel, BYTE byMobGrade);

float Dbo_ConvertToAgentRadius(float fObjectRadius);

float Dbo_GetTransformScale(eASPECTSTATE eAspect, BYTE byGrade);            ///< ���Žÿ� ���� ������ ��ȯ�Ѵ�.

const BYTE CalculateRPBallMaxByLevel(const BYTE byLevel);

DWORD GetSafeRetAdd(DWORD dwMax, DWORD dwCur, DWORD dwAdd);

int Dbo_IncreaseDboLp(int oldLp, int diff, int maxLP);
int Dbo_DecreaseDboLp(int oldLp, int diff, int maxLP);
WORD Dbo_IncreaseDboEp(WORD oldEp, WORD diff, WORD maxEP);
WORD Dbo_DecreaseDboEp(WORD oldEp, WORD diff, WORD maxEP);
int Dbo_IncreaseDboAp(int oldAp, int diff, int maxAP);
int Dbo_DecreaseDboAp(int oldAp, int diff, int maxAP);

float Dbo_GetWeightRateInPercent(DWORD dwTotalWeight, DWORD dwWeightLimit);

