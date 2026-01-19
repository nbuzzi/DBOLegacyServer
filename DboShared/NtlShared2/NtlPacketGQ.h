#pragma once

#include "NtlPacketCommon.h"
#include "NtlQuest.h"
#include "NtlItem.h"
#include "NtlSkill.h"
#include "NtlCharacter.h"
#include "NtlParty.h"
#include "NtlMail.h"
#include "NtlEventDef.h"
#include "NtlBudokai.h"
#include "NtlMatch.h"
#include "NtlDragonBall.h"
#include "NtlHlsItem.h"
#include "NtlTeleport.h"
#include "NtlRankBattle.h"
#include "NtlMascot.h"


enum eOPCODE_GQ
{
	GQ_OPCODE_BEGIN = 38000,

	GQ_NOTIFY_SERVER_BEGIN = GQ_OPCODE_BEGIN,

	GQ_LOAD_PC_DATA_REQ,
	GQ_ITEM_CREATE_REQ,
	GQ_ITEM_MOVE_REQ,
	GQ_ITEM_MOVE_STACK_REQ,
	GQ_ITEM_UPDATE_REQ,
	GQ_ITEM_DELETE_REQ,
	GQ_ITEM_EQUIP_REPAIR_REQ,
	GQ_ITEM_REPAIR_REQ,
	GQ_ITEM_DUR_DOWN,
	GQ_ITEM_DUR_UPDATE_REQ,
	GQ_ITEM_USE_REQ,
	GQ_ITEM_UPGRADE_REQ,
	GQ_ITEM_IDENTIFY_REQ,
	GQ_ITEM_CREATE_EX_REQ,						// ������ ���� Ȯ����(������ ���� ����)
	GQ_ITEM_DELETE_EX_REQ,						// ������ ���� Ȯ����(������ ���� ����)
	GQ_ITEM_AUTO_EQUIP_REQ,						// ������ �ڵ� ����(Quest���� ���)
	GQ_ITEM_AUTO_EQUIP_ROLLBACK_REQ,			// ������ �ڵ� ���� RollBack(Quest���� ���)

	GQ_ITEM_PICK_REQ,
	GQ_ZENNY_PICK_REQ,

	GQ_SKILL_ADD_REQ,
	GQ_SKILL_DEL_REQ,
	GQ_SKILL_UPDATE_REQ,
	GQ_SKILL_UPGRADE_REQ,
	GQ_SKILL_RP_BONUS_SETTING_REQ,
	GQ_SKILL_LOAD_REQ,
	GQ_BUFF_ADD_REQ,
	GQ_BUFF_DEL_REQ,
	GQ_BUFF_UPDATE_REQ,
	GQ_BUFF_LOAD_REQ,
	GQ_HTB_SKILL_LOAD_REQ,
	GQ_HTB_SKILL_ADD_REQ,

	GQ_PC_UPDATE_POSITION_REQ,
	GQ_PC_UPDATE_EXP_REQ,
	GQ_PC_UPDATE_LPEPRP_REQ,
	GQ_PC_UPDATE_LEVEL_REQ,
	GQ_SAVE_PC_DATA_REQ,
	GQ_SAVE_SKILL_DATA_REQ,
	GQ_SAVE_HTB_DATA_REQ,
	GQ_SAVE_BUFF_DATA_REQ,

	GQ_PC_EXIT,
	//GQ_MAKE_AUTH_KEY_REQ,	// 

	GQ_SHOP_BUY_REQ,
	GQ_SHOP_SELL_REQ,
	
	GQ_LOAD_PC_BANK_DATA_REQ,
	GQ_BANK_MOVE_REQ,
	GQ_BANK_MOVE_STACK_REQ,
	GQ_BANK_ZENNY_REQ,
	GQ_BANK_BUY_REQ,
	GQ_BANK_ADD_WITH_COMMAND_REQ,
	GQ_BANK_ITEM_DELETE_REQ,

	GQ_PARTY_LOOTING_ITEM_REQ,
	GQ_PARTY_LOOTING_ZENNY_REQ,

	GQ_PC_UPDATE_BIND_REQ,
	GQ_CHAR_CONVERT_CLASS_REQ,
	GQ_CHAR_CONVERT_GENDER_NFY,

	GQ_UPDATE_CHAR_ZENNY_REQ,
	GQ_UPDATE_CHAR_NETPY_REQ,

	GQ_QUEST_ITEM_CREATE_REQ,					// ����Ʈ ������ ����
	GQ_QUEST_ITEM_DELETE_REQ,					// ����Ʈ ������ ����
	GQ_QUEST_ITEM_MOVE_REQ,						// ����Ʈ ������ �̵�
	GQ_QUEST_PROGRESS_DATA_CREATE_REQ,			// ����Ʈ ���� ���� ����
	GQ_QUEST_PROGRESS_DATA_DELETE_REQ,			// ����Ʈ ���� ���� ����
	GQ_QUEST_COMPLETE_DATA_UPDATE_REQ,			// ����Ʈ �Ϸ� ���� ������Ʈ
	GQ_QUEST_DATA_RESET_REQ,					// ����Ʈ ������ ������ ������ŭ �����Ѵ�.
	GQ_QUEST_STATE_UPDATE_REQ,					// ������ ����
	GQ_QUEST_TSP_UPDATE_REQ,					// TS �������� ����
	GQ_QUEST_EXCEPTION_TIMER_UPDATE_REQ,		// ���� Ÿ�̸��� ��� �� ����-> ��� �� ����??
	GQ_QUEST_SSM_UPDATE_REQ,					// �޸� ������ ������Ʈ
	GQ_QUEST_SERVER_EVENT_UPDATE_REQ,			// ���� �̺�Ʈ �������� ������Ʈ
	GQ_QUEST_EXC_CLIENT_GROUP_REQ,				// Ŭ���̾�Ʈ ���� �׷� ���̵�
	GQ_QUEST_INFO_UPDATE_REQ,					// ����Ʈ ���� ��� �������� ������Ʈ

	GQ_QUICK_SLOT_UPDATE_REQ,	// QuikSlot Add or Update
	GQ_QUICK_SLOT_DEL_REQ,

	GQ_SAVE_SPAWNED_SUMMON_PET_DATA_REQ,		// ��ȯ�Ǿ� �ִ� summon pet�� ����
	GQ_SAVE_SPAWNED_ITEM_PET_DATA_REQ,			// ��ȯ�Ǿ� �ִ� item pet�� ����
	GQ_DELETE_SPAWNED_SUMMON_PET_DATA_REQ,		// ��ȯ�Ǿ� �ִ� summon pet�� ����
	GQ_DELETE_SPAWNED_ITEM_PET_DATA_REQ,		// ��ȯ�Ǿ� �ִ� item pet�� ����
	GQ_LOAD_SPAWNED_PET_DATA_REQ,				// ��ȯ�Ǿ� �ִ� �� ���� ��û
	GQ_DELETE_ALL_TEMPORARY_PET_DATA_REQ,		// �ӽ÷� ����� �� ������ ����

	GQ_CHAR_PUNISH_REQ,

	GQ_TRADE_REQ,
	GQ_TUTORIAL_HINT_UPDATE_REQ,

	GQ_BALL_ITEM_PICK_REQ,
	GQ_BALL_ITEM_PARTY_PICK_REQ,
	GQ_BALL_ITEM_REWARD_REQ,

	GQ_PRIVATESHOP_ITEM_BUYING_REQ,				//  [7/2/2007 SGpro]
	GQ_PRIVATESHOP_ITEM_INSERT_REQ,				//  [7/2/2007 SGpro]
	GQ_PRIVATESHOP_ITEM_DELETE_REQ,				//  [7/2/2007 SGpro]
	GQ_PRIVATESHOP_ITEM_UPDATE_REQ,				//  [7/2/2007 SGpro]
	GQ_PRIVATESHOP_SHOP_LOADING_REQ,			//  [7/2/2007 SGpro]
	GQ_PRIVATESHOP_CREATE_REQ,					//  [7/4/2007 SGpro]
	GQ_PRIVATESHOP_UPDATE_REQ,					//  [7/4/2007 SGpro]

	GQ_RANKBATTLE_SCORE_UPDATE_REQ,		// RankBattle Score Update
	GQ_TUTORIAL_DATA_UPDATE_REQ,		// Tutorial Data Update

	GQ_TMQ_DAYRECORD_RESET_REQ,			// DayRecord ���� ��û
	GQ_TMQ_DAYRECORD_UPDATE_REQ,		// DayRecord ��� ��û
	GQ_TMQ_DAYRECORD_LIST_REQ,			// DayRecord ����Ʈ ��û(�Խ���)
	GQ_TMQ_DAYRECORD_REQ,				// DayRecord ��û

	GQ_MAIL_START_REQ,
	GQ_MAIL_SEND_REQ,
	GQ_MAIL_READ_REQ,
	GQ_MAIL_DEL_REQ,
	GQ_MAIL_RETURN_REQ,
	GQ_MAIL_RELOAD_REQ,
	GQ_MAIL_LOAD_REQ,
	GQ_MAIL_ITEM_RECEIVE_REQ,
	GQ_MAIL_LOCK_REQ,
	GQ_MAIL_EVENT_SEND_REQ,
	GQ_MAIL_MULTI_DEL_REQ,

	GQ_CHAR_AWAY_REQ,

	GQ_CHAR_KEY_UPDATE_REQ,

	GQ_PORTAL_START_REQ,
	GQ_PORTAL_ADD_REQ,

	GQ_WAR_FOG_UPDATE_REQ,

	GQ_ITEM_UPGRADE_ALL_REQ,

	GQ_UPDATE_PLAYER_REPUTATION_REQ,

	GQ_GUILD_BANK_LOAD_REQ,
	GQ_GUILD_BANK_MOVE_REQ,
	GQ_GUILD_BANK_MOVE_STACK_REQ,
	GQ_GUILD_BANK_ZENNY_REQ,

	GQ_SHOP_ITEM_IDENTIFY_REQ,

	GQ_BUDOKAI_DATA_REQ,
	GQ_BUDOKAI_INIT_DATA_REQ,
	GQ_BUDOKAI_UPDATE_STATE_REQ,
	GQ_BUDOKAI_UPDATE_MATCH_STATE_REQ,
	GQ_BUDOKAI_UPDATE_CLEAR_REQ,
	GQ_RANKPOINT_RESET_REQ,
	GQ_RANKBATTLE_ALLOW_REQ,

	// õ�����Ϲ���ȸ ����
	GQ_BUDOKAI_INDIVIDUAL_ALLOW_REGISTER_REQ,
	GQ_BUDOKAI_JOIN_INDIVIDUAL_REQ,
	GQ_BUDOKAI_LEAVE_INDIVIDUAL_REQ,
	GQ_BUDOKAI_INDIVIDUAL_SELECTION_REQ,
	GQ_BUDOKAI_INDIVIDUAL_LIST_REQ,
	GQ_BUDOKAI_TOURNAMENT_INDIVIDUAL_ADD_ENTRY_LIST_REQ,	// ��ʸ�Ʈ ������ �߰�
	GQ_BUDOKAI_TOURNAMENT_INDIVIDUAL_ENTRY_LIST_REQ,		// ��ʸ�Ʈ ������ ����Ʈ ��û
	GQ_BUDOKAI_TOURNAMENT_INDIVIDUAL_ADD_MATCH_RESULT_REQ,	// ��ʸ�Ʈ ��� ��� �߰�
	GQ_BUDOKAI_TOURNAMENT_INDIVIDUAL_MATCH_RESULT_REQ,		// ��ʸ�Ʈ ��� ��� ����Ʈ ��û

	// õ�����Ϲ���ȸ ��
	GQ_BUDOKAI_TEAM_ALLOW_REGISTER_REQ,
	GQ_BUDOKAI_JOIN_TEAM_REQ,
	GQ_BUDOKAI_LEAVE_TEAM_REQ,
	GQ_BUDOKAI_LEAVE_TEAM_MEMBER_REQ,
	GQ_BUDOKAI_TEAM_SELECTION_REQ,
	GQ_BUDOKAI_TEAM_LIST_REQ,
	GQ_BUDOKAI_TOURNAMENT_TEAM_ADD_ENTRY_LIST_REQ,		// ��ʸ�Ʈ ������ �߰�
	GQ_BUDOKAI_TOURNAMENT_TEAM_ENTRY_LIST_REQ,			// ��ʸ�Ʈ ������ ����Ʈ ��û
	GQ_BUDOKAI_TOURNAMENT_TEAM_ADD_MATCH_RESULT_REQ,	// ��ʸ�Ʈ ��� ��� �߰�
	GQ_BUDOKAI_TOURNAMENT_TEAM_MATCH_RESULT_REQ,		// ��ʸ�Ʈ ��� ��� ����Ʈ ��û

	GQ_BUDOKAI_JOIN_INFO_REQ,							// ���� ���� ��û
	GQ_BUDOKAI_JOIN_STATE_REQ,							// ���� ���� ��û
	GQ_BUDOKAI_HISTORY_WRITE_REQ,						// ������ season history ���� ��û
	GQ_BUDOKAI_HISTORY_WINNER_PLAYER_REQ,				// �ش� season �� player info ��û
	GQ_BUDOKAI_JOIN_STATE_LIST_REQ,						// ���� ���� ����Ʈ ��û
	GQ_BUDOKAI_SET_OPEN_TIME_REQ,						// Set Open Time

	GQ_MATCH_REWARD_REQ,					// ��� ����

	GQ_SCOUTER_ITEM_SELL_REQ,				// ��ī���͸� ���� ������ �Ǹ�

	GQ_SHOP_EVENTITEM_BUY_REQ,				// [7/14/2008 SGpro]
	GQ_SHOP_GAMBLE_BUY_REQ,				// [7/14/2008 SGpro]

	GQ_BUDOKAI_UPDATE_MUDOSA_POINT_REQ,
	GQ_ITEM_REPLACE_REQ, // [7/23/2008 SGpro]
	
	GQ_UPDATE_SP_POINT_REQ,

	GQ_UPDATE_MARKING_REQ,// Īȣ [9/10/2008 SGpro]

	GQ_SKILL_BUY_REQ,
	GQ_SKILL_INIT_REQ,
	GQ_SKILL_ONE_RESET_REQ,
	
	GQ_RECIPE_REG_REQ,
	GQ_HOIPOIMIX_JOB_SET_REQ,
	GQ_HOIPOIMIX_JOB_RESET_REQ,
	GQ_HOIPOIMIX_ITEM_MAKE_REQ,
	
	GQ_RUNTIME_PCDATA_SAVE_NFY,

	GQ_ITEM_STACK_UPDATE_REQ,
	GQ_DOJO_BANK_HISTORY_REQ,
	GQ_DOJO_BANK_ZENNY_UPDATE_REQ,

	GQ_WORLD_SCHEDULE_SET_REQ,

	GQ_SWITCH_CHILD_ADULT,
	GQ_SET_CHANGE_CLASS_AUTHORITY,
	GQ_ITEM_CHANGE_ATTRIBUTE_REQ,

	GQ_CHANGE_NETP_REQ,

	GQ_ITEM_CHANGE_DURATIONTIME_REQ,
	
	GQ_SHOP_NETPYITEM_BUY_REQ,

	GQ_GM_COMMAND_LOG_REQ,

	GQ_DURATION_ITEM_BUY_REQ,
	GQ_DURATION_RENEW_REQ,

	GQ_CASHITEM_HLSHOP_REFRESH_REQ,
	GQ_CASHITEM_INFO_REQ,
	GQ_CASHITEM_MOVE_REQ,			// �κ����� �̵�
	GQ_CASHITEM_DEL_REQ,			// ���� ����
	GQ_CASHITEM_UNPACK_REQ,			// �Ѱ��� �������� ������ ����
	GQ_CASHITEM_BUY_REQ,			// ������ �߰�

	GQ_GMT_UPDATE_REQ,

	GQ_PC_DATA_LOAD_REQ,
	GQ_PC_ITEM_LOAD_REQ,
	GQ_PC_SKILL_LOAD_REQ,		
	GQ_PC_BUFF_LOAD_REQ,
	GQ_PC_HTB_LOAD_REQ,
	GQ_PC_QUEST_ITEM_LOAD_REQ,		
	GQ_PC_QUEST_COMPLETE_LOAD_REQ,
	GQ_PC_QUEST_PROGRESS_LOAD_REQ,	
	GQ_PC_QUICK_SLOT_LOAD_REQ,	
	GQ_PC_SHORTCUT_LOAD_REQ,
	GQ_PC_ITEM_RECIPE_LOAD_REQ,
	GQ_PC_GMT_LOAD_REQ,

	GQ_TENKAICHIDAISIJYOU_SERVERDATA_REQ,
	GQ_TENKAICHIDAISIJYOU_PERIODEND_REQ,
	GQ_TENKAICHIDAISIJYOU_SELL_CANCEL_REQ,
	GQ_TENKAICHIDAISIJYOU_BUY_REQ,

	GQ_QUICK_TELEPORT_INFO_LOAD_REQ,
	GQ_QUICK_TELEPORT_UPDATE_REQ,
	GQ_QUICK_TELEPORT_DEL_REQ,
	GQ_QUICK_TELEPORT_USE_REQ,

	GQ_PC_ITEM_COOL_TIME_LOAD_REQ,
	GQ_SAVE_ITEM_COOL_TIME_DATA_REQ,

	GQ_ACCOUNT_BANN,

	GQ_CHARTITLE_SELECT_REQ,
	GQ_CHARTITLE_ADD_REQ,
	GQ_CHARTITLE_DELETE_REQ,

	GQ_CASHINFOREFRESH_REQ,
	GQ_CASHITEM_SEND_GIFT_REQ,

	GQ_PC_MASCOT_LOAD_REQ,
	GQ_MASCOT_REGISTER_REQ,
	GQ_MASCOT_DELETE_REQ,
	GQ_MASCOT_SKILL_ADD_REQ,
	GQ_MASCOT_SKILL_UPDATE_REQ,
	GQ_MASCOT_SKILL_UPGRADE_REQ,
	GQ_MASCOT_FUSION_REQ,
	GQ_MASCOT_SAVE_DATA_REQ,

	GQ_MATERIAL_DISASSEMBLE_REQ,

	GQ_INVISIBLE_COSTUME_UPDATE_REQ,

	GQ_ITEM_SOCKET_INSERT_BEAD_REQ,
	GQ_ITEM_SOCKET_DESTROY_BEAD_REQ,

	GQ_CASHITEM_PUBLIC_BANK_ADD_REQ,
	GQ_ITEM_EXCHANGE_REQ,
	GQ_EVENT_COIN_ADD_REQ,
	GQ_WAGUWAGUMACHINE_COIN_INCREASE_REQ,
	GQ_CHARACTER_RENAME_REQ,
	GQ_ITEM_CHANGE_OPTION_REQ,
	GQ_ITEM_UPGRADE_WORK_REQ,
	GQ_DYNAMIC_FIELD_SYSTEM_COUNTING_REQ,

	GQ_ITEM_SEAL_REQ,
	GQ_ITEM_SEAL_EXTRACT_REQ,

	GQ_DRAGONBALL_SCRAMBLE_NFY,

	GQ_ITEM_UPGRADE_BY_COUPON_REQ,
	GQ_ITEM_GRADEINIT_BY_COUPON_REQ,

	GQ_EVENT_REWARD_LOAD_REQ,
	GQ_EVENT_REWARD_SELECT_REQ,
	GQ_MASCOT_SEAL_SET_REQ,
	GQ_MASCOT_SEAL_CLEAR_REQ,

	GQ_GM_LOG,

	GQ_CHAR_WAGUPOINT_UPDATE_REQ,

	GQ_OPCCODE_END_DUMMY,
	GQ_OPCODE_END = GQ_OPCCODE_END_DUMMY - 1
};


//------------------------------------------------------------------
//
//------------------------------------------------------------------
const char * NtlGetPacketName_GQ(WORD wOpCode);
//------------------------------------------------------------------


#pragma pack(push, 1)
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_NOTIFY_SERVER_BEGIN)
	SERVERCHANNELID		byServerChannelId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_LOAD_PC_DATA_REQ)
	ACCOUNTID		accountId;
	CHARACTERID		charId;
	DWORD			dwConnectingUserKey;
	//char			achAuthKey[NTL_MAX_SIZE_AUTH_KEY];
	QUESTID			startResetQID;
	QUESTID			endResetQID;
	DWORD			dwGMTCurTime;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_ITEM_CREATE_REQ)
HOBJECT			handle;
CHARACTERID		charId;
sITEM_DATA		sItem;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_ITEM_MOVE_REQ)
	HOBJECT			handle;
	CHARACTERID		charId;
	ITEMID			srcItemId;
	ITEMID			dstItemId;
	HOBJECT			hSrcItem;
	HOBJECT			hDstItem;
	BYTE			bySrcPlace;
	BYTE			bySrcPos;
	BYTE			byDstPlace;
	BYTE			byDstPos;
	bool			bRestrictUpdate;
	BYTE			bySrcRestrictState;
	BYTE			byDestRestrictState;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_ITEM_MOVE_STACK_REQ)
	HOBJECT			handle;
	CHARACTERID		charId;
	ITEMID			srcItemId;
	ITEMID			dstItemId;
	HOBJECT			hSrcItem;
	HOBJECT			hDstItem;
	BYTE			bySrcPlace;
	BYTE			bySrcPos;
	BYTE			byDstPlace;
	BYTE			byDstPos;
	BYTE			byStackCount1;
	BYTE			byStackCount2;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_ITEM_UPDATE_REQ)
	HOBJECT			handle;
	CHARACTERID		charId;
	sITEM_DATA		sItem;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_ITEM_DELETE_REQ)
	HOBJECT			handle;
	CHARACTERID		charId;
	BYTE			byPlace;
	BYTE			byPos;
	ITEMID			itemId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_ITEM_EQUIP_REPAIR_REQ )
	HOBJECT			handle;
	HOBJECT			npcHandle;
	CHARACTERID		charId;
	BYTE			byCount;
	DWORD			dwZenny;
	sITEM_REPAIR	asItemData[EQUIP_SLOT_TYPE_COUNT];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_ITEM_REPAIR_REQ )
	HOBJECT			handle;
	HOBJECT			npcHandle;
	CHARACTERID		charId;
	DWORD			dwZenny;
	sITEM_REPAIR	sItemData;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_ITEM_DUR_DOWN )
	HOBJECT			handle;
	CHARACTERID		charId;
	BYTE			byDur[NTL_MAX_EQUIP_ITEM_SLOT];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_ITEM_DUR_UPDATE_REQ )
	HOBJECT			handle;
	HOBJECT			itemhandle;
	CHARACTERID		charId;
	ITEMID			itemId;
	BYTE			byDur;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_ITEM_USE_REQ)
	HOBJECT			handle;
	CHARACTERID		charId;
	BYTE			byPlace;
	BYTE			byPos;
	HOBJECT			hItem;
	ITEMID			itemId;
	BYTE			byStack;
	DWORD			dwItemUseEventId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_ITEM_UPGRADE_REQ)
	HOBJECT			handle;
	CHARACTERID		charID;				// CHARID
	ITEMID			itemID;				// ���׷��̵� �� �������� ���̵�		-> Grade ������Ʈ
	ITEMID			stonitemID;			// ���� ���̵�							-> ����,���ú���
	BYTE			byStack;			// ���� ����� ���ð���					-> ����� ����
	BYTE			byGrade;			// ����� �׷��̵�						-> ����� �׷��̵�
	BYTE			byItemPlace;		// ���׷��̵� �� �������� �����̳� ��ġ 
	BYTE			byItemPos;			// ���׷��̵� �� �������� ��ġ
	BYTE			byStonPlace;		// ȣ�����̽��� �����̳� ��ġ
	BYTE			byStonPos;			// ȣ�����̽��� ��ġ
	bool			bType;				// true ���� false ���׷��̵�
	BYTE			byBattleAttribute;	// ������Ʈ �� �Ӽ�
	ITEMID			idCoreItem;
	BYTE			byCorePlace;
	BYTE			byCorePos;
	BYTE			byCoreStack;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_ITEM_IDENTIFY_REQ )
	CHARACTERID		charId;
	ITEMID			itemId;
	DWORD			dwZeni;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_ITEM_CREATE_EX_REQ)
	HOBJECT					handle;
	CHARACTERID				charId;
	BYTE					byRequestType;	// ������ ���� Ÿ�� eITEM_CREATE_TYPE
	uITEM_CREATE_SUB_DATA	uSubData;		// �߰� ����
	BYTE					byUpdateCount;	// Stack ���氹��
	sITEM_BASIC_DATA		asUpdateData[ITEM_CREATE_EX_MAX_COUNT];
	BYTE					byItemCount;	// ������ ����
	sITEM_DATA				aItem[1];		// ������ ����
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_ITEM_DELETE_EX_REQ)
	HOBJECT					handle;
	CHARACTERID				charId;
	BYTE					byRequestType;	// ������ ���� Ÿ�� eITEM_CREATE_TYPE
	uITEM_DELETE_SUB_DATA	uSubData;		// �߰� ����
	BYTE					byItemCount;	// ������ ����
	sITEM_DELETE_DATA		aItem[1];		// ������ ����
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_ITEM_AUTO_EQUIP_REQ)
	HOBJECT					handle;
	CHARACTERID				charId;
	sTSM_SERIAL				sTSMSerial;
	sITEM_AUTO_EQUIP_DATA	sItem;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_ITEM_AUTO_EQUIP_ROLLBACK_REQ)
	HOBJECT					handle;
	CHARACTERID				charId;
	sTSM_SERIAL				sTSMSerial;
	sITEM_AUTO_EQUIP_ROLLBACK_DATA	sItem;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_ITEM_PICK_REQ)
	HOBJECT					handle;
	CHARACTERID				charID;
	HOBJECT					itemhandle;
	bool					bIsNew;		// 1: ���� 0: ������Ʈ(��ġ��)
	sITEM_DATA				sItemData;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_ZENNY_PICK_REQ)
	HOBJECT					handle;
	CHARACTERID				charID;
	HOBJECT					itemhandle;
	CHARACTERID				charId;
	DWORD					dwZenny; //dwOrgZenny + dwBonusZenny
	DWORD					dwOrgZenny; //���� ���� Zenny
	DWORD					dwBonusZenny;//�߰� ȹ�� Zenny
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_SKILL_ADD_REQ)
	HOBJECT					handle;
	CHARACTERID				charId;
	BYTE					bySlot;
	TBLIDX					skillId;
	BYTE					byRpBonusType;
	BYTE					byNewClass;
	DWORD					dwZenny;
	DWORD					dwSP;
	BYTE					bySkillFrom;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_SKILL_DEL_REQ)
	CHARACTERID				charId;
	BYTE					bySkillIndex;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_SKILL_UPDATE_REQ)
	HOBJECT					handle;
	CHARACTERID				charId;
	sSKILL_DATA				sSkill;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_SKILL_UPGRADE_REQ)
	HOBJECT					handle;
	CHARACTERID				charId;
	TBLIDX					oldSkillId;
	TBLIDX					newSkillId;
	BYTE					bySlot;
	BYTE					byRpBonusType;
	bool					bIsRpBonusAuto;
	DWORD					dwSP;
	bool					bForced;
	bool					bForMax;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_SKILL_RP_BONUS_SETTING_REQ)
	HOBJECT					handle;
	CHARACTERID				charId;
	TBLIDX					skillId;
	BYTE					bySkillSlotIndex;
	BYTE					byRpBonusType;
	bool					bIsRpBonusAuto;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_SKILL_LOAD_REQ)
HOBJECT					handle;
CHARACTERID				charId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_BUFF_ADD_REQ)
CHARACTERID				charId;
sBUFF_DATA				sBuff;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_BUFF_DEL_REQ)
CHARACTERID				charId;
BYTE					buffIndex;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_BUFF_UPDATE_REQ)
	CHARACTERID				charId;
	TBLIDX					sourceTblidx;
	sBUFF_DATA				sBuff;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_BUFF_LOAD_REQ)
	HOBJECT					handle;
	CHARACTERID				charId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_HTB_SKILL_LOAD_REQ)
	HOBJECT					handle;
	CHARACTERID				charId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_HTB_SKILL_ADD_REQ)
	CHARACTERID				charId;
	TBLIDX					skillId;
	BYTE					skillIndex;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_PC_UPDATE_POSITION_REQ)
	HOBJECT					handle;
	CHARACTERID				charId;
	float					fPositionX;
	float					fPositionY;
	float					fPositionZ;
	float					fDirX;
	float					fDirZ;
	TBLIDX					worldTblidx;
	WORLDID					worldId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_PC_UPDATE_EXP_REQ)
	HOBJECT					handle;
	CHARACTERID				charId;
	DWORD					dwEXP;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_PC_UPDATE_LPEPRP_REQ)
	HOBJECT					handle;
	CHARACTERID				charId;
	DWORD					dwLP;
	DWORD					dwEP;
	DWORD					dwRP;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_PC_UPDATE_LEVEL_REQ)
	HOBJECT					handle;
	CHARACTERID				charId;
	BYTE					byLevel;
	DWORD					dwEXP;
	DWORD					dwSP;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_SAVE_PC_DATA_REQ)
sPC_DATA					sPcData;
sDBO_SERVER_CHANGE_INFO		serverChangeInfo;
DWORD						dwAddPlayTime;
char						IP[NTL_MAX_LENGTH_OF_IP_ADDRESS];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_SAVE_SKILL_DATA_REQ)
HOBJECT					handle;
CHARACTERID				charId;
BYTE					bySkillCount;
sSKILL_DATA				asSkill[NTL_MAX_PC_HAVE_SKILL];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_SAVE_HTB_DATA_REQ)
	HOBJECT					handle;
	CHARACTERID				charId;
	BYTE					bySkillCount;
	sHTB_SKILL_DATA			asHTBSkill[NTL_HTB_MAX_PC_HAVE_HTB_SKILL];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_SAVE_BUFF_DATA_REQ)
	HOBJECT					handle;
	CHARACTERID				charId;
	BYTE					byBuffCount;
	sBUFF_DATA				asBuff[DBO_MAX_BUFF_CHARACTER_HAS];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_PC_EXIT)
	ACCOUNTID				accountId;
	eCHARLEAVING_TYPE		eCharLeavingType;
	SERVERFARMID			lastServerFarmId;
END_PROTOCOL()
//------------------------------------------------------------------
/*BEGIN_PROTOCOL(GQ_MAKE_AUTH_KEY_REQ)
	HOBJECT					handle;
	ACCOUNTID				accountId;
	CHARACTERID				charId;
	BYTE					byNewState;
	char					achAuthKey[NTL_MAX_SIZE_AUTH_KEY];
END_PROTOCOL()*/
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_SHOP_BUY_REQ)
	HOBJECT					handle;
	HOBJECT					hNpchandle;
	CHARACTERID				charId;
	DWORD					dwMoney;
	BYTE					byBuyCount;
	sSHOP_BUY_INVEN			sInven[NTL_MAX_BUY_SHOPPING_CART];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_SHOP_SELL_REQ)
	HOBJECT					handle;
	HOBJECT					hNpchandle;
	CHARACTERID				charId;
	DWORD					dwMoney;
	BYTE					bySellCount;
	sSHOP_SELL_INVEN		sInven[NTL_MAX_SELL_SHOPPING_CART];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_LOAD_PC_BANK_DATA_REQ)
	HOBJECT					handle;
	HOBJECT					npchandle;
	ACCOUNTID				accountId;
	CHARACTERID				charId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_BANK_MOVE_REQ)
HOBJECT			handle;
HOBJECT			hNpcHandle;
CHARACTERID		charId;
ACCOUNTID		accountId;
ITEMID			srcItemId;
ITEMID			dstItemId;
HOBJECT			hSrcItem;
HOBJECT			hDstItem;
BYTE			bySrcPlace;
BYTE			bySrcPos;
BYTE			byDstPlace;
BYTE			byDstPos;
bool			bRestrictUpdate;
BYTE			bySrcRestrictState;
BYTE			byDestRestrictState;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_BANK_MOVE_STACK_REQ)
HOBJECT					handle;
HOBJECT					hNpcHandle;
CHARACTERID				charId;
ACCOUNTID				accountID;
ITEMID					srcItemID;
ITEMID					dstItemID;
HOBJECT					hSrcItem;
HOBJECT					hDstItem;
BYTE					bySrcPlace;
BYTE					bySrcPos;
BYTE					byDstPlace;
BYTE					byDstPos;
BYTE					byStackCount1;
BYTE					byStackCount2;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_BANK_ZENNY_REQ)
	HOBJECT					handle;
	HOBJECT					npchandle;		// NpcHandle
	CHARACTERID				charId;
	ACCOUNTID				accountID;
	DWORD					dwZenny;		// ���ų� ���� �׼�
	bool					bIsSave;		// 1 �� ���� ��� 0 �� ���°��
	DWORD					dwPlayerZeni;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_BANK_BUY_REQ)
	HOBJECT					handle;
	HOBJECT					npchandle;
	ACCOUNTID				accountID;
	BYTE					byMerchantTab;
	BYTE					byMerchantPos;
	DWORD					dwZenny;
	TBLIDX					itemNo;					// Template Number
	CHARACTERID				charId;					// Owner Serial
	BYTE					byPlace;				// eCONTAINER_TYPE
	BYTE					byPosition;	
	BYTE					byRank;					// 0 1 2 3 //â�������� ������
	BYTE					byDurationType;
	DBOTIME					nUseStartTime;
	DBOTIME					nUseEndTime;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_BANK_ADD_WITH_COMMAND_REQ)
HOBJECT					handle;
CHARACTERID				charId;
TBLIDX					itemNo;					// Template Number
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_BANK_ITEM_DELETE_REQ)
	HOBJECT					handle;
	CHARACTERID				charId;
	ACCOUNTID				accountID;
	BYTE					byPlace;
	BYTE					byPos;
	HOBJECT					hItem;
	ITEMID					itemId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_PARTY_LOOTING_ITEM_REQ)
	// The item owner can be found out with using 'sItemData.charId'.
	// �������� ���� �÷��̾�� 'sItemData.charId'�� �˾Ƴ� �� �ִ�.
	// by YOSHIKI(2007-03-09)
	HOBJECT				itemhandle;
	bool				bIsNew;		// 1: ���� 0: ������Ʈ(��ġ��)
	sITEM_DATA			sItemData;
	bool				bByAutoDistribution;

	// For Game Log
	ACCOUNTID							winnerAccountId;
	PARTYID								partyId;
	sDBO_ZENNY_DATA		aZennyData[NTL_MAX_MEMBER_IN_PARTY];
	bool				bIsPartyInven;
	BYTE				byItemSlot;
	BYTE				byMemberIndex;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_PARTY_LOOTING_ZENNY_REQ)
	CHARACTERID			looterCharId;
	HOBJECT				hDroppedZenny;
	DWORD				dwTotalZenny;
	bool				bIsShared;
	BYTE				byMemberCount;
	sZENNY_INFO			aZennyInfo[NTL_MAX_MEMBER_IN_PARTY];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_PC_UPDATE_BIND_REQ)
	HOBJECT					handle;
	CHARACTERID				charId;
	BYTE					byBindType;
	WORLDID					bindWorldId;
	TBLIDX					bindObjectTblidx;
	sVECTOR3				vBindLoc;
	sVECTOR3				vBindDir;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_CHAR_CONVERT_CLASS_REQ )
	HOBJECT					handle;
	CHARACTERID				charId;
	BYTE					byClass;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_CHAR_CONVERT_GENDER_NFY)
CHARACTERID				charId;
BYTE					byGender;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_UPDATE_CHAR_ZENNY_REQ)
HOBJECT					handle;
CHARACTERID				charId;
DWORD					dwZenny;
bool					bIsNew;
BYTE					byStuffUpdateType;
BYTE					byZennyChangeType;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_UPDATE_CHAR_NETPY_REQ)
HOBJECT					handle;
CHARACTERID				charId;
DWORD					dwPoints;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_QUEST_ITEM_CREATE_REQ )
HOBJECT						handle;
CHARACTERID					charId;
BYTE						byRequestType;
uQUEST_ITEM_UPDATE_SUB_DATA	uSubData;
BYTE						byItemCount;
sQUEST_ITEM_UPDATE_DATA		aItems;

END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_QUEST_ITEM_DELETE_REQ )
HOBJECT						handle;
CHARACTERID					charId;
BYTE						byRequestType;
uQUEST_ITEM_UPDATE_SUB_DATA	uSubData;
BYTE						byItemCount;
sQUEST_ITEM_UPDATE_DATA		aItems;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_QUEST_ITEM_MOVE_REQ)
HOBJECT						handle;
CHARACTERID					charId;
TBLIDX						dwSrcTblidx;	// �������� ���� ��� INVALID_TBLIDX
BYTE						bySrcPos;		// �̵��Ǳ� ���� ������ ��ġ
TBLIDX						dwDestTblidx;	// �������� ���� ��� INVALID_TBLIDX
BYTE						byDestPos;		// �̵��Ǳ� ���� �������� ��ġ
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_QUEST_PROGRESS_DATA_CREATE_REQ )
	HOBJECT						handle;
	DWORD						dwTimeStamp;
	CHARACTERID					charId;
	sPROGRESS_QUEST_INFO		progressInfo;
	bool						bIsComplete;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_QUEST_PROGRESS_DATA_DELETE_REQ )
HOBJECT						handle;
CHARACTERID					charId;
QUESTID						questID; // ����Ʈ ���̵�
BYTE						bySlotId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_QUEST_COMPLETE_DATA_UPDATE_REQ )
	HOBJECT						handle;
	DWORD						dwTimeStamp;
	CHARACTERID					charId;
	QUESTID						questID;
	BYTE						byCompleteStatus;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_QUEST_DATA_RESET_REQ )
	HOBJECT						handle;
	DWORD						dwTimeStamp;
	CHARACTERID					charId;
	bool						bCompleteQuestData;	// �Ϸ� ���� ���� ����
	bool						bProgressQuestData;	// ���� ���� ���� ����
	QUESTID						startResetQID;		// ���� ����
	QUESTID						endResetQID;		// ���� ����
	BYTE						byCount;			// Data����
	sCOMPLETE_QUEST_BIT_INFO	asData[eCOMPLETE_QUEST_QUEST_STRUCT_COUNT];	
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_QUEST_STATE_UPDATE_REQ )			// ������ ����
	HOBJECT						handle;
	DWORD						dwTimeStamp;
	CHARACTERID					charId;
	QUESTID						qId;					// Trigger ID(Quest ID)
	WORD						wTSState;				// ���� �� ���� ��
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_QUEST_TSP_UPDATE_REQ )			// TS �������� ����
	HOBJECT						handle;
	DWORD						dwTimeStamp;
	CHARACTERID					charId;
	QUESTID						qId;					// Trigger ID(Quest ID)
	sMAIN_TSP					sMainTSP;				// TS ������
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_QUEST_EXCEPTION_TIMER_UPDATE_REQ )// ���� Ÿ�̸��� ������Ʈ
	HOBJECT						handle;
	DWORD						dwTimeStamp;
	CHARACTERID					charId;
	QUESTID						qId;					// Trigger ID(Quest ID)
	sEXCEPT_TIMER_SLOT			sExceptTimerSlot;		// ���� Ÿ�̸� ����
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_QUEST_SSM_UPDATE_REQ )			// �޸� ������ ������Ʈ
	HOBJECT						handle;
	DWORD						dwTimeStamp;
	CHARACTERID					charId;
	QUESTID						qId;					// Trigger ID(Quest ID)
	BYTE						byIdx;
	DWORD						dwValue;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_QUEST_SERVER_EVENT_UPDATE_REQ )	// ���� �̺�Ʈ �������� ������Ʈ
HOBJECT						handle;
CHARACTERID					charId;
QUESTID						qId;					// Trigger ID(Quest ID)
sSTOC_EVT_DB_DATA			sSvrEvt;				// Server Event Data
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_QUEST_EXC_CLIENT_GROUP_REQ )// ���� �̵� �������� ������Ʈ
	HOBJECT						handle;
	DWORD						dwTimeStamp;
	CHARACTERID					charId;
	QUESTID						qId;					// Trigger ID(Quest ID)
	NTL_TS_TG_ID				tgExcCGroup;			// ������ Ŭ���̾�Ʈ �׷� ���̵�
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_QUEST_INFO_UPDATE_REQ )			// ����Ʈ ���� ��� �������� ������Ʈ
	HOBJECT						handle;
	DWORD						dwTimeStamp;
	CHARACTERID					charId;
	QUESTID						qId;					// Trigger ID(Quest ID)
	NTL_TS_TC_ID				tcId;					// Trigger Container ID
	NTL_TS_TA_ID				taId;					// Trigger Action ID
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_QUICK_SLOT_UPDATE_REQ )
	HOBJECT						handle;
	CHARACTERID					charId;
	TBLIDX						tblidx;					
	BYTE						bySlotID;				// QuickSlot ���̵�
	BYTE						byType;					// �������ΰ� ��ų�ΰ� �ҼȾ׼��ΰ�?
	BYTE						byPlace;
	BYTE						byPos;
	ITEMID						itemID;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_QUICK_SLOT_DEL_REQ )
	HOBJECT						handle;
	CHARACTERID					charId;
	BYTE						bySlotID;				// QuickSlot ���̵�
	bool						bIsServer;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_SAVE_SPAWNED_SUMMON_PET_DATA_REQ )
	HOBJECT						handle;
	CHARACTERID					ownerCharId;
	sSUMMON_PET_DATA			petData;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_SAVE_SPAWNED_ITEM_PET_DATA_REQ )
	HOBJECT						handle;
	CHARACTERID					ownerCharId;
	sITEM_PET_DATA				petData;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_DELETE_SPAWNED_SUMMON_PET_DATA_REQ )
	HOBJECT						handle;
	CHARACTERID					ownerCharId;
	BYTE						byPetIndex;
	BYTE						byAvatarType;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_DELETE_SPAWNED_ITEM_PET_DATA_REQ )
	HOBJECT						handle;
	CHARACTERID					ownerCharId;
	BYTE						byPetIndex;
	BYTE						byAvatarType;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_LOAD_SPAWNED_PET_DATA_REQ )
	HOBJECT						handle;
	CHARACTERID					ownerCharId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_DELETE_ALL_TEMPORARY_PET_DATA_REQ )
	HOBJECT						handle;
	CHARACTERID					ownerCharId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_CHAR_PUNISH_REQ )
	HOBJECT						handle;
	CHARACTERID					charID;
	BYTE						byQueryType;
	CHARACTERID					charTargetID;												// target CharID 	
	ACCOUNTID					AccountTargetID;											// Target AccountID
	BYTE						byPunishType;												// Punish Type
	DWORD						dwPunishAmount;												// How Many Day or Minutes
	WCHAR						awchGMCharName[NTL_MAX_SIZE_USERID + 1];			// GM Char Name
	bool						bDateOption;												// 0: Day 1: Minutes 
	bool						bIsOn;
	SERVERFARMID				serverFarmID;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_TRADE_REQ )
	HOBJECT						handle;	
	HOBJECT						hTarget;
	CHARACTERID					charID;
	CHARACTERID					dstcharID;	// �Ű��� ĳ���� ���̵�
	BYTE						byGiveCount;
	BYTE						byTakeCount;
	sTRADE_INVEN				asGiveData[TRADE_INVEN_MAX_COUNT];
	sTRADE_INVEN				asTakeData[TRADE_INVEN_MAX_COUNT];
	DWORD						dwGiveZenny;	// �� ����
	DWORD						dwTakeZenny;	// ���� ����
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_TUTORIAL_HINT_UPDATE_REQ )
	HOBJECT						handle;
	CHARACTERID					charID;
	DWORD						dwTutorialHint;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_BALL_ITEM_PICK_REQ )
	HOBJECT						handle;
	CHARACTERID					charID;
	HOBJECT						itemhandle;
	TBLIDX						aBallIdx[NTL_ITEM_MAX_DRAGONBALL];
	TBLIDX						junkBallIdx;
	sITEM_DATA					sItemData;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_BALL_ITEM_PARTY_PICK_REQ )
	HOBJECT						handle;
	CHARACTERID					charID;
	HOBJECT						itemhandle;
	TBLIDX						aBallIdx[NTL_ITEM_MAX_DRAGONBALL];
	TBLIDX						junkBallIdx;
	PARTYID						partyID;
	BYTE						byCount;
	CHARACTERID					aCharID[NTL_MAX_MEMBER_IN_PARTY];
	sEMPTY_INVEN				asEmpty[NTL_MAX_MEMBER_IN_PARTY];
	sITEM_DATA					sItemData;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_BALL_ITEM_REWARD_REQ )
	HOBJECT						handle;
	HOBJECT						hTarget;			// Object Handle
	CHARACTERID					charID;
	sITEM_DELETE_DATA			asData[NTL_ITEM_MAX_DRAGONBALL];
	BYTE						byRewardType;		//eDRAGONBALL_REWARD_TYPE
	TBLIDX						skillId;	
	sITEM_DATA					sItemData;
	DWORD						dwZenny;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_PRIVATESHOP_ITEM_BUYING_REQ )
	HOBJECT							handle;
	HOBJECT							hTarget;
	CHARACTERID						charID_From;
	CHARACTERID						charID_To;
	DWORD							dwAllZenny;
	BYTE							byCount;
	sPRIVATESHOP_ITEM_POS_DATA		asPrivateShopItemPos[NTL_MAX_BUY_SHOPPING_CART];
	sINVEN_ITEM_POS_DATA			asEmpty[NTL_MAX_BUY_SHOPPING_CART];
	BYTE							byPrivateShopState; //�� ���¿� ���� ��ó���� �޶�����(������ ���, ���� ��� ������ ���)
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_PRIVATESHOP_ITEM_INSERT_REQ )
	HOBJECT							handle;
	CHARACTERID						charID;
	sPRIVATESHOP_ITEM_POS_DATA		sPrivateShopEmpty;
	sINVEN_ITEM_POS_DATA			sInventoryItemPos;
	DWORD							dwZenny;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_PRIVATESHOP_ITEM_DELETE_REQ )
	HOBJECT							handle;
	CHARACTERID						charID;
	sPRIVATESHOP_ITEM_POS_DATA		sItemPos;
	sINVEN_ITEM_POS_DATA			sInventoryEmpty;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_PRIVATESHOP_ITEM_UPDATE_REQ )
	HOBJECT							handle;
	CHARACTERID						charID;
	DWORD							dwAfterZenny;
	DWORD							dwBeforeZenny;
	BYTE							byFromPos;
	BYTE							byToPos;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_PRIVATESHOP_SHOP_LOADING_REQ )
	HOBJECT						handle;
	CHARACTERID					charID;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_PRIVATESHOP_CREATE_REQ )
	HOBJECT						handle;
	CHARACTERID					charID;
	__int64						nCashShopStartTime; //���� ���� �� ��¥
	__int64						nCashShopEndTime;	//���� �� ������
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_PRIVATESHOP_UPDATE_REQ )
	HOBJECT						handle;
	CHARACTERID					charID;
	WCHAR						wcPrivateShopName[NTL_MAX_PRIVATESHOP_NAME_IN_UNICODE + 1];
	WCHAR						wcNotice[NTL_MAX_PRIVATESHOP_NOTICE_IN_UNICODE + 1];
	bool						bIsOwnerEmpty; //true�̸� ������ �ڸ��� ����	
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_RANKBATTLE_SCORE_UPDATE_REQ )
HOBJECT						handle;
CHARACTERID					charID;
BYTE						byBattleMode;		// eRANKBATTLE_MODE 0:���� 1:��Ƽ
sRANKBATTLE_SCORE_INFO		sScoreInfo;
DWORD						dwMudosaPoint;		// ������ ����Ʈ : update ���� MAX_MUDOSA_POINT ���� Ŭ ��� ������ ����Ʈ ���� MAX_MUDOSA_POINT�� �����Ѵ�.
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_TUTORIAL_DATA_UPDATE_REQ )
HOBJECT						handle;
CHARACTERID					charId;
bool						bTutorialFlag;		// Ʃ�丮�� �÷��� ��
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_TMQ_DAYRECORD_RESET_REQ )
	DWORD						dwDay;				// ����� (ex.20071116) �� ���� ���۵�
	TBLIDX						tmqTblidx;
	BYTE						byDifficult;		// eTIMEQUEST_DIFFICULTY
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_TMQ_DAYRECORD_UPDATE_REQ )
	WORLDID						worldId;		// res ���� TMQ �� �� ���� �ϱ� ����
	PARTYID						partyId;		// res ���� TMQ �� �� ���� �� Ȯ���� �ϱ� ����
	TBLIDX						tmqTblidx;		// tmq tblidx
	BYTE						byDifficult;	// difficult
	DWORD						dwClearTime;	// Clear Time
	BYTE						byMode;			// eTIMEQUEST_MODE
	WCHAR						wszPartyName[NTL_MAX_SIZE_PARTY_NAME + 1];
	BYTE						byMemberCount;
	CHARACTERID					asMemberId[NTL_MAX_MEMBER_IN_PARTY];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_TMQ_DAYRECORD_LIST_REQ )
	HOBJECT						handle;
	CHARACTERID					charId;
	TBLIDX						tmqTblidx;
	BYTE						byDifficult;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_TMQ_DAYRECORD_REQ )
	TBLIDX						tmqTblidx;
	ROOMID						roomId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_MAIL_START_REQ )
	HOBJECT						handle;
	HOBJECT						hObject;	// ���ϼۼ���ž ������Ʈ (��ī��Ʈ���� ����� INVALID_OBJECT )
	CHARACTERID					charID;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_MAIL_SEND_REQ )
	HOBJECT						handle;
	HOBJECT						hObject;	// ���ϼۼ���ž ������Ʈ (��ī��Ʈ���� ����� INVALID_OBJECT )
	CHARACTERID					charID;
	WCHAR						wszName[NTL_MAX_SIZE_CHAR_NAME + 1];	// ���� ĳ�� �̸�
	BYTE						byMailType;	// eMAIL_TYPE	
	sINVEN_ITEM_POS_DATA		sItemData;	// ����������
	DWORD						dwZenny;		// Req or Give Zenny
	BYTE						byDay;		// Valid expiration date up to 10 days
	WCHAR						wszTargetName[NTL_MAX_SIZE_CHAR_NAME + 1];	// ���� ĳ�� �̸�
	BYTE						byTextSize;
	WCHAR						wszText[NTL_MAX_LENGTH_OF_MAIL_MESSAGE + 1];	// ���ϳ���
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_MAIL_READ_REQ )
	HOBJECT						handle;
	HOBJECT						hObject;	// ���ϼۼ���ž ������Ʈ (��ī��Ʈ���� ����� INVALID_OBJECT )
	CHARACTERID					charID;
	MAILID						mailID;	// ���� ���̵�
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_MAIL_DEL_REQ )
	HOBJECT						handle;
	HOBJECT						hObject;	// ���ϼۼ���ž ������Ʈ (��ī��Ʈ���� ����� INVALID_OBJECT )
	CHARACTERID					charID;
	MAILID						mailID;	// ���� ���̵�
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_MAIL_RETURN_REQ )
	HOBJECT						handle;
	HOBJECT						hObject;	// ���ϼۼ���ž ������Ʈ (��ī��Ʈ���� ����� INVALID_OBJECT )
	CHARACTERID					charID;
	MAILID						mailID;	// ���� ���̵�
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_MAIL_RELOAD_REQ )
	HOBJECT						handle;
	HOBJECT						hObject;	// ���ϼۼ���ž ������Ʈ (��ī��Ʈ���� ����� INVALID_OBJECT )
	CHARACTERID					charID;
	bool						bIsSchedule;	// �����ٿ� ���� ���ε� �ΰ� ���� ��û�ΰ�? 0:User 1:������
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_MAIL_LOAD_REQ )
	HOBJECT						handle;
	HOBJECT						hObject;	// ���ϼۼ���ž ������Ʈ (��ī��Ʈ���� ����� INVALID_OBJECT )
	CHARACTERID					charID;
	BYTE						byCount;
	MAILID						aMailID[NTL_MAX_MAIL_SLOT_COUNT];	// ���� ���̵�
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_MAIL_ITEM_RECEIVE_REQ )
	HOBJECT						handle;
	HOBJECT						hObject;
	CHARACTERID					charID;
	MAILID						mailID;
	sEMPTY_INVEN				sInven;
	BYTE						byMailType;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_MAIL_LOCK_REQ )
	HOBJECT						handle;
	HOBJECT						hObject;	// ���ϼۼ���ž ������Ʈ (��ī��Ʈ���� ����� INVALID_OBJECT )
	CHARACTERID					charID;
	MAILID						mailID;	// ���� ���̵�
	bool						bIsLock;		// Lock 1: Unlock: 0
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_MAIL_EVENT_SEND_REQ )
	CHARACTERID 				charID;
	CHARACTERID 				targetCharID;
	BYTE 						byMailType;	// eMAIL_TYPE
	BYTE 						bySenderType;
	sITEM_DATA					sItemData;	// ����������
	DWORD						dwZenny;		// Req or Give Zenny
	BYTE						byDay;		// ��ȿ���ᳯ¥ �ִ� 10��
	WCHAR 						wszSenderName[NTL_MAX_SIZE_CHAR_NAME + 1];	// ĳ�� �̸�
	BYTE						byTextSize;
	WCHAR						wszText[NTL_MAX_LENGTH_OF_MAIL_MESSAGE + 1];	// ���ϳ���
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_MAIL_MULTI_DEL_REQ )
	HOBJECT						handle;
	HOBJECT						hObject;	// ���ϼۼ���ž ������Ʈ (��ī��Ʈ���� ����� INVALID_OBJECT )
	CHARACTERID					charID;
	BYTE						byCount;
	MAILID						aMailID[NTL_MAX_COUNT_OF_MULTI_DEL];	// ���� ���̵�
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_CHAR_AWAY_REQ )
	HOBJECT						handle;
	CHARACTERID					charID;
	bool						bIsAway;		// AwayOn 1: Off: 0
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_CHAR_KEY_UPDATE_REQ )
	HOBJECT						handle;
	CHARACTERID					charID;
	ACCOUNTID					accountID;
	BYTE						byCount;
	sSHORTCUT_UPDATE_DATA		asData[NTL_SHORTCUT_MAX_COUNT];	
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_PORTAL_START_REQ )
	HOBJECT						handle;
	HOBJECT						hNpcHandle;
	CHARACTERID					charID;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_PORTAL_ADD_REQ )
	HOBJECT						handle;
	HOBJECT						hNpcHandle;
	CHARACTERID 				charID;
	PORTALID					PortalID;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_WAR_FOG_UPDATE_REQ )
TBLIDX						contentsTblidx;
CHARACTERID					charID;
sCHAR_WAR_FOG_FLAG			sInfo;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_ITEM_UPGRADE_ALL_REQ )
	HOBJECT						handle;
	CHARACTERID					charID;
	ITEMID						aItemID[NTL_ITEM_UPGRADE_EQUIP_COUNT];
	BYTE						abyGrade[NTL_ITEM_UPGRADE_EQUIP_COUNT];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_UPDATE_PLAYER_REPUTATION_REQ )
	HOBJECT					handle;
	CHARACTERID				charID;
	DWORD					dwReputation;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_GUILD_BANK_LOAD_REQ )
	HOBJECT					handle;
	CHARACTERID				charID;
	GUILDID					guildID;
	HOBJECT					hNpcHandle;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_GUILD_BANK_MOVE_REQ)
	HOBJECT					handle;
HOBJECT					hNpcHandle;
	GUILDID					guildID;
	CHARACTERID				charId;
	ITEMID					srcItemID;
	ITEMID					dstItemId;
	HOBJECT					hSrcItem;
	HOBJECT					hDstItem;
	BYTE					bySrcPlace;
	BYTE					bySrcPos;
	BYTE					byDstPlace;
	BYTE					byDstPos;
	bool					bRestrictUpdate;
	BYTE					bySrcRestrictState;
	BYTE					byDestRestrictState;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_GUILD_BANK_MOVE_STACK_REQ)
	HOBJECT					handle;
HOBJECT					hNpcHandle;
	CHARACTERID				charId;
	GUILDID					guildID;
	ITEMID					srcItemID;
	ITEMID					dstItemID;
	HOBJECT					hSrcItem;
	HOBJECT					hDstItem;
	BYTE					bySrcPlace;
	BYTE					bySrcPos;
	BYTE					byDstPlace;
	BYTE					byDstPos;
	BYTE					byStackCount1;
	BYTE					byStackCount2;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_GUILD_BANK_ZENNY_REQ)
	HOBJECT					handle;
	HOBJECT					npchandle;		// NpcHandle
	CHARACTERID				charId;
	GUILDID					guildID;
	DWORD					dwZenny;		// ���ų� ���� �׼�
	bool					bIsSave;		// 1 �� ���� ��� 0 �� ���°��
	BYTE					byType;			// eDBO_GUILD_ZENNY_UPDATE_TYPE ������ü
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_SHOP_ITEM_IDENTIFY_REQ)
	HOBJECT					handle;
	CHARACTERID				charId;
	ITEMID					itemID; 
	HOBJECT					hNpchandle; 
	BYTE					byPlace; 
	BYTE					byPos; 
	DWORD					dwZenny;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_BUDOKAI_DATA_REQ)
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_BUDOKAI_INIT_DATA_REQ)
	bool							bIsFirstUpdate;		// true : ������ ó�� �����Ͽ� DB�� ����Ÿ�� ���� ���, false : ���ο� ������ �����ϰԵ� ���
	sBUDOKAI_UPDATEDB_INITIALIZE	sInitData;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_BUDOKAI_UPDATE_STATE_REQ)				// Main State
	sBUDOKAI_STATE_DATA				sStateData;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_BUDOKAI_UPDATE_MATCH_STATE_REQ)		// ������ �� ���� State
	BYTE							byMatchType;		// eBUDOKAI_MATCH_TYPE
	sBUDOKAI_MATCHSTATE_DATA		sStateData;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_BUDOKAI_UPDATE_CLEAR_REQ)
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_RANKPOINT_RESET_REQ)
	BYTE					byMinLevel;
	BYTE					byMaxLevel;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_RANKBATTLE_ALLOW_REQ)
	bool					bAllow;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_BUDOKAI_INDIVIDUAL_ALLOW_REGISTER_REQ)
	bool					bAllow;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_BUDOKAI_JOIN_INDIVIDUAL_REQ)
	HOBJECT					handle;
	CHARACTERID				charId;
	float					fPoint;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_BUDOKAI_LEAVE_INDIVIDUAL_REQ)
	HOBJECT					handle;
	CHARACTERID				charId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_BUDOKAI_INDIVIDUAL_SELECTION_REQ)
	// pc�� ���� ���¸� ������Ʈ �ϱ� ���� ����
	BYTE					byWinnerJoinResult;	// = BUDOKAI_JOIN_RESULT_MINORMATCH
	BYTE					byLoserResultCondition;	// JoinResult ���� byLoserResultCondition�� ��� ���ڵ��� JoinState�� byLoserJoinState�� �ٲ۴´�.
	BYTE					byLoserJoinState;	// = BUDOKAI_JOIN_STATE_DROPOUT
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_BUDOKAI_INDIVIDUAL_LIST_REQ)
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_BUDOKAI_TOURNAMENT_INDIVIDUAL_ADD_ENTRY_LIST_REQ)
	WORD					wJoinId;
	BYTE					byMatchIndex;

	// pc�� ���� ���¸� ������Ʈ �ϱ� ���� ����
	BYTE					byWinnerJoinResult;	// = BUDOKAI_JOIN_RESULT_ENTER_32
	BYTE					byLoserResultCondition;	// JoinResult ���� byLoserResultCondition�� ��� ���ڵ��� JoinState�� byLoserJoinState�� �ٲ۴´�.
	BYTE					byLoserJoinState;	// = BUDOKAI_JOIN_STATE_DROPOUT
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_BUDOKAI_TOURNAMENT_INDIVIDUAL_ENTRY_LIST_REQ)
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_BUDOKAI_TOURNAMENT_INDIVIDUAL_ADD_MATCH_RESULT_REQ)
	sBUDOKAI_TOURNAMENT_MATCH_DATA	sMatchData;

	// pc�� ���� ���¸� ������Ʈ �ϱ� ���� ����
	BYTE					byWinnerJoinResult;	// eBUDOKAI_JOIN_RESULT
	BYTE					byLoserResultCondition;	// JoinResult ���� byLoserResultCondition�� ��� ���ڵ��� JoinState�� byLoserJoinState�� �ٲ۴´�.
	BYTE					byLoserJoinState;	// = BUDOKAI_JOIN_STATE_DROPOUT
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_BUDOKAI_TOURNAMENT_INDIVIDUAL_MATCH_RESULT_REQ)
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_BUDOKAI_TEAM_ALLOW_REGISTER_REQ)
	bool					bAllow;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_BUDOKAI_JOIN_TEAM_REQ)
	HOBJECT						handle;
	CHARACTERID					charId;
	WCHAR						wszTeamName[NTL_MAX_LENGTH_BUDOKAI_TEAM_NAME_IN_UNICODE + 1];
	BYTE						byMemberCount;
	CHARACTERID					aMembers[NTL_MAX_MEMBER_IN_PARTY];
	float						fPoint;
	sBUDOKAI_TEAM_POINT_INFO	aTeamInfo[NTL_MAX_MEMBER_IN_PARTY];	// ���Ӽ������� ����ϱ� ���� ����
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_BUDOKAI_LEAVE_TEAM_REQ)			// �� ������ ���
	HOBJECT					handle;
	CHARACTERID				charId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_BUDOKAI_LEAVE_TEAM_MEMBER_REQ)	// �� ����� ���
	HOBJECT					handle;
	CHARACTERID				charId;
	float					fPoint;		// ���� ����Ʈ
	WCHAR					wszMemberName[NTL_MAX_SIZE_CHAR_NAME + 1];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_BUDOKAI_TEAM_SELECTION_REQ)
	// pc�� ���� ���¸� ������Ʈ �ϱ� ���� ����
	BYTE					byWinnerJoinResult;	// = BUDOKAI_JOIN_RESULT_MINORMATCH
	BYTE					byLoserResultCondition;	// JoinResult ���� byLoserResultCondition�� ��� ���ڵ��� JoinState�� byLoserJoinState�� �ٲ۴´�.
	BYTE					byLoserJoinState;	// = BUDOKAI_JOIN_STATE_DROPOUT
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_BUDOKAI_TEAM_LIST_REQ)
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_BUDOKAI_TOURNAMENT_TEAM_ADD_ENTRY_LIST_REQ)
	WORD					wJoinId;
	BYTE					byMatchIndex;

	// pc�� ���� ���¸� ������Ʈ �ϱ� ���� ����
	BYTE					byWinnerJoinResult;	// = BUDOKAI_JOIN_RESULT_ENTER16
	BYTE					byLoserResultCondition;	// JoinResult ���� byLoserResultCondition�� ��� ���ڵ��� JoinState�� byLoserJoinState�� �ٲ۴´�.
	BYTE					byLoserJoinState;	// = BUDOKAI_JOIN_STATE_DROPOUT
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_BUDOKAI_TOURNAMENT_TEAM_ENTRY_LIST_REQ)
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_BUDOKAI_TOURNAMENT_TEAM_ADD_MATCH_RESULT_REQ)
	sBUDOKAI_TOURNAMENT_MATCH_DATA	sMatchData;

	// pc�� ���� ���¸� ������Ʈ �ϱ� ���� ����
	BYTE					byWinnerJoinResult;	// eBUDOKAI_JOIN_RESULT
	BYTE					byLoserResultCondition;	// JoinResult ���� byLoserResultCondition�� ��� ���ڵ��� JoinState�� byLoserJoinState�� �ٲ۴´�.
	BYTE					byLoserJoinState;	// = BUDOKAI_JOIN_STATE_DROPOUT
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_BUDOKAI_TOURNAMENT_TEAM_MATCH_RESULT_REQ)
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_BUDOKAI_JOIN_INFO_REQ)
	HOBJECT					handle;
	CHARACTERID				charId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_BUDOKAI_JOIN_STATE_REQ)
	HOBJECT					handle;
	CHARACTERID				charId;
	bool					bIsClientReq;	// true : Ŭ���̾�Ʈ�� ��û(res ����), false : ���� ������ ��û(nfy ����)
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_BUDOKAI_HISTORY_WRITE_REQ)
	BYTE					byBudokaiType;
	BYTE					byMatchType;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_BUDOKAI_HISTORY_WINNER_PLAYER_REQ)
	WORD					wSeasonCount;			// �ش� season �� player info ��û
	BYTE					byBudokaiType;			// eBUDOKAI_TYPE
	BYTE					byMatchType;			// eBUDOKAI_MATCH_TYPE
	BYTE					byJoinResult;			// JoinResult ���� ũ�ų� ���� ���� �ε��Ѵ�.
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_BUDOKAI_JOIN_STATE_LIST_REQ)
	BYTE					byMatchType;			// eBUDOKAI_MATCH_TYPE
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_BUDOKAI_SET_OPEN_TIME_REQ)
	BYTE					byState;				// eBUDOKAI_STATE
	BUDOKAITIME				tmOpenTime;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_MATCH_REWARD_REQ)
	HOBJECT					hPc;
	CHARACTERID				charId;
	BYTE					byReqType;
	DWORD					dwMudosaPoint;					// plus point
	BYTE					byItemCount;					// create count Max Count 3EA
	sITEM_DATA				aItems[MAX_BUDOKAI_MATCH_REWARD_ITEM_COUNT];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_SCOUTER_ITEM_SELL_REQ)
	HOBJECT					handle;
	CHARACTERID				charId;
	DWORD					dwZenny;
	ITEMID					itemId; 
	BYTE					byPlace;
	BYTE					byPos;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_SHOP_EVENTITEM_BUY_REQ)// [7/14/2008 SGpro]
HOBJECT					handle;
HOBJECT					hNpchandle;
CHARACTERID				charId;
DWORD					dwMudosaPoint;
BYTE					byBuyCount;
BYTE					byDeleteItemCount;
sSHOP_BUY_INVEN			sInven[NTL_MAX_BUY_SHOPPING_CART];
sITEM_DELETE_DATA		asDeleteItem[NTL_MAX_BUY_SHOPPING_CART];
BYTE					byUpdateCount;
sITEM_BASIC_DATA		aUpdateItem[NTL_MAX_BUY_SHOPPING_CART];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_SHOP_GAMBLE_BUY_REQ)// [7/14/2008 SGpro]
	HOBJECT					handle;
	HOBJECT					hNpchandle;
	CHARACTERID				charId;
	bool					bIsZeni;
	DWORD					dwPoint;
	sITEM_DATA				sInven;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_BUDOKAI_UPDATE_MUDOSA_POINT_REQ)		
	HOBJECT					handle;
	CHARACTERID				charId;
	DWORD					dwMudosaPoint;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_ITEM_REPLACE_REQ)// [7/23/2008 SGpro]
	BYTE					byItemProcessType;//eITEM_PROCESS_TYPE
	HOBJECT					handle;
	CHARACTERID				charId;
	sITEM_DELETE_DATA		sDeleteItem;
	sITEM_DATA				sCreateItem;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_UPDATE_SP_POINT_REQ)		
	CHARACTERID				charId;
	DWORD					dwSpPoint;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_UPDATE_MARKING_REQ)
	HOBJECT					handle;
	CHARACTERID				charId;
//	sMARKING				sMarking;
	BYTE					byContentsType;		// eMARKING_CONTENTS_TYPE
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_SKILL_BUY_REQ)
	HOBJECT					handle;
	CHARACTERID				charId;
	HOBJECT					hNpchandle;
	BYTE					bySellType;
	TBLIDX					skillId;
	BYTE					bySlot;
	BYTE					byRpBonusType;
	BYTE					byNewClass;
	DWORD					dwZenny;
	DWORD					dwSP;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_SKILL_INIT_REQ)
HOBJECT					handle;
CHARACTERID				charId;
BYTE					byPlace;
BYTE					byPos;
ITEMID					itemId;
DWORD					dwSP;
DWORD					dwZenny;
BYTE					bySkillResetMethod; // 0 = init by npc / 1 = init by item / 2 = init by class change
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_SKILL_ONE_RESET_REQ)
CHARACTERID				charId;
BYTE					skillIndex;
DWORD					dwAddSP;
TBLIDX					newSkillTblidx;
ITEMID					itemId;
BYTE					byRpBonusType;
bool					bRpBonusAuto;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_RECIPE_REG_REQ)
	HOBJECT			handle;
	CHARACTERID		charId;
	BYTE			byPlace;
	BYTE			byPos;
	ITEMID			itemId;
	TBLIDX			recipeTblidx;
	BYTE			byRecipeType;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_HOIPOIMIX_JOB_SET_REQ )
	HOBJECT				handle;
	CHARACTERID			charId;
	HOBJECT				hNpchandle;
	BYTE				byRecipeType;
	DWORD				dwZeni;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_HOIPOIMIX_JOB_RESET_REQ )
	HOBJECT				handle;
	CHARACTERID			charId;
	HOBJECT				hNpchandle;
	BYTE				byRecipeType;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_HOIPOIMIX_ITEM_MAKE_REQ)			// ������ �����
HOBJECT					handle;
CHARACTERID				charId;
HOBJECT					objHandle;
TBLIDX					recipeTblidx;
DWORD					dwMixExp;				// Subject to change Exp
BYTE					byMixLevel;				// Level changed
bool					bIsNew;					// If the item is true if the nested generate false
DWORD					dwSpendZenny;
sITEM_DATA				sCreateData;			// Produced item
BYTE					byCount;
sITEM_BASIC_DATA		asData[DBO_MAX_COUNT_RECIPE_MATERIAL_ITEM];
DWORD					dwExpGained;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_RUNTIME_PCDATA_SAVE_NFY)
	CHARACTERID		charID;
	DWORD			dwExp;
	TBLIDX			worldTblidx;
	WORLDID			worldId;
	sVECTOR3		vLoc;
	sVECTOR3		vDir;
	DWORD			dwAddPlayTime;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_ITEM_STACK_UPDATE_REQ)
	HOBJECT			handle;
	CHARACTERID		charId;
	ITEMID			itemId;
	HOBJECT			hItem;
	BYTE			byPlace;
	BYTE			byPos;	
	BYTE			byStackCount;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_DOJO_BANK_HISTORY_REQ)
	HOBJECT			handle;
	CHARACTERID		charId;
	GUILDID			guildId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_DOJO_BANK_ZENNY_UPDATE_REQ)
	GUILDID					guildId;
	DWORD					dwZenny;		// ���ų� ���� �׼�
	bool					bIsSave;		// 1 �� ���� ��� 0 �� ���°��
	BYTE					byType;			// eDBO_GUILD_ZENNY_UPDATE_TYPE ������ü
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_WORLD_SCHEDULE_SET_REQ )
	sSCHEDULE_INFO		sInfo;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_SWITCH_CHILD_ADULT )
CHARACTERID				charId;
HOBJECT					hSubject;
bool					bIsAdult;
sTSM_SERIAL				sTSMSerial;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_SET_CHANGE_CLASS_AUTHORITY )
	CHARACTERID				charId;
	HOBJECT					hSubject;
	bool					bCanChangeClass;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_ITEM_CHANGE_ATTRIBUTE_REQ )
HOBJECT			handle;
CHARACTERID		charId;
ITEMID			itemId;
BYTE			byBattleAttribute;
DWORD			dwZeni;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_CHANGE_NETP_REQ )
	ACCOUNTID	accountID;
	CHARACTERID	charId; 
	HOBJECT		handle;
	NETP		netP; 
	BYTE		byUpdateType;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_ITEM_CHANGE_DURATIONTIME_REQ )				
	HOBJECT			handle;
	CHARACTERID		charId;
	sITEM_DATA		sItem;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_SHOP_NETPYITEM_BUY_REQ )
	HOBJECT					handle;
	CHARACTERID				charId;
	NETP					netpyPoint;
	BYTE					byBuyCount;
	sSHOP_BUY_INVEN			sInven[NTL_MAX_BUY_SHOPPING_CART];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_DURATION_ITEM_BUY_REQ )
	HOBJECT					handle;
	CHARACTERID				charId;
	BYTE					byPayType;
	DWORD					dwPayAmount;
	TBLIDX					merchantTblidx;
	BYTE					byPos;
	sITEM_DATA				sItem;		
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_DURATION_RENEW_REQ )
	HOBJECT					handle;
	CHARACTERID				charId;
	ACCOUNTID				accountId;
	BYTE					byPayType;
	DWORD					dwPayAmount;
	HOBJECT					hItemHandle;
	sITEM_DURATION			sData;		
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL( GQ_GM_COMMAND_LOG_REQ )
	SERVERFARMID	serverFarmId;
	char			szAccountID[NTL_MAX_SIZE_USERID + 1];
	char			szCommand[NTL_MAX_LENGTH_OF_CHAT_MESSAGE + 1];
	char			szTargetCharName[NTL_MAX_SIZE_MOB_NAME + 1];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_CASHITEM_HLSHOP_REFRESH_REQ)
HOBJECT			handle;
CHARACTERID		charId;
ACCOUNTID		accountId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_CASHITEM_INFO_REQ)
	HOBJECT			handle;
	CHARACTERID		charId;
	ACCOUNTID		accountId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_CASHITEM_MOVE_REQ)
	HOBJECT			handle;
	CHARACTERID		charId;
	ACCOUNTID		accountId;
	QWORD			qwProductId;
	sSHOP_BUY_INVEN sData;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_CASHITEM_DEL_REQ)
	HOBJECT			handle;
	CHARACTERID		charId;
	ACCOUNTID		accountId;
	QWORD			qwProductId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_CASHITEM_UNPACK_REQ)
	HOBJECT			handle;
	CHARACTERID		charId;
	ACCOUNTID		accountId;
	QWORD			qwProductId;
	BYTE			byCount;			// UnPack�ؼ� �־��� ������ ����
	sCASHITEM_BRIEF	asInfo[1];			// UnPack�ؼ� ���� CASHITEM
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_CASHITEM_BUY_REQ)
HOBJECT			handle;
ACCOUNTID		accountId;
CHARACTERID		characterId;
TBLIDX			HLSitemTblidx;
BYTE			byCount;			// UnPack�ؼ� �־��� ������ ����
DWORD			dwPrice;
DWORD			dwDuration;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_GMT_UPDATE_REQ)
	ACCOUNTID				accountId;
	CHARACTERID				charId;
	HOBJECT					hPc;
	sGAME_MANIA_TIME_DATA	sUpdateData;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_PC_DATA_LOAD_REQ)
	ACCOUNTID				accountId;
	CHARACTERID				charId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_PC_ITEM_LOAD_REQ)
	ACCOUNTID				accountId;
	CHARACTERID				charId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_PC_SKILL_LOAD_REQ)
	ACCOUNTID				accountId;
	CHARACTERID				charId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_PC_BUFF_LOAD_REQ)
	ACCOUNTID				accountId;
	CHARACTERID				charId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_PC_HTB_LOAD_REQ)
	ACCOUNTID				accountId;
	CHARACTERID				charId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_PC_QUEST_ITEM_LOAD_REQ)
	ACCOUNTID				accountId;
	CHARACTERID				charId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_PC_QUEST_COMPLETE_LOAD_REQ)
	ACCOUNTID				accountId;
	CHARACTERID				charId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_PC_QUEST_PROGRESS_LOAD_REQ)
	ACCOUNTID				accountId;
	CHARACTERID				charId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_PC_QUICK_SLOT_LOAD_REQ)
	ACCOUNTID				accountId;
	CHARACTERID				charId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_PC_SHORTCUT_LOAD_REQ)
	ACCOUNTID				accountId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_PC_ITEM_RECIPE_LOAD_REQ)
	ACCOUNTID				accountId;
	CHARACTERID				charId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_PC_GMT_LOAD_REQ)
	ACCOUNTID				accountId;
	DWORD					dwGMTCurTime;
END_PROTOCOL()
//------------------------------------------------------------------


//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_TENKAICHIDAISIJYOU_SERVERDATA_REQ)
	BOOL			bDBConnect;
	ACCOUNTID		accountId;
	CHARACTERID		charId;
	WCHAR			awchItemName[DBO_MAX_LENGTH_ITEM_NAME_TEXT + 1];
	BYTE			byClassType;
	BYTE			byTabType;
	BYTE			byItemType;
	BYTE			byMinLevel;
	BYTE			byMaxLevel;
	BYTE			byRank;
	unsigned int	uiPage;
	BYTE			bySortType;
	bool			bIsSystem;
END_PROTOCOL()
//------------------------------------------------------------------
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_TENKAICHIDAISIJYOU_PERIODEND_REQ)
	CHARACTERID		charId;
	QWORD			nItem;
	BYTE			bySenderType;
	BYTE			byMailType;
	char			chSystem[70 + 1];
	char			chText[NTL_MAX_LENGTH_OF_MAIL_MESSAGE + 1];
	int				nExpiredMinutes;
	unsigned int	nFee;
	HOBJECT			nHandle;
	BOOL			bList;
	ITEMID			itemId;
	DWORD			dwPrice;
END_PROTOCOL()
//------------------------------------------------------------------
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_TENKAICHIDAISIJYOU_SELL_CANCEL_REQ)
	BYTE			byServerChannelIndex;
	BYTE			byServerIndex;
	CHARACTERID		charId;
	ACCOUNTID		accountId;
	QWORD			nItem;
	ITEMID			itemId;
//	BYTE			byMailType;
//	BYTE			bySenderType;
//	char			chSystem[70 + 1];
//	char			chText[NTL_MAX_LENGTH_OF_MAIL_MESSAGE + 1];
//	long long		nExpiredMinutes;
//	HOBJECT			nHandle;
//	unsigned int	nFee;
END_PROTOCOL()
//------------------------------------------------------------------
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_TENKAICHIDAISIJYOU_BUY_REQ)
	BYTE			byServerChannelIndex;
	BYTE			byServerIndex;
	CHARACTERID		charId;
	ACCOUNTID		accountId;
	QWORD			nItem;
	ITEMID			itemId;
	DWORD			dwZeni;	//zeni from player
END_PROTOCOL()
//------------------------------------------------------------------

//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_QUICK_TELEPORT_INFO_LOAD_REQ)
	CHARACTERID	charId;
	BYTE		byPlace;
	BYTE		byPos;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_QUICK_TELEPORT_UPDATE_REQ)
	CHARACTERID				charId;
	sQUICK_TELEPORT_INFO	asData;
	BYTE					byPlace;
	BYTE					byPos;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_QUICK_TELEPORT_DEL_REQ)
	CHARACTERID	charId;
	BYTE		bySlot;
	BYTE		byPlace;
	BYTE		byPos;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_QUICK_TELEPORT_USE_REQ)
	CHARACTERID			charId;
	BYTE				bySlot;
	sSHOP_SELL_INVEN	sData;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_PC_ITEM_COOL_TIME_LOAD_REQ)
	ACCOUNTID		accountId;
	CHARACTERID		charId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_SAVE_ITEM_COOL_TIME_DATA_REQ)
	HOBJECT				hSubject;
	CHARACTERID			charId;
	BYTE				byItemCoolTimeCount;
	sDBO_ITEM_COOL_TIME aItemCoolTimeData[NTL_MAX_ITEM_COOL_DOWN];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_ACCOUNT_BANN)
ACCOUNTID		gmAccountID;
ACCOUNTID		targetAccountID;
char			szReason[NTL_MAX_LENGTH_OF_CHAT_MESSAGE + 1];
BYTE			byDuration; //in days. 255 = perma
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_CHARTITLE_SELECT_REQ)
	CHARACTERID		charId;
	TBLIDX			charTitle;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_CHARTITLE_ADD_REQ)
CHARACTERID		charId;
TBLIDX			charTitle;
BYTE			TitleIndexFlag[NTL_MAX_CHAR_TITLE_COUNT_IN_FLAG];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_CHARTITLE_DELETE_REQ)
CHARACTERID		charId;
TBLIDX			charTitle;
BYTE			TitleIndexFlag[NTL_MAX_CHAR_TITLE_COUNT_IN_FLAG];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_CASHINFOREFRESH_REQ)
ACCOUNTID			accountId;
CHARACTERID			charId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_CASHITEM_SEND_GIFT_REQ)
HOBJECT			handle;
CHARACTERID		SenderCharId;
ACCOUNTID		SenderAccountId;
BYTE			SenderServerFarmId;
WCHAR			wchSenderName[NTL_MAX_SIZE_USERID + 1];
WCHAR			wchDestName[NTL_MAX_SIZE_USERID + 1];
SERVERFARMID	DestServerFarmId;
TBLIDX			dwIdxHlsTable;
BYTE			byCount;
DWORD			dwDuration;
DWORD			dwPrice;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_PC_MASCOT_LOAD_REQ)
CHARACTERID			charId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_MASCOT_REGISTER_REQ)
CHARACTERID			charId;
TBLIDX				tblidx;
SLOTID				slotId;
BYTE				byRank;
DWORD				dwVP;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_MASCOT_DELETE_REQ)
CHARACTERID			charId;
SLOTID				slotId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_MASCOT_SKILL_ADD_REQ)
CHARACTERID			charId;
SLOTID				slotId;
TBLIDX				skillTblidx;
BYTE				byMascotIndex;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_MASCOT_SKILL_UPDATE_REQ)
CHARACTERID			charId;
SLOTID				slotId;
TBLIDX				skillTblidx;
BYTE				byMascotIndex;
BYTE				byItemCount;
ITEMID				itemId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_MASCOT_SKILL_UPGRADE_REQ)
CHARACTERID			charId;
SLOTID				slotId;
TBLIDX				skillTblidx;
BYTE				byMascotIndex;
BYTE				byItemPos;
BYTE				byItemPlace;
ITEMID				itemID;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_MASCOT_FUSION_REQ)
HOBJECT				handle;
CHARACTERID			charId;
bool				bIsSuccess;
BYTE				byNewRank;
DWORD				dwMaxVP;
TBLIDX				nextMascotTblidx;
BYTE				byMascotIndex;
BYTE				byOfferingMascotIndex;
BYTE				byItemPlace;
BYTE				byItemPos;
BYTE				byItemCount;
ITEMID				itemId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_MASCOT_SAVE_DATA_REQ)
CHARACTERID			charId;
BYTE				byCount;
SLOTID				slotId[GMT_MAX_PC_HAVE_MASCOT];
DWORD				dwVp[GMT_MAX_PC_HAVE_MASCOT];
DWORD				dwExp[GMT_MAX_PC_HAVE_MASCOT];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_MATERIAL_DISASSEMBLE_REQ)
HOBJECT					handle;
CHARACTERID				charId;
sITEM_DELETE_DATA		sDeleteItem;
BYTE					byCount;
sITEM_DATA				asCreateItem[ITEM_DISASSEMBLE_MAX_RESULT];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_INVISIBLE_COSTUME_UPDATE_REQ)
CHARACTERID				charId;
bool					bInvisibleCostume;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_ITEM_SOCKET_INSERT_BEAD_REQ)
HOBJECT				handle;
CHARACTERID			charId;
BYTE				byItemPlace;
BYTE				byItemPos;
ITEMID				ItemId;
BYTE				byRestrictState;
BYTE				byDurationType;
BYTE				byBeadPlace;
BYTE				byBeadPos;
ITEMID				BeadItemId;
BYTE				byBeadRemainStack;
sITEM_RANDOM_OPTION	sItemRandomOption[NTL_MAX_OPTION_IN_ITEM];
DBOTIME				nUseStartTime;
DBOTIME				nUseEndTime;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_ITEM_SOCKET_DESTROY_BEAD_REQ)
HOBJECT				handle;
CHARACTERID			charId;
BYTE				byItemPlace;
BYTE				byItemPos;
ITEMID				ItemId;
BYTE				byRestrictState;
BYTE				byDurationType;
bool				bTimeOut;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_CASHITEM_PUBLIC_BANK_ADD_REQ)
ACCOUNTID		accountID;
CHARACTERID		charId;
HOBJECT			handle;
ITEMID			dwProductId;
TBLIDX			hlsItemNo;
TBLIDX			itemNo;
BYTE			byPlace;
BYTE			byPosition;
BYTE			byRank;
BYTE			byDurationType;
DBOTIME			nUseStartTime;
DBOTIME			nUseEndTime;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_ITEM_EXCHANGE_REQ)
HOBJECT				handle;
CHARACTERID			charId;
DWORD				dwZenny;
DWORD				dwMudosapoint;
BYTE				byReplaceCount;
BYTE				byCreateCount;
sITEM_DATA			aCreateItem[ITEM_BULK_CREATE_COUNT]; // ITEM_BULK_CREATE_COUNT
BYTE				byUpdateCount;
sITEM_BASIC_DATA	aUpdateItem[ITEM_BULK_UPDATE_COUNT]; // ITEM_BULK_UPDATE_COUNT
BYTE				byDeleteCount;
sITEM_DELETE_DATA	aDeleteItem[ITEM_BULK_DELETE_COUNT]; // ITEM_BULK_DELETE_COUNT
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_EVENT_COIN_ADD_REQ)
HOBJECT			hPc;
ACCOUNTID		accountId;
CHARACTERID		charId;
BYTE			byPlace;
BYTE			byPos;
ITEMID			itemId;
BYTE			byIncreaseCoin;
ITEMID			spellCastingInfoId;
BYTE			byCoinType;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_WAGUWAGUMACHINE_COIN_INCREASE_REQ)
HOBJECT			handle;
ACCOUNTID		accountId;
CHARACTERID		characterId;
QWORD			qwProductId;
WORD			wWaguCoin;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_CHARACTER_RENAME_REQ)
HOBJECT			handle;
ACCOUNTID		accountId;
CHARACTERID		charId;
WCHAR			wszCharName[NTL_MAX_SIZE_CHAR_NAME + 1];
BYTE			byPlace;
BYTE			byPos;
ITEMID			itemId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_ITEM_CHANGE_OPTION_REQ)
HOBJECT				handle;
CHARACTERID			charId;
BYTE				byItemPlace;
BYTE				byItemPos;
ITEMID				ItemId;
BYTE				byKitPlace;
BYTE				byKitPos;
ITEMID				KitItemId;
BYTE				byKitStack;
sITEM_OPTION_SET	sItemOptionSet;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_ITEM_UPGRADE_WORK_REQ)
HOBJECT				handle;
CHARACTERID			charId;
bool				bIsSuccessful;
bool				bNeedToDestroyItem;
bool				bCoreItemUse;
ITEMID				itemId;
BYTE				byItemPlace;
BYTE				byItemPos;
BYTE				byItemGrade;
ITEMID				stoneId;
BYTE				byStonePlace;
BYTE				byStonePos;
BYTE				byStoneStack;
ITEMID				coreId;
BYTE				byCorePlace;
BYTE				byCorePos;
BYTE				byCoreStack;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_DYNAMIC_FIELD_SYSTEM_COUNTING_REQ)
SERVERFARMID			ServerFarmId;
SERVERCHANNELID			byChannelId;
TBLIDX					tblidx;
int						nAddCount;
DWORD					dwSettingCount;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_ITEM_SEAL_REQ)
HOBJECT					handle;
CHARACTERID				charId;
BYTE					byItemPlace;
BYTE					byItemPos;
ITEMID					itemId;
BYTE					byRestrictState;
BYTE					bySealPlace;
BYTE					bySealPos;
ITEMID					SealitemId;
BYTE					bySealStack;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_ITEM_SEAL_EXTRACT_REQ)
HOBJECT					handle;
CHARACTERID				charId;
BYTE					byItemPlace;
BYTE					byItemPos;
ITEMID					itemId;
BYTE					byRestrictState;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_DRAGONBALL_SCRAMBLE_NFY)
bool				bStart;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_ITEM_UPGRADE_BY_COUPON_REQ)
HOBJECT		handle;
CHARACTERID	charId;
ITEMID		itemId;
ITEMID		couponId;
BYTE		byGrade;
BYTE		byItemPlace;
BYTE		byItemPos;
BYTE		byCouponPlace;
BYTE		byCouponPos;
BYTE		byDurationType;
DBOTIME		nUseStartTime;
DBOTIME		nUseEndTime;
BYTE		byRestrictState;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_ITEM_GRADEINIT_BY_COUPON_REQ)
HOBJECT		handle;
CHARACTERID	charId;
ITEMID		itemId;
BYTE		byItemPlace;
BYTE		byItemPos;
BYTE		byRestrictState;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_EVENT_REWARD_LOAD_REQ)
HOBJECT		handle;
ACCOUNTID	accountId;
CHARACTERID	charId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_EVENT_REWARD_SELECT_REQ)
HOBJECT			handle;
ACCOUNTID		accountId;
CHARACTERID		charId;
TBLIDX			eventTblidx;
sITEM_DATA		sItem;
END_PROTOCOL()
//------------------------------------------------------------------
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_MASCOT_SEAL_SET_REQ)
HOBJECT					handle;
CHARACTERID				charId;
BYTE					byMascotPos;
BYTE					bySealPlace;
BYTE					bySealPos;
ITEMID					SealitemId;
BYTE					bySealStack;
sITEM_DATA				sNewItemData;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_MASCOT_SEAL_CLEAR_REQ)
HOBJECT					handle;
CHARACTERID				charId;
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_GM_LOG)
BYTE			byLogType;
CHARACTERID		charId;
WCHAR			wchBuffer[1024];
END_PROTOCOL()
//------------------------------------------------------------------
BEGIN_PROTOCOL(GQ_CHAR_WAGUPOINT_UPDATE_REQ)
ACCOUNTID		accountId;
CHARACTERID		charId;
DWORD			dwWaguPoints;
END_PROTOCOL()

#pragma pack(pop)
