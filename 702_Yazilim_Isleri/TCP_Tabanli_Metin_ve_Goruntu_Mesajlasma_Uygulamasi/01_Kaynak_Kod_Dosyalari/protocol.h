#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <cstddef>
#include <cstdint>

enum MessageType : uint8_t {
    MSG_TEXT  = 1,
    MSG_IMAGE = 2
};

constexpr uint32_t MAX_TEXT_SIZE  = 1024u * 1024u;
constexpr uint32_t MAX_IMAGE_SIZE = 50u * 1024u * 1024u;

#pragma pack(push, 1)
struct MessageHeader {
    uint8_t  type;
    uint16_t filename_len;
    uint32_t data_size;
    uint32_t checksum;
};
#pragma pack(pop)

static_assert(sizeof(MessageHeader) == 11,
              "MessageHeader 11 bayt olmali");

bool read_exact(int sock, void* buffer, std::size_t len);
bool send_exact(int sock, const void* buffer, std::size_t len);

uint32_t crc32_begin();
uint32_t crc32_update(
    uint32_t crc,
    const unsigned char* data,
    std::size_t len
);
uint32_t crc32_end(uint32_t crc);
uint32_t crc32_bytes(const void* data, std::size_t len);

#endif