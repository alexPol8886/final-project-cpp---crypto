#include "DownloadThread.h"
#define CPPHTTPLIB_OPENSSL_SUPPORT //enable HTTPS support in the httplib library
#include "httplib.h"
#include "nlohmann/json.hpp"

// Define the structure of CryptoInfo for JSON parsing
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CryptoInfo, id, symbol, name, current_price, market_cap, market_cap_rank, price_change_percentage_24h)

void DownloadThread::operator()(CommonObjects& common)
{
    // Create an HTTP client
    httplib::Client cli("https://api.coingecko.com");

    // Send a GET request to the specified URL
    auto res = cli.Get(download_url.c_str());

    // Check if the request was successful
    if (res && res->status == 200)
    {
        // Parse the JSON response
        auto json_result = nlohmann::json::parse(res->body);

        // Convert the JSON data to a vector of CryptoInfo objects
        std::vector<CryptoInfo> new_cryptos = json_result.get<std::vector<CryptoInfo>>();

        {
            // Lock the mutex to safely update shared data
            std::lock_guard<std::mutex> lock(common.data_mutex);

            // Update the shared cryptos data
            common.cryptos = std::move(new_cryptos);

            // Update the last updated timestamp
            common.last_updated = std::chrono::system_clock::now();

            // Set the data ready flag
            common.data_ready = true;
        }
    }
}

void DownloadThread::SetUrl(std::string_view new_url)
{
    // Update the download URL
    download_url = new_url;
}