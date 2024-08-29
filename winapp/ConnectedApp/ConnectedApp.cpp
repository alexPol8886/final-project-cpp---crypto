#include <iostream>     // For input/output operations
#include <thread>       // For creating and managing threads
#include <chrono>       // For time-related functions
#include "CommonObject.h"   // Custom header file for shared objects
#include "DrawThread.h"     // Custom header file for drawing thread
#include "DownloadThread.h" // Custom header file for download thread

int main()
{
    // Create instances of our custom classes
    CommonObjects common;   // Shared object for communication between threads
    DrawThread draw;        // Object responsible for drawing operations
    DownloadThread down;    // Object responsible for downloading data

    // Set the URL for downloading data
    down.SetUrl("https://api.coingecko.com/api/v3/coins/markets?vs_currency=usd&order=market_cap_desc&per_page=100&page=1&sparkline=false");

    // Create and start the drawing thread
    auto draw_th = std::jthread([&] {draw(common); });

    // Create and start the download thread
    auto down_th = std::jthread([&] {
        while (!common.exit_flag) {
            down(common);   // Perform the download operation
            std::this_thread::sleep_for(std::chrono::seconds(10)); // Wait for 10 seconds before next download
        }
        });

    // Inform the user that the application is running
    std::cout << "Running... Press Ctrl+C to exit.\n";

    // Wait for both threads to finish
    draw_th.join();
    down_th.join();
}