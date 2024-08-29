#pragma once
#include <atomic>
#include <string>
#include <vector>
#include <mutex>
#include <chrono>

struct CryptoInfo
{
    std::string id;
    std::string symbol;
    std::string name;
    double current_price;
    double market_cap;
    int market_cap_rank;
    double price_change_percentage_24h;
};

struct CommonObjects
{
    std::atomic_bool exit_flag = false;
    std::atomic_bool data_ready = false;
    std::string url;
    std::vector<CryptoInfo> cryptos;
    std::mutex data_mutex;
    std::chrono::system_clock::time_point last_updated;
};