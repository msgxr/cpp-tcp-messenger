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
#include "console.h"

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

    run_console(client_socket, "SUNUCU", "0.0.0.0:5000");
    close(client_socket);
    close(server_fd);
    return 0;
}
