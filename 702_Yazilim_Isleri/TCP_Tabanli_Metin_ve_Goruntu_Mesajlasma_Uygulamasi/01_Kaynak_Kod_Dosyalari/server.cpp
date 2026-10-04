#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include "chat_common.h"

constexpr int PORT = 5000;

int main() {
    ui::clear();
    ui::banner("SUNUCU", "0.0.0.0:5000");

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        ui::error("Socket oluşturulamadı.");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        ui::error(std::string("Port bağlanamadı: ") + std::strerror(errno));
        close(server_fd);
        return 1;
    }
    if (listen(server_fd, 1) < 0) {
        ui::error("Dinleme başlatılamadı.");
        close(server_fd);
        return 1;
    }

    ui::info("5000/TCP portu dinleniyor. İstemci bekleniyor...");

    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    int client_socket = accept(server_fd, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
    if (client_socket < 0) {
        ui::error("İstemci bağlantısı kabul edilemedi.");
        close(server_fd);
        return 1;
    }

    char ip[INET_ADDRSTRLEN]{};
    inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));
    ui::status(std::string("İstemci bağlandı: ") + ip);
    ui::info("Protokol: TCP + 11 bayt üst bilgi + CRC32 bütünlük kontrolü");

    std::thread receiver(receive_loop, client_socket);

    std::string line;
    while (true) {
        ui::prompt();
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) continue;
        if (line == "cikis") break;
        if (line == "!yardim") { ui::help(); continue; }
        if (line == "!durum") { ui::status("Sunucu aktif; istemci bağlantısı açık."); continue; }
        if (line == "!temizle") { ui::clear(); ui::banner("SUNUCU", "0.0.0.0:5000"); continue; }
        if (line.rfind("!resim ", 0) == 0) {
            send_image_message(client_socket, line.substr(7));
            continue;
        }
        if (!send_text_message(client_socket, line)) {
            ui::error("Mesaj gönderilemedi; bağlantı kapanmış olabilir.");
            break;
        }
        ui::status("Metin gönderildi.");
    }

    ui::info("Bağlantı kapatılıyor...");
    shutdown(client_socket, SHUT_RDWR);
    close(client_socket);
    close(server_fd);
    if (receiver.joinable()) receiver.join();
    return 0;
}
