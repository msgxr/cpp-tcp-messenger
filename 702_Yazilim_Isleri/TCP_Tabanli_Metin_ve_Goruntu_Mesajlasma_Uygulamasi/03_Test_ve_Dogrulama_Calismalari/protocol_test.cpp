#include "protocol.h"
#include <arpa/inet.h>
#include <cassert>
#include <cstring>
#include <sys/socket.h>
#include <thread>
#include <string>
#include <unistd.h>

int main() {
    static_assert(sizeof(MessageHeader) == 11);
    assert(crc32_bytes("", 0) == 0);
    assert(crc32_bytes("123456789", 9) == 0xcbf43926u);
    auto crc = crc32_update(crc32_begin(), reinterpret_cast<const unsigned char*>("1234"), 4);
    crc = crc32_update(crc, reinterpret_cast<const unsigned char*>("56789"), 5);
    assert(crc32_end(crc) == 0xcbf43926u);
    MessageHeader h{MSG_IMAGE, htons(3), htonl(0x01020304), htonl(0xcbf43926)};
    const unsigned char expected[]{2, 0, 3, 1, 2, 3, 4, 0xcb, 0xf4, 0x39, 0x26};
    assert(std::memcmp(&h, expected, sizeof(h)) == 0);
    int sockets[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
    std::thread sender([&] {
        for (char c : std::string("123456789")) assert(send_exact(sockets[0], &c, 1));
        shutdown(sockets[0], SHUT_WR);
    });
    char data[9]{};
    assert(read_exact(sockets[1], data, 9));
    assert(std::memcmp(data, "123456789", 9) == 0);
    assert(!read_exact(sockets[1], data, 1));
    sender.join();
    close(sockets[1]);
    assert(!send_exact(sockets[0], "x", 1)); // SIGPIPE programi kapatmamali.
    close(sockets[0]);
}
