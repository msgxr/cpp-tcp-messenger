#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <cstdint>
#include <unistd.h>
#include <sys/socket.h>

// Mesaj türleri
enum MessageType : uint8_t {
    MSG_TEXT  = 1,
    MSG_IMAGE = 2
};

constexpr uint32_t MAX_TEXT_SIZE = 1024 * 1024;
constexpr uint32_t MAX_IMAGE_SIZE = 50 * 1024 * 1024;

// 7 baytlık sabit üst bilgi (1B Tür + 2B Dosya Adı Uzunluğu + 4B Veri Boyutu)
#pragma pack(push, 1)
struct MessageHeader {
    uint8_t  type;          
    uint16_t filename_len;  
    uint32_t data_size;     
};
#pragma pack(pop)

// Soketten tam olarak 'len' bayt veri okuyana kadar döngü kurar
inline bool read_exact(int sock, void* buffer, size_t len) {
    size_t total_read = 0;
    char* ptr = static_cast<char*>(buffer);
    while (total_read < len) {
        ssize_t bytes_read = recv(sock, ptr + total_read, len - total_read, 0);
        if (bytes_read <= 0) {
            return false;
        }
        total_read += bytes_read;
    }
    return true;
}

// Sokete tam olarak 'len' bayt veri yazana kadar döngü kurar
inline bool send_exact(int sock, const void* buffer, size_t len) {
    size_t total_sent = 0;
    const char* ptr = static_cast<const char*>(buffer);
    while (total_sent < len) {
        ssize_t bytes_sent = send(sock, ptr + total_sent, len - total_sent, 0);
        if (bytes_sent <= 0) {
            return false;
        }
        total_sent += bytes_sent;
    }
    return true;
}

#endif // PROTOCOL_H