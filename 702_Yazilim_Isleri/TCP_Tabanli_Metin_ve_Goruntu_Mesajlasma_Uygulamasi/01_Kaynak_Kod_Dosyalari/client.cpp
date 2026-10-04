#include <iostream>
#include <string>
#include <thread>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include "console.h"

constexpr const char* SERVER_IP = "127.0.0.1";
constexpr int PORT = 5000;

int main() {
    ui::clear();
    ui::banner("İSTEMCİ", "127.0.0.1:5000");

    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        ui::error("Socket oluşturulamadı.");
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) != 1) {
        ui::error("Sunucu IP adresi geçersiz.");
        close(sock_fd);
        return 1;
    }

    ui::info("Sunucuya bağlanılıyor...");
    if (connect(sock_fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        ui::error(std::string("Bağlantı kurulamadı: ") + std::strerror(errno));
        close(sock_fd);
        return 1;
    }

    ui::status("Sunucuya bağlandı.");
    ui::info("Protokol: TCP + 11 bayt üst bilgi + CRC32 bütünlük kontrolü");

    run_console(sock_fd, "İSTEMCİ", "127.0.0.1:5000");
    close(sock_fd);
    return 0;
}
