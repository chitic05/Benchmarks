#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#include <windows.h>

namespace {

[[noreturn]] void fail(const std::string& message) {
    const DWORD err = GetLastError();
    throw std::runtime_error(message + " (Win32 error " + std::to_string(err) + ")");
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

    try {
        HANDLE file = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                                  FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) {
            fail("CreateFile failed");
        }

        LARGE_INTEGER sizeLi {};
        if (!GetFileSizeEx(file, &sizeLi)) {
            const DWORD saved = GetLastError();
            CloseHandle(file);
            SetLastError(saved);
            fail("GetFileSizeEx failed");
        }

        if (sizeLi.QuadPart <= 0) {
            std::cout << "File is empty\n";
            CloseHandle(file);
            return 0;
        }

        const std::uint64_t fileSize = static_cast<std::uint64_t>(sizeLi.QuadPart);

        HANDLE mapping = CreateFileMappingA(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
        if (mapping == nullptr) {
            const DWORD saved = GetLastError();
            CloseHandle(file);
            SetLastError(saved);
            fail("CreateFileMapping failed");
        }

        SYSTEM_INFO systemInfo {};
        GetSystemInfo(&systemInfo);
        const std::uint64_t granularity = static_cast<std::uint64_t>(systemInfo.dwAllocationGranularity);
        constexpr std::size_t kChunkBytes = 256ULL * 1024ULL * 1024ULL;

        std::uint64_t checksum = 0;
        double totalSeconds = 0.0;

        for (int run = 0; run < iterations; ++run) {
            const auto start = std::chrono::steady_clock::now();

            std::uint64_t runChecksum = 0;
            std::uint64_t offset = 0;
            while (offset < fileSize) {
                const std::uint64_t chunkSize64 =
                    (fileSize - offset) < static_cast<std::uint64_t>(kChunkBytes)
                        ? (fileSize - offset)
                        : static_cast<std::uint64_t>(kChunkBytes);

                const std::uint64_t alignedOffset = (offset / granularity) * granularity;
                const std::size_t delta = static_cast<std::size_t>(offset - alignedOffset);
                const std::size_t viewSize = static_cast<std::size_t>(chunkSize64) + delta;

                const DWORD offsetHigh = static_cast<DWORD>(alignedOffset >> 32);
                const DWORD offsetLow = static_cast<DWORD>(alignedOffset & 0xFFFFFFFFULL);
                const void* view = MapViewOfFile(mapping, FILE_MAP_READ, offsetHigh, offsetLow, viewSize);
                if (view == nullptr) {
                    const DWORD saved = GetLastError();
                    CloseHandle(mapping);
                    CloseHandle(file);
                    SetLastError(saved);
                    fail("MapViewOfFile failed");
                }

                const auto* bytes = static_cast<const std::uint8_t*>(view) + delta;
                for (std::uint64_t i = 0; i < chunkSize64; ++i) {
                    runChecksum += bytes[i];
                }

                if (!UnmapViewOfFile(view)) {
                    const DWORD saved = GetLastError();
                    CloseHandle(mapping);
                    CloseHandle(file);
                    SetLastError(saved);
                    fail("UnmapViewOfFile failed");
                }

                offset += chunkSize64;
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
        std::cout << "Method      : mmap-win32\n";
        std::cout << "File        : " << path << "\n";
        std::cout << "Size (GiB)  : " << gib << "\n";
        std::cout << "Iterations  : " << iterations << "\n";
        std::cout << "Avg time (s): " << avgSeconds << "\n";
        std::cout << "Speed GiB/s : " << gibPerSec << "\n";
        std::cout << "Checksum    : " << checksum << "\n";

        if (!CloseHandle(mapping)) {
            const DWORD saved = GetLastError();
            CloseHandle(file);
            SetLastError(saved);
            fail("CloseHandle(mapping) failed");
        }

        if (!CloseHandle(file)) {
            fail("CloseHandle(file) failed");
        }
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}

#else

int main() {
    std::cerr << "This source file is intended for Windows builds only.\n";
    return 1;
}

#endif
