#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {

std::uint64_t getFileSizeBytes(const std::string& path) {
#ifdef _WIN32
    HANDLE file = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("failed to open file");
    }

    LARGE_INTEGER sizeLi {};
    const BOOL ok = GetFileSizeEx(file, &sizeLi);
    const DWORD closeErr = CloseHandle(file) ? ERROR_SUCCESS : GetLastError();
    if (!ok) {
        throw std::runtime_error("failed to get file size");
    }
    if (closeErr != ERROR_SUCCESS) {
        throw std::runtime_error("failed to close file handle");
    }
    if (sizeLi.QuadPart < 0) {
        throw std::runtime_error("invalid file size");
    }

    return static_cast<std::uint64_t>(sizeLi.QuadPart);
#else
    std::ifstream sizeStream(path, std::ios::binary | std::ios::ate);
    if (!sizeStream) {
        throw std::runtime_error("failed to open file");
    }
    const std::streamsize streamSize = sizeStream.tellg();
    if (streamSize < 0) {
        throw std::runtime_error("failed to get file size");
    }
    return static_cast<std::uint64_t>(streamSize);
#endif
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

    std::uint64_t fileSize = 0;
    try {
        fileSize = getFileSizeBytes(path);
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }
    if (fileSize == 0) {
        std::cout << "File is empty\n";
        return 0;
    }

    constexpr std::size_t kBufferSize = 8 * 1024 * 1024;

    std::uint64_t checksum = 0;
    double totalSeconds = 0.0;
    std::vector<char> buffer(kBufferSize);

    for (int run = 0; run < iterations; ++run) {
        std::ifstream in(path, std::ios::binary);
        if (!in) {
            std::cerr << "Error: failed to open file for reading\n";
            return 1;
        }

        const auto start = std::chrono::steady_clock::now();
        std::uint64_t runChecksum = 0;

        while (in) {
            in.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
            const std::streamsize bytesRead = in.gcount();
            for (std::streamsize i = 0; i < bytesRead; ++i) {
                runChecksum += static_cast<unsigned char>(buffer[i]);
            }
        }

        if (!in.eof()) {
            std::cerr << "Error: read failed before EOF\n";
            return 1;
        }

        const auto end = std::chrono::steady_clock::now();
        const std::chrono::duration<double> elapsed = end - start;
        totalSeconds += elapsed.count();
        checksum += runChecksum;
    }

    const double gib = static_cast<double>(fileSize) / (1024.0 * 1024.0 * 1024.0);
    const double avgSeconds = totalSeconds / static_cast<double>(iterations);
    const double gibPerSec = gib / avgSeconds;

    std::cout << std::fixed << std::setprecision(3);
    std::cout << "Method      : ifstream-chunked\n";
    std::cout << "File        : " << path << "\n";
    std::cout << "Size (GiB)  : " << gib << "\n";
    std::cout << "Iterations  : " << iterations << "\n";
    std::cout << "Avg time (s): " << avgSeconds << "\n";
    std::cout << "Speed GiB/s : " << gibPerSec << "\n";
    std::cout << "Checksum    : " << checksum << "\n";

    return 0;
}
