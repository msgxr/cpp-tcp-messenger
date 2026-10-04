#ifndef CONSOLE_H
#define CONSOLE_H

#include <atomic>
#include <cerrno>
#include <poll.h>
#include <string>
#include <thread>
#include <unistd.h>
#include "chat_common.h"

// poll ile beklemek, baglanti kesildiginde Enter tusuna gerek kalmadan cikmayi saglar.
class ConsoleInput {
    std::string pending;
    bool eof = false;
public:
    bool next(std::string& line, const std::atomic<bool>& connected) {
        while (connected.load()) {
            const auto end = pending.find('\n');
            if (end != std::string::npos) {
                line = pending.substr(0, end);
                pending.erase(0, end + 1);
                if (!line.empty() && line.back() == '\r') line.pop_back();
                return true;
            }
            if (eof) {
                if (pending.empty()) return false;
                line.swap(pending);
                return true;
            }
            pollfd input{STDIN_FILENO, POLLIN, 0};
            const int ready = poll(&input, 1, 100);
            if (ready < 0) {
                if (errno == EINTR) continue;
                return false;
            }
            if (ready == 0) continue;
            if (input.revents & (POLLERR | POLLNVAL)) return false;
            if (input.revents & (POLLIN | POLLHUP)) {
                char buffer[4096];
                const ssize_t count = read(STDIN_FILENO, buffer, sizeof(buffer));
                if (count > 0) pending.append(buffer, static_cast<size_t>(count));
                else if (count == 0) eof = true;
                else if (errno != EINTR) return false;
            }
        }
        return false;
    }
};

inline void run_console(int sock, const std::string& role, const std::string& endpoint) {
    std::atomic<bool> connected{true};
    std::thread receiver([&] {
        receive_loop(sock);
        connected.store(false);
        shutdown(sock, SHUT_RDWR);
    });
    ConsoleInput input;
    std::string line;
    while (connected.load()) {
        ui::prompt();
        if (!input.next(line, connected)) break;
        if (!connected.load()) break;
        if (line.empty()) continue;
        if (line == "cikis") break;
        if (line == "!yardim") { ui::help(); continue; }
        if (line == "!durum") { ui::status(role + " aktif; TCP bağlantısı açık."); continue; }
        if (line == "!temizle") { ui::clear(); ui::banner(role, endpoint); continue; }
        if (line == "!resim" || line == "!resim ") {
            ui::error("Kullanım: !resim <dosya yolu>");
            continue;
        }
        if (line.rfind("!resim ", 0) == 0) {
            std::string path = line.substr(7);
            if (path.size() >= 2 &&
                ((path.front() == '"' && path.back() == '"') ||
                 (path.front() == '\'' && path.back() == '\''))) {
                path = path.substr(1, path.size() - 2);
            }
            send_image_message(sock, path);
            continue;
        }
        if (line.size() > MAX_TEXT_SIZE) {
            ui::error("Metin 1 MB'dan büyük olamaz.");
            continue;
        }
        if (!send_text_message(sock, line)) {
            ui::error("Mesaj gönderilemedi; bağlantı kapanmış olabilir.");
            break;
        }
        ui::status("Metin gönderildi.");
    }
    connected.store(false);
    ui::info("Bağlantı kapatılıyor...");
    shutdown(sock, SHUT_RDWR);
    if (receiver.joinable()) receiver.join();
}

#endif
