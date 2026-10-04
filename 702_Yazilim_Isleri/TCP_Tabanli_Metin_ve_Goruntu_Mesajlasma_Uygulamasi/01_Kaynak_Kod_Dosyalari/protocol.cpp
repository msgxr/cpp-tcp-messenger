#include "protocol.h"

#include <cerrno>
#include <sys/socket.h>

bool read_exact(int sock, void* buffer, std::size_t len) {
    std::size_t total = 0;
    auto* ptr = static_cast<char*>(buffer);

    while (total < len) {
        const ssize_t n =
            recv(sock, ptr + total, len - total, 0);

        if (n > 0) {
            total += static_cast<std::size_t>(n);
            continue;
        }

        if (n < 0 && errno == EINTR)
            continue;

        return false;
    }

    return true;
}

bool send_exact(int sock, const void* buffer, std::size_t len) {
    std::size_t total = 0;
    const auto* ptr = static_cast<const char*>(buffer);

    while (total < len) {
#ifdef MSG_NOSIGNAL
        constexpr int flags = MSG_NOSIGNAL;
#else
        constexpr int flags = 0;
#endif

        const ssize_t n =
            send(sock, ptr + total, len - total, flags);

        if (n > 0) {
            total += static_cast<std::size_t>(n);
            continue;
        }

        if (n < 0 && errno == EINTR)
            continue;

        return false;
    }

    return true;
}

uint32_t crc32_begin() {
    return 0xFFFFFFFFu;
}

uint32_t crc32_update(
    uint32_t crc,
    const unsigned char* data,
    std::size_t len
) {
    for (std::size_t i = 0; i < len; ++i) {
        crc ^= data[i];

        for (int bit = 0; bit < 8; ++bit) {
            const uint32_t mask = 0u - (crc & 1u);
            crc = (crc >> 1u) ^
                  (0xEDB88320u & mask);
        }
    }

    return crc;
}

uint32_t crc32_end(uint32_t crc) {
    return ~crc;
}

uint32_t crc32_bytes(const void* data, std::size_t len) {
    return crc32_end(
        crc32_update(
            crc32_begin(),
            static_cast<const unsigned char*>(data),
            len
        )
    );
}