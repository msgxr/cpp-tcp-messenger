#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <cstddef>
#include <cstdint>

enum MessageType : uint8_t {
    MSG_LIST_REQUEST = 0x01,
    MSG_TEXT         = 0x02,
    MSG_IMAGE        = 0x03,
    MSG_ID_ASSIGN    = 0x10,
    MSG_LIST_RESPONSE= 0x11,
    MSG_ERROR        = 0x12
};

constexpr uint32_t MAX_TEXT_SIZE  = 64u * 1024u;
constexpr uint32_t MAX_IMAGE_SIZE = 10u * 1024u * 1024u;
constexpr uint32_t MAX_ERROR_SIZE = 4u * 1024u;
constexpr uint16_t MAX_FILENAME_SIZE = 255u;
constexpr int MAX_CLIENTS = 5;

#pragma pack(push, 1)
struct MessageHeader {
    uint8_t  type;
    uint16_t client_id;
    uint16_t filename_len;
    uint32_t data_size;
    uint32_t checksum;
};
#pragma pack(pop)

static_assert(sizeof(MessageHeader) == 13,
              "MessageHeader 13 bayt olmali");

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
