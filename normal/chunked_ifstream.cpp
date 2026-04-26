#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

constexpr std::size_t kBlockSize = 8ULL * 1024ULL * 1024ULL;

std::uint64_t consumeBytes(const std::uint8_t* data, std::size_t size) {
    std::uint64_t sum = 0;
    for (std::size_t i = 0; i < size; ++i) {
        sum += data[i];
    }
    return sum;
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 3) {
        std::cerr << "Usage: " << argv[0] << " <file_path> [iterations]\n";
        return 1;
    }

    const std::string path = argv[1];
    int iterations = 1;
    if (argc == 3) {
        try {
            iterations = std::stoi(argv[2]);
        } catch (...) {
            std::cerr << "Invalid iterations value\n";
            return 1;
        }
        if (iterations <= 0) {
            std::cerr << "Iterations must be > 0\n";
            return 1;
        }
    }

    std::ifstream sizeStream(path, std::ios::binary | std::ios::ate);
    if (!sizeStream) {
        std::cerr << "Error: failed to open file\n";
        return 1;
    }

    const std::streamsize streamSize = sizeStream.tellg();
    if (streamSize < 0) {
        std::cerr << "Error: failed to get file size\n";
        return 1;
    }
    if (streamSize == 0) {
        std::cout << "File is empty\n";
        return 0;
    }
    const std::size_t fileSize = static_cast<std::size_t>(streamSize);

    std::vector<std::uint8_t> buffer(kBlockSize);
    std::uint64_t checksum = 0;
    double totalSeconds = 0.0;

    for (int run = 0; run < iterations; ++run) {
        std::ifstream in(path, std::ios::binary);
        if (!in) {
            std::cerr << "Error: failed to open file for reading\n";
            return 1;
        }

        const auto start = std::chrono::steady_clock::now();
        std::uint64_t runChecksum = 0;

        while (in) {
            in.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
            const std::streamsize bytesRead = in.gcount();
            if (bytesRead > 0) {
                runChecksum += consumeBytes(buffer.data(), static_cast<std::size_t>(bytesRead));
            }
        }

        if (!in.eof()) {
            std::cerr << "Error: read failed before EOF\n";
            return 1;
        }

        const auto end = std::chrono::steady_clock::now();
        const std::chrono::duration<double> elapsed = end - start;
        totalSeconds += elapsed.count();
        checksum ^= runChecksum;
    }

    const double gib = static_cast<double>(fileSize) / (1024.0 * 1024.0 * 1024.0);
    const double avgSeconds = totalSeconds / static_cast<double>(iterations);
    const double gibPerSec = gib / avgSeconds;

    std::cout << std::fixed << std::setprecision(3);
    std::cout << "Method      : ifstream\n";
    std::cout << "File        : " << path << "\n";
    std::cout << "Size (GiB)  : " << gib << "\n";
    std::cout << "Iterations  : " << iterations << "\n";
    std::cout << "Avg time (s): " << avgSeconds << "\n";
    std::cout << "Speed GiB/s : " << gibPerSec << "\n";
    std::cout << "Checksum    : " << checksum << "\n";

    return 0;
}
