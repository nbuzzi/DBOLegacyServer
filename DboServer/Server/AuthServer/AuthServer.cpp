//-----------------------------------------------------------------------------------
//      Auth Server
//-----------------------------------------------------------------------------------

#include "stdafx.h"
#include "AuthServer.h"
#include "mysql.h"

#define _WINSOCKAPI_     
#include <winsock2.h>
#include <windows.h>

#include <netfw.h>
#include <comutil.h>

// ATL para CComPtr
#include <atlbase.h>
#include <atlcomcli.h>

// STL / Boost
#include <atomic>
#include <algorithm>
#include <vector>
#include <string>
#include <functional>
#include <cstdio>
#include <cwchar>

#include <boost/unordered_map.hpp>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "comsuppw.lib") // unicode


Database* db_acc;
Database* db_log;

// -------------------------------------------
// Timeouts (configurables)
// -------------------------------------------
static const ULONGLONG LOGIN_TIMEOUT_MS = 60ull * 1000ull;   // 60 s para completar login
static const ULONGLONG IDLE_TIMEOUT_MS = 90ull * 1000ull;   // 90 s de inactividad post-login
static const ULONGLONG EVENT_TICK_MS = 1000ull;           // 1 s para eventos
static const ULONGLONG QUERY_TASK_PERIOD_MS = 3ull;              // ~3 ms para QueryTaskRun
static const ULONGLONG LOOP_WARN_THRESHOLD = 1000ull;           // warn si el loop salta > 1000 ms

// --- Rate limit / abuso por IP ---
static const uint16_t MAX_CONNS_PER_IP = 10;     // max ip connection
static const uint16_t MAX_FAILS_PER_2M = 20;     // si una IP falla 20 logins en 2 minutos -> block
static const ULONGLONG FAIL_WINDOW_MS = 2ull * 60ull * 1000ull;  // 2 minutos
static const ULONGLONG BLOCK_COOLDOWN_MS = 2ull * 60ull * 1000ull;  // 2 minutos antes de reintentar bloquear

// contadores por IP (host-order)
static boost::unordered_map<uint32_t, uint16_t> g_failByIp;        // fails acumulados en ventana
static boost::unordered_map<uint32_t, ULONGLONG> g_failWinStart;   // inicio de ventana por IP
static boost::unordered_map<uint32_t, ULONGLONG> g_ipCooldownUntil;// cooldown para no spamear bloqueos


// Verbosity
enum Verbosity : int { QUIET = 0, NORMAL = 1, VERBOSE = 2, DEBUGV = 3 };
static std::atomic<int> gVerbosity{ NORMAL };

struct ComInitRAII {
    HRESULT hr;
    ComInitRAII() : hr(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)) {}
    ~ComInitRAII() { if (SUCCEEDED(hr)) CoUninitialize(); }
};

#define VLOG(level, fmt, ...) \
    do { if (gVerbosity.load() >= (level)) NTL_PRINT(PRINT_APP, fmt, __VA_ARGS__); } while(0)


static inline const char* IpToStr(uint32_t ipHostOrder, char buf[16]) {
    unsigned char b1 = (ipHostOrder >> 24) & 0xFF;
    unsigned char b2 = (ipHostOrder >> 16) & 0xFF;
    unsigned char b3 = (ipHostOrder >> 8) & 0xFF;
    unsigned char b4 = (ipHostOrder) & 0xFF;
    snprintf(buf, 16, "%u.%u.%u.%u", b1, b2, b3, b4);
    return buf;
}

static std::wstring ToWStringIpHostOrder(uint32_t ipHost)
{
    wchar_t buf[32];
    unsigned b1 = (ipHost >> 24) & 0xFF;
    unsigned b2 = (ipHost >> 16) & 0xFF;
    unsigned b3 = (ipHost >> 8) & 0xFF;
    unsigned b4 = (ipHost) & 0xFF;
    swprintf(buf, 32, L"%u.%u.%u.%u", b1, b2, b3, b4);
    return std::wstring(buf);
}

// Config de m��tricas
static const ULONGLONG NETSTATS_PERIOD_MS = 60ull * 1000ull;  // each 60s
static const size_t     NETSTATS_TOP_IPS = 10;               // top 10
static const size_t     NETSTATS_WARN_CONN = 500;              // umbral alert

struct NetStatsSnapshot {
    ULONGLONG now = 0;
    uint64_t totalSessions = 0;
    uint64_t authed = 0;
    uint64_t loggingIn = 0;
    uint64_t idleOver30s = 0;
    uint64_t idleOver60s = 0;
    boost::unordered_map<uint32_t, uint32_t> connsByIp; // host-order IP -> count
    uint64_t maxHistorical = 0;
};

static std::atomic<uint64_t> gMaxHistoricalConns{ 0 };

enum eCloseReason
{
    CLOSE_NONE = 0,
    CLOSE_LOGIN_TIMEOUT,
    CLOSE_IDLE_TIMEOUT
};

static const wchar_t* FW_RULE_PREFIX = L"DBO-Auth-Block-";

// Creates rule "DBO-Auth-Block-<IP>-20200"
static std::wstring MakeRuleName(const std::wstring& ip, uint16_t port) {
    wchar_t buf[32]; swprintf(buf, 32, L"-%hu", port);
    return std::wstring(FW_RULE_PREFIX) + ip + buf;
}

// Block IP
bool Firewall_BlockIp(const std::wstring& ip, uint16_t port /*=20200*/)
{
    ComInitRAII com;
    if (FAILED(com.hr)) {
        NTL_PRINT(PRINT_APP, "[FIREWALL] CoInitializeEx failed 0x%08X", com.hr);
        return false;
    }

    CComPtr<INetFwPolicy2> policy;
    HRESULT hr = CoCreateInstance(__uuidof(NetFwPolicy2), NULL, CLSCTX_INPROC_SERVER,
        __uuidof(INetFwPolicy2), (void**)&policy);
    if (FAILED(hr)) {
        NTL_PRINT(PRINT_APP, "[FIREWALL] CoCreateInstance(NetFwPolicy2) failed 0x%08X", hr);
        return false;
    }

    // Create name and fields
    std::wstring ruleName = MakeRuleName(ip, port);
    _bstr_t bstrRuleName(ruleName.c_str());
    _bstr_t bstrDesc(L"Auto-block by AuthServer (too many attempts)");
    _bstr_t bstrRemote(ip.c_str());
    _bstr_t bstrPorts(std::to_wstring(port).c_str());

    CComPtr<INetFwRules> rules;
    hr = policy->get_Rules(&rules);
    if (FAILED(hr)) {
        NTL_PRINT(PRINT_APP, "[FIREWALL] get_Rules failed 0x%08X", hr);
        return false;
    }

    CComPtr<INetFwRule> ruleExisting;
    hr = rules->Item(bstrRuleName, &ruleExisting);
    if (SUCCEEDED(hr) && ruleExisting) {
        NTL_PRINT(PRINT_APP, "[FIREWALL] Rule already exists for %S:%hu", ip.c_str(), port);
        return true; // already blocked
    }

    // Creating rule to block attacker
    CComPtr<INetFwRule> rule;
    hr = CoCreateInstance(__uuidof(NetFwRule), NULL, CLSCTX_INPROC_SERVER,
        __uuidof(INetFwRule), (void**)&rule);
    if (FAILED(hr)) {
        NTL_PRINT(PRINT_APP, "[FIREWALL] CoCreateInstance(NetFwRule) failed 0x%08X", hr);
        return false;
    }

    rule->put_Name(bstrRuleName);
    rule->put_Description(bstrDesc);
    rule->put_Direction(NET_FW_RULE_DIR_IN);
    rule->put_Enabled(VARIANT_TRUE);
    rule->put_Action(NET_FW_ACTION_BLOCK);
    rule->put_Profiles(NET_FW_PROFILE2_ALL);
    rule->put_Protocol(NET_FW_IP_PROTOCOL_TCP);
    rule->put_LocalPorts(bstrPorts);         
    rule->put_RemoteAddresses(bstrRemote);   

    hr = rules->Add(rule);
    if (FAILED(hr)) {
        NTL_PRINT(PRINT_APP, "[FIREWALL] rules->Add failed 0x%08X", hr);
        return false;
    }

    NTL_PRINT(PRINT_APP, "[FIREWALL] Blocked %S on TCP %hu (rule: %S)",
        ip.c_str(), port, ruleName.c_str());
    return true;
}

// Unblock IP
bool Firewall_UnblockIp(const std::wstring& ip, uint16_t port /*=20300*/)
{
    ComInitRAII com;
    if (FAILED(com.hr)) return false;

    CComPtr<INetFwPolicy2> policy;
    HRESULT hr = CoCreateInstance(__uuidof(NetFwPolicy2), NULL, CLSCTX_INPROC_SERVER,
        __uuidof(INetFwPolicy2), (void**)&policy);
    if (FAILED(hr)) return false;

    CComPtr<INetFwRules> rules;
    hr = policy->get_Rules(&rules);
    if (FAILED(hr)) return false;

    std::wstring ruleName = MakeRuleName(ip, port);
    _bstr_t bstrRuleName(ruleName.c_str());

    hr = rules->Remove(bstrRuleName);
    if (SUCCEEDED(hr)) {
        NTL_PRINT(PRINT_APP, "[FIREWALL] Unblocked %S on TCP %hu (removed rule)", ip.c_str(), port);
        return true;
    }
    else {
        NTL_PRINT(PRINT_APP, "[FIREWALL] Remove rule failed (maybe not exists) 0x%08X", hr);
        return false;
    }
}


static inline void ForEachClientSession(std::function<void(CClientSession*)> fn);

static inline void CloseSession(CClientSession* s, eCloseReason reason);

// ----------------------------------------------------------------------------------

void CAuthServer::OnLoginFailure(CClientSession* sess)
{
    if (!sess) return;

    const ULONGLONG now = GetTickCount64();
    uint32_t ip = sess->GetRemoteIPv4(); // host-order esperado (si es net-order => ntohl)

    if (ip == 0) return; // sin IP -> nada que hacer

    // Ventana de 5 min: reinicia si ya venci��
    ULONGLONG& winStart = g_failWinStart[ip];
    if (winStart == 0 || (now - winStart) > FAIL_WINDOW_MS) {
        winStart = now;
        g_failByIp[ip] = 0;
    }

    // Suma fallo
    uint16_t fails = ++g_failByIp[ip];

    NTL_PRINT(PRINT_APP, "[ABUSE] Login failed from ip=%S (fails=%hu in window %.0fs)",
        ToWStringIpHostOrder(ip).c_str(),
        fails,
        double(FAIL_WINDOW_MS) / 1000.0);

    // Si excede el umbral y no est�� en cooldown de bloqueo -> bloquear firewall
    if (fails >= MAX_FAILS_PER_2M) {
        ULONGLONG& cool = g_ipCooldownUntil[ip];
        if (cool == 0 || now >= cool) {
            cool = now + BLOCK_COOLDOWN_MS; // to don't spam

            std::wstring ipw = ToWStringIpHostOrder(ip);
            uint16_t port = m_config.wClientAcceptPort; 

            if (Firewall_BlockIp(ipw, port)) {
                NTL_PRINT(PRINT_APP, "[ABUSE] IP %S blocked on TCP %hu due to excessive failures", ipw.c_str(), port);
            }
            else {
                NTL_PRINT(PRINT_APP, "[ABUSE] Failed to block IP %S on TCP %hu (firewall API error)", ipw.c_str(), port);
            }

            // Reset IP count for that IP
            g_failByIp[ip] = 0;
            winStart = now; // We can use 0 to start again with that 0 attemps
        }
    }
}

void CAuthServer::LogNetStats()
{
    NetStatsSnapshot s;
    s.now = GetTickCount64();

    ForEachClientSession([&](CClientSession* c) {
        if (!c) return;

        s.totalSessions++;

        if (c->IsAuthenticated()) s.authed++;
        else                      s.loggingIn++;

        ULONGLONG idle = s.now - c->GetLastActivityTick();
        if (idle >= 30'000) s.idleOver30s++;
        if (idle >= 60'000) s.idleOver60s++;

        // IP
        uint32_t ip = 0;

        ip = c->GetRemoteIPv4();
        if (ip != 0) {
            s.connsByIp[ip]++;
        }
        });

    // Historic max
    uint64_t prevMax = gMaxHistoricalConns.load();
    if (s.totalSessions > prevMax) {
        gMaxHistoricalConns.store(s.totalSessions);
        s.maxHistorical = s.totalSessions;
    }
    else {
        s.maxHistorical = prevMax;
    }

    // Top IPs
    std::vector<std::pair<uint32_t, uint32_t>> top;
    top.reserve(s.connsByIp.size());
    for (auto& kv : s.connsByIp) {
        top.emplace_back(kv.first, kv.second);
    }
    std::sort(top.begin(), top.end(), [](auto& a, auto& b) { return a.second > b.second; });
    if (top.size() > NETSTATS_TOP_IPS) top.resize(NETSTATS_TOP_IPS);

    // Summary log
    VLOG(NORMAL, "[NET] Sessions: total=%llu, authed=%llu, login=%llu, idle>=30s=%llu, idle>=60s=%llu, maxHistorical=%llu",
        (unsigned long long)s.totalSessions, (unsigned long long)s.authed, (unsigned long long)s.loggingIn,
        (unsigned long long)s.idleOver30s, (unsigned long long)s.idleOver60s, (unsigned long long)s.maxHistorical);

    // Simple alerts
    if (s.totalSessions >= NETSTATS_WARN_CONN) {
        NTL_PRINT(PRINT_APP, "[ALERT] High concurrent sessions: %llu (>= %zu)",
            (unsigned long long)s.totalSessions, NETSTATS_WARN_CONN);
    }

    // Top IPs
    if (!top.empty() && gVerbosity.load() >= VERBOSE) {
        NTL_PRINT(PRINT_APP, "[NET] Top %zu remote IPs:", top.size());
        size_t rank = 1;
        for (auto& e : top) {
            char ipbuf[16];
            NTL_PRINT(PRINT_APP, "  #%zu  %s  ->  %u conns",
                rank++, IpToStr(e.first, ipbuf), e.second);
        }
    }
}

CAuthServer::CAuthServer()
{
    m_pMasterServerSession = NULL;
    m_pClientAcceptor = NULL;
    m_pServerConnector = NULL;
}

CAuthServer::~CAuthServer()
{
}

int CAuthServer::OnInitApp()
{
    m_nMaxSessionCount = m_config.nMaxConnection + 2;

    NTL_PRINT(PRINT_APP, "Init Timed-Event Manager");
    EventMgr* m_pEventMgr = new EventMgr;
    UNREFERENCED_PARAMETER(m_pEventMgr);

    NTL_PRINT(PRINT_APP, "INIT NEIGHBOR SERVER INFO MANAGER");
    m_pNeighborServerInfoManager = new CSubNeighborServerInfoManager;
    g_pServerInfoManager->Create(NTL_SERVER_TYPE_AUTH);

    m_pSessionFactory = new CAuthSessionFactory;
    if (NULL == m_pSessionFactory)
    {
        return NTL_ERR_SYS_MEMORY_ALLOC_FAIL;
    }

    return NTL_SUCCESS;
}

int CAuthServer::OnCreate()
{
    return NTL_SUCCESS;
}

void CAuthServer::OnDestroy()
{
}

int CAuthServer::OnAppStart()
{
    int rc = NTL_SUCCESS;

    // client acceptor
    {
        std::unique_ptr<CNtlAcceptor> clientAcceptor(new CNtlAcceptor);
        if (clientAcceptor.get())
        {
            rc = clientAcceptor->Create(
                m_config.strClientAcceptAddr.c_str(),
                m_config.wClientAcceptPort,
                1,
                m_config.wClientAcceptPort,
                SESSION_CLIENT,
                m_config.nMaxConnection,
                m_config.nMaxConnection,
                m_config.nMaxConnection,
                m_config.nMaxConnection);
            if (NTL_SUCCESS != rc)
            {
                return rc;
            }

            rc = GetNetwork()->Associate(clientAcceptor.get(), true);
            if (NTL_SUCCESS != rc)
            {
                return rc;
            }

            m_pClientAcceptor = clientAcceptor.release();
        }
        else
        {
            return 100003;
        }
    }

    // connector to master server
    {
        std::unique_ptr<CNtlConnector> masterConnector(new CNtlConnector);
        if (masterConnector.get())
        {
            rc = masterConnector->Create(
                m_config.strMasterServerIP.c_str(),
                m_config.wMasterServerPort,
                SESSION_SERVER_CON_AUTH_TO_MASTER,
                INVALID_DWORD,
                INVALID_DWORD);
            if (NTL_SUCCESS != rc)
            {
                return rc;
            }

            rc = GetNetwork()->Associate(masterConnector.get(), true);
            if (NTL_SUCCESS != rc)
            {
                return rc;
            }

            m_pServerConnector = masterConnector.release();
        }
        else
        {
            return 100003;
        }
    }

    g_pServerInfoManager->StartEvents();

    return NTL_SUCCESS;
}

bool CAuthServer::AddPlayer(ACCOUNTID AccID, CClientSession* session)
{
    boost::unordered_map<ACCOUNTID, CClientSession*>::iterator it = m_map_Players.find(AccID);
    if (it == m_map_Players.end())
    {
        m_map_Players[AccID] = session;
        return true;
    }

    return false;
}

void CAuthServer::DelPlayer(ACCOUNTID AccID)
{
    boost::unordered_map<ACCOUNTID, CClientSession*>::iterator it = m_map_Players.find(AccID);
    if (it == m_map_Players.end())
    {
        return;
    }

    m_map_Players.erase(AccID);
}

CClientSession* CAuthServer::FindPlayer(ACCOUNTID AccID)
{
    boost::unordered_map<ACCOUNTID, CClientSession*>::iterator it = m_map_Players.find(AccID);
    if (it == m_map_Players.end())
    {
        // NTL_PRINT(PRINT_APP,"[CAuthServer::FindPlayer] %d not found", AccID);
        return NULL;
    }

    return it->second;
}

bool CAuthServer::IsAccountTempBlocked(const char* strUsername)
{
    std::map<std::string, QWORD>::iterator it = m_mapBlockedAccounts.find(strUsername);
    if (it != m_mapBlockedAccounts.end())
    {
        QWORD curTick64 = GetTickCount64();

        if (curTick64 < it->second)
        {
            return true;
        }
        else
        {
            m_mapBlockedAccounts.erase(it);
        }
    }

    return false;
}

void CAuthServer::RegisterAccountTempBann(const char* strUsername)
{
    m_mapBlockedAccounts.insert({ strUsername, GetTickCount64() + 300000 }); // 5 minutes
}

// ---------------------------------------------------------
// Housekeeping: enforce timeouts (login + idle post-login)
// ---------------------------------------------------------
void CAuthServer::TickHousekeeping()
{
    const ULONGLONG now = GetTickCount64();

    ForEachClientSession([&](CClientSession* s)
        {
            if (!s) return;

            const ULONGLONG createTick = s->GetCreateTick();
            const ULONGLONG lastTick = s->GetLastActivityTick(); 
            const bool authed = s->IsAuthenticated();

            // 1) Login timeout
            if (!authed && (now - createTick >= LOGIN_TIMEOUT_MS))
            {
                NTL_PRINT(PRINT_APP, "[TIMEOUT] Closing session (login deadline) create=%llu now=%llu",
                    (unsigned long long)createTick, (unsigned long long)now);
                CloseSession(s, CLOSE_LOGIN_TIMEOUT);
                return;
            }

            // 2) Idle post-login
            if (authed && (now - lastTick >= IDLE_TIMEOUT_MS))
            {
                NTL_PRINT(PRINT_APP, "[TIMEOUT] Closing session (idle) lastActivity=%llu now=%llu",
                    (unsigned long long)lastTick, (unsigned long long)now);
                CloseSession(s, CLOSE_IDLE_TIMEOUT);
                return;
            }
        });
}

void CAuthServer::Run()
{
    ULONGLONG now = 0;
    ULONGLONG lastLoopTs = GetTickCount64();
    ULONGLONG lastEventTs = GetTickCount64();
    ULONGLONG lastQueryTs = GetTickCount64();
    ULONGLONG lastStatsTs = GetTickCount64();

    while (IsRunnable())
    {
        now = GetTickCount64();

        if (now - lastQueryTs >= QUERY_TASK_PERIOD_MS) {
            GetAccDB.QueryTaskRun();
            lastQueryTs = now;
        }

        if (now - lastEventTs >= EVENT_TICK_MS) {
            const ULONGLONG diff = now - lastEventTs;
            g_pServerInfoManager->TickProcess(static_cast<DWORD>(diff));
            TickHousekeeping(); // login/idle timeouts
            lastEventTs = now;
        }

        // --- Log metrics each  60s ---
        if (now - lastStatsTs >= NETSTATS_PERIOD_MS) {
            LogNetStats();
            lastStatsTs = now;
        }

        const ULONGLONG loopNow = GetTickCount64();
        if (loopNow - now > LOOP_WARN_THRESHOLD) {
            NTL_PRINT(PRINT_APP, "Loop lag: now %llu - loopNow %llu = %llu > %llu ms. timeGetTime = %u",
                (unsigned long long)now, (unsigned long long)loopNow,
                (unsigned long long)(loopNow - now), (unsigned long long)LOOP_WARN_THRESHOLD, timeGetTime());
            ERR_LOG(LOG_GENERAL, "Loop lag: now %llu - loopNow %llu = %llu > %llu ms. timeGetTime = %u",
                (unsigned long long)now, (unsigned long long)loopNow,
                (unsigned long long)(loopNow - now), (unsigned long long)LOOP_WARN_THRESHOLD, timeGetTime());
        }

        lastLoopTs = loopNow;
        Wait(1);
    }
}

//-----------------------------------------------------------------------------------
//      Purpose :
//      Return  :
//-----------------------------------------------------------------------------------
BOOL CAuthServer::OnCommandInput(std::string& sCmd)
{
    UNREFERENCED_PARAMETER(sCmd);
    // Comandos debug opcionales aqu��
    return TRUE;
}

int main(int argc, _TCHAR* argv[])
{
    CAuthServer app;
    CNtlFileStream traceFileStream;

    SYSTEMTIME ti;
    GetLocalTime(&ti);

    SetConsoleTitle(TEXT("AuthServer"));

    // CHECK INI FILE AND START PROGRAM
    int rc = app.Create(argc, argv, ".\\config\\AuthServer.ini");
    if (NTL_SUCCESS != rc)
    {
        NTL_PRINT(PRINT_APP, "Server Application Create Fail %d(%s)", rc, NtlGetErrorMessage(rc));
        return rc;
    }

    // LOG FILE
    char m_LogFile[256];
    sprintf(m_LogFile, ".\\logs\\authserver\\log_%02u-%02u-%02u.txt", ti.wYear, ti.wMonth, ti.wDay);

    rc = traceFileStream.Create(m_LogFile);
    if (NTL_SUCCESS != rc)
        return rc;

    app.m_log.AttachLogStream(traceFileStream.GetFilePtr());
    NtlSetPrintFlag(PRINT_APP | PRINT_SYSTEM);

    // CONNECT TO MYSQL DATABASE
    NTL_PRINT(PRINT_APP, "CONNECTING TO DATABASE");

    db_acc = Database::CreateDatabaseInterface(1);
    if (!GetAccDB.Initialize(app.GetDatabaseHost(), app.GetDatabasePort(), app.GetDatabaseUser(),
        app.GetDatabasePassword(), app.GetDatabaseName(), 5))
    {
        NTL_PRINT(PRINT_APP, "sql : dbo_acc database initialization failed. Exiting.");
        Sleep(5000);
        return 0;
    }

    db_log = Database::CreateDatabaseInterface(1);
    if (!GetLogDB.Initialize(app.GetDatabaseHost(), app.GetDatabasePort(), app.GetDatabaseUser(),
        app.GetDatabasePassword(), "dbo_log", 5))
    {
        NTL_PRINT(PRINT_APP, "sql : dbo_log database initialization failed. Exiting.");
        Sleep(5000);
        return 0;
    }

    NTL_PRINT(PRINT_APP, "CONNECT TO DATABASE SUCCESS");

    unsigned int mysqlthreadsafe = mysql_thread_safe();
    if (!mysqlthreadsafe)
        NTL_PRINT(PRINT_APP, "mysql lib is not a thread safe mode!!!!!!!!!");

    Database::StartThread();

    app.Start();
    NTL_PRINT(PRINT_APP, "AUTH SERVER STARTED");
    app.WaitCommandInput();
    app.WaitForTerminate();
    return 0;
}


static inline void ForEachClientSession(std::function<void(CClientSession*)> fn)
{

    UNREFERENCED_PARAMETER(fn);
}

static inline void CloseSession(CClientSession* s, eCloseReason reason)
{
    NTL_PRINT(PRINT_APP, "[Close] reason=%d session=%p", (int)reason, s);
    s->Disconnect(false);
    UNREFERENCED_PARAMETER(s);
    UNREFERENCED_PARAMETER(reason);
}
