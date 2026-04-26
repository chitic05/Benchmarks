#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {

[[noreturn]] void fail(const std::string& message) {
    throw std::runtime_error(message + ": " + std::strerror(errno));
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
        const int fd = ::open(path.c_str(), O_RDONLY);
        if (fd < 0) {
            fail("open failed");
        }

        struct stat st {};
        if (::fstat(fd, &st) != 0) {
            const int saved_errno = errno;
            ::close(fd);
            errno = saved_errno;
            fail("fstat failed");
        }

        const std::size_t fileSize = static_cast<std::size_t>(st.st_size);
        if (fileSize == 0) {
            std::cout << "File is empty\n";
            ::close(fd);
            return 0;
        }

        void* mapping = ::mmap(nullptr, fileSize, PROT_READ, MAP_PRIVATE, fd, 0);
        if (mapping == MAP_FAILED) {
            const int saved_errno = errno;
            ::close(fd);
            errno = saved_errno;
            fail("mmap failed");
        }

        (void)::madvise(mapping, fileSize, MADV_NORMAL);

        const auto* bytes = static_cast<const std::uint8_t*>(mapping);

        std::uint64_t checksum = 0;
        double totalSeconds = 0.0;

        for (int run = 0; run < iterations; ++run) {
            const auto start = std::chrono::steady_clock::now();

            std::uint64_t runChecksum = 0;
            for (std::size_t i = 0; i < fileSize; ++i) {
                runChecksum += bytes[i];
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
        std::cout << "Method      : mmap\n";
        std::cout << "File        : " << path << "\n";
        std::cout << "Size (GiB)  : " << gib << "\n";
        std::cout << "Iterations  : " << iterations << "\n";
        std::cout << "Avg time (s): " << avgSeconds << "\n";
        std::cout << "Speed GiB/s : " << gibPerSec << "\n";
        std::cout << "Checksum    : " << checksum << "\n";

        if (::munmap(mapping, fileSize) != 0) {
            const int saved_errno = errno;
            ::close(fd);
            errno = saved_errno;
            fail("munmap failed");
        }

        if (::close(fd) != 0) {
            fail("close failed");
        }
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
