#pragma once

#include <string>
#include <mutex>

class DeepLTranslator
{
public:
    static DeepLTranslator& Instance()
    {
        static DeepLTranslator inst;
        return inst;
    }

    void Configure(bool enabled, const std::string& apiKey, bool useFreeApi)
    {
        std::lock_guard<std::mutex> lock(m_mu);
        m_enabled = enabled;
        m_apiKey = apiKey;
        m_useFree = useFreeApi;
    }

    bool IsEnabled() const { return m_enabled && !m_apiKey.empty(); }

    // target: DeepL target language code (e.g., EN, EN-US, ES, JA, ZH)
    // Returns true on success and fills outText (UTF-16)
    bool Translate(const std::wstring& inTextW, const std::string& target, std::wstring& outTextW, unsigned int timeoutMs = 3000);

private:
    DeepLTranslator() : m_enabled(false), m_useFree(true) {}
    ~DeepLTranslator() = default;
    DeepLTranslator(const DeepLTranslator&) = delete;
    DeepLTranslator& operator=(const DeepLTranslator&) = delete;

    // helpers
    static std::string UrlEncodeUtf8(const std::string& s);
    static std::string WToUtf8(const std::wstring& ws);
    static std::wstring Utf8ToW(const std::string& s);
    static bool ParseDeepLJson(const std::string& json, std::wstring& outW);

private:
    bool m_enabled;
    bool m_useFree;
    std::string m_apiKey;
    mutable std::mutex m_mu;
};
