#pragma once
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <chrono>
#include <mutex>

class IpGuard {
public:
    void SetWhitelist(std::unordered_set<std::string> ips) {
        std::lock_guard<std::mutex> lk(mx); whitelist = std::move(ips);
    }
    void Configure(int maxConcurrent, int maxPer5s, int cooldownSec) {
        std::lock_guard<std::mutex> lk(mx);
        maxConc = maxConcurrent; maxPerWindow = maxPer5s; cool = cooldownSec;
    }

    bool OnAccept(const std::string& ip, std::string& reason) {
        using namespace std::chrono;
        if (ip.empty()) { reason = "empty_ip"; return false; }

        std::lock_guard<std::mutex> lk(mx);
        auto now = steady_clock::now();

        if (!whitelist.empty() && !whitelist.count(ip)) {
            reason = "not_whitelisted"; return false;
        }
        auto itb = blocked.find(ip);
        if (itb != blocked.end() && now < itb->second) {
            reason = "cooldown";
            return false;
        }

        auto& w = table[ip];
        if (now - w.windowStart > seconds(5)) { w.windowStart = now; w.newConnInWindow = 0; }
        if (w.newConnInWindow >= maxPerWindow) {
            blocked[ip] = now + seconds(cool);
            reason = "rate_exceeded"; return false;
        }
        if (w.concurrent >= maxConc) {
            reason = "too_many_concurrent"; return false;
        }

        w.concurrent++; w.newConnInWindow++; w.lastSeen = now;
        return true;
    }

    void OnClose(const std::string& ip) {
        std::lock_guard<std::mutex> lk(mx);
        auto it = table.find(ip);
        if (it != table.end() && it->second.concurrent > 0) it->second.concurrent--;
    }

private:
    struct IpWindow {
        int concurrent = 0;
        int newConnInWindow = 0;
        std::chrono::steady_clock::time_point windowStart = std::chrono::steady_clock::now();
        std::chrono::steady_clock::time_point lastSeen;
    };

    std::mutex mx;
    std::unordered_map<std::string, IpWindow> table;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> blocked;
    std::unordered_set<std::string> whitelist;
    int maxConc = 2, maxPerWindow = 6, cool = 60;
};
