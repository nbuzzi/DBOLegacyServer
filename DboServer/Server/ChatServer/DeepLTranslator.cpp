#include "stdafx.h"
#include "DeepLTranslator.h"

#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

#include <vector>
#include <sstream>
#include <iomanip>

// Simple UTF conversions leveraging WideCharToMultiByte/MultiByteToWideChar (UTF-8)
std::string DeepLTranslator::WToUtf8(const std::wstring& ws)
{
    if (ws.empty()) return std::string();
    int sz = ::WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), nullptr, 0, nullptr, nullptr);
    std::string out(sz, 0);
    ::WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), &out[0], sz, nullptr, nullptr);
    return out;
}

std::wstring DeepLTranslator::Utf8ToW(const std::string& s)
{
    if (s.empty()) return std::wstring();
    int sz = ::MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring out(sz, 0);
    ::MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &out[0], sz);
    return out;
}

std::string DeepLTranslator::UrlEncodeUtf8(const std::string& s)
{
    std::ostringstream oss;
    for (unsigned char c : s)
    {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~')
            oss << c;
        else if (c == ' ') oss << "%20";
        else {
            oss << '%' << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << (int)c << std::nouppercase << std::dec;
        }
    }
    return oss.str();
}

static bool HttpPostWinHttp(const std::wstring& host, INTERNET_PORT port, const std::wstring& path, const std::wstring& headers, const std::string& body, std::string& out)
{
    bool ok = false;
    HINTERNET hSession = nullptr, hConnect = nullptr, hRequest = nullptr;
    do
    {
        hSession = WinHttpOpen(L"DBOChat/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (!hSession) break;
        hConnect = WinHttpConnect(hSession, host.c_str(), port, 0);
        if (!hConnect) break;
        hRequest = WinHttpOpenRequest(hConnect, L"POST", path.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
        if (!hRequest) break;
        DWORD dwTimeout = 4000;
        WinHttpSetTimeouts(hRequest, dwTimeout, dwTimeout, dwTimeout, dwTimeout);
        if (!WinHttpSendRequest(hRequest, headers.c_str(), (DWORD)headers.size(), (LPVOID)body.data(), (DWORD)body.size(), (DWORD)body.size(), 0)) break;
        if (!WinHttpReceiveResponse(hRequest, nullptr)) break;
        // Ensure HTTP 200
        DWORD dwStatus = 0, dwLen = sizeof(dwStatus);
        if (!WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &dwStatus, &dwLen, WINHTTP_NO_HEADER_INDEX)) break;
        if (dwStatus != 200) break;
        DWORD dwSize = 0;
        out.clear();
        do {
            if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;
            if (!dwSize) { ok = true; break; }
            std::vector<char> buf(dwSize);
            DWORD dwRead = 0;
            if (!WinHttpReadData(hRequest, buf.data(), dwSize, &dwRead)) break;
            out.append(buf.data(), dwRead);
        } while (dwSize > 0);
    } while (false);
    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);
    return ok;
}

// Very small JSON parser to extract translations[0].text
bool DeepLTranslator::ParseDeepLJson(const std::string& json, std::wstring& outW)
{
    // Find first translations[0].text string value robustly
    size_t pos = json.find("\"translations\"");
    if (pos == std::string::npos) return false;
    pos = json.find("\"text\"", pos);
    if (pos == std::string::npos) return false;
    pos = json.find('"', pos + 6);
    if (pos == std::string::npos) return false;
    ++pos; // now at beginning of string content
    std::string out;
    while (pos < json.size())
    {
        unsigned char c = (unsigned char)json[pos++];
        if (c == '"') break; // end of string
        if (c == '\\') {
            if (pos >= json.size()) break;
            char esc = json[pos++];
            switch (esc) {
            case '"': out.push_back('"'); break;
            case '\\': out.push_back('\\'); break;
            case '/': out.push_back('/'); break;
            case 'b': out.push_back('\b'); break;
            case 'f': out.push_back('\f'); break;
            case 'n': out.push_back('\n'); break;
            case 'r': out.push_back('\r'); break;
            case 't': out.push_back('\t'); break;
            case 'u': {
                // parse \uXXXX possibly surrogate pair
                if (pos + 4 > json.size()) return false;
                auto hex4 = [&](size_t at) -> int {
                    int v = 0; for (size_t i = 0; i < 4; ++i) { char ch = json[at + i]; v <<= 4; if (ch >= '0' && ch <= '9') v |= (ch - '0'); else if (ch >= 'a' && ch <= 'f') v |= (ch - 'a' + 10); else if (ch >= 'A' && ch <= 'F') v |= (ch - 'A' + 10); else return -1; } return v; };
                int w1 = hex4(pos); if (w1 < 0) return false; pos += 4;
                int codepoint = w1;
                // handle surrogate pair
                if (w1 >= 0xD800 && w1 <= 0xDBFF) {
                    if (pos + 6 <= json.size() && json[pos] == '\\' && pos + 1 < json.size() && json[pos + 1] == 'u') {
                        pos += 2;
                        int w2 = hex4(pos); if (w2 < 0) return false; pos += 4;
                        if (w2 >= 0xDC00 && w2 <= 0xDFFF) {
                            codepoint = 0x10000 + (((w1 - 0xD800) << 10) | (w2 - 0xDC00));
                        }
                    }
                }
                // encode as UTF-8
                if (codepoint <= 0x7F) out.push_back((char)codepoint);
                else if (codepoint <= 0x7FF) { out.push_back((char)(0xC0 | (codepoint >> 6))); out.push_back((char)(0x80 | (codepoint & 0x3F))); }
                else if (codepoint <= 0xFFFF) { out.push_back((char)(0xE0 | (codepoint >> 12))); out.push_back((char)(0x80 | ((codepoint >> 6) & 0x3F))); out.push_back((char)(0x80 | (codepoint & 0x3F))); }
                else { out.push_back((char)(0xF0 | (codepoint >> 18))); out.push_back((char)(0x80 | ((codepoint >> 12) & 0x3F))); out.push_back((char)(0x80 | ((codepoint >> 6) & 0x3F))); out.push_back((char)(0x80 | (codepoint & 0x3F))); }
                break;
            }
            default: out.push_back(esc); break;
            }
        } else {
            out.push_back((char)c);
        }
    }
    outW = Utf8ToW(out);
    return !outW.empty();
}

bool DeepLTranslator::Translate(const std::wstring& inTextW, const std::string& target, std::wstring& outTextW, unsigned int timeoutMs)
{
    (void)timeoutMs; // timeouts set inside WinHTTP
    if (!IsEnabled()) return false;
    std::string textUtf8 = WToUtf8(inTextW);
    std::ostringstream body;
    body << "auth_key=" << UrlEncodeUtf8(m_apiKey) << "&target_lang=" << UrlEncodeUtf8(target) << "&text=" << UrlEncodeUtf8(textUtf8);

    std::wstring host = m_useFree ? L"api-free.deepl.com" : L"api.deepl.com";
    INTERNET_PORT port = 443;
    std::wstring path = L"/v2/translate";
    std::wstring headers = L"Content-Type: application/x-www-form-urlencoded\r\n";
    std::string response;
    if (!HttpPostWinHttp(host, port, path, headers, body.str(), response))
        return false;
    std::wstring translated;
    if (!ParseDeepLJson(response, translated))
        return false;
    outTextW = translated;
    return true;
}
