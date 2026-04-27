#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstdint>

int main() {
    const std::uint64_t TARGET_GB = 10; // Change this to 15 if you want 15GB
    const std::uint64_t TARGET_BYTES = TARGET_GB * 1024ULL * 1024ULL * 1024ULL;
    std::uint64_t current_bytes = 0;

    std::ofstream file("financial_data.csv", std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file for writing.\n";
        return 1;
    }

    // Write the CSV Header
    std::string header = "Timestamp,Ticker,Price,Volume\n";
    file.write(header.c_str(), header.length());
    current_bytes += header.length();

    const char* tickers[] = {"BTC", "ETH", "AAPL", "TSLA", "NVDA"};
    char buffer[128]; // Small, fast buffer for row generation

    std::cout << "Forging " << TARGET_GB << "GB CSV file... Grab a coffee.\n";

    // Generate rows until we hit the target file size
    while (current_bytes < TARGET_BYTES) {
        // Create a realistic looking row: Timestamp, Ticker, Price, Volume
        int len = snprintf(buffer, sizeof(buffer), "1714150000,%s,%.2f,%d\n",
                           tickers[rand() % 5],
                           (rand() % 5000000) / 100.0, // Random price up to 50,000.00
                           rand() % 10000);            // Random volume

        file.write(buffer, len);
        current_bytes += len;
    }

    std::cout << "Massive CSV generated successfully!\n";
    return 0;
}
