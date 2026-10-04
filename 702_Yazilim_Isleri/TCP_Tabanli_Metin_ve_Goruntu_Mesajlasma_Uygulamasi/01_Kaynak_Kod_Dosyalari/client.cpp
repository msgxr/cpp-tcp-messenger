#include <iostream>
#include <string>
#include <thread>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include "chat_common.h"

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

    std::thread receiver(receive_loop, sock_fd);

    std::string line;
    while (true) {
        ui::prompt();
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) continue;
        if (line == "cikis") break;
        if (line == "!yardim") { ui::help(); continue; }
        if (line == "!durum") { ui::status("İstemci aktif; sunucu bağlantısı açık."); continue; }
        if (line == "!temizle") { ui::clear(); ui::banner("İSTEMCİ", "127.0.0.1:5000"); continue; }
        if (line.rfind("!resim ", 0) == 0) {
            send_image_message(sock_fd, line.substr(7));
            continue;
        }
        if (!send_text_message(sock_fd, line)) {
            ui::error("Mesaj gönderilemedi; bağlantı kapanmış olabilir.");
            break;
        }
        ui::status("Metin gönderildi.");
    }

    ui::info("Bağlantı kapatılıyor...");
    shutdown(sock_fd, SHUT_RDWR);
    close(sock_fd);
    if (receiver.joinable()) receiver.join();
    return 0;
}
