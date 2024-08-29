#include "DrawThread.h"
#include "GuiMain.h"
#include "../../shared/ImGuiSrc/imgui.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <map>
#include <vector>
#include <iostream>

// Function to draw the main application window
void DrawAppWindow(void* common_ptr)
{
    // Cast the void pointer to CommonObjects
    auto common = (CommonObjects*)common_ptr;

    // Static variables to maintain state across function calls
    static char search_query[256] = ""; // Buffer to store the search query
    static std::map<std::string, float> selected_coins; // To store selected coins and amounts
    static std::map<std::string, float> wallet_coins; // Coins added to the wallet
    static float total_price = 0.0f;

    // Set background color to dark teal
    ImVec4 clear_color = ImVec4(0.02f, 0.10f, 0.10f, 1.0f);
    ImGui::GetStyle().Colors[ImGuiCol_WindowBg] = clear_color;

    // Begin the main window
    ImGui::Begin("Crypto Tracker");
    ImGui::Text("Top 100 Cryptocurrencies by CoinGecko");

    // Search Bar
    ImGui::InputText("Search", search_query, IM_ARRAYSIZE(search_query));

    {
        // Lock the mutex to safely access shared data
        std::lock_guard<std::mutex> lock(common->data_mutex);
        if (common->data_ready)
        {
            // Display last update time
            auto now = std::chrono::system_clock::now();
            auto last_update_time = std::chrono::system_clock::to_time_t(common->last_updated);
            std::stringstream ss;
            ss << "Last updated: " << std::put_time(std::localtime(&last_update_time), "%Y-%m-%d %H:%M:%S");
            ImGui::Text("%s", ss.str().c_str());

            // Begin table to display cryptocurrency data
            if (ImGui::BeginTable("Cryptos", 8))
            {
                // Set up table columns
                ImGui::TableSetupColumn("Rank");
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("Symbol");
                ImGui::TableSetupColumn("Price (USD)");
                ImGui::TableSetupColumn("Market Cap (USD)");
                ImGui::TableSetupColumn("24h Change (%)");
                ImGui::TableSetupColumn("Amount");
                ImGui::TableSetupColumn("");
                ImGui::TableHeadersRow();

                // Iterate through all cryptocurrencies
                for (auto& crypto : common->cryptos)
                {
                    // Convert to lowercase for case-insensitive search
                    std::string name_lower = crypto.name;
                    std::string symbol_lower = crypto.symbol;
                    std::string query_lower = search_query;
                    std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
                    std::transform(symbol_lower.begin(), symbol_lower.end(), symbol_lower.begin(), ::tolower);
                    std::transform(query_lower.begin(), query_lower.end(), query_lower.begin(), ::tolower);

                    // Filter by search query
                    if (name_lower.find(query_lower) != std::string::npos || symbol_lower.find(query_lower) != std::string::npos)
                    {
                        // Set row color based on 24h change
                        if (crypto.price_change_percentage_24h >= 0)
                        {
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f)); // Green for positive change
                        }
                        else
                        {
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f)); // Red for negative change
                        }

                        // Display cryptocurrency data in table
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("%d", crypto.market_cap_rank);
                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%s", crypto.name.c_str());
                        ImGui::TableSetColumnIndex(2);
                        ImGui::Text("%s", crypto.symbol.c_str());
                        ImGui::TableSetColumnIndex(3);
                        ImGui::Text("$%.2f", crypto.current_price);
                        ImGui::TableSetColumnIndex(4);
                        ImGui::Text("$%.0f", crypto.market_cap);
                        ImGui::TableSetColumnIndex(5);
                        ImGui::Text("%.2f%%", crypto.price_change_percentage_24h);

                        // Amount Input Field
                        ImGui::TableSetColumnIndex(6);
                        float amount = 0.0f;
                        auto it = selected_coins.find(crypto.symbol);
                        if (it != selected_coins.end())
                        {
                            amount = it->second;
                        }

                        std::string amount_label = "##Amount_" + crypto.symbol;
                        if (ImGui::InputFloat(amount_label.c_str(), &amount, 1.0f, 10.0f, "%.2f"))
                        {
                            if (amount > 0.0f)
                            {
                                selected_coins[crypto.symbol] = amount;
                            }
                            else
                            {
                                selected_coins.erase(crypto.symbol);
                            }
                        }

                        // Add to Wallet Button
                        ImGui::TableSetColumnIndex(7);
                        std::string button_label = "Add##" + crypto.symbol;
                        if (ImGui::Button(button_label.c_str()))
                        {
                            if (amount > 0.0f)
                            {
                                wallet_coins[crypto.symbol] = amount;
                                std::cout << "Added " << crypto.symbol << " to wallet with amount " << amount << std::endl;
                            }
                            else
                            {
                                std::cout << "Amount is zero or negative for " << crypto.symbol << std::endl;
                            }
                        }

                        // Calculate total price of selected coins
                        total_price = 0.0f;
                        for (const auto& pair : selected_coins)
                        {
                            auto it = std::find_if(common->cryptos.begin(), common->cryptos.end(),
                                [&](const auto& c) { return c.symbol == pair.first; });
                            if (it != common->cryptos.end())
                            {
                                total_price += it->current_price * pair.second;
                            }
                        }

                        ImGui::PopStyleColor(); // Revert to default text color
                    }
                }
                ImGui::EndTable();
            }
        }
        else
        {
            ImGui::Text("Loading data...");
        }
    }

    // Display total price
    ImGui::Separator();
    ImGui::Text("Total Price: $%.2f", total_price);

    // Clear button
    if (ImGui::Button("Clear"))
    {
        selected_coins.clear(); // Clear all selections
        total_price = 0.0f; // Reset total price
    }

    ImGui::End();

    // "My Wallet" Window
    ImGui::Begin("My Wallet");

    if (!wallet_coins.empty())
    {
        if (ImGui::BeginTable("Wallet", 4))
        {
            ImGui::TableSetupColumn("Symbol");
            ImGui::TableSetupColumn("Amount");
            ImGui::TableSetupColumn("Value (USD)");
            ImGui::TableSetupColumn("Action");
            ImGui::TableHeadersRow();

            float wallet_total = 0.0f;

            for (auto it = wallet_coins.begin(); it != wallet_coins.end(); ++it)
            {
                auto crypto_it = std::find_if(common->cryptos.begin(), common->cryptos.end(),
                    [&](const auto& c) { return c.symbol == it->first; });
                if (crypto_it != common->cryptos.end())
                {
                    float value = crypto_it->current_price * it->second;
                    wallet_total += value;

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%s", it->first.c_str());
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%.2f", it->second);
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("$%.2f", value);

                    // Remove Button
                    ImGui::TableSetColumnIndex(3);
                    std::string remove_button_label = "Remove##" + it->first;
                    if (ImGui::Button(remove_button_label.c_str()))
                    {
                        wallet_coins.erase(it); // Remove the coin from the wallet
                        break; // Break to avoid modifying the map while iterating
                    }
                }
            }

            ImGui::EndTable();
            ImGui::Text("Total Wallet Value: $%.2f", wallet_total);
        }
    }
    else
    {
        ImGui::Text("Your wallet is empty.");
    }

    ImGui::End();
}

// DrawThread operator function
void DrawThread::operator()(CommonObjects& common)
{
    GuiMain(DrawAppWindow, &common);
    common.exit_flag = true;
}