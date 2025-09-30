//***********************************************************************************
//
//	File		:	NtlAcceptor.h
//
//	Begin		:	2005-12-13
//
//	Copyright	:	�� NTL-Inc Co., Ltd
//
//	Author		:	Hyun Woo, Koo   ( zeroera@ntl-inc.com )
//
//	Desc		:	Network ������ �޾Ƶ��̴� Acceptor Ŭ����
//
//***********************************************************************************

#pragma once

#include "NtlIocp.h"
#include "NtlSocket.h"
#include "NtlSockAddr.h"

#include "NtlMutex.h"
#include "NtlBitFlagManager.h"

class CNtlAcceptingSessionList;
class CNtlNetwork;
class CNtlConnection;
class CNtlAcceptor
{
	friend class CAcceptorThread;

	const int c_CheckTime = 3600;

public:

	CNtlAcceptor();

	virtual ~CNtlAcceptor();


public:


	int						Create(	const char * lpszAddr,
									unsigned short nBasePort,
									unsigned short nRange,
									unsigned short nSelectedPort,
									SESSIONTYPE sessionType,
									int nMaxAcceptCount,
									int nPostAcceptCount,
									int nMinAcceptCount,
									int nCreateAcceptCount);

	void					ChangeListenPort(unsigned short sDestPort);

	void					Destroy();


public:

	int						OnAssociated(CNtlNetwork * pNetwork);

	void					OnAccepted(CNtlConnection * pConnection);

	void					OnDisconnected(bool bConnected);


public:

	void					IncreaseCurAcceptingCount();

	void					DecreaseCurAcceptingCount();

	void					IncreaseCurAcceptedCount();

	void					DecreaseCurAcceptedCount();

	int						GetReserveAcceptCount();

	unsigned short			GetListenSocketSize() { return m_nListenSocketSize; }

	//GetSelectSocketIdx

	SESSIONTYPE				GetSessionType() { return m_sessionType; }


public:

	CNtlSocket &			GetListenSocket() { return m_pArrListenSocket[m_idxSelectedSocket]; }

	CNtlSocket &			GetListenSocketByIdx(int nIdx) { return m_pArrListenSocket[nIdx]; }

	CNtlSockAddr &			GetListenAddr() { return m_listenAddr; }

	const char *			GetListenIP() { return m_listenAddr.GetDottedAddr(); }

	WORD					GetListenPort() { return m_listenAddr.GetPort(); }

	DWORD					GetTotalAcceptCount() { return m_dwTotalAcceptCount; }

	// Diagnostics helpers
	int							GetAcceptingCount() const { return m_nAcceptingCount; }
	int							GetAcceptedCount() const { return m_nAcceptedCount; }


protected:

	void					Init();

	int						Listen();

	int						CreateThread();

	int						ReserveAccept(int nReserveCount);




private:

	CNtlNetwork *			m_pNetwork;

	CNtlSockAddr			m_listenAddr;

	SESSIONTYPE				m_sessionType;


public:

	CNtlSocket*				m_pArrListenSocket;

	unsigned short			m_nListenSocketSize;
	unsigned short			m_idxSelectedSocket;
	unsigned short			m_nPortBase;

	CNtlBitFlagManager		m_BitFlagManager;
	CNtlAcceptingSessionList*	m_pAcceptingSessionList;

	int						m_nMaxAcceptCount;  // Acceptor���� ���ÿ� Accept �� �� �ִ� �ִ� ����

	int						m_nMinAcceptCount; // �� ���� ���� CurAcceptCount�� ������ PostAcceptCount��ŭ Accept��Ų��

	int						m_nPostAcceptCount; // Accept�� Reserve�ϴ� ����

	int						m_nCreateAcceptCount; // ���� ������ Accept�� Reserve�ϴ� ����


	int						m_nAcceptingCount; // ���� Accept�ϰ� �ִ� ����

	int						m_nAcceptedCount; // ���� Accept�� ����

	DWORD					m_dwTotalAcceptCount; 


private:

	CAcceptorThread *		m_pThread;

	CNtlMutex				m_mutex;
};



//-----------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------
inline void	CNtlAcceptor::IncreaseCurAcceptingCount()
{
	CNtlAutoMutex mutex( &m_mutex );
	mutex.Lock();

	++m_nAcceptingCount;
}


inline void CNtlAcceptor::DecreaseCurAcceptingCount()
{
	CNtlAutoMutex mutex( &m_mutex );
	mutex.Lock();

	--m_nAcceptingCount;
}


inline void CNtlAcceptor::IncreaseCurAcceptedCount()
{
	CNtlAutoMutex mutex( &m_mutex );
	mutex.Lock();

	++m_nAcceptedCount;
}


inline void CNtlAcceptor::DecreaseCurAcceptedCount()
{
	CNtlAutoMutex mutex( &m_mutex );
	mutex.Lock();

	--m_nAcceptedCount;
}


inline void CNtlAcceptor::OnDisconnected(bool bConnected)
{
	if( bConnected )
		DecreaseCurAcceptedCount();
	else
		DecreaseCurAcceptingCount();
}

//-----------------------------------------------------------------------------------