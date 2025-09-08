#pragma once

#include "NtlSession.h"
#include "NtlSharedType.h"

// #include "NtlPacketEncoder_RandKeyNew.h"

class CNtlPacket;
class CAuthServer;

class CClientSession : public CNtlSession
{
public:
	CClientSession(bool bAliveCheck = true, bool bOpcodeCheck = false)
		: CNtlSession(SESSION_CLIENT)
		, AccountID(0)
		, m_byLoginTrys(0)
		, m_authenticated(false)
		, m_createTick(0)
		, m_lastActivityTick(0)
	{
		if (bAliveCheck)
			SetControlFlag(CONTROL_FLAG_CHECK_ALIVE);
		if (bOpcodeCheck)
			SetControlFlag(CONTROL_FLAG_CHECK_OPCODE);

		// SetControlFlag(CONTROL_FLAG_LIMITED_RECV_SIZE);
		// SetPacketEncoder(&m_packetEncoder);
	}

	~CClientSession();

public:
	// ---- NtlSession overrides ----
	virtual DWORD GetMaxRecvPacketCount() { return 10; }
	virtual DWORD GetMaxSendPacketCount() { return 10; }
	virtual DWORD GetAliveCheckTime() { return 60000 * 3; } // 3 Minutes

	virtual int  OnAccept();
	virtual void OnClose();
	virtual int  OnDispatch(CNtlPacket* pPacket);

	// ---- API existente ----
public:
	void SendCharLogInReq(CNtlPacket* pPacket, CAuthServer* app);
	void SendLoginDcReq(CNtlPacket* pPacket, CAuthServer* app);

public:
	uint32_t GetRemoteIPv4() const { return m_remoteIpHostOrder; }

	// Timestamps
	ULONGLONG GetCreateTick()      const { return m_createTick; }
	ULONGLONG GetLastActivityTick()const { return m_lastActivityTick; }

	bool      IsAuthenticated()     const { return m_authenticated; }
	void      SetAuthenticated(bool v = true) { m_authenticated = v; }

	void      RefreshActivity() { m_lastActivityTick = GetTickCount64(); }
	
	ACCOUNTID GetAccountID()        const { return AccountID; }
	void      SetAccountID(ACCOUNTID id) { AccountID = id; }

private:
	// CNtlPacketEncoder_RandKeyNew m_packetEncoder;

	ACCOUNTID AccountID;
	BYTE      m_byLoginTrys;

	bool      m_authenticated;

	ULONGLONG m_createTick;
	ULONGLONG m_lastActivityTick;	
	uint32_t m_remoteIpHostOrder = 0;
};
